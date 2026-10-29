// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_inittrace.c - draw the running initcall straight onto the panel.
 *
 * WHY THIS EXISTS (artix36)
 * -------------------------
 * artix35 boots hang in silence: LK logs "[start_linux] start linux at
 * PC:0x80080000" and the panel then stays frozen on the LK warning text
 * forever - no panic, no reboot, no eMMC write (cache stays byte-identical
 * to older dumps), no fbcon.  Every other diagnostic channel lives further
 * down the boot than the hang:
 *
 *   fbcon / vxtux_lkfb  - device_initcall, registered LATE in link order,
 *                         after most of the suspect drivers;
 *   pstore / flightrec  - need the eMMC, which needs the clock tree,
 *                         which is exactly what changed in artix35;
 *   /vxtux/init         - userspace, far below the hang.
 *
 * So this file reports through the one memory the kernel can always reach:
 * the framebuffer LK is already scanning out (lcd_base=0x9e000000,
 * RGB565 1200x2000, reserved by vxtux_beacon.c via vxtuxfb=...).  It hooks
 * do_one_initcall() (init/main.c - the single chokepoint every initcall
 * passes through, early/0..7) and prints each initcall's symbol name
 * BEFORE the call runs.  When the boot freezes, the last line on the panel
 * IS the initcall that hung:
 *
 *     VXTUX-ITRACE: kernel alive - watching initcalls, last line = hang
 *     000 builtin_branch_check
 *     ...
 *     612 sprd_i2c_probe+0x0/0x4a0    <- frozen line = culprit
 *
 * It also prints a banner from vxtux_beacon.c at early_param time, so a
 * boot that dies before the first initcall still changes the panel (banner
 * present + no initcall line = hang before do_initcalls; no banner at all
 * = hang before parse_early_param).
 *
 * Once vxtux_lkfb registers the fb console it calls vxtux_itrace_stop():
 * fbcon's wipe + 200-line dmesg replay takes over the panel (initcall_debug
 * output included), and two writers scribbling over each other would only
 * obscure that.
 *
 * COHERENCY (lesson from the removed beacon)
 * -------------------------------------------
 * Everything here is written through the cacheable linear map, so each
 * painted strip must be flushed to the point of coherency
 * (dcache_clean_poc) or the display controller never sees the pixels.
 *
 * COST
 * ----
 * One 16-px strip (3840 bytes) + one cache flush per line of text, a
 * full-screen memmove every 125 lines.  Negligible next to initcall_debug=1.
 */
#define pr_fmt(fmt) "vxtux-it: " fmt

#include <linux/font.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/cacheflush.h>
#include <asm/memory.h>

#define IT_PHYS		0x9e000000UL	/* lcd_base= bootargs LK (lihat vxtux_lkfb.c) */
#define IT_WIDTH	1200U
#define IT_HEIGHT	2000U
#define IT_STRIDE	(IT_WIDTH * 2U)	/* RGB565, 2 byte/piksel */
#define IT_SIZE		((size_t)IT_STRIDE * IT_HEIGHT)
#define IT_CACHELINE	64UL

static const struct font_desc *it_font;
static void *it_fb;
static unsigned int it_row, it_col;
static unsigned int it_rows, it_cols;
static unsigned int it_count;
static bool it_on;		/* banner sudah digambar, tracer aktif */
static bool it_dead;		/* fbcon mengambil alih layar - jangan sentuh lagi */

static void it_flush(unsigned long off, unsigned long len)
{
	unsigned long s = (unsigned long)it_fb + off;
	unsigned long e = s + len;

	dcache_clean_poc(s & ~(IT_CACHELINE - 1),
			 (e + IT_CACHELINE - 1) & ~(IT_CACHELINE - 1));
}

/* Hitamkan strip baris (menghapus sisa teks LK di bawahnya). */
static void it_clear_row(unsigned int row)
{
	size_t off = (size_t)row * it_font->height * IT_STRIDE;

	memset((char *)it_fb + off, 0, (size_t)it_font->height * IT_STRIDE);
	it_flush(off, (size_t)it_font->height * IT_STRIDE);
}

