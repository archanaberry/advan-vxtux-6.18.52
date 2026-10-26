// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_beacon.c — indikator tahap boot di framebuffer LK (UMS512).
 *
 * Masalah: tablet ini tidak mengekspos UART sebagai COM, dan boot yang mati
 * sebelum driver eMMC probe tidak bisa menulis apa pun ke partisi cache.
 * Akibatnya semua kegagalan boot sebelum MMC terlihat sama: layar diam di
 * logo, tanpa jejak.
 *
 * Solusi: LK meninggalkan framebuffer panel tetap menyala, dan display
 * controller terus memindai memori itu walau kernel sudah berjalan (terbukti:
 * kernel 6.18 tidak pernah mengambil alih panel, layar tetap menampilkan
 * splash LK). Driver ini memanfaatkannya sebagai kanal keluaran:
 *
 *   setiap tahap initcall mengecat SATU kotak putih tambahan di kiri-atas.
 *
 * Cara membaca hasilnya: hitung kotak putih di layar.
 *
 *   1 = early_initcall   5 = subsys_initcall
 *   2 = core_initcall    6 = fs_initcall
 *   3 = postcore         7 = device_initcall
 *   4 = arch_initcall    8 = late_initcall (seluruh initcall selesai)
 *
 * Kotak merah selebar layar = kernel panic (notifier panic).
 *
 * Alamat/format framebuffer diambil dari log LK sendiri:
 *   [sprdfb][logo_mem_init] splashimage addr is 0x0x9d000000, fb size is 0x9e4000
 *   [sprdlk][lcd_init] [LCD] Using LCD frambuffer at 0x9e000000
 *   lcd_name=lcd_hx83102e_qc_1200x2000, lcd_base=9e000000
 *   lcd_console: have 150/125 col/rws on scr 1200x2000
 *     -> 150*8 = 1200 dan 125*16 = 2000 (font 8x16) => layar 1200x2000
 *   fb size 0x9e4000 = align4096(1200 * 2160 * 4) => 32 bpp XRGB8888,
 *   stride 1200*4 = 4800.  (Versi awal memakai 1200x1920 / stride 1200*3,
 *   salah — barisnya bergeser dan kotak jadi noda yang tak terbaca.)
 *
 * Kedua buffer dicat supaya benar apa pun yang sedang dipindai DISPC.
 *
 * Memori itu TIDAK direservasi di DT stok, jadi tanpa penjagaan kernel bisa
 * memakainya untuk hal lain. Karena itu reservasi dilakukan pada early_param
 * ("vxtuxfb=0x9d000000,0x9e000000"), saat memblock masih bisa diatur — jauh
 * sebelum buddy allocator mengambil alih.
 */
#define pr_fmt(fmt)	"vxtux-beacon: " fmt

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/notifier.h>
#include <linux/panic_notifier.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>

#define BEACON_FB_SIZE	0x9e4000U	/* sama dengan "fb size" LK */
#define BEACON_WIDTH	1200U
#define BEACON_STRIDE	(BEACON_WIDTH * 4)	/* 32 bpp XRGB8888 */
#define BEACON_BOX	80U
#define BEACON_GAP	96U
#define BEACON_LEFT	40U
#define BEACON_TOP	40U
#define BEACON_MAX_AREAS 4

struct beacon_area {
	phys_addr_t	base;
	void __iomem	*va;
};

static struct beacon_area beacon_areas[BEACON_MAX_AREAS];
static int beacon_nareas;
static int beacon_stage;

static void beacon_box(int idx, u8 r, u8 g, u8 b)
{
	unsigned int y, x, k;

	for (k = 0; k < beacon_nareas; k++) {
		u8 __iomem *base = beacon_areas[k].va;
		unsigned int x0;

		if (!base)
			continue;

		x0 = BEACON_LEFT + (unsigned int)idx * BEACON_GAP;
		for (y = BEACON_TOP; y < BEACON_TOP + BEACON_BOX; y++) {
			u8 __iomem *row = base + (size_t)y * BEACON_STRIDE;

			for (x = x0; x < x0 + BEACON_BOX; x++) {
				writeb(b, row + x * 4 + 0);
				writeb(g, row + x * 4 + 1);
				writeb(r, row + x * 4 + 2);
				writeb(0xff, row + x * 4 + 3);
			}
		}
	}
}

