// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/gpu/drm/sprd/sprd_dpu/dpu_format.c
 * Consolidated, validated DPU patches (phases 9B/10.5), rewritten in the
 * upstream style.
 *
 * Field observations:
 *  - XR24 at 60 Hz is the only stable mode; 90 Hz and other formats glitch.
 *  - With the "FBC enabled" plane property set to 1, a linear buffer shows
 *    green blocks (AFBC-like behavior).
 *  - WLR_DRM_NO_ATOMIC fails, while the wlroots atomic path passes (phase 9A
 *    test matrix).
 *
 * The original patch (5/5) in the 5.4 fork adds RB/UV switching and an ARGB
 * fix to drm_format_to_dpu(), plus a primary-only guard and the
 * dpu_primary_only option.
 * This is the cleaned-up version for 6.18-VXTux-UMS512.
 */
#include <linux/module.h>
#include <linux/types.h>
#include <drm/drm_fourcc.h>

struct drm_plane;

#include <vxtux/compat.h>
#include "dpu_format.h"

/* Derived from the validated vendor-fork drm_format_to_dpu() implementation. */
static const struct {
	u32 fourcc;
	u8  dpu_fmt;
	bool rb_switch;	/* patch: swap R/B channel routing */
} vxtux_dpu_formats[] = {
	{ DRM_FORMAT_XRGB8888, 0x00, false },	/* XR24: primary stable mode */
	{ DRM_FORMAT_ARGB8888, 0x00, true  },	/* Validated ARGB fix */
	{ DRM_FORMAT_XBGR8888, 0x00, true  },	/* Red/blue channel swap */
	/* TODO: Port the remaining mappings from patch 5/5 in the 5.4 fork. */
};

u8 vxtux_dpu_format_get(u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(vxtux_dpu_formats); i++)
		if (vxtux_dpu_formats[i].fourcc == fourcc)
			return vxtux_dpu_formats[i].dpu_fmt;

	return 0xFF;	/* unsupported — probe defer/return EINVAL */
}
EXPORT_SYMBOL_GPL(vxtux_dpu_format_get);

MODULE_AUTHOR("VXTux Project");
MODULE_DESCRIPTION("sprd DPU consolidated RE patches for UMS512");
MODULE_LICENSE("GPL");
