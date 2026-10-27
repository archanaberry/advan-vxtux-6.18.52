// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_lkfb.c — framebuffer LK didaftarkan sebagai fbdev agar fbcon bisa
 * mencetak teks kernel di layar panel UMS512 (HX83102E, 1200x2000).
 *
 * MASALAH
 * -------
 * Tablet ini tidak mengekspos UART sebagai COM, jadi teks kernel hanya bisa
 * dibaca dari layar.  Config sudah memuat CONFIG_FRAMEBUFFER_CONSOLE=y dan
 * cmdline memuat console=tty0, tetapi fbcon butuh sebuah PERANGKAT fb untuk
 * diikat — dan di tree ini tidak ada satu pun yang pernah mendaftar:
 *
 *   * CONFIG_FB_SIMPLE = n  (stok tidak menyediakan node DT
 *     simple-framebuffer; LK membangun DTB sendiri dari multi-DTB
 *     "sprd,sc-id=ums512 1h10" lalu men-merge dtbo, tanpa node fb)
 *   * DRM SPRD DPU tidak pernah probe (layar tetap memindai buffer LK)
 *
 * Akibatnya `console=tty0` mengirim semua printk ke konsol maya yang tidak
 * pernah punya layar: layar diam di logo LK walau kernel sudah jalan jauh.
 *
 * SOLUSI
 * ------
 * Daftarkan framebuffer yang SUDAH dipindai DISPC (lcd_base=0x9e000000,
 * dari bootargs LK: `lcd_base=9e000000 lcd_size=2000x1200
 * lcd_name=lcd_hx83102e_qc_1200x2000`) sebagai struct fb_info.  Begitu
 * register_framebuffer() jalan, fbcon mengikat diri dan seluruh pesan
 * console=tty0 (loglevel=8 ignore_loglevel) tercetak di panel — persis di
 * bawah baris terakhir LK seperti
 * `INFO: LOCK FLAG IS : UNLOCK!!!`.
 *
 * Geometri diambil dari log LK sendiri + bukti uji pita artix21-24:
 *   lcd_console: have 150/125 col/rws on scr 1200x2000  -> 150*8=1200,
 *   125*16=2000 (font 8x16) -> layar 1200x2000, bukan 1920.
 *   [LCD] Using LCD frambuffer at 0x9e000000
 *   SCANOUT BUKAN 32 bpp: DPU membaca RGB565 16-bit (dua piksel per
 *   word 32-bit).  Bukti: stub mk_boot_artix.py menulis 0xF800F800 ->
 *   layar MERAH, 0x07E007E0 -> HIJAU, dan 4 pita x 300.000 word
 *   (=1,2 MB = 600 baris @ 1200 px) membagi layar persis 4 -> stride
 *   1200*2 = 2400.  "fb size 0x9e4000" hanya alokasi 4 B/piksel milik
 *   LK, bukan format scanout-nya.
 *
 * Memori DIPERSENTRUMKAN lewat early_param `vxtuxfb=0x9d000000,0x9e000000`
 * di vxtux_beacon.c (memblock_reserve 0x9e4000 tiap alamat), jadi bagian
 * ini tidak lagi dialokasikan buddy allocator.  Tanpa reservasi itu baris
 * console akan tercampur dengan data userspace.
 *
 * Saat DRM SPRD DPU akhirnya probe, drm_fbdev membuat fb miliknya sendiri;
 * fbcon (FRAMEBUFFER_CONSOLE_DETECT_PRIMARY) pindah ke sana sehingga layar
 * di-refresh oleh KMS — fb ini sengaja TIDAK di-unregister agar tidak ada
 * jendela tanpa konsol.
 */
#define pr_fmt(fmt) "vxtux-lkfb: " fmt

#include <linux/fb.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

#include <asm/cacheflush.h>
#include <asm/memory.h>

#define LKFB_PHYS	0x9e000000UL	/* lcd_base= bootargs LK */
#define LKFB_WIDTH	1200U
#define LKFB_HEIGHT	2000U
#define LKFB_BPP	16U		/* RGB565 — terbukti uji pita artix21-24 */
#define LKFB_STRIDE	(LKFB_WIDTH * (LKFB_BPP / 8))
#define LKFB_SIZE	((size_t)LKFB_STRIDE * LKFB_HEIGHT)

struct lkfb_par {
	u32 palette[16];
};

/*
 * Sinkronisasi D-cache (2026-10-27)
 * ----------------------------------
 * Bila memremap(MEMREMAP_WC) ditolak (0x9e000000 berada di dalam /memory),
 * kita jatuh ke __va() yang CACHEABLE: fbcon menulis lewat screen_base ke
 * D-cache, DISPC membaca DRAM -> tulisan lama/sampah yang tak pernah
 * ter-flush muncul sebagai garis-garis, kolom kotor dan teks pecah.
 *
 * Perbaikan: setiap gambar fbcon (imageblit/fillrect/copyarea) dibersihkan
 * sampai PoC untuk baris-baris yang disentuh.  flush_dcache_page() tidak
 * dipakai karena fbcon tidak menandai halaman; kita flush rentang barisnya
 * langsung.
 */
static bool lkfb_need_flush;

static void lkfb_flush_rows(struct fb_info *info, unsigned int dy,
			    unsigned int height)
{
	unsigned long start, end;
	unsigned long base = (unsigned long)info->screen_base;

	if (!lkfb_need_flush || height == 0)
		return;
	start = base + (unsigned long)dy * info->fix.line_length;
	end = base + (unsigned long)(dy + height) * info->fix.line_length;
	if (end > base + LKFB_SIZE)
		end = base + LKFB_SIZE;
	dcache_clean_poc(start, end);
}

