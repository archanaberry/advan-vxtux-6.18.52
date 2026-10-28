// SPDX-License-Identifier: GPL-2.0
//
// /vxtux/init -- init statis arm64 untuk initramfs BAWAAN kernel Artix UMS512.
//
// Kenapa ada: kernel 6.18 di tablet ini tidak punya UART yang bisa dibaca
// (earlycon lama menunjuk 0x71400000 = kontroler eMMC, bukan UART) dan tidak
// punya console=tty0, jadi setiap kegagalan boot tampak "diam di logo".
//
// Init ini melakukan empat hal, semuanya tanpa userspace lain:
//   1. memasang partisi `cache` (100 MiB, ext4) lalu menyimpan SELURUH isi
//      ring buffer kernel (klogctl READ_ALL) sebagai berkas biasa
//      /cache/boot_log.txt SEBELUM partisi super dipindai -- kanal yang
//      terbukti terbaca lewat spd_dump tanpa perlu UART/layar;
//   2. menyalin log yang sama ke kanal mentah vendor `common_rs1_a`
//      (8 MiB tanpa filesystem, offset 0) -- salinan cadangan yang tetap
//      hidup walau mount ext4 gagal (lihat penjelasan RS1_* di bawah);
//   3. mencetak ringkasan yang sama ke /dev/console (fbcon -> panel) supaya
//      kelihatan di layar kalau DRM/panel hidup;
//   4. mencari partisi root ext4 (super) dengan memeriksa superblock tiap
//      partisi mmcblk0p*, mount, lalu chroot + exec init Artix (dinit).
//
// Kalau root tidak ditemukan: laporan tetap ditulis, pesan tetap dicetak,
// lalu init tidur (bukan panic) supaya layar tidak berkedip.
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#define CACHE_BLOCKS_1K 102400   /* partisi cache = 100 MiB -> 102400 KiB */
#define LOG_MAX        (1024 * 1024)
#define REPORT_MAX     (2 * 1024 * 1024)
/* Cache is mounted as ext4; persist diagnostics as ordinary files. */
#define CACHE_MOUNT       "/cache"
#define EARLY_LOG_PATH    "/cache/boot_log.txt"
#define EARLY_LOG_TMP     "/cache/boot_log.txt.tmp"
#define REPORT_PATH       "/cache/vxtux_report.txt"
#define TEST_PATH         "/cache/.vxtux_write_test"
#define CACHE_PART_BLOCKS 102400UL

/*
 * Kanal mentah vendor (RE kernel stok 5.4.210, 2026-10-28)
 * --------------------------------------------------------
 * Sumber: realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-
 * AndroidT-kernel-source (Makefile: VERSION=5 PATCHLEVEL=4 SUBLEVEL=210),
 * file drivers/soc/sprd/debug/sysdump/last_kmsg.c:
 *
 *     static const char devicename[] = "/dev/block/by-name/common_rs1_a";
 *     #define LAST_KMSG_OFFSET 0
 *     #define KMSG_BUF_SIZE    (128 * 1024)
 *
 * Partisi `common_rs1_a` (8 MiB, mentah, TANPA filesystem) berisi teks
 * log kernel boot sebelumnya: dump stok di PC mulai dengan
 * "<14>[   71.081754] init: Sending signal 15 ...".  Dump stok juga
 * membuktikan `cache` adalah ext4 Android (magic 0xEF53 di 1024+0x38),
 * jadi tulisan mentah ke `cache` merusak filesystem -- sebabnya kanal
 * utama sekarang berupa berkas di atas ext4 yang dipasang.
 *
 * Karena Sembilan partisi di perangkat ini berukuran persis 8 MiB (dtb,
 * dtbo, logo, fbootlogo, init_boot, common_rs1_a/b), ukuran saja tidak
 * cukup untuk mengenalinya: pencocokan WAJIB lolos uji isi -- baris log
 * "<14>[..." milik stok ATAU tanda kita sendiri di offset 0.  Kalau tidak
 * ada yang cocok, kanal dilewati (tidak pernah menulis membabi buta).
 */
#define RS1_MAGIC     "### VXTUXRS1"
#define RS1_SIZE_KIB  8192UL       /* 8 MiB, sesuai common_rs1_a */
#define RS1_MAX       (128 * 1024) /* KMSG_BUF_SIZE stok */

static char *report;             /* kumpulan teks yang ditulis ke cache */
static size_t report_len;
static int console_fd = -1;
static int cache_mounted;        /* ext4 cache berhasil dipasang */

/* ---------- keluaran ---------- */

static void con(const char *s)
{
	if (console_fd >= 0)
		write(console_fd, s, strlen(s));
}

