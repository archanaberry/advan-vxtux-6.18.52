// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_flightrec.c — perekam log boot ("flight recorder") untuk UMS512.
 *
 * Latar belakang: boot kernel 6.18 di perangkat ini berhenti tanpa jejak —
 * layar tetap di logo LK, tidak ada panic, tidak ada reboot, dan partisi
 * `cache` tidak pernah menerima laporan dari init userspace. Artinya kernel
 * mati SEBELUM userspace, di mana satu-satunya kanal yang tersedia hanyalah
 * log buffer kernel itu sendiri, yang selama ini tidak bisa dibaca siapa pun
 * (UART tidak selalu tersambung, tidak ada antarmuka lain untuk membaca RAM).
 *
 * Driver ini menutup celah tersebut sepenuhnya dari dalam kernel:
 *
 *  - kthread dimulai sejak core_initcall() (sebelum SMP dinyalakan, jadi
 *    thread tetap bisa jalan di CPU lain bila CPU boot menggantung di
 *    initcall berikutnya),
 *  - isi log buffer (dmesg) disalin ke partisi `cache` (mmcblk0p49) di
 *    offset 8 MiB setiap ~3 detik — cukup untuk merekam initcall terakhir
 *    yang sempat jalan (cmdline memuat initcall_debug=1),
 *  - sekali lagi saat panic, lewat notifier.
 *
 * Tata letak di partisi cache (little endian, 512-byte aligned):
 *
 *   +0x00000000  laporan init diagnostik (VXTUXLOG1) — milik userspace
 *   +0x00800000  struct fr_hdr (64 B) + teks dmesg (maks 128 KiB)
 *
 * Membaca hasilnya: dump partisi cache lalu cari "VXTUXFR1".
 */
#define pr_fmt(fmt)	"vxtux-fr: " fmt

#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/device/class.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kmsg_dump.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/panic_notifier.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/stringify.h>
#include <linux/timekeeping.h>
#include <linux/vmalloc.h>

#define FR_MAGIC	"VXTUXFR1"
#define FR_VERSION	1U
#define FR_DISK		"mmcblk0"
#define FR_PARTNO	49		/* cache, 100 MiB, tak dipakai rootfs */
#define FR_OFFSET	(8ULL << 20)	/* 8 MiB di dalam cache */
#define FR_TEXT_MAX	(128U << 10)
#define FR_HDR_SIZE	64U

enum fr_reason {
	FR_PERIODIC	= 0,
	FR_PANIC	= 1,
};

struct fr_hdr {
	char	magic[8];
	u32	version;
	u32	reason;
	u64	seq;
	u64	uptime_ns;
	u32	text_len;
	u32	hdr_size;
	u64	pad[2];
};

static char *fr_cur;			/* header + teks yang akan ditulis */
static char *fr_prev;			/* teks terakhir yang sudah ditulis */
static size_t fr_prev_len;
static u64 fr_seq;
static bool fr_announced;
static int fr_last_err;		/* errno terakhir yang sudah dicetak */
static struct task_struct *fr_task;
static DEFINE_MUTEX(fr_lock);

/*
 * Kanal tulis ke eMMC belum pernah terbukti bekerja di perangkat ini:
 * 28 dump partisi cache (artix6..artix33) tidak pernah memuat VXTUXFR1,
 * bahkan pada boot yang TERBUKTI driver MMC sudah jalan penuh (daftar
 * mmcblk0p1..p71 muncul di layar).  Bedah dump artix33 (out/_bedah33.py)
 * menunjukkan partisi cache 100% identik antara dump pra-flash dan
 * pasca-boot: nol byte ditulis, dan tiga penulis berbeda (pstore_blk,
 * flightrec, init userspace) semuanya gagal serentak.
 *
 * Agar kegagalan berikutnya TIDAK butuh dump lagi: setiap kegagalan yang
 * berbeda dicetak SEKALI ke dmesg (fbcon memutarnya kembali saat 200 baris
 * terakhir), dan setiap tulisan sukses diverifikasi dengan baca-balik.
 */
static void fr_report_err(const char *what, int err)
{
	if (err == fr_last_err)
		return;
	fr_last_err = err;
	pr_err("%s failed: %d (%pe) - visible on fbcon\n", what, err,
	       ERR_PTR(err));
}/*
 * Cari device node partisi cache TANPA bergantung pada devtmpfs: devtmpfs
 * baru di-mount kernel_init setelah seluruh initcall selesai, sementara kita
 * justru ingin merekam saat initcall masih berjalan.
 *
 * Dua strategi dicoba berurutan (artix34):
 *   1. langsung dari gendisk lewat dev_t - jalan sebelum devtmpfs ada;
 *   2. lewat node /dev/mmcblk0p49 - jalan setelah devtmpfs ter-mount.
 * Kalau keduanya gagal, errno masing-masing tercetak di fbcon sehingga
 * langsung terlihat apakah p49 memang tidak dibuat (-ENODEV), disk read-
 * only (-EROFS), atau hal lain.
 */
static struct file *fr_open(void)
{
	struct device *dev;
	struct gendisk *disk;
	dev_t devt = 0;
	struct file *f;

	dev = class_find_device_by_name(&block_class, FR_DISK);
	if (dev) {
		disk = dev_to_disk(dev);
		if (disk)
			devt = part_devt(disk, FR_PARTNO);
		put_device(dev);
	}

	if (devt) {
		f = bdev_file_open_by_dev(devt, BLK_OPEN_WRITE, NULL, NULL);
		if (!IS_ERR(f))
			return f;
		fr_report_err("open " FR_DISK "p" __stringify(FR_PARTNO)
			      " by devt", PTR_ERR(f));
	}

