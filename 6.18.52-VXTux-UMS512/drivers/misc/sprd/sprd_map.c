// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_map.c — Unisoc map_user (mmap memori reserved), port 5.4 -> 6.18.
 *
 * Sumber GPL: fork Samsung Tab A8 (T618/UMS512) drivers/misc/sprd_map/sprd_map.c
 *   (C) 2019 Unisoc Communications, Author: Sheng Xu <sheng.xu@unisoc.com>
 *
 * Adaptasi 6.18 (terverifikasi di tree):
 *   - remove() wajib void (platform_device.h:238).
 *   - jalur compat_ioctl 32-bit dibuang: compat_alloc_user_space tidak tersedia
 *     untuk build ini; tablet memakai userspace aarch64 murni.
 *   - header uapi dibawa lokal (sprd_map.h) — ABI ioctl dipertahankan identik.
 */
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "sprd_map.h"

#define MAP_USER_MINOR	MISC_DYNAMIC_MINOR

struct reserved_mem_cfg {
	bool		no_reserved;
	unsigned long	reserved_mem_addr;
	size_t		reserved_mem_size;
};

static struct reserved_mem_cfg mem_cfg;

static int map_user_open(struct inode *inode, struct file *file)
{
	struct sprd_pmem_info *mem_info = kzalloc(sizeof(*mem_info), GFP_KERNEL);

	if (!mem_info)
		return -ENODEV;
	file->private_data = mem_info;

	return 0;
}

static int map_user_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	file->private_data = NULL;

	return 0;
}

static int map_user_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct sprd_pmem_info *mem_info = file->private_data;

	if (!mem_info)
		return -ENODEV;

	if (mem_info->phy_addr && mem_info->size) {
		vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
		pr_info("%s: phy_addr=0x%lx, size=0x%zx\n", __func__,
			mem_info->phy_addr, mem_info->size);
		return vm_iomap_memory(vma, mem_info->phy_addr, mem_info->size);
	}

	pr_err("%s: phy_addr=0x%lx, size=0x%zx err!\n", __func__,
	       mem_info->phy_addr, mem_info->size);

	return -EINVAL;
}

static long map_user_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	void __user *arg_user = (void __user *)arg;
	struct sprd_pmem_info *mem_info = file->private_data;

	if (!mem_info)
		return -ENODEV;

	if (mem_cfg.no_reserved)
		return -ENOMEM;

	switch (cmd) {
	case MAP_USER_VIR: {
		struct sprd_pmem_info data;

		if (copy_from_user(&data, arg_user, sizeof(data))) {
			pr_err("%s: copy_from_user error!\n", __func__);
			return -EFAULT;
		}

		if (data.phy_addr < mem_cfg.reserved_mem_addr ||
		    (data.phy_addr + data.size) >
		     (mem_cfg.reserved_mem_addr + mem_cfg.reserved_mem_size)) {
			pr_err("%s: user mem di luar reserved memory!\n",
			       __func__);
			return -EFAULT;
		}

		mem_info->phy_addr = data.phy_addr;
		mem_info->size = data.size;
		pr_debug("%s: phy_addr=0x%lx, size=0x%zx\n", __func__,
			 data.phy_addr, data.size);
		break;
	}
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations map_user_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= map_user_ioctl,
	.mmap		= map_user_mmap,
	.open		= map_user_open,
	.release	= map_user_release,
};

static struct miscdevice map_user_dev = {
	.minor	= MAP_USER_MINOR,
	.name	= "map_user",
	.fops	= &map_user_fops,
};

static int map_user_probe(struct platform_device *pdev)
{
	struct device_node *reserved_mem_node, *fd_reserved_node;
	struct resource r;
	int ret;

	reserved_mem_node = of_find_node_by_name(NULL, "reserved-memory");
	if (!reserved_mem_node) {
		mem_cfg.no_reserved = true;
		dev_err(&pdev->dev, "find reserved memory node failed\n");
		goto reg;
	}

	fd_reserved_node = of_get_child_by_name(reserved_mem_node, "faceid-mem");
	if (!fd_reserved_node) {
		mem_cfg.no_reserved = true;
		dev_err(&pdev->dev, "find faceid-mem node failed\n");
		goto reg;
	}

	if (of_address_to_resource(fd_reserved_node, 0, &r)) {
		mem_cfg.no_reserved = true;
		dev_err(&pdev->dev, "invalid fd reserved memory node!\n");
	} else {
		mem_cfg.reserved_mem_addr = r.start;
		mem_cfg.reserved_mem_size = resource_size(&r);
		mem_cfg.no_reserved = false;
		dev_info(&pdev->dev, "faceid-mem: 0x%lx +0x%zx\n",
			 mem_cfg.reserved_mem_addr, mem_cfg.reserved_mem_size);
	}

reg:
	ret = misc_register(&map_user_dev);
	if (ret)
		dev_err(&pdev->dev, "can't register miscdev (%d)\n", ret);

	return ret;
}

static void map_user_remove(struct platform_device *pdev)
{
	misc_deregister(&map_user_dev);
	pr_debug("%s Success!\n", __func__);
}

static const struct of_device_id of_match_table_map[] = {
	{ .compatible = "sprd,map-user", },
	{ }
};
MODULE_DEVICE_TABLE(of, of_match_table_map);

static struct platform_driver map_user_driver = {
	.probe	= map_user_probe,
	.remove	= map_user_remove,
	.driver	= {
		.name		= "map_user",
		.of_match_table	= of_match_table_map,
	}
};
module_platform_driver(map_user_driver);

MODULE_DESCRIPTION("Unisoc map_user driver (VXTux port 6.18)");
MODULE_LICENSE("GPL");
