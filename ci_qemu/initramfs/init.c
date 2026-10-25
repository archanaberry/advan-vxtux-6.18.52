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
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#define CACHE_BLOCKS_1K 102400   /* partisi cache = 100 MiB -> 102400 KiB */
#define LOG_MAX        (1024 * 1024)
#define REPORT_MAX     (2 * 1024 * 1024)

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
		both_fmt("  (klogctl gagal: %s)\n", strerror(errno));
		free(b);
		return;
	}
	b[n] = 0;
	rep("----- dmesg (ring buffer kernel, awal) -----\n");
	rep(b);
	if (b[n - 1] != '\n')
		rep("\n");
	rep("----- akhir dmesg -----\n");

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
		con("\n----- 60 baris terakhir dmesg -----\n");
		con(q);
		con("\n----- akhir dmesg -----\n");
	}
	free(b);
}

/* ---------- partisi ---------- */

struct part {
	char name[64];
	unsigned long blocks;   /* dalam KiB */
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
		n++;
	}
	fclose(f);
	return n;
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

/* tulis laporan ke partisi cache (100 MiB, tidak dipakai Artix) */
static void write_report_to_cache(const struct part *parts, int np)
{
	int i, fd, found = 0;

	for (i = 0; i < np; i++) {
		char dev[96];

		if (parts[i].blocks != CACHE_BLOCKS_1K)
			continue;
		found = 1;
		snprintf(dev, sizeof dev, "/dev/%s", parts[i].name);
		both_fmt("\nVXTUX-STEP: menulis laporan ke %s\n", dev);
		fd = open(dev, O_WRONLY);
		if (fd < 0) {
			both_fmt("VXTUX-STEP: gagal buka %s: %s\n", dev,
				 strerror(errno));
			continue;
		}
		if (pwrite(fd, report, report_len, 0) != (ssize_t)report_len)
			both_fmt("VXTUX-STEP: tulis %s tidak penuh: %s\n", dev,
				 strerror(errno));
		else
			both_fmt("VXTUX-STEP: OK %zu byte di %s (offset 0)\n",
				 report_len, dev);
		fsync(fd);
		close(fd);
		break;
	}
	if (!found)
		both("VXTUX-STEP: partisi cache (100 MiB) tidak ditemukan\n");
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
			rep_fmt("  %-16s %8lu KiB  bukan ext4\n",
				parts[i].name, parts[i].blocks);
			continue;
		}
		rep_fmt("  %-16s %8lu KiB  ext4 -> coba mount\n",
			parts[i].name, parts[i].blocks);
		if (mount(dev, "/newroot", "ext4", 0, "") == 0) {
			if (access("/newroot/sbin/init", X_OK) == 0) {
				snprintf(rootdev, rootdev_len, "%s", dev);
				return 0;
			}
			rep_fmt("  %s ter-mount tapi /sbin/init tidak ada\n",
				parts[i].name);
			umount("/newroot");
		} else {
			rep_fmt("  mount %s gagal: %s\n", parts[i].name,
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
		both_fmt("VXTUX-STEP: chroot gagal: %s\n", strerror(errno));
		return;
	}
	if (chdir("/") != 0)
		;
	both("VXTUX-STEP: chroot OK, exec /sbin/init\n");

	execl("/sbin/init", "/sbin/init", (char *)NULL);
	execl("/bin/sh", "sh", (char *)NULL);
	both_fmt("VXTUX-STEP: exec /sbin/init gagal: %s\n", strerror(errno));
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
		 "mesin  : %s\n"
		 "===============================\n",
		 u.release, u.version, u.machine);
	both(banner);

	rep("### VXTUXLOG1  laporan boot initramfs Artix (UMS512)\n");
	rep_fmt("kernel: %s %s\n", u.release, u.version);

	both("VXTUX-STEP: 1/5 daftar partisi\n");
	rep("\n----- /proc/partitions -----\n");
	dump_file("/proc/partitions", 0);
	rep("----- akhir /proc/partitions -----\n");
	np = list_partitions(parts, 128);
	both_fmt("VXTUX-STEP: %d partisi terlihat\n", np);

	both("VXTUX-STEP: 2/5 salin dmesg ke laporan\n");
	dump_kernel_log(1);

	rep("\n----- pindai partisi ext4 -----\n");
	both("VXTUX-STEP: 3/5 cari rootfs ext4\n");
	if (np > 0) {
		if (find_and_mount_root(parts, np, rootdev, sizeof rootdev) == 0) {
			both_fmt("VXTUX-STEP: root=%s (ter-mount)\n", rootdev);
			rep_fmt("ROOT: %s ter-mount, /sbin/init ada\n", rootdev);
		} else {
			both("VXTUX-STEP: rootfs ext4 dengan /sbin/init TIDAK ditemukan\n");
			rep("ROOT: tidak ditemukan\n");
		}
	}

	both("VXTUX-STEP: 4/5 tulis laporan ke partisi cache\n");
	write_report_to_cache(parts, np);

	if (rootdev[0]) {
		both("VXTUX-STEP: 5/5 pindah ke root sebenarnya\n");
		switch_root_and_exec();
	}

	both("\n===== BOOT BERHENTI DI INITRAMFS =====\n"
	     "Laporan lengkap sudah ditulis ke partisi cache (100 MiB).\n"
	     "Dump lewat spd_dump: read_part cache, lalu cari teks VXTUXLOG1.\n"
	     "=====================================\n");

	for (i = 0; i < 3; i++)
		sync();
	sleep(3);
	/* jangan panic: biarkan layar menampilkan pesan di atas */
	for (;;)
		sleep(3600);
	return 0;
}
