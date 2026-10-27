// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_7sreset - PMIC 7s (long-press) reset controller.
 *
 * Port of the vendor 5.4 module `sprd_7sreset.ko` (15 fungsi, 23.350 B).
 * Bukti: docs/evidence-20261023/sprd_7sreset.objdump.txt (objdump -dr
 * dengan relokasi) dan ringkasan docs/RE-SPRD-7SRESET.md.
 *
 * Model yang terbaca dari blob:
 *
 *   struct sprd_7sreset (32 B, devm_kmalloc(dev, 32))
 *     +0x00 struct device *dev
 *     +0x08 struct regmap *regmap     (dev_get_regmap(dev->parent))
 *     +0x10 u32 reg                    (DT "reg" index 0)
 *     +0x14 u32 reg_ctrl               (DT "reg" index 1)
 *     +0x18 const struct sprd_7sreset_data *data
 *
 *   struct sprd_7sreset_data (16 B, .data)
 *     +0x00 u32 chip_id   0x2720/0x2721/0x2730/0x2731/0x9620
 *     +0x04 u32 reserved
 *     +0x08 u32 unlock_reg  (hanya UMP9620: 0x23d8)
 *     +0x0c u32 reserved2
 *
 * Setiap atribut sysfs memakai pola yang sama: kstrtoint(buf, 10, &v),
 * lalu regmap_update_bits(regmap, reg_ctrl, mask, bit) dengan
 *   module_disable  mask 0x002  bit 1
 *   hard_mode       mask 0x001  bit 0
 *   short_mode      mask 0x100  bit 8
 *   keypad_mode     mask 0x200  bit 9 (nilai dibalik: tulis 0 = bit 1)
 *   threshold_time  mask 0x0f0  bit 4..7 (selalu 0xf0)
 * dan bila data->unlock_reg != 0 didahului/diakhiri "kunci" 0x3991/0x3990
 * dengan bit 15 sebagai status.
 *
 * Show: regmap_read(regmap, reg_ctrl) lalu scnprintf("%d\n", field).
 * keypad_mode memakai reg (index 0) kecuali chip_id == 0x2731 (SC2731),
 * yang memakai reg_ctrl (index 1) — persis percabangan di blob.
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>

#define SPRD_7SRESET_UNLOCK_MASK	0x3991
#define SPRD_7SRESET_UNLOCK_SET		0x7fff
#define SPRD_7SRESET_LOCK_MASK		0x3990

struct sprd_7sreset_data {
	u32 chip_id;
	u32 reserved;
	u32 unlock_reg;
	u32 reserved2;
};

struct sprd_7sreset {
	struct device *dev;
	struct regmap *regmap;
	u32 reg;
	u32 reg_ctrl;
	const struct sprd_7sreset_data *data;
};

static const struct sprd_7sreset_data sc2720_7sreset_data = {
	.chip_id = 0x2720,
};

static const struct sprd_7sreset_data sc2721_7sreset_data = {
	.chip_id = 0x2721,
};

static const struct sprd_7sreset_data sc2730_7sreset_data = {
	.chip_id = 0x2730,
};

static const struct sprd_7sreset_data sc2731_7sreset_data = {
	.chip_id = 0x2731,
};

static const struct sprd_7sreset_data ump9620_7sreset_data = {
	.chip_id = 0x9620,
	.unlock_reg = 0x23d8,
};

/* --- helper: tulis satu field lewat regmap ------------------------------ */

/*
 * Urutan blob: bila chip punya unlock_reg, register itu dikunci dulu
 * (mask 0x3991, nilai 0x7fff), dibaca balik, dan bit 15 harus set; setelah
 * field utama ditulis, register yang sama dikunci lagi (mask 0x3990) dan
 * bit 15 tidak boleh set.
 */
static int sprd_7sreset_unlock(struct sprd_7sreset *info)
{
	const struct sprd_7sreset_data *d = info->data;
	u32 val;
	int ret;

	if (!d->unlock_reg)
		return 0;

	regmap_update_bits(info->regmap, d->unlock_reg,
			   SPRD_7SRESET_UNLOCK_MASK, SPRD_7SRESET_UNLOCK_SET);
	ret = regmap_read(info->regmap, d->unlock_reg, &val);
	if (ret)
		return ret;

	return (val & BIT(15)) ? 0 : -EINVAL;
}

static int sprd_7sreset_lock(struct sprd_7sreset *info)
{
	const struct sprd_7sreset_data *d = info->data;
	u32 val;
	int ret;

	if (!d->unlock_reg)
		return 0;

	regmap_update_bits(info->regmap, d->unlock_reg,
			   SPRD_7SRESET_LOCK_MASK, SPRD_7SRESET_UNLOCK_SET);
	ret = regmap_read(info->regmap, d->unlock_reg, &val);
	if (ret)
		return ret;

	return (val & BIT(15)) ? -EINVAL : 0;
}

static int sprd_7sreset_store_bit(struct device *dev, u32 mask, u32 val)
{
	struct sprd_7sreset *info = dev_get_drvdata(dev);
	int ret;

	ret = sprd_7sreset_unlock(info);
	if (ret)
		return ret;

	ret = regmap_update_bits(info->regmap, info->reg_ctrl, mask, val);
	if (ret)
		return ret;

	return sprd_7sreset_lock(info);
}

/*
 * Blob membaca register lewat regmap_read() lalu mencetak field-nya dengan
 * scnprintf(buf, 0x1000, "%d\n", ...); bila regmap_read gagal, nilai error
 * itulah yang dicetak (csel di setiap fungsi show).
 */
