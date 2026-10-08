// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_cache_efuse.c — Unisoc AP cache eFuse reader (read-only nvmem),
 * port 5.4 -> 6.18.
 *
 * GPL source: fork Samsung Tab A8 (T618/UMS512)
 *   drivers/nvmem/sprd_cache_efuse.c (C) 2018 Spreadtrum, Freeman Liu.
 *
 * 6.18 adaptation (verified):
 *   - vm_map_ram() lost prot parameter (vmalloc.h:143) ->
 *     vm_map_ram(pages, count, NUMA_NO_NODE); noncached prot dropped
 *     (efuse cache = static RAM snapshot, cached read safe).
 *   - of_device.h -> of.h; of_device_get_match_data now in of.h.
 *   - remove() not present since original (full devm).
 *
 * Stock live DT (reference): efuse@800 compatible "sprd,ums312-cache-efuse
 * sprd,ums512-cache-efuse", reg = <0x800 0x3ff>, cells uid-start@5c/uid-end@58.
 */
#include <linux/genalloc.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define SPRD_EFUSE_BLOCK_WIDTH	4
#define SPRD_EFUSE_BLOCK_SIZE	(SPRD_EFUSE_BLOCK_WIDTH * BITS_PER_BYTE)

/*
 * Since different sprd chip can have different block max,
 * we should save address in the device data structure.
 */
struct sprd_efuse_variant_data {
	u32 blk_start;
	u32 blk_max;
	u32 blk_num;
};

struct sprd_efuse {
	struct device *dev;
	struct mutex mutex;
	phys_addr_t base;
	u32 size;
	const struct sprd_efuse_variant_data *var_data;
};

static const struct sprd_efuse_variant_data sharkl5_data = {
	.blk_start = 0,
	.blk_max = 23,
	.blk_num = 24,
};

static void *efuse_mem_ram_vmap(struct sprd_efuse *efuse,
				phys_addr_t start, size_t size, u32 *count)
{
	struct page **pages;
	phys_addr_t page_start;
	phys_addr_t addr;
	void *vaddr;
	u32 i, page_count;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);
	*count = page_count;

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		dev_err(efuse->dev, "efuse malloc error\n");
		return NULL;
	}

	for (i = 0; i < page_count; i++) {
		addr = page_start + i * PAGE_SIZE;
		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}

	vaddr = vm_map_ram(pages, page_count, NUMA_NO_NODE);
	if (!vaddr)
		dev_err(efuse->dev, "efuse vmap error\n");
	else
		vaddr += offset_in_page(start);

	kfree(pages);
	return vaddr;
}

static void efuse_mem_ram_unmap(struct sprd_efuse *efuse,
				const void *mem, u32 count)
{
	vm_unmap_ram(mem - offset_in_page(mem), count);
}

static int sprd_efuse_read_from_phy_addr(struct sprd_efuse *efuse,
					 phys_addr_t phy_addr,
					 void *dest, u32 size)
{
	char *virt_addr, *dest_addr = (char *)dest;
	u32 cnt;

	virt_addr = efuse_mem_ram_vmap(efuse, phy_addr, size, &cnt);
	if (!virt_addr) {
		dev_err(efuse->dev, "efuse memory vmap failed\n");
		return -ENOMEM;
	}
	memcpy(dest_addr, virt_addr, size);
	efuse_mem_ram_unmap(efuse, virt_addr, cnt);

	return 0;
}

static int sprd_efuse_raw_read(struct sprd_efuse *efuse, int index, u32 *val)
{
	phys_addr_t phy_addr = efuse->base + 0x4 +
			       index * SPRD_EFUSE_BLOCK_WIDTH;

	return sprd_efuse_read_from_phy_addr(efuse, phy_addr, val,
					     sizeof(u32));
}

static int sprd_efuse_read(void *context, u32 offset, void *val, size_t bytes)
{
	struct sprd_efuse *efuse = context;
	u32 data, index = offset / SPRD_EFUSE_BLOCK_WIDTH;
	u32 blk_offset = (offset % SPRD_EFUSE_BLOCK_WIDTH) * BITS_PER_BYTE;
	int ret;

	if (index < efuse->var_data->blk_start ||
	    index > efuse->var_data->blk_max)
		return -EINVAL;

	mutex_lock(&efuse->mutex);
	ret = sprd_efuse_raw_read(efuse, index, &data);
	mutex_unlock(&efuse->mutex);

	if (!ret) {
		data >>= blk_offset;
		memcpy(val, &data, bytes);
	}

	return ret;
}

static int sprd_efuse_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct nvmem_device *nvmem;
	struct nvmem_config econfig = { };
	struct sprd_efuse *efuse;
	struct resource res;
	const struct sprd_efuse_variant_data *pdata;
	int ret;

	pdata = of_device_get_match_data(&pdev->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "No matching driver data found\n");
		return -EINVAL;
	}

	efuse = devm_kzalloc(&pdev->dev, sizeof(*efuse), GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "get resource failed\n");
		return ret;
	}
	efuse->base = res.start;
	efuse->size = resource_size(&res);

	mutex_init(&efuse->mutex);
	efuse->dev = &pdev->dev;
	efuse->var_data = pdata;

	econfig.stride = 1;
	econfig.word_size = 1;
	econfig.read_only = true;
	econfig.name = "sprd-cache-efuse";
	/* 6.18: child DT cells (uid-start@5c, uid-end@58) only parsed when this flag set */
	econfig.add_legacy_fixed_of_cells = true;
	econfig.size = efuse->var_data->blk_num * SPRD_EFUSE_BLOCK_WIDTH;
	econfig.reg_read = sprd_efuse_read;
	econfig.priv = efuse;
	econfig.dev = &pdev->dev;
	nvmem = devm_nvmem_register(&pdev->dev, &econfig);
	if (IS_ERR(nvmem)) {
		dev_err(&pdev->dev, "failed to register nvmem\n");
		return PTR_ERR(nvmem);
	}

	platform_set_drvdata(pdev, efuse);
	return 0;
}

static const struct of_device_id sprd_efuse_of_match[] = {
	{ .compatible = "sprd,ums512-cache-efuse", .data = &sharkl5_data },
	{ .compatible = "sprd,ums312-cache-efuse", .data = &sharkl5_data },
	{ .compatible = "sprd,sharkl5-cache-efuse", .data = &sharkl5_data },
	{ }
};
MODULE_DEVICE_TABLE(of, sprd_efuse_of_match);

static struct platform_driver sprd_efuse_driver = {
	.probe = sprd_efuse_probe,
	.driver = {
		.name = "sprd-cache-efuse",
		.of_match_table = sprd_efuse_of_match,
	},
};
module_platform_driver(sprd_efuse_driver);

MODULE_AUTHOR("Freeman Liu <freeman.liu@spreadtrum.com> (fork GPL)");
MODULE_AUTHOR("LinDroid/VXTux — port 6.18");
MODULE_DESCRIPTION("Spreadtrum AP cache efuse driver (VXTux port)");
MODULE_LICENSE("GPL v2");
