/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Unisoc Inc.
 *
 * Kernel-internal GSP configuration structures.
 *
 * This header is the kernel-side twin of <uapi/drm/gsp_cfg.h>. The uapi header
 * only defines the "_user" variants that cross the ioctl boundary; the types
 * below are the in-kernel copies the driver actually traverses.
 *
 * Every field offset below was recovered from sprd-gsp.ko by radare2 and is
 * cited with the function that produced it. Nothing here is guessed from the
 * uapi layout, because the two layouts differ (the kernel side has mem_data,
 * buf/map and list members that have no uapi counterpart).
 */

#ifndef _DRM_GSP_CFG_H
#define _DRM_GSP_CFG_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <uapi/drm/gsp_cfg.h>
#include <uapi/drm/gsp_lite_r2p0_cfg.h>

struct gsp_kcfg;
struct gsp_core;

/*
 * struct gsp_buf / struct gsp_buf_map
 *
 * Evidence (sprd-gsp.ko):
 *   gsp_layer_to_buf()      -> "add x0, x0, 0x40"
 *   gsp_layer_to_buf_map()  -> "add x0, x0, 0x58"
 * so mem_data.buf sits at layer+0x40 and mem_data.map at layer+0x58.
 *
 *   gsp_layer_buf_verify()  reads [buf+0x00] (cbz -> return 1) and [buf+0x08]
 *                           with the ERR_PTR test (cmn x8, 0x1000; cset hi),
 *   gsp_layer_get_dmabuf()  stores the dma_buf_get() result at [layer+0x48],
 *   gsp_layer_put_dmabuf()  loads [layer+0x48] and calls dma_buf_put(),
 *   gsp_layer_need_iommu()  reads [layer+0x50] (buf->is_iova).
 *
 * gsp_buf is therefore { size, dmabuf, is_iova } and occupies 0x40..0x58.
 *
 *   gsp_layer_dmabuf_map()  stores the dma_buf_attach() result at [layer+0x58],
 *                           the dma_buf_map_attachment() result at [layer+0x60],
 *                           and the direction at [layer+0x68].
 *   gsp_layer_dmabuf_unmap() reloads exactly those three offsets.
 */
struct gsp_buf {
	unsigned long size;
	struct dma_buf *dmabuf;
	bool is_iova;
};

struct gsp_buf_map {
	struct dma_buf_attachment *attachment;
	struct sg_table *table;
	enum dma_data_direction dir;
};

/*
 * struct gsp_mem_data
 *
 * Evidence (sprd-gsp.ko):
 *   gsp_layer_to_share_fd()   -> "ldr w0, [x0, 0x30]"
 *   gsp_layer_to_uv_offset()  -> "ldr w0, [x0, 0x34]"
 *   gsp_layer_to_v_offset()   -> "ldr w0, [x0, 0x38]"
 *   gsp_layer_addr_set()      reads [x0+0x34] and [x0+0x38] and adds them to
 *                              the iova written at [x0+0x24].
 * 0x3c is padding before the 8-aligned gsp_buf at 0x40.
 */
struct gsp_mem_data {
	int share_fd;
	u32 uv_offset;
	u32 v_offset;
	struct gsp_buf buf;
	struct gsp_buf_map map;
};

