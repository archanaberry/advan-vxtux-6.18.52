// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc SharkL5Pro ISP driver - VXTux stub
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

#define ISP_REG_SIZE 0x100000
#define ISP_IRQ_NUM 2

struct sprd_isp {
    struct device *dev;
    void __iomem *base;
    struct clk *clk_isp;
    struct clk *clk_axi;
    struct clk *clk_parent;
    struct regmap *syscon;
    int irq[ISP_IRQ_NUM];
    bool enabled;
};

static int sprd_isp_clk_enable(struct sprd_isp *isp) {
    int ret;
    ret = clk_prepare_enable(isp->clk_isp);
    if (ret) return ret;
    ret = clk_prepare_enable(isp->clk_axi);
    if (ret) goto disable_isp;
    ret = clk_prepare_enable(isp->clk_parent);
    if (ret) goto disable_axi;
    return 0;
disable_axi: clk_disable_unprepare(isp->clk_axi);
disable_isp: clk_disable_unprepare(isp->clk_isp);
    return ret;
}

static void sprd_isp_clk_disable(struct sprd_isp *isp) {
    clk_disable_unprepare(isp->clk_parent);
    clk_disable_unprepare(isp->clk_axi);
    clk_disable_unprepare(isp->clk_isp);
}

static irqreturn_t sprd_isp_irq_handler(int irq, void *dev_id) {
    struct sprd_isp *isp = dev_id;
    dev_dbg_ratelimited(isp->dev, "ISP IRQ %d\n", irq);
    return IRQ_HANDLED;
}

static int sprd_isp_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct sprd_isp *isp;
    struct resource *res;
    int ret, i;

    isp = devm_kzalloc(dev, sizeof(*isp), GFP_KERNEL);
    if (!isp) return -ENOMEM;
    isp->dev = dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    isp->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(isp->base)) return PTR_ERR(isp->base);

    isp->clk_isp = devm_clk_get(dev, "isp_eb");
    if (IS_ERR(isp->clk_isp)) return PTR_ERR(isp->clk_isp);
    isp->clk_axi = devm_clk_get(dev, "isp_axi_eb");
    if (IS_ERR(isp->clk_axi)) return PTR_ERR(isp->clk_axi);
    isp->clk_parent = devm_clk_get(dev, "isp_clk_parent");
    if (IS_ERR(isp->clk_parent)) return PTR_ERR(isp->clk_parent);

    isp->syscon = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,cam-ahb-syscon");
    if (IS_ERR(isp->syscon)) dev_warn(dev, "Failed to get cam-ahb-syscon\n");

    for (i = 0; i < ISP_IRQ_NUM; i++) {
        isp->irq[i] = platform_get_irq(pdev, i);
        if (isp->irq[i] < 0) return isp->irq[i];
        ret = devm_request_irq(dev, isp->irq[i], sprd_isp_irq_handler, IRQF_SHARED, "sprd-isp", isp);
        if (ret) return ret;
    }

    platform_set_drvdata(pdev, isp);
    pm_runtime_enable(dev);
    pm_runtime_set_active(dev);
    ret = pm_runtime_get_sync(dev);
    if (ret < 0) { pm_runtime_disable(dev); return ret; }

    ret = sprd_isp_clk_enable(isp);
    if (ret) { pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }

    dev_info(dev, "Unisoc SharkL5Pro ISP probed (stub)\n");
    dev_warn(dev, "Full ISP programming requires vendor GPL source\n");
    isp->enabled = true;
    of_platform_populate(dev->of_node, NULL, NULL, dev);
    return 0;
}

static void sprd_isp_remove(struct platform_device *pdev) {
    struct sprd_isp *isp = platform_get_drvdata(pdev);
    if (isp->enabled) sprd_isp_clk_disable(isp);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
}

static int __maybe_unused sprd_isp_suspend(struct device *dev) {
    struct sprd_isp *isp = dev_get_drvdata(dev);
    if (isp->enabled) sprd_isp_clk_disable(isp);
    return 0;
}

static int __maybe_unused sprd_isp_resume(struct device *dev) {
    struct sprd_isp *isp = dev_get_drvdata(dev);
    return sprd_isp_clk_enable(isp);
}

static const struct dev_pm_ops sprd_isp_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sprd_isp_suspend, sprd_isp_resume)
};

static const struct of_device_id sprd_isp_of_match[] = {
    { .compatible = "sprd,sharkl5pro-isp" },
    { .compatible = "sprd,isp" },
    { },
};
MODULE_DEVICE_TABLE(of, sprd_isp_of_match);

static struct platform_driver sprd_isp_driver = {
    .probe = sprd_isp_probe,
    .remove = sprd_isp_remove,
    .driver = {
        .name = "sprd-isp",
        .of_match_table = sprd_isp_of_match,
        .pm = &sprd_isp_pm_ops,
    },
};
module_platform_driver(sprd_isp_driver);

MODULE_DESCRIPTION("Unisoc SharkL5Pro ISP driver (stub)");
MODULE_AUTHOR("VXTux Team");
MODULE_LICENSE("GPL");