static void it_newline(void)
{
	it_flush((size_t)it_row * it_font->height * IT_STRIDE,
		 (size_t)it_font->height * IT_STRIDE);
	it_col = 0;
	it_row++;
	if (it_row >= it_rows) {
		size_t fh = it_font->height;

		/* Layar penuh: geser naik satu baris font. */
		memmove(it_fb, (char *)it_fb + fh * IT_STRIDE,
			IT_SIZE - fh * IT_STRIDE);
		memset((char *)it_fb + IT_SIZE - fh * IT_STRIDE, 0,
		       fh * IT_STRIDE);
		it_row = it_rows - 1;
		it_flush(0, IT_SIZE);
	}
	it_clear_row(it_row);
}

static void it_putc(char c)
{
	const unsigned char *g;
	unsigned int fw, fh, x, y;

	if (c == '\n') {
		it_newline();
		return;
	}
	if (c == '\r') {
		it_col = 0;
		return;
	}
	if (it_col >= it_cols)
		it_newline();

	fw = it_font->width;
	fh = it_font->height;
	if ((unsigned char)c >= it_font->charcount)
		c = '?';
	g = (const unsigned char *)it_font->data + (unsigned char)c * fh;

	for (y = 0; y < fh; y++) {
		u16 *px = (u16 *)((char *)it_fb +
				  ((size_t)(it_row * fh + y)) * IT_STRIDE) +
			  it_col * fw;

		for (x = 0; x < fw; x++) {
			if (g[y] & (0x80U >> x))
				px[x] = 0xffff;	/* piksel putih RGB565 */
		}
	}
	it_col++;
}

static void it_puts(const char *s)
{
	while (*s)
		it_putc(*s++);
}

/*
 * Banner: bersihkan seluruh panel lalu tulis baris pembuka.  Dipanggil
 * dari vxtux_beacon.c tepat setelah framebuffer di-reserve (waktu paling
 * awal yang memungkinkan), dan sebagai fallback dari hook initcall.
 */
void vxtux_itrace_banner(void)
{
	if (it_dead || it_on)
		return;
	it_font = find_font("VGA8x16");
	if (!it_font || !it_font->height || !it_font->width) {
		/*
		 * JANGAN menyerah permanen.  find_font() membaca array statik
		 * lib/fonts/fonts.c yang sudah ada sejak link (CONFIG_FONT_8x16=y),
		 * tetapi artix37 pernah melihatnya kosong; kalau suatu saat benar
		 * kosong, banner dicoba ulang di setiap initcall sampai fontnya
		 * terdaftar.  Menyetel it_dead di sini membuat tracer mustahil
		 * dipakai lagi di sisa boot (penyebab artix37 "nol informasi").
		 */
		return;
	}
	it_fb = (void *)__va(IT_PHYS);
	it_rows = IT_HEIGHT / it_font->height;
	it_cols = IT_WIDTH / it_font->width;
	it_row = 0;
	it_col = 0;
	it_count = 0;

	memset(it_fb, 0, IT_SIZE);
	dcache_clean_poc((unsigned long)it_fb,
			 (unsigned long)it_fb + IT_SIZE);
	it_on = true;
	it_puts("VXTUX-ITRACE: kernel alive - watching initcalls, last line = hang\n");
	/*
	 * Bila banner baru berhasil sekarang (font telat), sebagian initcall
	 * sudah lewat.  Baris ini menandai titik mulai supaya jumlah baris di
	 * bawahnya tidak disalahartikan sebagai nomor initcall absolut.
	 */
	if (it_count)
		it_puts("VXTUX-ITRACE: tracer started late, count resumed here\n");
	pr_info("panel tracer on: %ux%u font, %u rows, %u cols\n",
		it_font->width, it_font->height, it_rows, it_cols);
}

/* fbcon mengambil alih layar (vxtux_lkfb terdaftar) - berhenti menggambar. */
void vxtux_itrace_stop(void)
{
	it_on = false;
	it_dead = true;
}

/*
 * Hook dari do_one_initcall() di init/main.c: gambar SIMBOL initcall
 * sebelum dieksekusi, jadi bila boot membeku di dalam initcall tersebut,
 * namanya tetap terakhir terlihat di panel.
 */
void vxtux_itrace_initcall(initcall_t fn)
{
	char buf[96];

	if (it_dead)
		return;
	if (!it_on)
		vxtux_itrace_banner();
	if (!it_on)
		return;

	snprintf(buf, sizeof(buf), "%3u %ps\n", it_count++, fn);
	it_puts(buf);
}
