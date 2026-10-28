// SPDX-License-Identifier: GPL-2.0
//
// /vxtux/init -- init statis arm64 untuk initramfs BAWAAN kernel Artix UMS512.
//
// Kenapa ada: kernel 6.18 di tablet ini tidak punya UART yang bisa dibaca
// (earlycon lama menunjuk 0x71400000 = kontroler eMMC, bukan UART) dan tidak
// punya console=tty0, jadi setiap kegagalan boot tampak "diam di logo".
//
// Init ini melakukan tiga hal, semuanya tanpa userspace lain:
//   1. mencatat SELURUH isi ring buffer kernel (klogctl READ_ALL) + hasil
//      pemeriksaan partisi ke partisi `cache` (100 MiB, tidak dipakai Artix)
//      supaya bisa di-dump lewat spd_dump tanpa perlu UART/layar;
//   2. mencetak ringkasan yang sama ke /dev/console (fbcon -> panel) supaya
//      kelihatan di layar kalau DRM/panel hidup;
//   3. mencari partisi root ext4 (super) dengan memeriksa superblock tiap
//      partisi mmcblk0p*, mount, lalu chroot + exec /sbin/init.
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
/*
 * Laporan DITULIS DI SINI, bukan offset 0: pstore/blk (CONFIG_PSTORE_BLK,
 * best_effort) memakai awal partisi yang sama untuk catatan console/kmsg
 * sehingga menimpa laporan (atau sebaliknya).  4 MiB jauh di luar zona
 * pstore (~512 KiB) dan masih jauh di bawah 100 MiB partisi cache.
 */
#define REPORT_OFFSET  (4 * 1024 * 1024)
/*
 * Zona uji tulis-baca-balik (artix34): 6 MiB jatuh di celah kosong di
 * antara zona pstore (~0-512 KiB), laporan (4 MiB) dan flightrec (8 MiB).
 */
#define TEST_OFFSET    (6 * 1024 * 1024)

static char *report;             /* kumpulan teks yang ditulis ke cache */
static size_t report_len;
static int console_fd = -1;

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
		/* di panel cukup 60 baris terakhir */
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

/* ---------- partisi ---------- */

struct part {
	char name[64];
	unsigned long blocks;   /* dalam KiB */
	unsigned major, minor;  /* untuk fallback mknod */
};

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
}

/*
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

static int is_ext4(const char *dev)
{
	unsigned char sb[2];
	int fd = open(dev, O_RDONLY);
	int ok = 0;

	if (fd < 0)
		return 0;
	/* ext4 superblock magic 0xEF53 ada di offset 1024 + 0x38 */
	if (pread(fd, sb, 2, 1024 + 0x38) == 2)
		ok = (sb[0] == 0x53 && sb[1] == 0xEF);
	close(fd);
	return ok;
}

static void mkdir_p(const char *p)
{
	mkdir(p, 0755);
}

/*
 * pstore -> layar + laporan (2026-10-27)
 * ----------------------------------------
 * Kernel memakai CONFIG_PSTORE + PSTORE_BLK (/dev/mmcblk0p49 = partisi
 * cache, sesuai dmesg stok 5.4: "mmcblk0p49" 112 kemunculan) +
 * PSTORE_CONSOLE.  Catatan console/kmsg boot SEBELUMNYA (termasuk teks
 * panic) tersimpan di sana; dengan `pstore_blk.best_effort=1` di cmdline
 * backend benar-benar menulis (tanpa flag itu pstore_blk init batal di
 * __best_effort_init).
 *
 * Fungsi ini MEMBACA-nya kembali: mount -t pstore lalu cetak console-* dan
 * dmesg-* ke /dev/console (fbcon -> panel) dan ke laporan cache.  Dengan
 * begitu walau boot kali ini berantakan, jejak boot sebelumnya tetap
 * kelihatan di layar tanpa perlu dump ulang.
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
		n++;
		/* laporan: seluruh isi; layar: 8 KiB pertama supaya fbcon
		 * tidak terusir oleh ratusan KiB catatan lama */
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

