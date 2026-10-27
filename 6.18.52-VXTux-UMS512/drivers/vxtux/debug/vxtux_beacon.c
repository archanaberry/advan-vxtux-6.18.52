// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_fb_mem.c (build name: vxtux_beacon.o) - keep the bootloader
 * framebuffer memory out of the buddy allocator (UMS512 / Advan T1030).
 *
 * WHY THIS EXISTS
 * ---------------
 * LK leaves the panel scanning out of two buffers that the stock device tree
 * does NOT reserve:
 *
 *   [sprdfb][logo_mem_init] splashimage addr is 0x9d000000, fb size 0x9e4000
 *   [sprdlk][lcd_init] [LCD] Using LCD frambuffer at 0x9e000000
 *   lcd_name=lcd_hx83102e_qc_1200x2000, lcd_base=9e000000
 *
 * vxtux_lkfb.c registers the second buffer as an fbdev so fbcon can print
 * kernel messages on the panel.  If the kernel ever handed those pages to
 * somebody else, the framebuffer would be scribbled over and the console
 * would fill with garbage.  So both ranges are claimed here.
 *
 * HOW
 * ---
 * early_param runs while memblock is still editable, long before the buddy
 * allocator exists, which is the only point where this is safe:
 *
 *   cmdline: ... vxtuxfb=0x9d000000,0x9e000000
 *
 * The command line is built by ci_qemu/mk_boot_artix.py (CMDLINE constant).
 *
 * HISTORY
 * -------
 * This file used to also draw a debug beacon: one white box per initcall
 * level in the top-left corner, plus a red bar on panic.  Those boxes were
 * the only feedback channel back when nothing could print on the panel.  The
 * framebuffer console (vxtux_lkfb.c, device_initcall) plus its 200-line dmesg
 * replay now cover that job, and the leftover boxes made the handover from LK
 * to the kernel look dirty, so the whole drawing path is gone.
 *
 * If early-boot feedback is ever needed again, note that anything drawn from
 * this file is drawn THROUGH THE CACHEABLE LINEAR MAP, so it must be flushed
 * to the point of coherency (dcache_clean_poc) or the display controller
 * never sees it - see git history for the removed beacon code.
 */
#define pr_fmt(fmt) "vxtux-fbmem: " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>

#define VXTUXFB_SIZE	0x9e4000U	/* "fb size" reported by LK */
#define VXTUXFB_MAX	4

static phys_addr_t vxtuxfb_base[VXTUXFB_MAX];
static int vxtuxfb_n;

/*
 * vxtuxfb=0x9d000000,0x9e000000
 *
 * Reserve every address listed, before the buddy allocator can take it.
 */
static int __init vxtuxfb_setup(char *str)
{
	char *tok;

	while (str && *str && vxtuxfb_n < VXTUXFB_MAX) {
		unsigned long long addr;

		tok = strsep(&str, ",");
		if (!tok || !*tok)
			continue;
		addr = memparse(tok, NULL);
		if (!addr)
			continue;
		if (memblock_reserve(addr, VXTUXFB_SIZE)) {
			pr_err("cannot reserve %pa\n", &addr);
			continue;
		}
		vxtuxfb_base[vxtuxfb_n++] = addr;
	}
	if (!vxtuxfb_n) {
		pr_warn("no framebuffer buffer reserved - pass vxtuxfb=<phys>,...\n");
		return 0;
	}
	pr_info("reserved %d bootloader framebuffer buffer(s), %#x bytes each\n",
		vxtuxfb_n, VXTUXFB_SIZE);
	return 0;
}
early_param("vxtuxfb", vxtuxfb_setup);