static void beacon_mark(int stage)
{
	if (stage <= beacon_stage)
		return;
	beacon_stage = stage;
	beacon_box(stage - 1, 0xff, 0xff, 0xff);
}

/*
 * early_param: reservasi kedua buffer SEBELUM buddy allocator memakai RAM.
 * Format: vxtuxfb=0x9d000000,0x9e000000
 */
static int __init beacon_setup(char *str)
{
	char *tok;

	while (str && *str && beacon_nareas < BEACON_MAX_AREAS) {
		unsigned long long addr;

		tok = strsep(&str, ",");
		if (!tok || !*tok)
			continue;
		addr = memparse(tok, NULL);
		if (!addr)
			continue;
		if (memblock_reserve(addr, BEACON_FB_SIZE)) {
			pr_err("reservasi %pa gagal\n", &addr);
			continue;
		}
		beacon_areas[beacon_nareas].base = addr;
		beacon_nareas++;
	}
	if (beacon_nareas)
		pr_info("%d buffer framebuffer direservasi (%#x byte)\n",
			beacon_nareas, BEACON_FB_SIZE);
	return 0;
}
early_param("vxtuxfb", beacon_setup);

static int __init beacon_map(void)
{
	int k;

	for (k = 0; k < beacon_nareas; k++) {
		beacon_areas[k].va = memremap(beacon_areas[k].base,
					      BEACON_FB_SIZE, MEMREMAP_WC);
		if (!beacon_areas[k].va)
			pr_err("memremap %pa gagal\n", &beacon_areas[k].base);
	}
	return beacon_nareas;
}

/* --- tahap-tahap ------------------------------------------------------- */

static int __init beacon_early(void)
{
	if (!beacon_map())
		return 0;
	beacon_mark(1);
	return 0;
}
early_initcall(beacon_early);

static int __init beacon_core(void)
{
	beacon_mark(2);
	return 0;
}
core_initcall(beacon_core);

static int __init beacon_postcore(void)
{
	beacon_mark(3);
	return 0;
}
postcore_initcall(beacon_postcore);

static int __init beacon_arch(void)
{
	beacon_mark(4);
	return 0;
}
arch_initcall(beacon_arch);

static int __init beacon_subsys(void)
{
	beacon_mark(5);
	return 0;
}
subsys_initcall(beacon_subsys);

static int __init beacon_fs(void)
{
	beacon_mark(6);
	return 0;
}
fs_initcall(beacon_fs);

static int __init beacon_device(void)
{
	beacon_mark(7);
	return 0;
}
device_initcall(beacon_device);

static int __init beacon_late(void)
{
	beacon_mark(8);
	return 0;
}
late_initcall(beacon_late);

/* --- panic ------------------------------------------------------------- */

static int beacon_panic(struct notifier_block *nb, unsigned long action,
			void *data)
{
	unsigned int y, x, k;

	/* pita merah selebar layar di paling atas = panic terdeteksi */
	for (k = 0; k < beacon_nareas; k++) {
		u8 __iomem *base = beacon_areas[k].va;

		if (!base)
			continue;
		for (y = 0; y < 24; y++) {
			u8 __iomem *row = base + (size_t)y * BEACON_STRIDE;

			for (x = 0; x < BEACON_WIDTH; x++) {
				writeb(0x00, row + x * 4 + 0);
				writeb(0x00, row + x * 4 + 1);
				writeb(0xff, row + x * 4 + 2);
				writeb(0xff, row + x * 4 + 3);
			}
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block beacon_panic_nb = {
	.notifier_call	= beacon_panic,
	.priority	= 0,
};

static int __init beacon_notifier(void)
{
	if (beacon_nareas)
		atomic_notifier_chain_register(&panic_notifier_list,
					       &beacon_panic_nb);
	return 0;
}
late_initcall(beacon_notifier);
