// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_pmic_syscon.c — Unisoc PMIC global register access (sysfs debug),
 * port 5.4 -> 6.18.
 *
 * GPL source: fork Samsung Tab A8 (T618/UMS512)
 *   drivers/soc/sprd/sprd_pmic_syscon.c (C) 2018 Spreadtrum, Erick Chen.
 *
 * 6.18 adaptation (verified in tree):
 *   - remove() must be void (platform_device.h:238).
 *   - sprintf sysfs -> sysfs_emit (sysfs.h:485).
 * Regmap obtained from parent (MFD SC27xx PMIC, CONFIG_MFD_SC27XX_PMIC):
 *   /sys/.../pmic_reg  -> select register (absolute offset)
 *   /sys/.../pmic_value-> read/write selected register value
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>

struct pmic_glb {
	u32 reg;
	u32 base;
	struct regmap *regmap;
	struct device *dev;
};

static ssize_t pmic_reg_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct pmic_glb *sc27xx_glb = dev_get_drvdata(dev);

	return sysfs_emit(buf, "0x%x\n", sc27xx_glb->reg);
}

static ssize_t pmic_reg_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct pmic_glb *sc27xx_glb = dev_get_drvdata(dev);
	int ret;

	ret = kstrtou32(buf, 16, &sc27xx_glb->reg);
	if (ret) {
		dev_err(dev, "error input\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(pmic_reg);

static ssize_t pmic_value_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct pmic_glb *sc27xx_glb = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	if (sc27xx_glb->reg < sc27xx_glb->base) {
		dev_err(dev, "out of reg range\n");
		return -EINVAL;
	}

	ret = regmap_read(sc27xx_glb->regmap, sc27xx_glb->reg, &value);
	if (ret) {
		dev_err(dev, "unable to get glb\n");
		return ret;
	}

	return sysfs_emit(buf, "%x\n", value);
}

static ssize_t pmic_value_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct pmic_glb *sc27xx_glb = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtou32(buf, 16, &value);
	if (ret) {
		dev_err(dev, "error input\n");
		return -EINVAL;
	}

	if (sc27xx_glb->reg < sc27xx_glb->base) {
		dev_err(dev, "out of reg range\n");
		return -EINVAL;
	}

	ret = regmap_write(sc27xx_glb->regmap, sc27xx_glb->reg, value);
	if (ret) {
		dev_err(dev, "unable to write glb\n");
		return ret;
	}

	return count;
}
static DEVICE_ATTR_RW(pmic_value);

static struct attribute *pmic_syscon_attrs[] = {
	&dev_attr_pmic_reg.attr,
	&dev_attr_pmic_value.attr,
	NULL
};
ATTRIBUTE_GROUPS(pmic_syscon);

static int sprd_pmic_glb_probe(struct platform_device *pdev)
{
	struct pmic_glb *sc27xx_glb;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	sc27xx_glb = devm_kzalloc(dev, sizeof(*sc27xx_glb), GFP_KERNEL);
	if (!sc27xx_glb)
		return -ENOMEM;

	sc27xx_glb->regmap = dev_get_regmap(dev->parent, NULL);
	if (!sc27xx_glb->regmap) {
		dev_err(dev, "get regmap fail\n");
		return -ENODEV;
	}

	ret = of_property_read_u32_index(np, "reg", 0, &sc27xx_glb->base);
	if (ret) {
		dev_err(dev, "get base register failed\n");
		return -EINVAL;
	}

	sc27xx_glb->dev = dev;

	ret = sysfs_create_groups(&dev->kobj, pmic_syscon_groups);
	if (ret)
		dev_warn(dev, "failed to create pmic_syscon attributes\n");

	dev_set_drvdata(dev, sc27xx_glb);
	dev_info(dev, "pmic syscon ready (base=0x%x)\n", sc27xx_glb->base);

	return 0;
}

static void sprd_pmic_glb_remove(struct platform_device *pdev)
{
	sysfs_remove_groups(&pdev->dev.kobj, pmic_syscon_groups);
}

static const struct of_device_id sprd_pmic_glb_match[] = {
	{ .compatible = "sprd,sc27xx-syscon" },
	{ .compatible = "sprd,ump962x-syscon" },
	{ .compatible = "sprd,ump9621-syscon" },
	{ .compatible = "sprd,ump9622-syscon" },
	{ }
};
MODULE_DEVICE_TABLE(of, sprd_pmic_glb_match);

static struct platform_driver sprd_pmic_glb_driver = {
	.probe	= sprd_pmic_glb_probe,
	.remove	= sprd_pmic_glb_remove,
	.driver	= {
		.name		= "sprd-pmic-glb",
		.of_match_table	= sprd_pmic_glb_match,
	},
};
module_platform_driver(sprd_pmic_glb_driver);

MODULE_AUTHOR("Erick Chen <erick.chen@unisoc.com> (fork GPL)");
MODULE_AUTHOR("LinDroid/VXTux — port 6.18");
MODULE_DESCRIPTION("Spreadtrum PMIC global register debug (VXTux port)");
MODULE_LICENSE("GPL v2");
