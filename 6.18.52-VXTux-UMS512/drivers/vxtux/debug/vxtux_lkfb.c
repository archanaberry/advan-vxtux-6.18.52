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
 * Geometri diambil dari log LK sendiri:
 *   lcd_console: have 150/125 col/rws on scr 1200x2000  -> 150*8=1200,
 *   125*16=2000 (font 8x16) -> layar 1200x2000, bukan 1920.
 *   [LCD] Using LCD frambuffer at 0x9e000000
 *   fb size 0x9e4000 = align4096(1200*2160*4) -> 32 bpp (XRGB8888).
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

#define LKFB_PHYS	0x9e000000UL	/* lcd_base= bootargs LK */
#define LKFB_WIDTH	1200U
#define LKFB_HEIGHT	2000U
#define LKFB_BPP	32U
#define LKFB_STRIDE	(LKFB_WIDTH * (LKFB_BPP / 8))
#define LKFB_SIZE	((size_t)LKFB_STRIDE * LKFB_HEIGHT)

struct lkfb_par {
	u32 palette[16];
};

static int lkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	/* Geometri dipaku: panel tidak bisa diubah oleh fbcon. */
	var->xres = LKFB_WIDTH;
	var->yres = LKFB_HEIGHT;
	var->xres_virtual = LKFB_WIDTH;
	var->yres_virtual = LKFB_HEIGHT;
	var->bits_per_pixel = LKFB_BPP;
	var->grayscale = 0;

	var->red.offset = 16;
	var->red.length = 8;
	var->green.offset = 8;
	var->green.length = 8;
	var->blue.offset = 0;
	var->blue.length = 8;
	var->transp.offset = 24;
	var->transp.length = 8;

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
	FB_DEFAULT_IOMEM_OPS,
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

	mem = memremap(LKFB_PHYS, LKFB_SIZE, MEMREMAP_WC);
	if (!mem) {
		pr_err("gagal memetakan fb LK di %#lx\n", LKFB_PHYS);
		return -ENOMEM;
	}

	info = framebuffer_alloc(sizeof(*par), NULL);
	if (!info) {
		memunmap(mem);
		return -ENOMEM;
	}

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
		memunmap(mem);
		framebuffer_release(info);
		return -ENODEV;
	}

	pr_info("fb LK %#lx %ux%u/%ubu terdaftar -> fbcon aktif\n",
		LKFB_PHYS, LKFB_WIDTH, LKFB_HEIGHT, LKFB_BPP);
	return 0;
}
core_initcall(lkfb_init);
