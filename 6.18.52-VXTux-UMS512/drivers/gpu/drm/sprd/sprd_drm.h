/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Unisoc Inc.
 */

#ifndef _SPRD_DRM_H_
#define _SPRD_DRM_H_

#include <drm/drm_atomic.h>
#include <drm/drm_print.h>

struct device;

/*
 * The GSP driver reaches back into the DRM master device through this
 * back-pointer: gsp_drm_dev_set() stores the platform device of the bound
 * GSP component here, and the ioctl path retrieves it again to get at the
 * platform device (for devm_/pm_runtime_ calls). It is cleared back to NULL
 * on unbind, so a NULL check is required before use.
 */
struct sprd_drm {
	struct drm_device drm;
	struct device *gsp_dev;
};

extern struct platform_driver sprd_dpu_driver;
extern struct platform_driver sprd_dsi_driver;

#endif /* _SPRD_DRM_H_ */
