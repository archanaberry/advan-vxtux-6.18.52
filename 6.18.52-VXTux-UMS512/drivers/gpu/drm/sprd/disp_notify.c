// SPDX-License-Identifier: GPL-2.0
/*
 * vxtux_disp_notify.c - MSM-style blank/unblank notifier chain for
 * vendor touch drivers (Himax/Chipone) on kernel 6.18-VXTux-UMS512.
 * Events are emitted by the display layer (DRM port, phase 14).
 *
 * LinDroid/VXTux v1.0
 */
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/msm_drm_notify.h>

BLOCKING_NOTIFIER_HEAD(vxtux_disp_notify_list);
EXPORT_SYMBOL_GPL(vxtux_disp_notify_list);

/* USB-state notifier chain for the vendor Chipone cts_oem touch driver. */
BLOCKING_NOTIFIER_HEAD(vxtux_usb_notify_list);
EXPORT_SYMBOL_GPL(vxtux_usb_notify_list);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VXTux display blank/unblank notifier (MSM-compatible API)");