static ssize_t sprd_7sreset_show_field(struct device *dev, char *buf,
				       u32 mask, u32 shift)
{
	struct sprd_7sreset *info = dev_get_drvdata(dev);
	u32 val;
	int ret;

	ret = regmap_read(info->regmap, info->reg_ctrl, &val);
	if (ret)
		return scnprintf(buf, PAGE_SIZE, "%d\n", ret);

	return scnprintf(buf, PAGE_SIZE, "%d\n", (val & mask) >> shift);
}

/* --- atribut sysfs ------------------------------------------------------ */

/* blob: regmap_read(info->reg_ctrl) lalu (val >> 4) & 0xf */
static ssize_t threshold_time_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprd_7sreset_show_field(dev, buf, GENMASK(7, 4), 4);
}

static ssize_t threshold_time_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int val, ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;

	/* blob mengabaikan nilai yang ditulis dan selalu men-set 0xf0 */
	ret = sprd_7sreset_store_bit(dev, 0xf0, 0xf0);
	return ret ? ret : count;
}

/* blob: regmap_read(reg / reg_ctrl tergantung chip) lalu ~(val >> 9) & 1 */
static ssize_t keypad_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sprd_7sreset *info = dev_get_drvdata(dev);
	u32 reg = info->data->chip_id == 0x2731 ? info->reg_ctrl : info->reg;
	u32 val;
	int ret;

	ret = regmap_read(info->regmap, reg, &val);
	if (ret)
		return scnprintf(buf, PAGE_SIZE, "%d\n", ret);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ((val >> 9) & 1) ? 0 : 1);
}

static ssize_t keypad_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;

	/* bit 9 aktif-rendah: tulis 0 men-set bit */
	ret = sprd_7sreset_store_bit(dev, 0x200, (!val) << 9);
	return ret ? ret : count;
}

/* blob: (val >> 8) & 1 */
static ssize_t short_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprd_7sreset_show_field(dev, buf, BIT(8), 8);
}

static ssize_t short_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int val, ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;

	ret = sprd_7sreset_store_bit(dev, 0x100, (val != 0) << 8);
	return ret ? ret : count;
}

/* blob: val & 1 */
static ssize_t hard_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprd_7sreset_show_field(dev, buf, BIT(0), 0);
}

static ssize_t hard_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int val, ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;

	ret = sprd_7sreset_store_bit(dev, 0x1, val != 0);
	return ret ? ret : count;
}

/* blob: (val >> 1) & 1 */
static ssize_t module_disable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprd_7sreset_show_field(dev, buf, BIT(1), 1);
}

static ssize_t module_disable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int val, ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return ret;

	ret = sprd_7sreset_store_bit(dev, 0x2, (val != 0) << 1);
	return ret ? ret : count;
}

static DEVICE_ATTR_RW(module_disable);
static DEVICE_ATTR_RW(hard_mode);
static DEVICE_ATTR_RW(short_mode);
static DEVICE_ATTR_RW(keypad_mode);
static DEVICE_ATTR_RW(threshold_time);

static struct attribute *sprd_7sreset_attrs[] = {
	&dev_attr_module_disable.attr,
	&dev_attr_hard_mode.attr,
	&dev_attr_short_mode.attr,
	&dev_attr_keypad_mode.attr,
	&dev_attr_threshold_time.attr,
	NULL,
};

static const struct attribute_group sprd_7sreset_group = {
	.attrs = sprd_7sreset_attrs,
};

static const struct attribute_group *sprd_7sreset_groups[] = {
	&sprd_7sreset_group,
	NULL,
};

/* --- lifecycle ---------------------------------------------------------- */

static int sprd_7sreset_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sprd_7sreset *info;
	u32 val;
	int ret;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	info->regmap = dev_get_regmap(dev->parent, NULL);
	if (!info->regmap) {
		dev_err(dev, "NULL regmap for pmic\n");
		return -EINVAL;
	}

	info->data = of_device_get_match_data(dev);
	if (!info->data) {
		dev_err(dev, "get id of 7sreset private failed!\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_index(np, "reg", 0, &val);
	if (ret)
		return ret;
	info->reg = val;

	ret = of_property_read_u32_index(np, "reg", 1, &val);
	if (ret)
		return ret;
	info->reg_ctrl = val;

	/* drvdata dipasang lebih dulu supaya atribut sysfs yang langsung
	 * dibaca tidak melihat NULL (blob memasangnya setelah grup).
	 */
	platform_set_drvdata(pdev, info);

	ret = sysfs_create_groups(&dev->kobj, sprd_7sreset_groups);
	if (ret) {
		dev_err(dev, "failed to create device attributes.\n");
		return ret;
	}

	return 0;
}

static void sprd_7sreset_remove(struct platform_device *pdev)
{
	sysfs_remove_groups(&pdev->dev.kobj, sprd_7sreset_groups);
}

static const struct of_device_id sprd_7sreset_of_match[] = {
	{ .compatible = "sprd,sc2720-7sreset", .data = &sc2720_7sreset_data },
	{ .compatible = "sprd,sc2721-7sreset", .data = &sc2721_7sreset_data },
	{ .compatible = "sprd,sc2730-7sreset", .data = &sc2730_7sreset_data },
	{ .compatible = "sprd,sc2731-7sreset", .data = &sc2731_7sreset_data },
	{ .compatible = "sprd,ump9620-7sreset", .data = &ump9620_7sreset_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sprd_7sreset_of_match);

static struct platform_driver sprd_7sreset_driver = {
	.probe = sprd_7sreset_probe,
	.remove = sprd_7sreset_remove,
	.driver = {
		.name = "sprd-7sreset",
		.of_match_table = sprd_7sreset_of_match,
	},
};
module_platform_driver(sprd_7sreset_driver);

MODULE_DESCRIPTION("Spreadtrum PMIC 7s reset driver");
MODULE_LICENSE("GPL v2");