static void lkfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	cfb_imageblit(info, image);
	lkfb_flush_rows(info, image->dy, image->height);
}

static void lkfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	cfb_fillrect(info, rect);
	lkfb_flush_rows(info, rect->dy, rect->height);
}

static void lkfb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	cfb_copyarea(info, area);
	/* sumber maupun tujuan baris yang sama dibersihkan */
	lkfb_flush_rows(info, area->dy, area->height);
}

static int lkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	/* Geometri dipaku: panel tidak bisa diubah oleh fbcon. */
	var->xres = LKFB_WIDTH;
	var->yres = LKFB_HEIGHT;
	var->xres_virtual = LKFB_WIDTH;
	var->yres_virtual = LKFB_HEIGHT;
	var->bits_per_pixel = LKFB_BPP;
	var->grayscale = 0;

	/* RGB565: [15:11]=R, [10:5]=G, [4:0]=B, tanpa alpha */
	var->red.offset = 11;
	var->red.length = 5;
	var->green.offset = 5;
	var->green.length = 6;
	var->blue.offset = 0;
	var->blue.length = 5;
	var->transp.offset = 0;
	var->transp.length = 0;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->height = -1;
	var->width = -1;
	var->accel_flags = 0;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->rotate = 0;
	return 0;
}

static int lkfb_set_par(struct fb_info *info)
{
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux = 0;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = LKFB_STRIDE;
	info->fix.xpanstep = 0;
	info->fix.ypanstep = 0;
	info->fix.ywrapstep = 0;
	info->fix.mmio_start = 0;
	info->fix.mmio_len = 0;
	return 0;
}

static int lkfb_setcolreg(unsigned int regno, unsigned int red,
			  unsigned int green, unsigned int blue,
			  unsigned int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 v;

	if (regno >= 16)
		return -EINVAL;

	red >>= 16 - info->var.red.length;
	green >>= 16 - info->var.green.length;
	blue >>= 16 - info->var.blue.length;
	transp >>= 16 - info->var.transp.length;

	v = (red << info->var.red.offset) |
	    (green << info->var.green.offset) |
	    (blue << info->var.blue.offset) |
	    (transp << info->var.transp.offset);
	pal[regno] = v;
	return 0;
}

static const struct fb_ops lkfb_ops = {
	.owner		= THIS_MODULE,
	/* I/O + mmap sama dengan FB_DEFAULT_IOMEM_OPS; hanya tiga operasi
	 * gambar diganti wrapper agar hasilnya di-flush ke PoC. */
	.fb_read	= fb_io_read,
	.fb_write	= fb_io_write,
	.fb_mmap	= fb_io_mmap,
	.fb_fillrect	= lkfb_fillrect,
	.fb_copyarea	= lkfb_copyarea,
	.fb_imageblit	= lkfb_imageblit,
	.fb_check_var	= lkfb_check_var,
	.fb_set_par	= lkfb_set_par,
	.fb_setcolreg	= lkfb_setcolreg,
};

static int __init lkfb_init(void)
{
	struct fb_info *info;
	struct lkfb_par *par;
	void __iomem *mem;

	if (fb_get_options("vxtuxlkfb", NULL))
		return -ENODEV;

	/*
	 * Peta framebuffer sebagai WRITE-COMBINE (bukan cacheable biasa).
	 *
	 * Kenapa: 0x9E000000 berada di dalam RAM mesin (bukan MMIO), jadi
	 * memremap(MEMREMAP_WC) sebelumnya mengembalikan NULL dan kita jatuh ke
	 * __va() biasa yang MENYIMPAN piksel di D-cache.  DISPC membaca dari
	 * DRAM langsung (bukan lewat cache CPU), sehingga framebuffer yang
	 * ditulis fbcon tidak pernah terlihat -> teks vertikal tersusun
	 * menyamping + glitch/stutter (garis-garis + gosong), persis seperti
	 * foto layar.  WC menulis langsung ke DRAM tanpa cache, jadi DISPC
	 * selalu membaca isi terkini -> teks bersih, tanpa glitch.
	 */
	mem = (void __iomem *)memremap(LKFB_PHYS, LKFB_SIZE, MEMREMAP_WC);	if (!mem) {
		/*
		 * Fallback: __va cacheable.  flush sekali di awal HANYA
		 * membersihkan sisa logo LK; tulisan fbcon berikutnya tetap
		 * masuk cache, jadi tiap gambar wajib di-flush juga
		 * (lkfb_need_flush -> lkfb_flush_rows).
		 */
		pr_warn("memremap(WC) gagal, pakai __va + flush per-blit\n");
		mem = (void __iomem *)__va(LKFB_PHYS);
		dcache_clean_inval_poc((unsigned long)mem,
				       (unsigned long)mem + LKFB_SIZE);
		lkfb_need_flush = true;
	}

	info = framebuffer_alloc(sizeof(*par), NULL);
	if (!info)
		return -ENOMEM;

	par = info->par;

	info->fbops = &lkfb_ops;
	info->screen_base = mem;
	info->screen_size = LKFB_SIZE;
	info->pseudo_palette = par->palette;

	lkfb_check_var(&info->var, info);
	strscpy(info->fix.id, "vxtux-lkfb", sizeof(info->fix.id));
	info->fix.smem_start = LKFB_PHYS;
	info->fix.smem_len = LKFB_SIZE;
	lkfb_set_par(info);

	if (register_framebuffer(info)) {
		pr_err("register_framebuffer gagal\n");
		framebuffer_release(info);
		return -ENODEV;
	}

	pr_info("fb LK %#lx %ux%u/%ubu terdaftar -> fbcon aktif\n",
		LKFB_PHYS, LKFB_WIDTH, LKFB_HEIGHT, LKFB_BPP);
	return 0;
}
core_initcall(lkfb_init);