static void con_fmt(const char *fmt, ...)
{
	char b[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	con(b);
}

/* tambahkan ke laporan (yang akan ditulis ke partisi cache) */
static void rep(const char *s)
{
	size_t n = strlen(s);
	if (!report || report_len + n >= REPORT_MAX)
		return;
	memcpy(report + report_len, s, n);
	report_len += n;
}

static void rep_fmt(const char *fmt, ...)
{
	char b[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	rep(b);
}

static void both(const char *s)
{
	con(s);
	rep(s);
}

static void both_fmt(const char *fmt, ...)
{
	char b[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	both(b);
}

/* salin isi berkas teks ke laporan + console (dipotong) */
static void dump_file(const char *path, int to_console)
{
	int fd = open(path, O_RDONLY);
	char buf[4096];
	ssize_t n;

	if (fd < 0) {
		both_fmt("  (%s: %s)\n", path, strerror(errno));
		return;
	}
	while ((n = read(fd, buf, sizeof buf)) > 0) {
		if (report_len + (size_t)n < REPORT_MAX) {
			memcpy(report + report_len, buf, n);
			report_len += n;
		}
		if (to_console)
			write(console_fd, buf, n);
	}
	close(fd);
}

/* ---------- ring buffer kernel ---------- */

static void dump_kernel_log(int to_console)
{
	char *b = malloc(LOG_MAX);
	int n;

	if (!b)
		return;
	n = klogctl(3 /* SYSLOG_ACTION_READ_ALL */, b, LOG_MAX - 1);
	if (n <= 0) {
		both_fmt("  (klogctl failed: %s)\n", strerror(errno));
		free(b);
		return;
	}
	b[n] = 0;
	rep("----- dmesg (kernel ring buffer, from the start) -----\n");
	rep(b);
	if (b[n - 1] != '\n')
		rep("\n");
	rep("----- end of dmesg -----\n");

	if (to_console) {
		/* Show only the last 60 lines on the panel. */
		char *p = b + n, *q = b;
		int lines = 0;
		while (p > b && lines < 60) {
			if (*--p == '\n')
				lines++;
		}
		if (p > b)
			q = p + 1;
		con("\n----- last 60 dmesg lines -----\n");
		con(q);
		con("\n----- end of dmesg -----\n");
	}
	free(b);
}

/* ---------- partisi ---------- */struct part {
	char name[64];
	unsigned long blocks;   /* dalam KiB */
	unsigned major, minor;   /* untuk fallback mknod */
};

/* Static: PID 1's kernel stack is only 16 KiB; keep large arrays off it. */
static struct part g_parts[128];
static int np_parts;             /* jumlah entri g_parts yang terisi */

static int list_partitions(struct part *out, int max)
{
	FILE *f = fopen("/proc/partitions", "r");
	char line[256];
	int n = 0;

	if (!f)
		return -1;
	while (fgets(line, sizeof line, f) && n < max) {
		unsigned major, minor;
		unsigned long blocks;
		char name[64];

		if (sscanf(line, " %u %u %lu %63s", &major, &minor, &blocks,
			   name) != 4)
			continue;
		snprintf(out[n].name, sizeof out[n].name, "%s", name);
		out[n].blocks = blocks;
		out[n].major = major;
		out[n].minor = minor;
		n++;
	}
	fclose(f);
	return n;
}

/*
 * Tampilkan baris dmesg TERAKHIR yang relevan dengan penyimpanan/clock/
 * deferred probe di layar (dan laporan).  Tanpa ini, penyebab eMMC telat
 * atau mati hanya terlihat lewat dump; dengan ini, layar selalu
 * memperlihatkan penjelas terkini selama init menunggu.
 */
static void dmesg_hints(int maxlines)
{
	static const char *keys[] = { "sdio", "mmc", "sdhci", "-517",
				      "deferred probe", "enable clock",
				      "EPROBE_DEFER" };
	static char *hits[64];
	char *b, *line, *save;
	int n, i, nh = 0, from;

	b = malloc(LOG_MAX);
	if (!b)
		return;
	n = klogctl(3 /* SYSLOG_ACTION_READ_ALL */, b, LOG_MAX - 1);
	if (n <= 0) {
		free(b);
		return;
	}
	b[n] = 0;
	for (line = strtok_r(b, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		for (i = 0; i < (int)(sizeof keys / sizeof keys[0]); i++)
			if (strstr(line, keys[i])) {
				if (nh == 64) {
					memmove(hits, hits + 1,
						63 * sizeof hits[0]);
					nh--;
				}
				hits[nh++] = line;
				break;
			}
	}
	from = nh > maxlines ? nh - maxlines : 0;
	if (nh > from) {
		both("  --- dmesg hints (storage/clock/defer) ---\n");
		for (i = from; i < nh; i++)
			both_fmt("  %s\n", hits[i]);
	}
	free(b);
}	/*
	 * Tunggu sampai eMMC (mmcblk*) terlihat di /proc/partitions.
	 *
 * Latar belakang (artix35): pada boot artix34, init berjalan di detik ~30
 * dan SEKALI membaca daftar partisi -- saat itu host eMMC belum/bahkan
 * tidak pernah terdaftar (driver clock UMS512 =m -> clock "sdio"/"enable"
 * host tidak ada -> probe mengembalikan -EPROBE_DEFER selamanya), sehingga
 * semua langkah berikut (uji tulis, laporan, pindai ext4) gagal serentak
 * hanya karena terlalu cepat/memasif.  Loop ini:
 *   - menunggu maksimum timeout_s detik,
 *   - memperbarui layar tiap 10 detik (jumlah partisi + petunjuk dmesg),
 *   - mengembalikan daftar partisi TERBARU begitu mmcblk* muncul.
 */
static int wait_for_storage(struct part *parts, int max, int timeout_s)
{
	int waited = 0, np = 0;

	for (;;) {
		int i, mmc = 0;

		np = list_partitions(parts, max);
		if (np < 0)
			np = 0;
		for (i = 0; i < np; i++)
			if (!strncmp(parts[i].name, "mmcblk", 6))
				mmc++;
		if (mmc) {
			both_fmt("VXTUX-STEP: storage ready after %ds: "
				 "%d mmc partitions (of %d visible)\n",
				 waited, mmc, np);
			return np;
		}
		if (waited >= timeout_s) {
			both_fmt("VXTUX-STEP: eMMC ABSENT after %ds "
				 "(%d partitions, none mmc)\n",
				 waited, np);
			dmesg_hints(20);
			return np;
		}
		if (waited % 10 == 0) {
			both_fmt("VXTUX-STEP: waiting for eMMC (%ds, %d "
				 "partitions, none mmc yet)\n",
				 waited, np);
			dmesg_hints(6);
		}
		sleep(2);
		waited += 2;
	}
}

/*
 * Cadangan bila devtmpfs tidak sempat membuat node: buat node blok yang
 * hilang langsung dari major/minor /proc/partitions.  Tidak merusak apa pun
 * - bila node sudah ada (devtmpfs), langkah ini dilewati.
 */
static void ensure_nodes(struct part *parts, int np)
{
	int i, made = 0;

	for (i = 0; i < np; i++) {
		char dev[96];
		struct stat st;

		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		if (stat(dev, &st) == 0)
			continue;
		if (mknod(dev, S_IFBLK | 0600,
			  makedev(parts[i].major, parts[i].minor)) == 0)
			made++;
	}
	if (made)
		both_fmt("VXTUX-STEP: created %d missing block nodes "
			 "with mknod\n", made);
}

/* Returns 1 for ext4, 0 for a readable non-ext4 block, or -errno on I/O error. */
static int is_ext4(const char *dev)
{
	unsigned char sb[2];
	ssize_t n;
	int fd = open(dev, O_RDONLY | O_CLOEXEC);
	int err;

	if (fd < 0)
		return -errno;
	/* ext4 superblock magic 0xEF53 ada di offset 1024 + 0x38 */
	n = pread(fd, sb, sizeof sb, 1024 + 0x38);
	err = errno;
	close(fd);
	if (n != (ssize_t)sizeof sb)
		return n < 0 ? -err : -EIO;
	return sb[0] == 0x53 && sb[1] == 0xEF;
}

/* Read the GPT partition name from sysfs uevent when the kernel exposes it. */
static int get_partname(const char *devname, char *name, size_t name_len)
{
	char path[160], line[256];
	FILE *f;
	int found = 0;

	if (snprintf(path, sizeof path, "/sys/class/block/%s/uevent", devname)
	    >= (int)sizeof path)
		return 0;
	f = fopen(path, "r");
	if (!f)
		return 0;
	while (fgets(line, sizeof line, f)) {
		if (strncmp(line, "PARTNAME=", 9) == 0) {
			size_t n = strcspn(line + 9, "\r\n");

			if (n >= name_len)
				n = name_len - 1;
			memcpy(name, line + 9, n);
			name[n] = 0;
			found = 1;
			break;
		}
	}
	fclose(f);
	return found;
}

static void mkdir_p(const char *p)
{
	mkdir(p, 0755);
}

/*
 * pstore -> screen + report (2026-10-27)
 * ---------------------------------------
 * Read console-* and dmesg-* records from the previous boot, if present.
 */
static void dump_pstore(void)
{
	DIR *d;
	struct dirent *e;
	int n = 0;

	mkdir_p("/sys/fs");
	mkdir_p("/sys/fs/pstore");
	if (mount("pstore", "/sys/fs/pstore", "pstore", 0, "") != 0) {
		both_fmt("VXTUX-STEP: mount pstore failed: %s\n",
			 strerror(errno));
		return;
	}

	both("VXTUX-STEP: 2c/5 pstore records from the previous boot\n");
	d = opendir("/sys/fs/pstore");
	if (!d) {
		both_fmt("  (cannot open /sys/fs/pstore: %s)\n",
			 strerror(errno));
		umount("/sys/fs/pstore");
		return;
	}
	while ((e = readdir(d)) != NULL) {
		char path[256];
		int fd, budget = 8192;
		char buf[512];
		ssize_t r;

		if (e->d_name[0] == '.')
			continue;
		if (!strstr(e->d_name, "console") &&
		    !strstr(e->d_name, "dmesg"))
			continue;
		snprintf(path, sizeof path, "/sys/fs/pstore/%s", e->d_name);
		both_fmt("  --- pstore %s ---\n", e->d_name);
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			both_fmt("    (cannot open: %s)\n", strerror(errno));
			continue;
		}
		n++;		/* Keep the full report; limit panel output to 8 KiB. */
		while ((r = read(fd, buf, sizeof buf)) > 0) {
			if (report_len + (size_t)r < REPORT_MAX) {
				memcpy(report + report_len, buf, r);
				report_len += (size_t)r;
			}
			if (budget > 0) {
				int w = r > budget ? budget : (int)r;
				write(console_fd, buf, w);
				budget -= w;
			}
		}
		close(fd);
		if (report && report_len && report[report_len - 1] != '\n')
			rep("\n");
	}
	closedir(d);
	if (!n)
		both("  (pstore empty -- the backend has never written)\n");
	umount("/sys/fs/pstore");
}

/* Write a complete regular file and flush it to the mounted cache. */
static int write_cache_file(const char *path, const char *data, size_t len)
{
	size_t done = 0;
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd < 0)
		return -errno;
	while (done < len) {
		ssize_t n = write(fd, data + done, len - done);

		if (n < 0) {
			int err = -errno;

			close(fd);
			return err;
		}
		if (!n) {
			close(fd);
			return -EIO;
		}
		done += (size_t)n;
	}
	if (fsync(fd) != 0) {
		int err = -errno;

		close(fd);
		return err;
	}
	close(fd);
	return 0;
}

/*
 * Mount only the cache partition, found by its known 100 MiB size.  Do not
 * fall back to guessing a hard-coded partition number when discovery failed.
 */
static int mount_cache_partition(const struct part *parts, int np)
{
	int i;

	mkdir_p(CACHE_MOUNT);
	for (i = 0; i < np; i++) {
		char dev[96];

		if (parts[i].blocks != CACHE_PART_BLOCKS ||
		    strncmp(parts[i].name, "mmcblk", 6))
			continue;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		if (mount(dev, CACHE_MOUNT, "ext4", 0, "") == 0) {
			cache_mounted = 1;
			return 1;
		}
		both_fmt("VXTUX-STEP: mount cache %s failed: %s\n", dev,
			 strerror(errno));
		return 0;
	}
	both("VXTUX-STEP: cache partition not found by size\n");
	return 0;
}

/*
 * Salin log ke common_rs1_a. Pada dump Advan, offset 0 berisi newline dan
 * NUL sebelum record printk "<14>[..."; jangan perlakukan header sebagai
 * string C dan jangan mensyaratkan signature tepat di byte 0.
 */
static char raw_log_dev[96];

static int has_vendor_log_signature(const unsigned char *head, size_t len)
{
	size_t i;
	const size_t magic_len = strlen(RS1_MAGIC);

	if (len >= magic_len && memcmp(head, RS1_MAGIC, magic_len) == 0)
		return 1;
	if (len > 64)
		len = 64;
	for (i = 0; i + 4 < len; i++) {
		size_t j;

		if (head[i] != '<')
			continue;
		j = i + 1;
		while (j < len && j - i <= 3 && head[j] >= '0' && head[j] <= '9')
			j++;
		if (j > i + 1 && j + 1 < len && head[j] == '>' &&
		    head[j + 1] == '[')
			return 1;
	}
	return 0;
}

static int write_raw_vendor_log(const char *data, size_t len,
				const struct part *parts, int np)
{
	unsigned char head[64];
	ssize_t got;
	int i, fd;

	if (len > RS1_MAX)
		len = RS1_MAX;

	for (i = 0; parts && i < np; i++) {
		char partname[96] = "";
		char dev[96];
		int named;

		if (parts[i].blocks != RS1_SIZE_KIB ||
		    strncmp(parts[i].name, "mmcblk", 6))
			continue;
		named = get_partname(parts[i].name, partname, sizeof partname);
		if (named && strcmp(partname, "common_rs1_a") != 0)
			continue;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		fd = open(dev, O_RDONLY | O_CLOEXEC);
		if (fd < 0) {
			if (named)
				both_fmt("VXTUX-STEP: raw log read %s failed: %s\n",
					 dev, strerror(errno));
			continue;
		}
		got = pread(fd, head, sizeof head, 0);
		close(fd);
		if (got < 0 || !has_vendor_log_signature(head, (size_t)got))
			continue;

		fd = open(dev, O_RDWR | O_CLOEXEC);
		if (fd < 0) {
			both_fmt("VXTUX-STEP: raw log open %s failed: %s\n",
				 dev, strerror(errno));
			continue;
		}
		if (pwrite(fd, data, len, 0) != (ssize_t)len) {
			int err = errno;

			both_fmt("VXTUX-STEP: raw log write %s failed: %s\n",
				 dev, strerror(err));
			close(fd);
			continue;
		}
		if (fsync(fd) != 0 || pread(fd, head, strlen(RS1_MAGIC), 0) !=
		    (ssize_t)strlen(RS1_MAGIC) ||
		    memcmp(head, RS1_MAGIC, strlen(RS1_MAGIC)) != 0) {
			int err = errno ? errno : EIO;

			both_fmt("VXTUX-STEP: raw log readback FAILED on %s: %s\n",
				 dev, strerror(err));
			close(fd);
			continue;
		}
		close(fd);
		snprintf(raw_log_dev, sizeof raw_log_dev, "%s", dev);
		both_fmt("VXTUX-STEP: raw vendor log saved to %s (%zu bytes, "
			 "readback OK)\n", raw_log_dev, len);
		return 1;
	}
	both("VXTUX-STEP: raw vendor log partition not found by name/content "
	     "(skipped)\n");
	return 0;
}

/*
 * Save the requested early dmesg snapshot before any attempt to mount super.
 */
static void save_early_dmesg(const struct part *parts, int np)
{
	char *buf, *raw;
	int n, ret, mounted;

	mounted = mount_cache_partition(parts, np);
	/* The snapshot must be made before mount_super/find_and_mount_root.
	 * (Reading old pstore records later may clear the pstore record.) */
	buf = malloc(LOG_MAX);
	if (!buf) {
		both("VXTUX-STEP: cannot allocate early dmesg buffer\n");
		return;
	}
	n = klogctl(3 /* SYSLOG_ACTION_READ_ALL */, buf, LOG_MAX - 1);
	if (n <= 0) {
		int err = errno;

		free(buf);
		both_fmt("VXTUX-STEP: early dmesg read failed: %s\n",
			 strerror(err));
		return;
	}
	/* Kanal mentah dicoba SETIAP kali: tetap hidup walau ext4 gagal. */
	raw = malloc((size_t)n + strlen(RS1_MAGIC) + 2);
	if (raw) {
		size_t hlen = (size_t)snprintf(raw,
			strlen(RS1_MAGIC) + 2, "%s\n", RS1_MAGIC);

		memcpy(raw + hlen, buf, (size_t)n);
		write_raw_vendor_log(raw, hlen + (size_t)n, parts, np);
		free(raw);
	}
	if (!mounted) {
		free(buf);
		return;
	}
	ret = write_cache_file(EARLY_LOG_TMP, buf, (size_t)n);
	free(buf);
	if (ret) {
		both_fmt("VXTUX-STEP: early log write failed: %s\n",
			 strerror(-ret));
		return;
	}
	if (rename(EARLY_LOG_TMP, EARLY_LOG_PATH) != 0) {
		both_fmt("VXTUX-STEP: early log rename failed: %s\n",
			 strerror(errno));
		return;
	}
	both_fmt("VXTUX-STEP: mounted cache and saved early dmesg to %s "
		 "(%d bytes, fsync OK)\n", EARLY_LOG_PATH, n);
}

static void write_report_to_cache(void)
{
	char *raw;
	int ret;

	if (!report || !report_len) {
		both("VXTUX-STEP: report buffer is empty\n");
		return;
	}
	if (cache_mounted) {
		ret = write_cache_file(REPORT_PATH, report, report_len);
		if (ret)
			both_fmt("VXTUX-STEP: write %s failed: %s\n", REPORT_PATH,
				 strerror(-ret));
		else
			both_fmt("VXTUX-STEP: report saved to %s (%zu bytes, "
				 "fsync OK)\n", REPORT_PATH, report_len);
		return;
	}
	/*
	 * cache tidak terpasang: laporan tetap harus tersimpan di suatu tempat.
	 * Tulis dengan kepala VXTUXRS1 juga -- tanpa kepala itu, boot berikutnya
	 * tidak akan mengenali partisi (isi pertamanya jadi "#" bukan "<14>["),
	 * dan kanal cadangan ini jatuh sendiri setelah sekali dipakai.
	 */
	raw = malloc(report_len + strlen(RS1_MAGIC) + 2);
	if (raw) {
		size_t hlen = (size_t)snprintf(raw, strlen(RS1_MAGIC) + 2,
					       "%s\n", RS1_MAGIC);

		memcpy(raw + hlen, report, report_len);
		write_raw_vendor_log(raw, hlen + report_len, g_parts, np_parts);
		free(raw);
	}
}

/*
 * Verify the cache write path using a temporary file inside its mounted
 * ext4 filesystem. Never write diagnostic bytes to arbitrary raw offsets:
 * cache has a filesystem, so raw writes would corrupt it.
 */
static void write_readback_test(const struct part *parts, int np)
{
	static const char sig[] = "VXTUXWRT\x01" "UMS512!";
	char back[sizeof(sig)];
	int fd;

	(void)parts;
	(void)np;
	/*
	 * Kalau ext4 cache tidak terpasang, berkas ini jatuhnya ke rootfs initramfs
	 * (ramfs) -- uji akan bilang OK padahal kanal yang kita tuju mati.  Lebih
	 * jujur melewatinya dengan baris SKIP.
	 */
	if (!cache_mounted) {
		both("VXTUX-WRITE-TEST: SKIP (cache not mounted, no filesystem "
		     "channel to probe)\n");
		return;
	}
	fd = open(TEST_PATH, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		both_fmt("VXTUX-WRITE-TEST: FAIL open %s: %s\n", TEST_PATH,
			 strerror(errno));
		return;
	}
	if (write(fd, sig, sizeof(sig)) != (ssize_t)sizeof(sig)) {
		both_fmt("VXTUX-WRITE-TEST: FAIL write %s: %s\n", TEST_PATH,
			 strerror(errno));
		close(fd);
		return;
	}
	if (fsync(fd) != 0 || pread(fd, back, sizeof(back), 0) !=
	    (ssize_t)sizeof(back)) {
		both_fmt("VXTUX-WRITE-TEST: FAIL readback %s: %s\n", TEST_PATH,
			 strerror(errno));
		close(fd);
		return;
	}
	close(fd);
	if (memcmp(back, sig, sizeof(sig)) != 0) {
		both("VXTUX-WRITE-TEST: FAIL readback mismatch "
		     "(wrote one thing, read another)\n");
		return;
	}
	unlink(TEST_PATH);
	both_fmt("VXTUX-WRITE-TEST: OK file write+readback on %s\n", CACHE_MOUNT);
}

/* ---------- find and mount root ---------- */

static const char *const init_candidates[] = {
	"/sbin/init", "/sbin/dinit-init", "/sbin/dinit",
	"/bin/dinit", "/usr/bin/dinit-init", "/usr/bin/dinit",
	"/bin/sh", "/usr/bin/sh", NULL
};

static int root_init_path(char *found, size_t found_len)
{
	char path[160];
	int i;

	for (i = 0; init_candidates[i]; i++) {
		if (snprintf(path, sizeof path, "/newroot%s", init_candidates[i])
		    >= (int)sizeof path)
			continue;
		if (access(path, X_OK) == 0) {
			snprintf(found, found_len, "%s", init_candidates[i]);
			return 1;
		}
	}
	return 0;
}

static int find_and_mount_root(const struct part *parts, int np, char *rootdev,
			       size_t rootdev_len)
{
	int i;

	mkdir_p("/newroot");
	for (i = 0; i < np; i++) {
		char dev[96], partname[96] = "", initpath[96] = "";
		int probe, named;

		if (!strstr(parts[i].name, "mmcblk") ||
		    !strchr(parts[i].name, 'p'))
			continue;
		named = get_partname(parts[i].name, partname, sizeof partname);
		/* GPT says super: inspect it regardless of reported size. Without
		 * PARTNAME support, retain the previous >=1 GiB fallback heuristic. */
		if (named && strcmp(partname, "super") != 0)
			continue;
		if (!named && parts[i].blocks < 1024 * 1024)
			continue;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		probe = is_ext4(dev);
		if (probe < 0) {
			rep_fmt("  %s (%s): ext4 probe I/O failed: %s\n",
				 parts[i].name, named ? partname : "large candidate",
				 strerror(-probe));
			continue;
		}
		if (!probe) {
			rep_fmt("  %s (%s): ext4 magic mismatch\n", parts[i].name,
				 named ? partname : "large candidate");
			continue;
		}
		rep_fmt("  %s (%s, %lu KiB): ext4 -> trying mount\n",
			 parts[i].name, named ? partname : "large candidate",
			 parts[i].blocks);
		if (mount(dev, "/newroot", "ext4", 0, "") != 0) {
			int err = errno;

			rep_fmt("  mount %s failed: %s\n", parts[i].name,
				strerror(err));
			continue;
		}
		if (root_init_path(initpath, sizeof initpath)) {
			snprintf(rootdev, rootdev_len, "%s", dev);
			rep_fmt("  executable init candidate: %s\n", initpath);
			return 0;
		}
		rep_fmt("  %s mounted, but no executable init candidate found "
			 "(/sbin/init, dinit-init, dinit, sh)\n", parts[i].name);
		umount("/newroot");
	}
	return -1;
}

static void switch_root_and_exec(void)
{
	mkdir_p("/newroot/dev");
	mkdir_p("/newroot/proc");
	mkdir_p("/newroot/sys");
	mkdir_p("/newroot/tmp");

	/* pindahkan mount ke root baru supaya dinit melihat /dev, /proc, /sys */
	mount(NULL, "/newroot/dev", NULL, MS_MOVE, NULL);
	mount(NULL, "/newroot/proc", NULL, MS_MOVE, NULL);
	mount(NULL, "/newroot/sys", NULL, MS_MOVE, NULL);

	if (chroot("/newroot") != 0) {
		both_fmt("VXTUX-STEP: chroot failed: %s\n", strerror(errno));
		return;
	}
	if (chdir("/") != 0)
		;

	/* /init in the boot initramfs aliases /vxtux/init; it is not the
	 * Artix root's init.  Once chrooted, use whatever init the rootfs
	 * actually ships: /bin/dinit-run does NOT exist in this image (raw scan
	 * of the 5.87 GB super image: 0 hits), /sbin/init -> dinit-init does. */

	/*
	 * PID 1 inherits an empty environment from the kernel: only HOME and TERM
	 * are set, PATH is not.  /sbin/init is a symlink whose target may be a
	 * script, and dinit then starts service scripts (agetty on tty1..tty6 =
	 * the fbcon CLI) that resolve bare command names through PATH.  Set it
	 * here instead of hoping every script exports its own.
	 */
	putenv("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");
	putenv("TERM=linux");
	putenv("HOME=/root");
	int i;
	for (i = 0; init_candidates[i]; i++) {
		struct stat st;
		if (stat(init_candidates[i], &st) != 0 ||
		    access(init_candidates[i], X_OK) != 0)
			continue;
		both_fmt("VXTUX-STEP: chroot OK, exec %s\n", init_candidates[i]);
		execl(init_candidates[i], init_candidates[i], (char *)NULL);
		both_fmt("VXTUX-STEP: exec %s failed: %s\n", init_candidates[i],
			 strerror(errno));
	}
	both("VXTUX-STEP: every init candidate failed\n");
}

/* ---------- UMS512 clock modules (artix37) ---------- */

/*
 * Why load the clock tree here instead of built in (=y):
 *
 *   artix34  SPRD_COMMON_CLK/UMS512_CLK =m dan modulnya tidak pernah
 *            dimuat -> semua konsumen (eMMC, i2c, PMIC) deferred abadi ->
 *            kernel SELALU selamat sampai fbcon, layar hidup.
 *   artix35/36  =y -> clock tree ada sejak initcall -> konsumen probe
 *            terlalu dini -> salah satunya menggantung senyap SEBELUM
 *            fbcon -> layar buta, tanpa panic, tanpa jejak apa pun.
 *	 * Load the clock tree as modules AFTER fbcon is alive.  driver_register()
	 * from finit_module() retriggers deferred probing, so consumers get
	 * retried after the provider registers. Any hang is now visible on screen.
 *	 * Modules come from the same kernel build (matching vermagic) and are
	 * placed in /vxtux/ko/ by the boot_artix37 pipeline. CI artifacts are
	 * unsigned, so CONFIG_MODULE_SIG_FORCE is disabled; otherwise
	 * finit_module() rejects them with EKEYREJECTED.
 */
static int try_insmod(const char *path)
{
	int fd, ret;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	ret = (int)syscall(SYS_finit_module, fd, "", 0);
	if (ret < 0)
		ret = -errno;
	close(fd);
	return ret;
}

static int framebuffer_is_ready(void)
{
	struct stat st;

	/* The kernel registers fb0 during device_initcalls before /init runs. */
	return stat("/sys/class/graphics/fb0", &st) == 0;
}

static void wait_for_framebuffer(void)
{
	int waited;

	both("VXTUX-MOD: waiting for fb0/fbcon before late providers\n");
	for (waited = 0; waited < 180; waited += 2) {
		if (framebuffer_is_ready()) {
			both_fmt("VXTUX-MOD: fb0 registered after %ds; late probes allowed\n",
				 waited);
			return;
		}
		if (waited % 20 == 0)
			both_fmt("VXTUX-MOD: fb0 not registered (%ds)\n", waited);
		sleep(2);
	}
	both("VXTUX-MOD: fb0 timeout; continue boot with serial/diagnostic fallback\n");
}

static void load_clock_modules(void)
{
	static const char *const mods[] = {
		"/vxtux/ko/clk-sprd.ko",	/* framework before clock provider */
		"/vxtux/ko/ums512-clk.ko",	/* publish UMS512 clocks for deferred consumers */
		"/vxtux/ko/sc2730-regulator.ko", /* bind exact PMIC regulator child */
	};
	unsigned int i;

	wait_for_framebuffer();
	both("VXTUX-MOD: loading UMS512 clock modules after fbcon is visible\n");
	for (i = 0; i < sizeof mods / sizeof mods[0]; i++) {
		struct stat st;
		int ret;

		if (stat(mods[i], &st) != 0) {
			both_fmt("VXTUX-MOD: %s MISSING: %s\n", mods[i],
				 strerror(errno));
			continue;
		}
		ret = try_insmod(mods[i]);
		if (ret == 0)
			both_fmt("VXTUX-MOD: %s loaded (%lld bytes)\n",
				 mods[i], (long long)st.st_size);
		else
			both_fmt("VXTUX-MOD: %s FAILED: %s\n", mods[i],
				 strerror(-ret));
	}
	both("VXTUX-MOD: providers registered after fbcon; deferred consumers may retry\n");
}

int main(void)
{
	struct part *parts = g_parts;
	int np;
	char rootdev[96] = "";
	char banner[256];
	struct utsname u;
	int i;

	/* Set up pseudo-filesystems required for console, block nodes and sysfs. */
	mount("devtmpfs", "/dev", "devtmpfs", 0, "mode=0755");
	mount("proc", "/proc", "proc", 0, "");
	mount("sysfs", "/sys", "sysfs", 0, "");
	/* Early userspace opens console before printing; it must exist even if
	 * devtmpfs did not create the special node on this kernel config. */
	if (access("/dev/console", F_OK) != 0)
		mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1));

	console_fd = open("/dev/console", O_WRONLY | O_NOCTTY);
	if (console_fd < 0)
		console_fd = 1;

	report = malloc(REPORT_MAX);
	if (report)
		report[0] = 0;

	/* Register late clock providers before waiting for the eMMC host. */
	load_clock_modules();
	np = wait_for_storage(parts, 128, 180);
	np_parts = np;
	ensure_nodes(parts, np);
	save_early_dmesg(parts, np);

	uname(&u);
	snprintf(banner, sizeof banner,
		 "\n\n===== VXTUX ARTIX DIAG (initramfs) =====\n"
		 "kernel : %s %s\n"
		 "machine: %s\n"
		 "===============================\n",
		 u.release, u.version, u.machine);
	both(banner);

	rep("### VXTUXLOG1  Artix initramfs boot report (UMS512)\n");
	rep_fmt("kernel: %s %s\n", u.release, u.version);

	both("VXTUX-STEP: 1/5 partition list\n");
	rep("\n----- /proc/partitions -----\n");
	dump_file("/proc/partitions", 1);
	rep("----- end of /proc/partitions -----\n");
	both_fmt("VXTUX-STEP: %d partitions visible\n", np);

	/* The log file is now persisted; verify a separate temporary file. */
	both("VXTUX-STEP: 1b/5 write-readback test on the cache partition\n");
	write_readback_test(parts, np);

	both("VXTUX-STEP: 2/5 copying dmesg into the report\n");
	dump_kernel_log(1);

	/* Read prior-boot pstore records to the panel and report. */
	dump_pstore();

	/* Save the report before scanning or mounting the large super partition. */
	both("VXTUX-STEP: 2b/5 writing the first report to the cache partition\n");
	write_report_to_cache();

	rep("\n----- scanning partitions for ext4 -----\n");
	both("VXTUX-STEP: 3/5 looking for an ext4 rootfs\n");
	if (np > 0) {
		if (find_and_mount_root(parts, np, rootdev, sizeof rootdev) == 0) {
			both_fmt("VXTUX-STEP: root=%s (mounted)\n", rootdev);
			rep_fmt("ROOT: %s mounted, /sbin/init present\n", rootdev);
		} else {
			both("VXTUX-STEP: no ext4 rootfs with /sbin/init found\n");
			rep("ROOT: not found\n");
		}
	}

	both("VXTUX-STEP: 4/5 writing the report to the cache partition\n");
	write_report_to_cache();

	if (rootdev[0]) {
		both("VXTUX-STEP: 5/5 switching to the real root\n");
		switch_root_and_exec();
	}

	both("\n===== BOOT STOPPED IN THE INITRAMFS =====\n"
	     "Root was not found, so dinit was never started.\n"
	     "Dump the trail with spd_dump:\n"
	     "  read_part cache        -> /cache/boot_log.txt + vxtux_report.txt\n"
	     "  read_part common_rs1_a -> raw head \"### VXTUXRS1\" at offset 0\n"
	     "=========================================\n");

	for (i = 0; i < 3; i++)
		sync();
	sleep(3);
	/* Do not panic; keep the diagnostic message on screen. */
	for (;;)
		sleep(3600);
	return 0;
}
