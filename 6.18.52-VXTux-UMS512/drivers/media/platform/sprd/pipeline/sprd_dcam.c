// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc SharkL5Pro DCAM driver - VXTux stub
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define DCAM_REG_COUNT 4
#define DCAM_IRQ_NUM 3

struct sprd_dcam {
    struct device *dev;
    void __iomem *base[DCAM_REG_COUNT];
    struct clk *clk_dcam;
    struct clk *clk_axi;
    struct clk *clk_parent;
    int irq[DCAM_IRQ_NUM];
    int dcam_count;
    int superzoom_id;
    int project_id;
    struct platform_device *isp_pdev;
    /* Assigned in the probe from "sprd,cam-ahb-syscon". The field had been
     * dropped from this struct while the assignment to it stayed, so the
     * build failed with "no member named 'cam_syscon'". */
    struct regmap *cam_syscon;
    struct v4l2_subdev sd;
    struct media_pad pads[3];
    bool enabled;
};

static int sprd_dcam_clk_enable(struct sprd_dcam *dcam) {
    int ret;
    ret = clk_prepare_enable(dcam->clk_dcam);
    if (ret) return ret;
    ret = clk_prepare_enable(dcam->clk_axi);
    if (ret) goto disable_dcam;
    ret = clk_prepare_enable(dcam->clk_parent);
    if (ret) goto disable_axi;
    return 0;
disable_axi: clk_disable_unprepare(dcam->clk_axi);
disable_dcam: clk_disable_unprepare(dcam->clk_dcam);
    return ret;
}

static void sprd_dcam_clk_disable(struct sprd_dcam *dcam) {
    clk_disable_unprepare(dcam->clk_parent);
    clk_disable_unprepare(dcam->clk_axi);
    clk_disable_unprepare(dcam->clk_dcam);
}

static irqreturn_t sprd_dcam_irq_handler(int irq, void *dev_id) {
    struct sprd_dcam *dcam = dev_id;
    dev_dbg_ratelimited(dcam->dev, "DCAM IRQ %d\n", irq);
    return IRQ_HANDLED;
}

static int sprd_dcam_subdev_init(struct sprd_dcam *dcam) {
    struct v4l2_subdev *sd = &dcam->sd;
    int ret;
    v4l2_subdev_init(sd, NULL);
    sd->dev = dcam->dev;
    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    snprintf(sd->name, sizeof(sd->name), "sprd-dcam");
    dcam->pads[0].flags = MEDIA_PAD_FL_SINK;
    dcam->pads[1].flags = MEDIA_PAD_FL_SINK;
    dcam->pads[2].flags = MEDIA_PAD_FL_SINK;
    ret = media_entity_pads_init(&sd->entity, 3, dcam->pads);
    if (ret) return ret;
    return 0;
}

static int sprd_dcam_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct sprd_dcam *dcam;
    struct resource *res;
    int ret, i;

    dcam = devm_kzalloc(dev, sizeof(*dcam), GFP_KERNEL);
    if (!dcam) return -ENOMEM;
    dcam->dev = dev;

    for (i = 0; i < DCAM_REG_COUNT; i++) {
        res = platform_get_resource(pdev, IORESOURCE_MEM, i);
        if (!res) break;
        dcam->base[i] = devm_ioremap_resource(dev, res);
        if (IS_ERR(dcam->base[i])) return PTR_ERR(dcam->base[i]);
    }

    dcam->clk_dcam = devm_clk_get(dev, "dcam_eb");
    if (IS_ERR(dcam->clk_dcam)) return PTR_ERR(dcam->clk_dcam);
    dcam->clk_axi = devm_clk_get(dev, "dcam_axi_eb");
    if (IS_ERR(dcam->clk_axi)) return PTR_ERR(dcam->clk_axi);
    dcam->clk_parent = devm_clk_get(dev, "dcam_clk_parent");
    if (IS_ERR(dcam->clk_parent)) return PTR_ERR(dcam->clk_parent);

    dcam->cam_syscon = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,cam-ahb-syscon");
    dcam->isp_pdev = of_find_device_by_node(of_parse_phandle(dev->of_node, "sprd,isp", 0));

    of_property_read_u32(dev->of_node, "sprd,dcam-count", &dcam->dcam_count);
    of_property_read_u32(dev->of_node, "sprd,dcam-superzoom", &dcam->superzoom_id);
    of_property_read_u32(dev->of_node, "sprd,project-id", &dcam->project_id);

    for (i = 0; i < DCAM_IRQ_NUM; i++) {
        dcam->irq[i] = platform_get_irq(pdev, i);
        if (dcam->irq[i] < 0) return dcam->irq[i];
        ret = devm_request_irq(dev, dcam->irq[i], sprd_dcam_irq_handler, IRQF_SHARED, "sprd-dcam", dcam);
        if (ret) return ret;
    }

    platform_set_drvdata(pdev, dcam);
    pm_runtime_enable(dev);
    pm_runtime_set_active(dev);
    ret = pm_runtime_get_sync(dev);
    if (ret < 0) { pm_runtime_disable(dev); return ret; }

    ret = sprd_dcam_clk_enable(dcam);
    if (ret) { pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }

    ret = sprd_dcam_subdev_init(dcam);
    if (ret) { sprd_dcam_clk_disable(dcam); pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }

    dev_info(dev, "Unisoc SharkL5Pro DCAM probed (stub)\n");
    dev_warn(dev, "Full DCAM programming requires vendor GPL source\n");
    dcam->enabled = true;
    of_platform_populate(dev->of_node, NULL, NULL, dev);
    return 0;
}

static void sprd_dcam_remove(struct platform_device *pdev) {
    struct sprd_dcam *dcam = platform_get_drvdata(pdev);
    if (dcam->enabled) { sprd_dcam_clk_disable(dcam); media_entity_cleanup(&dcam->sd.entity); }
    if (dcam->isp_pdev) platform_device_put(dcam->isp_pdev);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
}

static int __maybe_unused sprd_dcam_suspend(struct device *dev) {
    struct sprd_dcam *dcam = dev_get_drvdata(dev);
    if (dcam->enabled) sprd_dcam_clk_disable(dcam);
    return 0;
}

static int __maybe_unused sprd_dcam_resume(struct device *dev) {
    struct sprd_dcam *dcam = dev_get_drvdata(dev);
    return sprd_dcam_clk_enable(dcam);
}

static const struct dev_pm_ops sprd_dcam_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sprd_dcam_suspend, sprd_dcam_resume)
};

static const struct of_device_id sprd_dcam_of_match[] = {
    { .compatible = "sprd,sharkl5pro-cam" },
    { .compatible = "sprd,sharkl5pro-dcam" },
    { },
};
MODULE_DEVICE_TABLE(of, sprd_dcam_of_match);

static struct platform_driver sprd_dcam_driver = {
    .probe = sprd_dcam_probe,
    .remove = sprd_dcam_remove,
    .driver = {
        .name = "sprd-dcam",
        .of_match_table = sprd_dcam_of_match,
        .pm = &sprd_dcam_pm_ops,
        .suppress_bind_attrs = true,
    },
};
module_platform_driver(sprd_dcam_driver);

MODULE_DESCRIPTION("Unisoc SharkL5Pro DCAM driver (stub)");
MODULE_AUTHOR("VXTux Team");
MODULE_LICENSE("GPL");