/* tulis laporan ke satu node blok; 1 = berhasil */
static int write_report_to(const char *dev)
{
	char back[12];
	int fd = open(dev, O_RDWR);

	if (fd < 0) {
		both_fmt("VXTUX-STEP: open %s failed: %s\n", dev,
			 strerror(errno));
		return 0;
	}
	if (pwrite(fd, report, report_len, REPORT_OFFSET) !=
	    (ssize_t)report_len) {
		both_fmt("VXTUX-STEP: short write to %s: %s\n", dev,
			 strerror(errno));
		close(fd);
		return 0;
	}
	/* Baca-balik kepala laporan "### VXTUXLOG1": bukti byte sampai. */
	if (pread(fd, back, sizeof back, REPORT_OFFSET) !=
	    (ssize_t)sizeof back ||
	    memcmp(back, "### VXTUXLOG1", sizeof back) != 0) {
		both_fmt("VXTUX-STEP: readback FAILED on %s: %s\n", dev,
			 strerror(errno));
		close(fd);
		return 0;
	}
	both_fmt("VXTUX-STEP: OK %zu bytes at %s (offset 0x%x, readback verified)\n",
		 report_len, dev, REPORT_OFFSET);
	fsync(fd);
	close(fd);
	return 1;
}

/* tulis laporan ke partisi cache (100 MiB, tidak dipakai Artix).
 * Fallback: nama perangkat tetap mmcblk0p49 (tabel partisi stok, cache =
 * partisi ke-48 -> p49) supaya laporan tetap tersimpan walau
 * /proc/partitions tidak sempat terbaca penuh. */
static void write_report_to_cache(const struct part *parts, int np)
{
	int i, ok = 0;

	for (i = 0; i < np && !ok; i++) {
		char dev[96];

		if (parts[i].blocks != CACHE_BLOCKS_1K)
			continue;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		both_fmt("\nVXTUX-STEP: writing report to %s\n", dev);
		ok = write_report_to(dev);
	}
	if (!ok) {
		static const char *cand[] = { "/dev/mmcblk0p49",
					      "/dev/mmcblk0p48", NULL };
		both("VXTUX-STEP: cache not found by size, trying fixed names\n");
		for (i = 0; cand[i] && !ok; i++)
			ok = write_report_to(cand[i]);
	}
	if (!ok)
		both("VXTUX-STEP: FAILED to write the report anywhere\n");
}

/*
 * Uji tulis-baca-balik kanal cache (artix34)
 * -------------------------------------------
 * Bukti dari bedah dump artix33: partisi cache 100% identik antara dump
 * pra-flash dan pasca-boot - ketiga penulis (pstore_blk, flightrec, init
 * ini) menulis ke mmcblk0p49 dan semuanya nol byte.  Uji ini menjawab
 * "kenapa" tanpa perlu dump: tanda tangan ditulis di zona 6 MiB lalu
 * dibaca balik.  Hasil dicetak ke console (fbcon) DAN ke laporan:
 *   VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49
 * atau
 *   VXTUX-WRITE-TEST: FAIL <open|write|readback> ...: <errno>
 */
static void write_readback_test(const struct part *parts, int np)
{
	static const char sig[16] = "VXTUXWRT\x01" "UMS512!";
	char dev[96], back[16];
	int fd, i;

	dev[0] = 0;
	for (i = 0; i < np; i++) {
		if (parts[i].blocks == CACHE_BLOCKS_1K) {
			snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
			break;
		}
	}
	if (!dev[0])
		snprintf(dev, sizeof dev, "/dev/mmcblk0p49");

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		both_fmt("VXTUX-WRITE-TEST: FAIL open %s: %s\n", dev,
			 strerror(errno));
		return;
	}
	if (pwrite(fd, sig, sizeof sig, TEST_OFFSET) != (ssize_t)sizeof sig) {
		both_fmt("VXTUX-WRITE-TEST: FAIL write %s: %s\n", dev,
			 strerror(errno));
		close(fd);
		return;
	}
	fsync(fd);
	if (pread(fd, back, sizeof back, TEST_OFFSET) !=
	    (ssize_t)sizeof back) {
		both_fmt("VXTUX-WRITE-TEST: FAIL readback %s: %s\n", dev,
			 strerror(errno));
		close(fd);
		return;
	}
	close(fd);
	if (memcmp(back, sig, sizeof sig) != 0) {
		both("VXTUX-WRITE-TEST: FAIL readback mismatch "
		     "(wrote one thing, read another)\n");
		return;
	}
	both_fmt("VXTUX-WRITE-TEST: OK open+write+readback on %s\n", dev);
}

