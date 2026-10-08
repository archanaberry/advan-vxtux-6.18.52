/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _GSP_LITE_R2P0_CORE_H
#define _GSP_LITE_R2P0_CORE_H

#include <linux/device.h>
#include <linux/list.h>
#include <drm/gsp_cfg.h>
#include "gsp_core.h"
#include "gsp_debug.h"

/*
 * Scaler coefficient cache entry.
 *
 * struct COEF_ENTRY_T is referenced by the gsp_lite_r2p0 coefficient
 * generator but is defined in NEITHER the Unisoc 5.4 donor nor any other
 * donor available here, so it had to be reconstructed from use. It is
 * defined here rather than in gsp_lite_r2p0_coef_cal.h because the
 * coef_cache[] array below needs a complete type, and this header does
 * not include the coef_cal header. Every member is forced by the code
 * that touches it, not guessed:
 *
 *   list    - list_for_each_entry() / list_entry() in
 *             gsp_lite_r2p0_coef_cache_hit_check() and the eviction path.
 *   in_w/in_h/out_w/out_h/hor_tap/ver_tap - set by LIST_SET_ENTRY_KEY()
 *             and compared against uint16_t parameters in
 *             gsp_lite_r2p0_coef_cache_hit_check(), hence uint16_t.
 *             in_w is also tested against 0 to log "add" vs "swap", so it
 *             must be a plain integer, not a pointer or flag word.
 *   coef    - gsp_lite_r2p0_gen_block_scaler_coef() returns uint32_t *,
 *             writes &entry->coef[0] and &entry->coef[64] and memcpy()s
 *             MAX_PHASE * MAX_TAP * 4 / 2 bytes into each half, i.e. 64
 *             uint32_t per half, 128 total.
 *
 * The layout is an in-memory LRU cache only: it never reaches hardware,
 * so its exact byte layout cannot affect device behaviour. coef[] is
 * 8-byte aligned because the code warns when &entry->coef[] is not.
 */
struct COEF_ENTRY_T {
	struct list_head list;
	uint16_t in_w;
	uint16_t in_h;
	uint16_t out_w;
	uint16_t out_h;
	uint16_t hor_tap;
	uint16_t ver_tap;
	uint32_t coef[128] __aligned(8);
};

#define LITE_R2P0_GSP_CLK ("clk_gsp")
#define LITE_R2P0_GSP_PARENT_CLK ("clk_gsp_parent")
#define LITE_R2P0_GSP_EB_CLK ("clk_gsp_eb")

#define MIN_POOL_SIZE				   (6 * 1024)
#define LITE_R2P0_GSP_COEF_CACHE_MAX		 32

struct gsp_lite_r2p0_core {
	struct gsp_core common;
	struct list_head coef_list;
	struct COEF_ENTRY_T coef_cache[LITE_R2P0_GSP_COEF_CACHE_MAX];

	ulong gsp_coef_force_calc;
	uint32_t cache_coef_init_flag;
	char coef_buf_pool[MIN_POOL_SIZE];

	/* module ctl reg base, virtual	0x63500000 */
	void __iomem *gsp_ctl_reg_base;

	struct clk *gsp_clk;
	struct clk *gsp_clk_parent;
	struct clk *gsp_eb_clk;
};

#define MEM_OPS_ADDR_ALIGN_MASK (0x7UL)

int gsp_lite_r2p0_core_parse_dt(struct gsp_core *core);

int gsp_lite_r2p0_core_copy_cfg(struct gsp_kcfg *kcfg, void *arg, int index);

int gsp_lite_r2p0_core_init(struct gsp_core *core);

int gsp_lite_r2p0_core_alloc(struct gsp_core **core, struct device_node *node);

int gsp_lite_r2p0_core_trigger(struct gsp_core *core);

int gsp_lite_r2p0_core_enable(struct gsp_core *core);

void gsp_lite_r2p0_core_disable(struct gsp_core *core);

int gsp_lite_r2p0_core_release(struct gsp_core *core);

int __user *gsp_lite_r2p0_core_intercept(void __user *arg, int index);
void gsp_lite_r2p0_core_reset(struct gsp_core *core);
void gsp_lite_r2p0_core_dump(struct gsp_core *core);

#endif
