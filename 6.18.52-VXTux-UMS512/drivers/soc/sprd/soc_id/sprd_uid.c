// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_uid.c — Unisoc device UID via nvmem efuse, port 5.4 -> 6.18.
 *
 * Sumber GPL: fork Samsung Tab A8 (T618/UMS512)
 *   drivers/misc/sprd_uid.c (C) 2019 Spreadtrum, Freeman Liu.
 *
 * Adaptasi 6.18 (terverifikasi):
 *   - remove() wajib void; of_device.h -> of.h.
 *   - sysfs sprintf -> sysfs_emit.
 *   - misc.this_device di-set otomatis oleh misc_register() sejak 5.15,
 *     jadi group dihapus langsung dari this_device (bukan kobj parent).
 *
 * DT live stok (acuan): node /sprd_uid compatible "sprd-uid",
 *   nvmem-cells = <&uid_start &uid_end> (efuse@800: uid-start@5c, uid-end@58).
 */
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#define UID_NAME	"sprd_uid"

struct sprd_uid {
	struct miscdevice misc;
	int start;
	int end;
};

static int sprd_uid_cal_read(struct device_node *np, const char *cell_id,
			     u32 *val)
{
	struct nvmem_cell *cell;
	void *buf;
	size_t len;

	cell = of_nvmem_cell_get(np, cell_id);
	if (IS_ERR(cell))
		return PTR_ERR(cell);

	buf = nvmem_cell_read(cell, &len);
	if (IS_ERR(buf)) {
		nvmem_cell_put(cell);
		return PTR_ERR(buf);
	}

	memcpy(val, buf, min_t(size_t, len, sizeof(u32)));

	kfree(buf);
	nvmem_cell_put(cell);
	return 0;
}

static ssize_t uid_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sprd_uid *uid = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%08x%08x\n", uid->start, uid->end);
}
static DEVICE_ATTR_RO(uid);

static struct attribute *uid_device[] = {
	&dev_attr_uid.attr,
	NULL,
};

static const struct attribute_group uid_attribute_group = {
	.attrs = uid_device,
};

static const struct attribute_group *uid_attribute_groups[] = {
	&uid_attribute_group,
	NULL,
};

static int sprd_uid_probe(struct platform_device *pdev)
{
	struct sprd_uid *uid;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	uid = devm_kzalloc(&pdev->dev, sizeof(*uid), GFP_KERNEL);
	if (!uid)
		return -ENOMEM;

	ret = sprd_uid_cal_read(np, "uid_start", &uid->start);
	if (ret)
		return ret;

	ret = sprd_uid_cal_read(np, "uid_end", &uid->end);
	if (ret)
		return ret;

	uid->misc.name = UID_NAME;
	uid->misc.parent = &pdev->dev;
	uid->misc.minor = MISC_DYNAMIC_MINOR;
	uid->misc.groups = uid_attribute_groups;
	ret = misc_register(&uid->misc);
	if (ret) {
		dev_err(&pdev->dev, "unable to register misc dev\n");
		return ret;
	}

	platform_set_drvdata(pdev, uid);

	return 0;
}

static void sprd_uid_remove(struct platform_device *pdev)
{
	struct sprd_uid *uid = platform_get_drvdata(pdev);

	misc_deregister(&uid->misc);
}

static const struct of_device_id sprd_uid_of_match[] = {
	{ .compatible = "sprd-uid" },
	{ },
};
MODULE_DEVICE_TABLE(of, sprd_uid_of_match);

static struct platform_driver sprd_uid_driver = {
	.probe = sprd_uid_probe,
	.remove = sprd_uid_remove,
	.driver = {
		.name = "sprd-uid",
		.of_match_table = sprd_uid_of_match,
	},
};
module_platform_driver(sprd_uid_driver);

MODULE_AUTHOR("Freeman Liu <freeman.liu@spreadtrum.com> (fork GPL)");
MODULE_AUTHOR("LinDroid/VXTux — port 6.18");
MODULE_DESCRIPTION("Spreadtrum uid driver (VXTux port)");
MODULE_LICENSE("GPL v2");