/* ---------- cari + pasang root ---------- */

static int find_and_mount_root(const struct part *parts, int np, char *rootdev,
			       size_t rootdev_len)
{
	int i;

	mkdir_p("/newroot");
	for (i = 0; i < np; i++) {
		char dev[96];

		if (!strstr(parts[i].name, "mmcblk") ||
		    !strchr(parts[i].name, 'p'))
			continue;
		if (parts[i].blocks < 1024 * 1024)   /* < 1 GiB: bukan rootfs */
			continue;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		if (!is_ext4(dev)) {
			rep_fmt("  %-16s %8lu KiB  not ext4\n",
				parts[i].name, parts[i].blocks);
			continue;
		}
		rep_fmt("  %-16s %8lu KiB  ext4 -> trying mount\n",
			parts[i].name, parts[i].blocks);
		if (mount(dev, "/newroot", "ext4", 0, "") == 0) {
			if (access("/newroot/sbin/init", X_OK) == 0) {
				snprintf(rootdev, rootdev_len, "%s", dev);
				return 0;
			}
			rep_fmt("  %s mounted but /sbin/init is missing\n",
				parts[i].name);
			umount("/newroot");
		} else {
			rep_fmt("  mount %s failed: %s\n", parts[i].name,
				strerror(errno));
		}
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

	/* Artix (proot-distro) memakai dinit, BUKAN sysvinit/upstart:
	 *   /init          -> skel dinit (exec dinit-init / dinit-initramfs)
	 *   /bin/dinit-run -> launcher dinit khas Artix
	 *   /sbin/init     -> symlink -> dinit (fallback systemd/gentoo)
	 * Urutan ini penting: coba /init dulu (paling autentik), lalu dinit-run,
	 * baru /sbin/init + /bin/sh.  Bug sebelumnya: hanya /sbin/init + /bin/sh,
	 * padahal /sbin/init TIDAK ADA di rootfs Artix -> exec gagal -> kernel
	 * panic "VFS: Unable to mount root fs". */
	static const char *inits[] = {
		"/init", "/bin/dinit-run", "/sbin/init",
		"/sbin/dinit", "/bin/dinit", "/bin/sh", NULL
	};
	int i;
	for (i = 0; inits[i]; i++) {
		struct stat st;
		if (stat(inits[i], &st) != 0)
			continue;
		both_fmt("VXTUX-STEP: chroot OK, exec %s\n", inits[i]);
		execl(inits[i], inits[i], (char *)NULL);
		both_fmt("VXTUX-STEP: exec %s failed: %s\n", inits[i],
			 strerror(errno));
	}
	both("VXTUX-STEP: every init candidate failed\n");
}

/* ---------- modul clock UMS512 (artix37) ---------- */

/*
 * Mengapa clock tree dimuat DI SINI, bukan builtin (=y):
 *
 *   artix34  SPRD_COMMON_CLK/UMS512_CLK =m dan modulnya tidak pernah
 *            dimuat -> semua konsumen (eMMC, i2c, PMIC) deferred abadi ->
 *            kernel SELALU selamat sampai fbcon, layar hidup.
 *   artix35/36  =y -> clock tree ada sejak initcall -> konsumen probe
 *            terlalu dini -> salah satunya menggantung senyap SEBELUM
 *            fbcon -> layar buta, tanpa panic, tanpa jejak apa pun.
 *
 * Jadi di sini clock tree dimuat sebagai MODUL, SETELAH fbcon hidup dan
 * layar sudah bisa bicara.  driver_register() dari finit_module() memicu
 * driver_deferred_probe_trigger(), jadi konsumen yang tadinya deferred
 * langsung dicoba ulang begitu provider clock terdaftar.  Kalau salah
 * satunya menggantung, gantungnya terjadi SESUDAH semua pesan di layar
 * ini tercetak - bukan boot buta lagi.
 *
 * Modul berasal dari build kernel yang sama (vermagic cocok) dan ditaruh di
 * /vxtux/ko/ oleh pipeline boot_artix37.  Artefak CI ternyata keluar TANPA
 * tanda tangan (kunci certs/signing_key.pem tidak digenerate saat
 * `make modules`), jadi CONFIG_MODULE_SIG_FORCE dimatikan di
 * enhanced_config.cfg; tanpa itu finit_module() menolak keduanya dengan
 * EKEYREJECTED dan clock tree tidak pernah terdaftar.
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

static void load_clock_modules(void)
{
	static const char *const mods[] = {
		"/vxtux/ko/clk-sprd.ko",	/* framework clock SPRD dulu */
		"/vxtux/ko/ums512-clk.ko",	/* lalu ap-clk + apapb-gate */
	};
	unsigned int i;

	both("VXTUX-MOD: loading UMS512 clock modules (late, fbcon is up)\n");
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
	both("VXTUX-MOD: deferred probes retriggered, waiting for eMMC\n");
}

