// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc SharkL5Pro CSI controller driver - VXTux stub
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/slab.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define CSI_REG_SIZE 0x1000
#define CSI_IRQ_NUM 4

struct sprd_csi {
    struct device *dev;
    void __iomem *base;
    struct clk *clk_gate;
    struct clk *clk_csi;
    struct clk *clk_src;
    struct regmap *aon_syscon;
    struct regmap *cam_syscon;
    struct regmap *phy_regs;
    int irq[CSI_IRQ_NUM];
    int csi_id;
    int dcam_id;
    u32 ip_version;
    struct v4l2_subdev sd;
    struct media_pad pads[2];
    bool enabled;
};

static int sprd_csi_clk_enable(struct sprd_csi *csi) {
    int ret;
    ret = clk_prepare_enable(csi->clk_gate);
    if (ret) return ret;
    ret = clk_prepare_enable(csi->clk_csi);
    if (ret) goto disable_gate;
    ret = clk_prepare_enable(csi->clk_src);
    if (ret) goto disable_csi;
    return 0;
disable_csi: clk_disable_unprepare(csi->clk_csi);
disable_gate: clk_disable_unprepare(csi->clk_gate);
    return ret;
}

static void sprd_csi_clk_disable(struct sprd_csi *csi) {
    clk_disable_unprepare(csi->clk_src);
    clk_disable_unprepare(csi->clk_csi);
    clk_disable_unprepare(csi->clk_gate);
}

static irqreturn_t sprd_csi_irq_handler(int irq, void *dev_id) {
    struct sprd_csi *csi = dev_id;
    dev_dbg_ratelimited(csi->dev, "CSI%d IRQ %d\n", csi->csi_id, irq);
    return IRQ_HANDLED;
}

static int sprd_csi_subdev_init(struct sprd_csi *csi) {
    struct v4l2_subdev *sd = &csi->sd;
    int ret;
    v4l2_subdev_init(sd, NULL);
    sd->dev = csi->dev;
    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    snprintf(sd->name, sizeof(sd->name), "sprd-csi%d", csi->csi_id);
    csi->pads[0].flags = MEDIA_PAD_FL_SINK;
    csi->pads[1].flags = MEDIA_PAD_FL_SOURCE;
    ret = media_entity_pads_init(&sd->entity, 2, csi->pads);
    if (ret) return ret;
    return 0;
}

static int sprd_csi_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct sprd_csi *csi;
    struct resource *res;
    int ret, i;

    csi = devm_kzalloc(dev, sizeof(*csi), GFP_KERNEL);
    if (!csi) return -ENOMEM;
    csi->dev = dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    csi->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(csi->base)) return PTR_ERR(csi->base);

    csi->clk_gate = devm_clk_get(dev, "clk_mipi_csi_gate_eb");
    if (IS_ERR(csi->clk_gate)) return PTR_ERR(csi->clk_gate);
    csi->clk_csi = devm_clk_get(dev, "clk_csi_eb");
    if (IS_ERR(csi->clk_csi)) return PTR_ERR(csi->clk_csi);
    csi->clk_src = devm_clk_get(dev, "mipi_csi_src_eb");
    if (IS_ERR(csi->clk_src)) return PTR_ERR(csi->clk_src);

    csi->aon_syscon = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,aon-apb-syscon");
    csi->cam_syscon = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,cam-ahb-syscon");
    csi->phy_regs = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,anlg-phy-g10-controller");

    of_property_read_u32(dev->of_node, "sprd,csi-id", &csi->csi_id);
    of_property_read_u32(dev->of_node, "sprd,dcam-id", &csi->dcam_id);
    of_property_read_u32(dev->of_node, "sprd,ip-version", &csi->ip_version);

    for (i = 0; i < CSI_IRQ_NUM; i++) {
        csi->irq[i] = platform_get_irq(pdev, i);
        if (csi->irq[i] < 0) return csi->irq[i];
        ret = devm_request_irq(dev, csi->irq[i], sprd_csi_irq_handler, IRQF_SHARED, "sprd-csi", csi);
        if (ret) return ret;
    }

    platform_set_drvdata(pdev, csi);
    pm_runtime_enable(dev);
    pm_runtime_set_active(dev);
    ret = pm_runtime_get_sync(dev);
    if (ret < 0) { pm_runtime_disable(dev); return ret; }

    ret = sprd_csi_clk_enable(csi);
    if (ret) { pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }

    ret = sprd_csi_subdev_init(csi);
    if (ret) { sprd_csi_clk_disable(csi); pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }

    dev_info(dev, "Unisoc SharkL5Pro CSI%d probed (stub)\n", csi->csi_id);
    dev_warn(dev, "Full CSI-2 programming requires vendor GPL source\n");
    csi->enabled = true;
    of_platform_populate(dev->of_node, NULL, NULL, dev);
    return 0;
}

static void sprd_csi_remove(struct platform_device *pdev) {
    struct sprd_csi *csi = platform_get_drvdata(pdev);
    if (csi->enabled) { sprd_csi_clk_disable(csi); media_entity_cleanup(&csi->sd.entity); }
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
}

static int __maybe_unused sprd_csi_suspend(struct device *dev) {
    struct sprd_csi *csi = dev_get_drvdata(dev);
    if (csi->enabled) sprd_csi_clk_disable(csi);
    return 0;
}

static int __maybe_unused sprd_csi_resume(struct device *dev) {
    struct sprd_csi *csi = dev_get_drvdata(dev);
    return sprd_csi_clk_enable(csi);
}

static const struct dev_pm_ops sprd_csi_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sprd_csi_suspend, sprd_csi_resume)
};

static const struct of_device_id sprd_csi_of_match[] = {
    { .compatible = "sprd,sharkl5pro-csi" },
    { .compatible = "sprd,csi-controller" },
    { },
};
MODULE_DEVICE_TABLE(of, sprd_csi_of_match);

static struct platform_driver sprd_csi_driver = {
    .probe = sprd_csi_probe,
    .remove = sprd_csi_remove,
    .driver = {
        .name = "sprd-csi",
        .of_match_table = sprd_csi_of_match,
        .pm = &sprd_csi_pm_ops,
        .suppress_bind_attrs = true,
    },
};
module_platform_driver(sprd_csi_driver);

MODULE_DESCRIPTION("Unisoc SharkL5Pro CSI controller driver (stub)");
MODULE_AUTHOR("VXTux Team");
MODULE_LICENSE("GPL");
