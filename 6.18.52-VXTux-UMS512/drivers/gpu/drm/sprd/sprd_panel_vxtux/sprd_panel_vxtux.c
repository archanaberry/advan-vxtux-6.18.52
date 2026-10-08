// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_panel_vxtux.c - Generic VXTux MIPI-DSI panel (UMS512 / Advan T1030).
 *
 * DRM/KMS path:
 *   DPU (dpu-r4p0) -> CRTC/plane -> DSI host -> this panel (connector).
 *
 * Status: modern skeleton using the 6.18 devm_drm_panel_alloc() API, with a
 *   default 1200x2000 mode based on the stock-device runtest. Vendor timings
 *   and the initialization sequence are NOT included; they must be extracted
 *   from the stock DT or firmware (see "Remaining work" in
 *   docs/drm_kms_map.md). Without the initialization sequence, the panel may
 *   remain off even when KMS is active.
 */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#define DRV_NAME	"sprd-panel-vxtux"

struct vxtux_panel {
	struct drm_panel panel;
	struct device *dev;
	struct gpio_desc *reset_gpio;
	struct regulator *vcc;
	struct regulator *iovcc;
	bool prepared;
};

static inline struct vxtux_panel *panel_to_vxtux(struct drm_panel *panel)
{
	return container_of(panel, struct vxtux_panel, panel);
}

/*
 * Placeholder mode: 1200x2000 at approximately 56 Hz.
 * With a 160 MHz clock, htotal 1400, and vtotal 2062, the refresh rate is
 * approximately 55.4 Hz.
 * Replace these values with vendor timings extracted from the stock DT or
 * firmware.
 */
static const struct drm_display_mode vxtux_default_mode = {
	.clock		= 160000,
	.hdisplay	= 1200,
	.hsync_start	= 1260,
	.hsync_end	= 1320,
	.htotal		= 1400,
	.vdisplay	= 2000,
	.vsync_start	= 2012,
	.vsync_end	= 2054,
	.vtotal		= 2062,
	.width_mm	= 95,
	.height_mm	= 158,
	.type		= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static int vxtux_panel_prepare(struct drm_panel *panel)
{
	struct vxtux_panel *ctx = panel_to_vxtux(panel);
	int ret;

	if (ctx->prepared)
		return 0;

	if (ctx->vcc) {
		ret = regulator_enable(ctx->vcc);
		if (ret) {
			dev_err(ctx->dev, "failed to enable vcc: %d\n", ret);
			return ret;
		}
	}

	if (ctx->iovcc) {
		ret = regulator_enable(ctx->iovcc);
		if (ret) {
			dev_err(ctx->dev, "failed to enable iovcc: %d\n", ret);
			goto err_vcc;
		}
	}

	msleep(20);

	if (ctx->reset_gpio) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		usleep_range(10000, 11000);
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		usleep_range(10000, 11000);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		msleep(120);
	}

	ctx->prepared = true;
	dev_info(ctx->dev, "panel ready (default mode %ux%u)\n",
		 vxtux_default_mode.hdisplay, vxtux_default_mode.vdisplay);

	return 0;

err_vcc:
	if (ctx->vcc)
		regulator_disable(ctx->vcc);
	return ret;
}

static int vxtux_panel_unprepare(struct drm_panel *panel)
{
	struct vxtux_panel *ctx = panel_to_vxtux(panel);

	if (!ctx->prepared)
		return 0;

	if (ctx->reset_gpio)
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);

	if (ctx->iovcc)
		regulator_disable(ctx->iovcc);
	if (ctx->vcc)
		regulator_disable(ctx->vcc);

	ctx->prepared = false;
	return 0;
}

static int vxtux_panel_get_modes(struct drm_panel *panel,
				 struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &vxtux_default_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = vxtux_default_mode.width_mm;
	connector->display_info.height_mm = vxtux_default_mode.height_mm;

	return 1;
}

static const struct drm_panel_funcs vxtux_panel_funcs = {
	.prepare	= vxtux_panel_prepare,
	.unprepare	= vxtux_panel_unprepare,
	.get_modes	= vxtux_panel_get_modes,
};

static int vxtux_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct vxtux_panel *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct vxtux_panel, panel,
				   &vxtux_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->dev = dev;

	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						  GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
					     "failed to get reset GPIO\n");

	ctx->vcc = devm_regulator_get_optional(dev, "vcc");
	if (IS_ERR(ctx->vcc)) {
		if (PTR_ERR(ctx->vcc) == -ENODEV)
			ctx->vcc = NULL;
		else
			return dev_err_probe(dev, PTR_ERR(ctx->vcc),
						     "failed to get vcc regulator\n");
	}

	ctx->iovcc = devm_regulator_get_optional(dev, "iovcc");
	if (IS_ERR(ctx->iovcc)) {
		if (PTR_ERR(ctx->iovcc) == -ENODEV)
			ctx->iovcc = NULL;
		else
			return dev_err_probe(dev, PTR_ERR(ctx->iovcc),
						     "failed to get iovcc regulator\n");
	}

	/* Video mode, RGB888 (XR24/XB24 are mapped by the DPU on the CRTC side). */
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	/* drm_panel_add() mengembalikan void di 6.18 */
	drm_panel_add(&ctx->panel);

	ret = devm_mipi_dsi_attach(dev, dsi);
	if (ret) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "mipi_dsi_attach\n");
	}

	dev_info(dev, "VXTux panel registered: %ux%u, %u lanes, RGB888\n",
		 vxtux_default_mode.hdisplay, vxtux_default_mode.vdisplay,
		 dsi->lanes);
	return 0;
}

static void vxtux_panel_remove(struct mipi_dsi_device *dsi)
{
	struct vxtux_panel *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id vxtux_panel_of_match[] = {
	{ .compatible = "vxtux,ums512-panel" },
	{ .compatible = "sprd,ums512-panel" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vxtux_panel_of_match);

static struct mipi_dsi_driver vxtux_panel_driver = {
	.probe	= vxtux_panel_probe,
	.remove	= vxtux_panel_remove,
	.driver	= {
		.name		= DRV_NAME,
		.of_match_table	= vxtux_panel_of_match,
	},
};
module_mipi_dsi_driver(vxtux_panel_driver);

MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_DESCRIPTION("Generic UMS512 VXTux MIPI-DSI panel");
MODULE_LICENSE("GPL v2");
