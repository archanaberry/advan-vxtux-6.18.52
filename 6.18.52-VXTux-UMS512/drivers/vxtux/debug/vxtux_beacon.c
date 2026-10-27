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
 *   fb size 0x9e4000 = alokasi LK 4 byte/piksel, TAPI scanout DPU terbukti
 *   RGB565 16-bit lewat uji boot artix21-24 (mk_boot_artix.py): word
 *   0x00FF00FF tampil biru (=2 piksel 0x00FF), 0xF800F800 tampil merah,
 *   dan 4 pita x 300.000 word (= 1,2 MB = 600 baris @ 1200 px) membagi
 *   layar 1200x2000 persis menjadi 4 -> stride 1200*2 = 2400, 16 bpp
 *   RGB565.  (Versi awal 32 bpp XRGB8888 dan 1200x1920/stride 1200*3
 *   keduanya salah -- barisnya bergeser dan kotak jadi noda.)
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

#include <asm/cacheflush.h>
#include <asm/memory.h>

#define BEACON_FB_SIZE	0x9e4000U	/* sama dengan "fb size" LK */
#define BEACON_WIDTH	1200U
#define BEACON_STRIDE	(BEACON_WIDTH * 2)	/* 16 bpp RGB565 (terbukti artix21-24) */
#define BEACON_BOX	80U
#define BEACON_GAP	96U
#define BEACON_LEFT	40U
#define BEACON_TOP	260U	/* di bawah 4 pita stub relay (baris 0..199) */
#define BEACON_MAX_AREAS 4

struct beacon_area {
	phys_addr_t	base;
	void __iomem	*va;
};

static struct beacon_area beacon_areas[BEACON_MAX_AREAS];
static int beacon_nareas;
static int beacon_stage;

/*
 * RGB888 -> RGB565 (format scanout DPU yang terbukti dari uji pita
 * artix21-24) dan tulis sebagai 2 byte little-endian per piksel.
 */
static inline u16 beacon_rgb565(u8 r, u8 g, u8 b)
{
	return (u16)((r & 0xF8) << 8) | (u16)((g & 0xFC) << 3) | (u16)(b >> 3);
}

static void beacon_box(int idx, u8 r, u8 g, u8 b)
{
	unsigned int y, x, k;
	u16 v = beacon_rgb565(r, g, b);

	for (k = 0; k < beacon_nareas; k++) {
		u8 __iomem *base = beacon_areas[k].va;
		unsigned int x0;

		if (!base)
			continue;

		x0 = BEACON_LEFT + (unsigned int)idx * BEACON_GAP;
		for (y = BEACON_TOP; y < BEACON_TOP + BEACON_BOX; y++) {
			u8 __iomem *row = base + (size_t)y * BEACON_STRIDE;

			for (x = x0; x < x0 + BEACON_BOX; x++) {
				writeb(v & 0xff, row + x * 2 + 0);
				writeb(v >> 8,   row + x * 2 + 1);
			}
		}

		/* linear map cacheable -> flush agar DISPC melihatnya */
		dcache_clean_poc((unsigned long)base +
				 (size_t)BEACON_TOP * BEACON_STRIDE,
				 (unsigned long)base +
				 (size_t)(BEACON_TOP + BEACON_BOX) * BEACON_STRIDE);
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

/*
 * BUG LAMA: di sini dipakai memremap(..., MEMREMAP_WC).  Untuk alamat di
 * dalam /memory (0x9D000000 dan 0x9E000000 keduanya System RAM menurut
 * memory@80000000 reg = <0x80000000 0x80000000> di DT stok) memremap()
 * MENGEMBALIKAN NULL + WARN "memremap attempted on ram", sehingga tidak
 * satu kotak beacon pun pernah tercat sejak versi memremap dipakai.
 * Sekarang: linear map apa adanya + flush D-cache sampai PoC.
 */
static int __init beacon_map(void)
{
	int k;

	for (k = 0; k < beacon_nareas; k++)
		beacon_areas[k].va = (void __iomem *)__va(beacon_areas[k].base);
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

	/* pita merah (RGB565 0xF800) selebar layar di paling atas = panic */
	for (k = 0; k < beacon_nareas; k++) {
		u8 __iomem *base = beacon_areas[k].va;
		u16 v = beacon_rgb565(0xff, 0x00, 0x00);

		if (!base)
			continue;
		for (y = 0; y < 24; y++) {
			u8 __iomem *row = base + (size_t)y * BEACON_STRIDE;

			for (x = 0; x < BEACON_WIDTH; x++) {
				writeb(v & 0xff, row + x * 2 + 0);
				writeb(v >> 8,   row + x * 2 + 1);
			}
		}
		dcache_clean_poc((unsigned long)base,
				 (unsigned long)base + 24 * BEACON_STRIDE);
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