/* statis: stack PID1 di kernel hanya 16 KiB, jangan taruh array besar di sana */
static struct part g_parts[128];

int main(void)
{
	struct part *parts = g_parts;
	int np;
	char rootdev[96] = "";
	char banner[256];
	struct utsname u;
	int i;

	/* devtmpfs dulu supaya /dev/console pasti ada sebelum kita bicara */
	mount("devtmpfs", "/dev", "devtmpfs", 0, "mode=0755");
	mount("proc", "/proc", "proc", 0, "");
	mount("sysfs", "/sys", "sysfs", 0, "");

	console_fd = open("/dev/console", O_WRONLY | O_NOCTTY);
	if (console_fd < 0)
		console_fd = 1;

	report = malloc(REPORT_MAX);
	if (report)
		report[0] = 0;

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

	/* artix37: clock tree dimuat sebagai modul, setelah fbcon hidup. */
	load_clock_modules();

	both("VXTUX-STEP: 1/5 partition list\n");
	rep("\n----- /proc/partitions -----\n");
	dump_file("/proc/partitions", 1);
	rep("----- end of /proc/partitions -----\n");

	/*
	 * Tunggu eMMC muncul (maks 180 detik, layar diperbarui tiap 10 s).
	 * Tanpa ini semua langkah berikut bisa gagal serentak hanya karena
	 * host eMMC lebih lambat terdaftar daripada init berjalan.
	 */
	np = wait_for_storage(parts, 128, 180);
	both_fmt("VXTUX-STEP: %d partitions visible\n", np);
	ensure_nodes(parts, np);

	/*
	 * Uji kanal tulis SEBELUM laporan ditulis: kalau gagal, errno-nya
	 * langsung terlihat di layar (fbcon) tanpa perlu dump partisi cache.
	 */
	both("VXTUX-STEP: 1b/5 write-readback test on the cache partition\n");
	write_readback_test(parts, np);

	both("VXTUX-STEP: 2/5 copying dmesg into the report\n");
	dump_kernel_log(1);

	/* pstore boot sebelumnya (panic/console lama) -> layar + laporan */
	dump_pstore();

	/* LAPORAN AWAL ditulis di sini, sebelum pindai/mount ext4. Langkah 3/5
	 * adalah titik yang paling mungkin menggantung (probe/port eMMC belum
	 * stabil). Bila mount macet, dmesg yang sudah terkumpul tetap tersimpan
	 * di partisi cache dan bisa di-dump lewat spd_dump tanpa layar. */
	both("VXTUX-STEP: 2b/5 writing the first report to the cache partition\n");
	write_report_to_cache(parts, np);

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
	write_report_to_cache(parts, np);

	if (rootdev[0]) {
		both("VXTUX-STEP: 5/5 switching to the real root\n");
		switch_root_and_exec();
	}

	both("\n===== BOOT STOPPED IN THE INITRAMFS =====\n"
	     "The full report was written to the cache partition (100 MiB).\n"
	     "Dump it with spd_dump: read_part cache, then look for VXTUXLOG1.\n"
	     "=========================================\n");

	for (i = 0; i < 3; i++)
		sync();
	sleep(3);
	/* jangan panic: biarkan layar menampilkan pesan di atas */
	for (;;)
		sleep(3600);
	return 0;
}
