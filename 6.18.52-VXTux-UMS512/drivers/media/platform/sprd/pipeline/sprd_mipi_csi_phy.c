// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc MIPI CSI PHY driver - VXTux stub
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>

struct sprd_mipi_csi_phy {
    struct device *dev;
    void __iomem *base;
    struct phy *phy;
    struct regmap *regmap;
    struct clk *clk;
    bool enabled;
};

static int sprd_mipi_csi_phy_init(struct phy *phy) {
    struct sprd_mipi_csi_phy *sphy = phy_get_drvdata(phy);
    dev_dbg(sphy->dev, "PHY init\n");
    return 0;
}

static int sprd_mipi_csi_phy_exit(struct phy *phy) {
    struct sprd_mipi_csi_phy *sphy = phy_get_drvdata(phy);
    dev_dbg(sphy->dev, "PHY exit\n");
    return 0;
}

static int sprd_mipi_csi_phy_power_on(struct phy *phy) {
    struct sprd_mipi_csi_phy *sphy = phy_get_drvdata(phy);
    int ret = clk_prepare_enable(sphy->clk);
    if (ret) return ret;
    dev_dbg(sphy->dev, "PHY power on\n");
    return 0;
}

static int sprd_mipi_csi_phy_power_off(struct phy *phy) {
    struct sprd_mipi_csi_phy *sphy = phy_get_drvdata(phy);
    clk_disable_unprepare(sphy->clk);
    dev_dbg(sphy->dev, "PHY power off\n");
    return 0;
}

static const struct phy_ops sprd_mipi_csi_phy_ops = {
    .init = sprd_mipi_csi_phy_init,
    .exit = sprd_mipi_csi_phy_exit,
    .power_on = sprd_mipi_csi_phy_power_on,
    .power_off = sprd_mipi_csi_phy_power_off,
    .owner = THIS_MODULE,
};

static int sprd_mipi_csi_phy_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct sprd_mipi_csi_phy *sphy;
    struct resource *res;
    struct phy_provider *phy_provider;
    int ret;

    sphy = devm_kzalloc(dev, sizeof(*sphy), GFP_KERNEL);
    if (!sphy) return -ENOMEM;
    sphy->dev = dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    sphy->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(sphy->base)) return PTR_ERR(sphy->base);

    sphy->clk = devm_clk_get(dev, NULL);
    if (IS_ERR(sphy->clk)) { dev_warn(dev, "No clock for PHY\n"); sphy->clk = NULL; }

    sphy->regmap = dev_get_regmap(dev->parent, NULL);
    if (IS_ERR(sphy->regmap)) dev_warn(dev, "No parent regmap\n");

    platform_set_drvdata(pdev, sphy);
    pm_runtime_enable(dev);
    pm_runtime_set_active(dev);
    ret = pm_runtime_get_sync(dev);
    if (ret < 0) { pm_runtime_disable(dev); return ret; }

    if (sphy->clk) {
        ret = clk_prepare_enable(sphy->clk);
        if (ret) { pm_runtime_put_sync(dev); pm_runtime_disable(dev); return ret; }
    }

    sphy->phy = devm_phy_create(dev, NULL, &sprd_mipi_csi_phy_ops);
    if (IS_ERR(sphy->phy)) {
        ret = PTR_ERR(sphy->phy);
        if (sphy->clk) clk_disable_unprepare(sphy->clk);
        pm_runtime_put_sync(dev); pm_runtime_disable(dev);
        return ret;
    }

    phy_set_drvdata(sphy->phy, sphy);
    phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
    if (IS_ERR(phy_provider)) {
        ret = PTR_ERR(phy_provider);
        if (sphy->clk) clk_disable_unprepare(sphy->clk);
        pm_runtime_put_sync(dev); pm_runtime_disable(dev);
        return ret;
    }

    dev_info(dev, "Unisoc MIPI CSI PHY probed (stub)\n");
    dev_warn(dev, "Full PHY programming requires vendor GPL source\n");
    sphy->enabled = true;
    return 0;
}

static void sprd_mipi_csi_phy_remove(struct platform_device *pdev) {
    struct sprd_mipi_csi_phy *sphy = platform_get_drvdata(pdev);
    if (sphy->enabled && sphy->clk) clk_disable_unprepare(sphy->clk);
    pm_runtime_put_sync(&pdev->dev);
    pm_runtime_disable(&pdev->dev);
}

static int __maybe_unused sprd_mipi_csi_phy_suspend(struct device *dev) {
    struct sprd_mipi_csi_phy *sphy = dev_get_drvdata(dev);
    if (sphy->enabled && sphy->clk) clk_disable_unprepare(sphy->clk);
    return 0;
}

static int __maybe_unused sprd_mipi_csi_phy_resume(struct device *dev) {
    struct sprd_mipi_csi_phy *sphy = dev_get_drvdata(dev);
    if (sphy->clk) return clk_prepare_enable(sphy->clk);
    return 0;
}

static const struct dev_pm_ops sprd_mipi_csi_phy_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sprd_mipi_csi_phy_suspend, sprd_mipi_csi_phy_resume)
};

static const struct of_device_id sprd_mipi_csi_phy_of_match[] = {
    { .compatible = "sprd,mipi-csi-phy" },
    { },
};
MODULE_DEVICE_TABLE(of, sprd_mipi_csi_phy_of_match);

static struct platform_driver sprd_mipi_csi_phy_driver = {
    .probe = sprd_mipi_csi_phy_probe,
    .remove = sprd_mipi_csi_phy_remove,
    .driver = {
        .name = "sprd-mipi-csi-phy",
        .of_match_table = sprd_mipi_csi_phy_of_match,
        .pm = &sprd_mipi_csi_phy_pm_ops,
    },
};
module_platform_driver(sprd_mipi_csi_phy_driver);

MODULE_DESCRIPTION("Unisoc MIPI CSI PHY driver (stub)");
MODULE_AUTHOR("VXTux Team");
MODULE_LICENSE("GPL");
