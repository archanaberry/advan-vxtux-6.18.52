/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/vxtux/compat.h — VXTux kernel compatibility layer
 * Purpose: single file for all 5.4 <-> 6.18 API bridges (folio, platform, i2c).
 * All files in drivers/vxtux/ must include this, not <linux/version.h> hacks.
 * (This comment intentionally avoids "slash-star" sequences because clang
 *  gives -Wcomment warning if they appear inside comment blocks.)
 *
 * LinDroid/VXTux v1.0 — design gen 3.0
 */
#ifndef _VXTUX_COMPAT_H
#define _VXTUX_COMPAT_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define VXTUX_OS_VERSION	"LinDroid/VXTux v1.0"
#define VXTUX_DESIGN_GEN	"3.0"

/* ------------------------------------------------------------------ *
 * Phase 14: folio-aware helpers (6.18) vs struct page (5.4)
 * RE drivers NEVER touch struct page directly.
 * ------------------------------------------------------------------ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
/* folio available; wrap here so 6.18 migration is unilateral */
#define VX_FOLIO_AVAILABLE	1
static inline struct page *vx_folio_page(struct folio *folio)
{
	return folio_page(folio, 0);
}
#else
#define VX_FOLIO_AVAILABLE	0
struct folio;	/* 5.4: not present — avoid folio type in RE drivers */
#endif

/* ------------------------------------------------------------------ *
 * platform_get_resource deprecation (6.18: platform_get_mem_or_io)
 * ------------------------------------------------------------------ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
#define VX_PLATFORM_GET_MEM(pdev, idx) \
	platform_get_mem_or_io((pdev), (idx))
#else
#include <linux/ioport.h>
#define VX_PLATFORM_GET_MEM(pdev, idx) \
	platform_get_resource((pdev), IORESOURCE_MEM, (idx))
#endif

/* ------------------------------------------------------------------ *
 * common helpers: probe error path, register write-read verification
 * ------------------------------------------------------------------ */
#define VX_REG_FMT	"0x%08x: 0x%08x\n"

#define vx_err_once(dev, fmt, ...)	\
	dev_err_once(dev, "[vxtux] " fmt, ##__VA_ARGS__)
#define vx_dbg(dev, fmt, ...)		\
	dev_dbg(dev, "[vxtux] " fmt, ##__VA_ARGS__)

/* ABI guard: all vxtux drivers GPL-2.0, single license, no vendor blobs */
MODULE_INFO(vxtux_os, VXTUX_OS_VERSION);

/* ------------------------------------------------------------------ *
 * DE-SHIM (2026-09-18): GLOBAL API bridge 5.4 -> 6.18 that was previously emitted
 * by tools/re/classify_missing.py to `include/vxtux/compat_618.h` IS NO LONGER PRESENT.
 * Usage audit proves its content is unused by the port:
 *   - 4 `static inline` dead (no callers whatsoever),
 *   - 2 that were used only used by WCN and now remain native in
 *     drivers/vxtux/net/wireless/sprdwcn/include/wcn-618-compat.h.
 * So that header was DELETED, not kept as transition baggage. This file
 * intentionally no longer includes anything from the compat layer: files needing
 * 6.18 API mapping must use 6.18 API directly.
 * Invariant maintained: tools/re/check_layout.sh (INV-DESHIM) and
 * tools/re/verify_compat_618.py (header must be .deceased status).
 * ------------------------------------------------------------------ */

#endif /* _VXTUX_COMPAT_H */