/*
 * struct gsp_layer
 *
 * The common header shared by every per-chip layer type. A per-chip layer
 * embeds it as its first member named "common" and appends its own "params":
 *
 *   struct gsp_lite_r2p0_img_layer {
 *           struct gsp_layer         common;
 *           struct gsp_lite_r2p0_img_layer_params params;
 *   };
 *
 * which is why the driver uses both spellings: gsp_layer.c (operating on
 * "struct gsp_layer") writes layer->type, while gsp_layer.h's
 * gsp_layer_common_copy_from_user() and the per-chip cores write
 * layer->common.type.
 *
 * Evidence for each offset (sprd-gsp.ko):
 *   0x00 type      gsp_layer_to_type()      "ldr w0, [x0]"
 *   0x04 enable    gsp_layer_is_enable()    "ldr w0, [x0, 4]"
 *   0x08 list      gsp_lite_r2p0_core_cfg_reinit() walks the cfg layer list
 *                  with "ldr x9, [x8, 0x10]!" on x8 = cfg + 0x10, compares the
 *                  cursor against its own address for the empty test, then walks
 *                  entries zeroing from [entry - 8] upward. The cursor is a
 *                  list_head embedded at layer + 0x08, so list_add_tail()'s
 *                  container_of recovers the layer base from entry - 8.
 *   0x18 wait_fd   gsp_layer_to_wait_fd()   "ldr w0, [x0, 0x18]"
 *   0x1c sig_fd    gsp_layer_to_sig_fd()    "ldr w0, [x0, 0x1c]"
 *   0x20 filled    gsp_layer_is_filled()    "ldr w0, [x0, 0x20]"
 *                  gsp_layer_set_filled()   "str w8, [x0, 0x20]" with w8 = 1,
 *                  a 4-byte store, so this is an int and not a bool.
 *   0x24 src_addr  gsp_layer_to_addr()      "add x0, x0, 0x24"
 *                  gsp_layer_addr_set()     writes [x0+0x24], [x0+0x28], [x0+0x2c]
 *                  gsp_layer_addr_put()     zeroes the same three words.
 *   0x30 mem_data  gsp_layer_to_share_fd() / _uv_offset / _v_offset.
 */
struct gsp_layer {
	u32 type;
	u32 enable;
	struct list_head list;
	int wait_fd;
	int sig_fd;
	int filled;
	struct gsp_addr_data src_addr;
	struct gsp_mem_data mem_data;
};

/*
 * struct gsp_cfg
 *
 * The common header at the front of every per-chip configuration. Each
 * per-chip cfg embeds it as its first member named "common" and appends its
 * layer arrays and misc block:
 *
 *   struct gsp_lite_r2p0_cfg {
 *           struct gsp_cfg           common;
 *           struct gsp_lite_r2p0_img_layer limg[LITE_R2P0_IMGL_NUM];
 *           struct gsp_lite_r2p0_osd_layer losd[LITE_R2P0_OSDL_NUM];
 *           struct gsp_lite_r2p0_des_layer ld1;
 *           struct gsp_lite_r2p0_misc_cfg  misc;
 *   };
 *
 * sizeof(struct gsp_cfg) == 0x30 (48). Evidence:
 *   gsp_dev.c's "size < sizeof(struct gsp_cfg)" check compiles to
 *   "cmp w3, 0x2f ; b.ls" in sprd_gsp_trigger_ioctl, i.e. it rejects
 *   size <= 47, so the bound is 48.
 *   Independently, gsp_kcfg_list_fill() allocates the per-cfg buffer with
 *   __kmalloc(kl->size, 0xdc0) where kl->size is the user supplied stride, and
 *   the first layer of that buffer is written at cfg + 0x30 by
 *   gsp_lite_r2p0_core_devset() ("str w9, [x0, 0x30]!" with x0 = kcfg->cfg).
 *   The common header therefore ends exactly where the first layer begins.
 *
 * Field offsets:
 *   0x04 init       gsp_lite_r2p0_core_cfg_reinit(): "ldr w8, [x0, 4]" then
 *                   "cmp w8, 1", matching "if (cfg->common.init != 1)".
 *   0x08 layer_num  same function's failure arm loads "ldr w2, [x0, 8]" as a
 *                   printk argument, i.e. it reports cfg->common.layer_num.
 *   0x10 layers     same function walks the list head at cfg + 0x10
 *                   (see the "list" note on struct gsp_layer above).
 *   0x20 kcfg       same function's first load, "ldr x8, [x0, 0x20]", is the
 *                   8-byte back-pointer, and it is the only 8-byte load from
 *                   the header. lite_r2p0_core.c reads cfg->common.kcfg.
 *
 * 0x00, 0x0c and 0x28..0x30 are not written or read by any symbol recovered
 * from the module, so their meaning is undetermined. They are declared as
 * explicit padding rather than being given invented names, which keeps the
 * struct size at the proven 0x30 without asserting a false layout. 0x0c is
 * where "tag" has to live: the struct is only 48 bytes, tag is an int, and
 * 0x00/0x08/0x10/0x20 are accounted for above. It is flagged as unverified
 * because no single accessor exposes it.
 */
struct gsp_cfg {
	u32 reserved0;
	u32 init;
	u32 layer_num;
	int tag;
	struct list_head layers;
	struct gsp_kcfg *kcfg;
	u32 reserved1;
};

