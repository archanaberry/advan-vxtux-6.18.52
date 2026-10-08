/* SPDX-License-Identifier: GPL-2.0 */
/*
 * drivers/gpu/drm/sprd/sprd_dpu/dpu_format.h
 * FOURCC-to-vendor-DPU-format mapping (validated phase 9B patch set).
 *
 * This header declares vxtux_dpu_format_get(), which is exported with
 * EXPORT_SYMBOL_GPL. Without a public prototype, Clang reports
 * -Wmissing-prototypes. The warning surfaced after disp/sprd_dpu/ was wired up
 * on 2026-09-18; the file had not been compiled before, so the issue went
 * unnoticed.
 */
#ifndef _VXTUX_SPRD_DPU_FORMAT_H
#define _VXTUX_SPRD_DPU_FORMAT_H

#include <linux/types.h>

/* 0xFF indicates that the DPU table does not support this FOURCC. */
#define VXTUX_DPU_FMT_UNSUPPORTED	0xFF

u8 vxtux_dpu_format_get(u32 fourcc);

#endif /* _VXTUX_SPRD_DPU_FORMAT_H */
