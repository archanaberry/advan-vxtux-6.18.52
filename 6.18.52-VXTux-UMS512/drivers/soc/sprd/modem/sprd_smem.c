// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc Shared Memory driver for Modem IPC
 * Provides shared memory regions for AP<->CP communication
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

struct sprd_smem_region {
	char name[32];
	phys_addr_t phys;
	size_t size;
	void __iomem *virt;
	struct list_head list;
};

struct sprd_smem {
	struct device *dev;
	struct list_head regions;
	struct mutex lock;
};

static LIST_HEAD(sprd_smem_regions);
static DEFINE_MUTEX(sprd_smem_mutex);

int sprd_smem_register_region(const char *name, phys_addr_t phys, size_t size)
{
	struct sprd_smem_region *region;

	region = kzalloc(sizeof(*region), GFP_KERNEL);
	if (!region)
		return -ENOMEM;

	strscpy(region->name, name, sizeof(region->name));
	region->phys = phys;
	region->size = size;
	region->virt = ioremap(phys, size);
	if (!region->virt) {
		kfree(region);
		return -ENOMEM;
	}

	mutex_lock(&sprd_smem_mutex);
	list_add_tail(&region->list, &sprd_smem_regions);
	mutex_unlock(&sprd_smem_mutex);

	pr_info("sprd_smem: registered region '%s' @ 0x%pa +%zx\n", name, &phys, size);
	return 0;
}
EXPORT_SYMBOL(sprd_smem_register_region);

void sprd_smem_unregister_region(const char *name)
{
	struct sprd_smem_region *region, *tmp;

	mutex_lock(&sprd_smem_mutex);
	list_for_each_entry_safe(region, tmp, &sprd_smem_regions, list) {
		if (!strcmp(region->name, name)) {
			list_del(&region->list);
			iounmap(region->virt);
			kfree(region);
			break;
		}
	}
	mutex_unlock(&sprd_smem_mutex);
}
EXPORT_SYMBOL(sprd_smem_unregister_region);

void *sprd_smem_get_region(const char *name, size_t *size)
{
	struct sprd_smem_region *region;
	void *virt = NULL;

	mutex_lock(&sprd_smem_mutex);
	list_for_each_entry(region, &sprd_smem_regions, list) {
		if (!strcmp(region->name, name)) {
			virt = region->virt;
			if (size)
				*size = region->size;
			break;
		}
	}
	mutex_unlock(&sprd_smem_mutex);

	return virt;
}
EXPORT_SYMBOL(sprd_smem_get_region);

static int sprd_smem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child;
	phys_addr_t phys;
	size_t size;
	int ret;

	dev_info(dev, "Unisoc Shared Memory driver probed\n");

	for_each_child_of_node(np, child) {
		ret = of_property_read_u64(child, "reg", (u64 *)&phys);
		if (ret)
			continue;

		ret = of_property_read_u64(child, "size", (u64 *)&size);
		if (ret)
			size = 0x100000; /* default 1MB */

		ret = sprd_smem_register_region(child->name, phys, size);
		if (ret)
			dev_warn(dev, "Failed to register region '%s'\n", child->name);
	}

	return 0;
}

static const struct of_device_id sprd_smem_of_match[] = {
	{ .compatible = "sprd,modem-shmem" },
	{ .compatible = "unisoc,modem-shmem" },
	{ },
};
MODULE_DEVICE_TABLE(of, sprd_smem_of_match);

static struct platform_driver sprd_smem_driver = {
	.probe = sprd_smem_probe,
	.driver = {
		.name = "sprd-smem",
		.of_match_table = sprd_smem_of_match,
	},
};
module_platform_driver(sprd_smem_driver);

MODULE_DESCRIPTION("Unisoc Shared Memory driver");
MODULE_AUTHOR("VXTux Team");
MODULE_LICENSE("GPL");