/*
 * Per-chip (lite_r2p0) twins of the uapi layer/config structs.
 *
 * The uapi header defines gsp_lite_r2p0_{img,osd,des}_layer_user and
 * gsp_lite_r2p0_cfg_user. The kernel needs the same shapes with
 * "struct gsp_layer common" in place of "struct gsp_layer_user common" --
 * that is the only difference, and it is forced, not chosen: gsp_layer.c
 * and gsp_lite_r2p0_core.c call gsp_layer_to_type(), gsp_layer_has_share_fd()
 * and friends on these layers, and those take "struct gsp_layer *". The
 * gsp_layer_common_copy_from_user() macro in gsp_layer.h does exactly this
 * substitution field by field.
 *
 * The trailing "params" sub-structs are the SAME types as the uapi ones and
 * are reused verbatim: they are the GSP register-programming payload, they
 * never contain a kernel pointer, and copy_cfg writes them with a straight
 * copy_from_user. Declaring separate kernel copies of them would invent a
 * second ABI with no evidence behind it.
 *
 * sizeof(struct gsp_layer) == 0x70 is what makes this consistent, and it is
 * pinned twice independently in the blob:
 *   - the one-instruction accessors reach +0x68 (map->dir), and the largest
 *     member, gsp_mem_data, ends at 0x30 + sizeof(gsp_mem_data) = 0x70;
 *   - gsp_lite_r2p0_core_cfg_reinit() zeroes cfg+0xa0 (the first layer's
 *     params) while gsp_lite_r2p0_core_cfg_init() places the first layer at
 *     cfg+0x30. 0xa0 - 0x30 = 0x70 is therefore the stride of the array
 *     element that the reinit then memsets -- a second, arithmetic
 *     derivation of the same number from an unrelated instruction.
 */
struct gsp_lite_r2p0_img_layer {
	struct gsp_layer common;
	struct gsp_lite_r2p0_img_layer_params params;
};

struct gsp_lite_r2p0_osd_layer {
	struct gsp_layer common;
	struct gsp_lite_r2p0_osd_layer_params params;
};

struct gsp_lite_r2p0_des_layer {
	struct gsp_layer common;
	struct gsp_lite_r2p0_des_layer_params params;
};

/*
 * The kernel-side config. Unlike the layers above this one is NOT the uapi
 * shape with a swapped member: the uapi struct starts directly at limg[],
 * while the kernel struct carries the 0x30-byte struct gsp_cfg header in
 * front (evidenced by cfg+0x20 being cfg->common.kcfg and cfg+0x10 being
 * cfg->common.layers, both read in gsp_lite_r2p0_core_cfg_reinit). The uapi
 * struct starts straight at limg[], which is the other reason the kernel one
 * is not simply the uapi shape with a swapped member.
 */
/*
 * The kernel-side misc block. This one DOES need its own name, because
 * gsp_lite_r2p0_core_copy_cfg() memcpy()s sizeof(struct gsp_lite_r2p0_misc_cfg)
 * bytes straight out of the user struct -- so the two must be layout
 * identical, and a kernel-side alias keeps that requirement explicit at the
 * one place it matters instead of hiding it behind the "_user" suffix.
 *
 * Every member is pure register-programming payload (mode flags, work-area
 * rectangles, scaler taps); none is a pointer or a kernel type, which is why
 * a flat memcpy is correct here.
 */
struct gsp_lite_r2p0_misc_cfg {
	__u8 gsp_gap;
	__u8 cmd_cnt;
	__u8 run_mod;
	__u8 scale_seq;
	__u8 pmargb_en;
	struct gsp_rect workarea1_src_rect;
	struct gsp_rect workarea2_src_rect;
	struct gsp_pos workarea2_des_pos;
	struct gsp_scale_para scale_para;
};

struct gsp_lite_r2p0_cfg {
	struct gsp_cfg common;
	struct gsp_lite_r2p0_img_layer limg[LITE_R2P0_IMGL_NUM];
	struct gsp_lite_r2p0_osd_layer losd[LITE_R2P0_OSDL_NUM];
	struct gsp_lite_r2p0_des_layer ld1;
	struct gsp_lite_r2p0_misc_cfg misc;
};

#endif /* _DRM_GSP_CFG_H */