/*
 * BLDB - Berry Linux Debug Bridge (userspace daemon)
 *
 * Copyright (c) 2026 Berry Lab Foundation
 *
 * License: Berry Foundation Public v1 - MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * NOTE: Independent creation by Berry Lab Foundation.  Does NOT contain,
 * link, or derive from any proprietary Unisoc/Spreadtrum source code.  It
 * talks to the Linux kernel only through the standard open/read/write/poll
 * device API (/dev/bldb0 from drivers/vxtux/debug/bldb_core.c).
 *
 * ------------------------------------------------------------------------
 * Apa yang dilakukan daemon ini (dua pintu, satu kabel):
 *
 *   [PC] bldb-cli  <--USB ACM (/dev/ttyGS0)-->  [bldbd] <-->/dev/bldb0 (kernel)
 *
 *   * isi ring 16 MiB kernel  -> diteruskan ke PC (log live sejak detik 0)
 *   * perintah PC (help/stats/clear/mark/peek32/poke32) -> kernel, hasilnya
 *     masuk ring balik ke PC.
 *
 *   Daemon INI BOLEH DIMATIKAN kapan saja (kill / Ctrl+C / SIGTERM):
 *   inti kernel tetap merekam di ring selama kernel hidup, jadi log yang
 *   terlewat selama daemon mati tetap bisa disedot begitu daemon nyala lagi.
 *
 *   Perintah:
 *     bldbd [-f] [-d /dev/bldb0] [-t /dev/ttyGS0] [-v] [-h]
 *       -f  foreground (default: daemonize ke background)
 *       -d  perangkat inti BLDB      (default /dev/bldb0)
 *       -t  serial gadget USB ke PC  (default /dev/ttyGS0)
 *       -v  verbose ke stderr
 *       -h  bantuan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLDB_VERSION "1.0.0"
#define BLDB_DEVICE_DEFAULT "/dev/bldb0"
#define BLDB_TTY_DEFAULT "/dev/ttyGS0"
#define IO_BUF 65536

static volatile sig_atomic_t keep_running = 1;
static int verbose = 0;

static void on_signal(int sig)
{
	(void)sig;
	keep_running = 0;
}

static void logv(const char *fmt, ...)
{
	va_list ap;

	if (!verbose)
		return;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void usage(const char *argv0)
{
	printf("BLDB - Berry Linux Debug Bridge v%s (Berry Lab Foundation)\n",
	       BLDB_VERSION);
	printf("Usage: %s [OPTIONS]\n\n", argv0);
	printf("Options:\n");
	printf("  -d DEV   perangkat inti kernel BLDB (default %s)\n",
	       BLDB_DEVICE_DEFAULT);
	printf("  -t TTY   serial gadget USB ke PC   (default %s)\n",
	       BLDB_TTY_DEFAULT);
	printf("  -f       foreground (default: daemon background)\n");
	printf("  -v       verbose\n");
	printf("  -h       help ini\n\n");
	printf("Perintah dari PC (lewat bldb-cli, masuk ke kernel):\n");
	printf("  help | stats | clear | mark <teks>\n");
	printf("  peek32 <hex>              baca RAM (hanya RAM valid)\n");
	printf("  poke32 <hex> <hex>        tulis RAM (dilarang di teks kernel)\n\n");
	printf("Daemon ini boleh dimatikan (kill) kapan saja; ring kernel\n");
	printf("tetap merekam selama kernel hidup.\n\n");
	printf("License: Berry Foundation Public v1 - MIT\n");
}

static int open_retry(const char *path, int flags, int interval_ms)
{
	int fd;

	while (keep_running) {
		fd = open(path, flags | O_NOCTTY | O_CLOEXEC);
		if (fd >= 0)
			return fd;
		logv("bldbd: %s belum siap (%s), coba lagi...\n",
		     path, strerror(errno));
		usleep(interval_ms * 1000);
	}
	return -1;
}

int main(int argc, char **argv)
{
	const char *bldb_dev = BLDB_DEVICE_DEFAULT;
	const char *tty_dev = BLDB_TTY_DEFAULT;
	int foreground = 0;
	int bldb_fd, tty_fd;
	int opt;

	while ((opt = getopt(argc, argv, "d:t:fvh")) != -1) {
		switch (opt) {
		case 'd': bldb_dev = optarg; break;
		case 't': tty_dev = optarg; break;
		case 'f': foreground = 1; break;
		case 'v': verbose = 1; break;
		case 'h': usage(argv[0]); return 0;
		default: usage(argv[0]); return 1;
		}
	}

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	signal(SIGPIPE, SIG_IGN);

	/* Buka inti kernel dulu: kalau /dev/bldb0 tidak ada, daemon memang
	 * tidak ada gunanya (artinya kernel tanpa CONFIG_VXTUX_BLDB). */
	bldb_fd = open(bldb_dev, O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (bldb_fd < 0) {
		fprintf(stderr, "bldbd: tidak bisa membuka %s: %s\n",
			bldb_dev, strerror(errno));
		fprintf(stderr, "bldbd: pastikan kernel dibangun dengan CONFIG_VXTUX_BLDB=y\n");
		return 1;
	}

	if (!foreground) {
		if (daemon(0, 0) != 0) {
			perror("bldbd: daemon");
			return 1;
		}
	}

	fprintf(stderr, "bldbd v%s siap (pid %d): %s <-> PC\n",
		BLDB_VERSION, (int)getpid(), tty_dev);

	/* Tunggu port serial gadget; kalau belum hidup, biarkan poll yang
	 * mencoba ulang tiap 2 detik (gadget dibind oleh init belakangan). */
	tty_fd = open_retry(tty_dev, O_RDWR, 2000);
	if (tty_fd < 0) {
		fprintf(stderr, "bldbd: berhenti (%s)\n", strerror(errno));
		return 1;
	}

	/* Umumkan diri ke PC sekali terhubung. */
	dprintf(tty_fd, "[BLDB:DEVICE:VXTUX-UMS512:BLDB-v%s:READY]\n",
		BLDB_VERSION);

	while (keep_running) {
		struct pollfd pfd[2];
		int n, ret;
		char *buf;

		pfd[0].fd = bldb_fd;
		pfd[0].events = POLLIN;
		pfd[1].fd = tty_fd;
		pfd[1].events = POLLIN;

		ret = poll(pfd, 2, 1000);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("bldbd: poll");
			break;
		}
		if (ret == 0)
			continue;

		buf = malloc(IO_BUF);
		if (!buf)
			break;

		/* 1) Ring kernel -> PC (log live) */
		if (pfd[0].revents & POLLIN) {
			while ((n = read(bldb_fd, buf, IO_BUF)) > 0) {
				if (write(tty_fd, buf, n) != n) {
					logv("bldbd: tulis ke PC gagal (%s), buka ulang tty\n",
					     strerror(errno));
					break;
				}
			}
		}

		/* 2) Perintah PC -> kernel */
		if (pfd[1].revents & POLLIN) {
			while ((n = read(tty_fd, buf, IO_BUF)) > 0) {
				if (write(bldb_fd, buf, n) != n) {
					logv("bldbd: tulis perintah ke kernel gagal (%s)\n",
					     strerror(errno));
					break;
				}
			}
			if (n == 0) {
				/* PC menutup port: buka lagi, jangan mati */
				logv("bldbd: port PC tertutup, tunggu ulang...\n");
				close(tty_fd);
				tty_fd = open_retry(tty_dev, O_RDWR, 2000);
				if (tty_fd < 0)
					break;
				dprintf(tty_fd,
					"[BLDB:DEVICE:VXTUX-UMS512:BLDB-v%s:RECONNECTED]\n",
					BLDB_VERSION);
			}
		}

		/* port hilang (unbind gadget)? */
		if (pfd[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			logv("bldbd: tty hangup, buka ulang...\n");
			close(tty_fd);
			tty_fd = open_retry(tty_dev, O_RDWR, 2000);
			if (tty_fd < 0)
				break;
		}

		free(buf);
	}

	fprintf(stderr, "bldbd: berhenti bersih (ring kernel tetap merekam)\n");
	if (tty_fd >= 0)
		close(tty_fd);
	close(bldb_fd);
	return 0;
}