	f = bdev_file_open_by_path("/dev/" FR_DISK "p" __stringify(FR_PARTNO),
				   BLK_OPEN_WRITE, NULL, NULL);
	if (IS_ERR(f))
		fr_report_err("open /dev/" FR_DISK "p" __stringify(FR_PARTNO),
			      PTR_ERR(f));
	return f;
}

static int fr_dump_locked(u32 reason)
{
	struct kmsg_dump_iter iter = { };
	struct fr_hdr *h = (struct fr_hdr *)fr_cur;
	char *text = fr_cur + FR_HDR_SIZE;
	struct file *file;
	loff_t pos = FR_OFFSET;
	size_t len = 0;
	size_t total;
	int ret;

	kmsg_dump_rewind(&iter);
	if (!kmsg_dump_get_buffer(&iter, false, text, FR_TEXT_MAX, &len))
		return -EIO;
	if (!len)
		return -ENODATA;
	len = min_t(size_t, len, FR_TEXT_MAX);

	/* Jangan buang siklus eMMC bila isi log belum berubah. */
	if (len == fr_prev_len && !memcmp(text, fr_prev, len))
		return -EALREADY;

	memset(h, 0, FR_HDR_SIZE);
	memcpy(h->magic, FR_MAGIC, sizeof(h->magic));
	h->version = FR_VERSION;
	h->reason = reason;
	h->seq = fr_seq + 1;
	h->uptime_ns = ktime_get_ns();
	h->text_len = len;
	h->hdr_size = FR_HDR_SIZE;

	total = ALIGN(FR_HDR_SIZE + len, 512);

	file = fr_open();
	if (IS_ERR(file))
		return PTR_ERR(file);

	ret = kernel_write(file, fr_cur, total, &pos);
	if (ret != (int)total) {
		int err = ret < 0 ? ret : -EIO;

		fput(file);
		fr_report_err("write", err);
		return err;
	}

	/* Baca-balik: bukti fisik bahwa byte benar-benar sampai ke eMMC. */
	{
		char back[8];
		loff_t rpos = FR_OFFSET;
		ssize_t rn = kernel_read(file, back, sizeof(back), &rpos);

		if (rn != (ssize_t)sizeof(back) ||
		    memcmp(back, FR_MAGIC, sizeof(back))) {
			fput(file);
			fr_report_err("readback after write", -EIO);
			return -EIO;
		}
	}
	fput(file);
	if (fr_last_err) {
		pr_info("write channel recovered (was %d) - dmesg %zu B recorded at %s p%d + %#llx\n",
			fr_last_err, len, FR_DISK, FR_PARTNO,
			(unsigned long long)FR_OFFSET);
		fr_last_err = 0;
	}

	fr_seq++;
	memcpy(fr_prev, text, len);
	fr_prev_len = len;

	return 0;
}

static int fr_thread(void *unused)
{
	while (!kthread_should_stop()) {
		int ret;

		mutex_lock(&fr_lock);
		ret = fr_dump_locked(FR_PERIODIC);
		mutex_unlock(&fr_lock);

		if (ret == 0 && !fr_announced) {
			fr_announced = true;
			pr_info("OK - dmesg %zu B recorded at %s p%d + %#llx\n",
				fr_prev_len, FR_DISK, FR_PARTNO,
				(unsigned long long)FR_OFFSET);
		}

		/* cepat di awal boot, santai setelah rekaman pertama */
		msleep(fr_announced ? 3000 : 500);
	}
	return 0;
}

static int fr_panic_notify(struct notifier_block *nb, unsigned long action,
			   void *data)
{
	/* Jangan pernah memblokir jalur panic: CPU lain bisa memegang lock. */
	if (mutex_trylock(&fr_lock)) {
		fr_dump_locked(FR_PANIC);
		mutex_unlock(&fr_lock);
	}
	return NOTIFY_OK;
}

static struct notifier_block fr_panic_nb = {
	.notifier_call	= fr_panic_notify,
	.priority	= 0,
};

static int __init vxtux_flightrec_init(void)
{
	if (!IS_ENABLED(CONFIG_PRINTK) || !IS_ENABLED(CONFIG_BLOCK))
		return -ENODEV;

	fr_cur = vzalloc(FR_HDR_SIZE + FR_TEXT_MAX);
	fr_prev = vzalloc(FR_TEXT_MAX);
	if (!fr_cur || !fr_prev)
		goto nomem;

	atomic_notifier_chain_register(&panic_notifier_list, &fr_panic_nb);

	fr_task = kthread_run(fr_thread, NULL, "vxtux-fr");
	if (IS_ERR(fr_task)) {
		int ret = PTR_ERR(fr_task);

		atomic_notifier_chain_unregister(&panic_notifier_list,
						 &fr_panic_nb);
		fr_task = NULL;
		vfree(fr_cur);
		vfree(fr_prev);
		fr_cur = fr_prev = NULL;
		return ret;
	}

	pr_info("active - dmesg -> /dev/%sp%d + %#llx every 3 s (write+readback verified)\n",
		FR_DISK, FR_PARTNO, (unsigned long long)FR_OFFSET);
	return 0;

nomem:
	vfree(fr_cur);
	vfree(fr_prev);
	fr_cur = fr_prev = NULL;
	return -ENOMEM;
}
/*
 * core_initcall: berjalan SEBELUM SMP dan sebelum sebagian besar initcall
 * driver, sehingga thread sudah hidup saat kernel menggantung di driver mana
 * pun setelah titik ini.
 */
core_initcall(vxtux_flightrec_init);
