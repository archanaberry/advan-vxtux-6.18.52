/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/msm_drm_notify.h — compat VXTux (6.18) untuk driver touch vendor
 * (hxchipset himax / chipone) yang mengandalkan API notifier MSM.
 *
 * LinDroid/VXTux v1.0 — UMS512. Event blank/unblank akan diemit oleh
 * lapisan display VXTux (port DRM Fase 14) lewat chain vxtux_disp_notify.
 */
#ifndef _VXTUX_MSM_DRM_NOTIFY_COMPAT_H
#define _VXTUX_MSM_DRM_NOTIFY_COMPAT_H

#include <linux/notifier.h>

enum {
	MSM_DRM_EARLY_EVENT_BLANK = 0x01,
	MSM_DRM_EVENT_BLANK,
};

enum {
	MSM_DRM_NOTIFY_UNBLANK = 0,
	MSM_DRM_NOTIFY_BLANK,
	MSM_DRM_NOTIFY_SUSPEND,
	MSM_DRM_NOTIFY_RESUME,
};

/* Aliases gaya MSM lama (nilai identik dipakai driver vendor) */
#define MSM_DRM_BLANK_UNBLANK	MSM_DRM_NOTIFY_UNBLANK
#define MSM_DRM_BLANK_POWERDOWN	MSM_DRM_NOTIFY_BLANK

struct msm_drm_notifier {
	int id;			/* display id (0 = panel utama) */
	int *data;		/* pointer ke nilai MSM_DRM_NOTIFY_* */
	void *data_raw;		/* konteks tambahan (opsional) */
};

extern struct blocking_notifier_head vxtux_disp_notify_list;

#endif /* _VXTUX_MSM_DRM_NOTIFY_COMPAT_H */
