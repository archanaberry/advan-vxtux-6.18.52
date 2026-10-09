// SPDX-License-Identifier: GPL-2.0-only
/*
 * Unisoc IOMMU driver
 *
 * Copyright (C) 2020 Unisoc, Inc.
 * Author: Chunyan Zhang <chunyan.zhang@unisoc.com>
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/iommu.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/sprd_iommu.h>

#define SPRD_IOMMU_PAGE_SHIFT	12
#define SPRD_IOMMU_PAGE_SIZE	SZ_4K

#define SPRD_EX_CFG		0x0
#define SPRD_IOMMU_VAOR_BYPASS	BIT(4)
#define SPRD_IOMMU_GATE_EN	BIT(1)
#define SPRD_IOMMU_EN		BIT(0)
#define SPRD_EX_UPDATE		0x4
#define SPRD_EX_FIRST_VPN	0x8
#define SPRD_EX_VPN_RANGE	0xc
#define SPRD_EX_FIRST_PPN	0x10
#define SPRD_EX_DEFAULT_PPN	0x14

#define SPRD_IOMMU_VERSION	0x0
#define SPRD_VERSION_MASK	GENMASK(15, 8)
#define SPRD_VERSION_SHIFT	0x8
#define SPRD_VAU_CFG		0x4
#define SPRD_VAU_UPDATE		0x8
#define SPRD_VAU_AUTH_CFG	0xc
#define SPRD_VAU_FIRST_PPN	0x10
#define SPRD_VAU_DEFAULT_PPN_RD	0x14
#define SPRD_VAU_DEFAULT_PPN_WR	0x18
#define SPRD_VAU_FIRST_VPN	0x1c
#define SPRD_VAU_VPN_RANGE	0x20

enum sprd_iommu_version {
	SPRD_IOMMU_EX,
	SPRD_IOMMU_VAU,
};

/*
 * struct sprd_iommu_device - high-level sprd IOMMU device representation,
 * including hardware information and configuration, also driver data, etc
 *
 * @ver: sprd IOMMU IP version
 * @prot_page_va: protect page base virtual address
 * @prot_page_pa: protect page base physical address, data would be
 *		  written to here while translation fault
 * @base: mapped base address for accessing registers
 * @dev: pointer to basic device structure
 * @iommu: IOMMU core representation
 * @group: IOMMU group
 * @eb: gate clock which controls IOMMU access
 */
struct sprd_iommu_device {
	struct sprd_iommu_domain	*dom;
	enum sprd_iommu_version	ver;
	u32			*prot_page_va;
	dma_addr_t		prot_page_pa;
	void __iomem		*base;
	struct device		*dev;
	struct iommu_device	iommu;
	struct clk		*eb;
};

struct sprd_iommu_domain {
	spinlock_t		pgtlock; /* lock for page table */
	struct iommu_domain	domain;
	u32			*pgt_va; /* page table virtual address base */
	dma_addr_t		pgt_pa; /* page table physical address base */
	struct sprd_iommu_device	*sdev;
};

static const struct iommu_ops sprd_iommu_ops;

static struct sprd_iommu_domain *to_sprd_domain(struct iommu_domain *dom)
{
	return container_of(dom, struct sprd_iommu_domain, domain);
}

static inline void
sprd_iommu_write(struct sprd_iommu_device *sdev, unsigned int reg, u32 val)
{
	writel_relaxed(val, sdev->base + reg);
}

static inline u32
sprd_iommu_read(struct sprd_iommu_device *sdev, unsigned int reg)
{
	return readl_relaxed(sdev->base + reg);
}

static inline void
sprd_iommu_update_bits(struct sprd_iommu_device *sdev, unsigned int reg,
		  u32 mask, u32 shift, u32 val)
{
	u32 t = sprd_iommu_read(sdev, reg);

	t = (t & (~(mask << shift))) | ((val & mask) << shift);
	sprd_iommu_write(sdev, reg, t);
}

static inline int
sprd_iommu_get_version(struct sprd_iommu_device *sdev)
{
	int ver = (sprd_iommu_read(sdev, SPRD_IOMMU_VERSION) &
		   SPRD_VERSION_MASK) >> SPRD_VERSION_SHIFT;

	switch (ver) {
	case SPRD_IOMMU_EX:
	case SPRD_IOMMU_VAU:
		return ver;
	default:
		return -EINVAL;
	}
}

static size_t
sprd_iommu_pgt_size(struct iommu_domain *domain)
{
	return ((domain->geometry.aperture_end -
		 domain->geometry.aperture_start + 1) >>
		SPRD_IOMMU_PAGE_SHIFT) * sizeof(u32);
}

static struct iommu_domain *sprd_iommu_domain_alloc_paging(struct device *dev)
{
	struct sprd_iommu_domain *dom;

	dom = kzalloc(sizeof(*dom), GFP_KERNEL);
	if (!dom)
		return NULL;

	spin_lock_init(&dom->pgtlock);

	dom->domain.pgsize_bitmap = SPRD_IOMMU_PAGE_SIZE;

	dom->domain.geometry.aperture_start = 0;
	dom->domain.geometry.aperture_end = SZ_256M - 1;
	dom->domain.geometry.force_aperture = true;

	return &dom->domain;
}

static void sprd_iommu_first_vpn(struct sprd_iommu_domain *dom)
{
	struct sprd_iommu_device *sdev = dom->sdev;
	u32 val;
	unsigned int reg;

	if (sdev->ver == SPRD_IOMMU_EX)
		reg = SPRD_EX_FIRST_VPN;
	else
		reg = SPRD_VAU_FIRST_VPN;

	val = dom->domain.geometry.aperture_start >> SPRD_IOMMU_PAGE_SHIFT;
	sprd_iommu_write(sdev, reg, val);
}

static void sprd_iommu_vpn_range(struct sprd_iommu_domain *dom)
{
	struct sprd_iommu_device *sdev = dom->sdev;
	u32 val;
	unsigned int reg;

	if (sdev->ver == SPRD_IOMMU_EX)
		reg = SPRD_EX_VPN_RANGE;
	else
		reg = SPRD_VAU_VPN_RANGE;

	val = (dom->domain.geometry.aperture_end -
	       dom->domain.geometry.aperture_start) >> SPRD_IOMMU_PAGE_SHIFT;
	sprd_iommu_write(sdev, reg, val);
}

static void sprd_iommu_first_ppn(struct sprd_iommu_domain *dom)
{
	u32 val = dom->pgt_pa >> SPRD_IOMMU_PAGE_SHIFT;
	struct sprd_iommu_device *sdev = dom->sdev;
	unsigned int reg;

	if (sdev->ver == SPRD_IOMMU_EX)
		reg = SPRD_EX_FIRST_PPN;
	else
		reg = SPRD_VAU_FIRST_PPN;

	sprd_iommu_write(sdev, reg, val);
}

static void sprd_iommu_default_ppn(struct sprd_iommu_device *sdev)
{
	u32 val = sdev->prot_page_pa >> SPRD_IOMMU_PAGE_SHIFT;

	if (sdev->ver == SPRD_IOMMU_EX) {
		sprd_iommu_write(sdev, SPRD_EX_DEFAULT_PPN, val);
	} else if (sdev->ver == SPRD_IOMMU_VAU) {
		sprd_iommu_write(sdev, SPRD_VAU_DEFAULT_PPN_RD, val);
		sprd_iommu_write(sdev, SPRD_VAU_DEFAULT_PPN_WR, val);
	}
}

static void sprd_iommu_hw_en(struct sprd_iommu_device *sdev, bool en)
{
	unsigned int reg_cfg;
	u32 mask, val;

	if (sdev->ver == SPRD_IOMMU_EX)
		reg_cfg = SPRD_EX_CFG;
	else
		reg_cfg = SPRD_VAU_CFG;

	mask = SPRD_IOMMU_EN | SPRD_IOMMU_GATE_EN;
	val = en ? mask : 0;
	sprd_iommu_update_bits(sdev, reg_cfg, mask, 0, val);
}

static void sprd_iommu_cleanup(struct sprd_iommu_domain *dom)
{
	size_t pgt_size;

	/* Nothing need to do if the domain hasn't been attached */
	if (!dom->sdev)
		return;

	pgt_size = sprd_iommu_pgt_size(&dom->domain);
	dma_free_coherent(dom->sdev->dev, pgt_size, dom->pgt_va, dom->pgt_pa);
	sprd_iommu_hw_en(dom->sdev, false);
	dom->sdev = NULL;
}

static void sprd_iommu_domain_free(struct iommu_domain *domain)
{
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);

	sprd_iommu_cleanup(dom);
	kfree(dom);
}

static int sprd_iommu_ops_attach_device(struct iommu_domain *domain,
					struct device *dev)
{
	struct sprd_iommu_device *sdev = dev_iommu_priv_get(dev);
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);
	size_t pgt_size = sprd_iommu_pgt_size(domain);

	/* The device is attached to this domain */
	if (sdev->dom == dom)
		return 0;

	/* The first time that domain is attaching to a device */
	if (!dom->pgt_va) {
		dom->pgt_va = dma_alloc_coherent(sdev->dev, pgt_size, &dom->pgt_pa, GFP_KERNEL);
		if (!dom->pgt_va)
			return -ENOMEM;

		dom->sdev = sdev;
	}

	sdev->dom = dom;

	/*
	 * One sprd IOMMU serves one client device only, disabled it before
	 * configure mapping table to avoid access conflict in case other
	 * mapping table is stored in.
	 */
	sprd_iommu_hw_en(sdev, false);
	sprd_iommu_first_ppn(dom);
	sprd_iommu_first_vpn(dom);
	sprd_iommu_vpn_range(dom);
	sprd_iommu_default_ppn(sdev);
	sprd_iommu_hw_en(sdev, true);

	return 0;
}

static int sprd_iommu_ops_map(struct iommu_domain *domain, unsigned long iova,
			      phys_addr_t paddr, size_t pgsize, size_t pgcount,
			      int prot, gfp_t gfp, size_t *mapped)
{
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);
	size_t size = pgcount * SPRD_IOMMU_PAGE_SIZE;
	unsigned long flags;
	unsigned int i;
	u32 *pgt_base_iova;
	u32 pabase = (u32)paddr;
	unsigned long start = domain->geometry.aperture_start;
	unsigned long end = domain->geometry.aperture_end;

	if (!dom->sdev) {
		pr_err("No sprd_iommu_device attached to the domain\n");
		return -EINVAL;
	}

	if (iova < start || (iova + size) > (end + 1)) {
		dev_err(dom->sdev->dev, "(iova(0x%lx) + size(%zx)) are not in the range!\n",
			iova, size);
		return -EINVAL;
	}

	pgt_base_iova = dom->pgt_va + ((iova - start) >> SPRD_IOMMU_PAGE_SHIFT);

	spin_lock_irqsave(&dom->pgtlock, flags);
	for (i = 0; i < pgcount; i++) {
		pgt_base_iova[i] = pabase >> SPRD_IOMMU_PAGE_SHIFT;
		pabase += SPRD_IOMMU_PAGE_SIZE;
	}
	spin_unlock_irqrestore(&dom->pgtlock, flags);

	*mapped = size;
	return 0;
}

static size_t sprd_iommu_ops_unmap(struct iommu_domain *domain,
				   unsigned long iova,
				   size_t pgsize, size_t pgcount,
				   struct iommu_iotlb_gather *iotlb_gather)
{
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);
	unsigned long flags;
	u32 *pgt_base_iova;
	size_t size = pgcount * SPRD_IOMMU_PAGE_SIZE;
	unsigned long start = domain->geometry.aperture_start;
	unsigned long end = domain->geometry.aperture_end;

	if (iova < start || (iova + size) > (end + 1))
		return 0;

	pgt_base_iova = dom->pgt_va + ((iova - start) >> SPRD_IOMMU_PAGE_SHIFT);

	spin_lock_irqsave(&dom->pgtlock, flags);
	memset(pgt_base_iova, 0, pgcount * sizeof(u32));
	spin_unlock_irqrestore(&dom->pgtlock, flags);

	return size;
}

static int sprd_iommu_sync_map(struct iommu_domain *domain,
			       unsigned long iova, size_t size)
{
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);
	unsigned int reg;

	if (dom->sdev->ver == SPRD_IOMMU_EX)
		reg = SPRD_EX_UPDATE;
	else
		reg = SPRD_VAU_UPDATE;

	/* clear IOMMU TLB buffer after page table updated */
	sprd_iommu_write(dom->sdev, reg, 0xffffffff);
	return 0;
}

/*
 * Public (non-ops) API used by the GSP driver, declared in
 * <linux/sprd_iommu.h>.
 *
 * The vendor driver had these as the primary entry points and exposed only
 * the ops callbacks to the IOMMU core. In this tree the file is the upstream
 * iommu_ops rewrite, which kept only the static callbacks -- so the two
 * public symbols had no definition at all and any consumer failed to link.
 * There is no donor carrying the vendor bodies (checked the UMS9230 donor
 * and the T618 fork: both already use the same ops-only shape), so they are
 * reimplemented here on top of the generic API rather than copied.
 *
 * Semantics taken from the callers (gsp_layer.c), which are the contract:
 *   - map fills data->iova_addr with the IOVA it chose and maps the whole
 *     scatterlist at data->iova_size; the caller checks iova_addr != 0 to
 *     decide success.
 *   - unmapping is by (iova_addr, iova_size) only.
 *   - the caller passes data->buf as the dmabuf carrier, not a table, so the
 *     scatterlist is taken from data->table when present.
 *
 * Known limitation, stated rather than hidden: gsp_layer_iommu_map() in this
 * tree fills only buf/iova_size/ch_type and leaves data->table NULL, because
 * 6.18 has no way to reach the sg_table out of a dma_buf. So the GSP caller
 * hits the -EINVAL below and its layer mapping fails. Wiring that up needs a
 * dma_buf_attachment on the GSP side, which is a separate change -- this does
 * not fake a mapping just to make the call return success.
 *
 * iova == 0 means "pick one for me". It is taken from the base of the domain
 * aperture rather than from a gen_pool: this driver has no allocator, and the
 * aperture is exactly what the ops map validates against, so the range is
 * guaranteed legal.
 */
static int sprd_iommu_prot(enum sprd_iommu_chtype ch_type)
{
	switch (ch_type) {
	case SPRD_IOMMU_PF_CH_READ:
	case SPRD_IOMMU_EX_CH_READ:
		return IOMMU_READ;
	case SPRD_IOMMU_PF_CH_WRITE:
	case SPRD_IOMMU_EX_CH_WRITE:
		return IOMMU_WRITE;
	case SPRD_IOMMU_FM_CH_RW:
	default:
		return IOMMU_READ | IOMMU_WRITE;
	}
}

static unsigned long sprd_iommu_find_iova(struct iommu_domain *domain,
					 size_t size)
{
	unsigned long start = domain->geometry.aperture_start;
	unsigned long end = domain->geometry.aperture_end;
	unsigned long iova;

	/*
	 * The page table is a flat array indexed by (iova - start) >> shift, so
	 * any page-aligned IOVA inside the aperture is mappable. This driver
	 * owns no allocator, so there is nothing to track handed-out ranges
	 * against; the vendor driver left the same job to the client's pool.
	 * Callers that map more than one buffer must pass an explicit iova in
	 * data->iova_addr rather than relying on this.
	 */
	iova = round_up(start, SPRD_IOMMU_PAGE_SIZE);

	if (iova + size > end + 1)
		return 0;

	return iova;
}

int sprd_iommu_map(struct device *dev, struct sprd_iommu_map_data *data)
{
	struct iommu_domain *domain;
	unsigned long iova;
	ssize_t mapped;

	if (!dev || !data || !data->table)
		return -EINVAL;

	domain = iommu_get_domain_for_dev(dev);
	if (!domain)
		return -ENODEV;

	iova = data->iova_addr;
	if (!iova) {
		iova = sprd_iommu_find_iova(domain, data->iova_size);
		if (!iova)
			return -ENOMEM;
	}

	mapped = iommu_map_sg(domain, iova, data->table->sgl,
			      data->table->orig_nents,
			      sprd_iommu_prot(data->ch_type), GFP_KERNEL);
	if (mapped < 0)
		return (int)mapped;

	data->iova_addr = iova;
	return 0;
}

int sprd_iommu_unmap(struct device *dev, struct sprd_iommu_unmap_data *data)
{
	struct iommu_domain *domain;

	if (!dev || !data)
		return -EINVAL;

	domain = iommu_get_domain_for_dev(dev);
	if (!domain)
		return -ENODEV;

	iommu_unmap(domain, data->iova_addr, data->iova_size);
	return 0;
}

EXPORT_SYMBOL(sprd_iommu_map);
EXPORT_SYMBOL(sprd_iommu_unmap);

/*
 * sprd_iommu_attach_device() -- declared in include/linux/sprd_iommu.h and
 * called by clients that need to know whether they can use the IOMMU path or
 * must fall back to a physical address (sprd_jpg_get_iova(), camera, ISP).
 *
 * In 5.4 this function performed the vendor's own attach. In 6.18 the attach
 * is done by the generic iommu core from the DT `iommus` property, so there is
 * nothing left to do here: the question a client is really asking is "does
 * this device have an iommu domain?", and that is exactly
 * iommu_get_domain_for_dev(). Reporting it truthfully keeps the clients'
 * fallback path reachable instead of silently handing out unmapped IOVAs.
 */
int sprd_iommu_attach_device(struct device *dev)
{
	if (!dev || !iommu_get_domain_for_dev(dev))
		return -ENODEV;

	return 0;
}
EXPORT_SYMBOL(sprd_iommu_attach_device);

/*
 * sprd_iommu_restore() -- declared in include/linux/sprd_iommu.h and called
 * by clients (vsp_get_iova/vsp_free_iova, gsp) on the enable path -- the
 * vendor 5.4 counterpart of sprd_iommu_suspend(), which this tree does not
 * have. In 5.4 it re-enabled the vendor iommu hardware after a suspend; in
 * 6.18 the domain survives across the client's clk toggling (the iommu core
 * owns device power state), so -- same philosophy as attach_device() above --
 * the honest answer is "does this device still have a domain?". Build device
 * 2026-10-09: without this definition modpost reported
 * "sprd_iommu_restore [sprd_vsp.ko] undefined".
 */
int sprd_iommu_restore(struct device *dev)
{
	if (!dev || !iommu_get_domain_for_dev(dev))
		return -ENODEV;

	return 0;
}
EXPORT_SYMBOL(sprd_iommu_restore);


static void sprd_iommu_sync(struct iommu_domain *domain,
			    struct iommu_iotlb_gather *iotlb_gather)
{
	sprd_iommu_sync_map(domain, 0, 0);
}

static phys_addr_t sprd_iommu_iova_to_phys(struct iommu_domain *domain,
					   dma_addr_t iova)
{
	struct sprd_iommu_domain *dom = to_sprd_domain(domain);
	unsigned long flags;
	phys_addr_t pa;
	unsigned long start = domain->geometry.aperture_start;
	unsigned long end = domain->geometry.aperture_end;

	if (WARN_ON(iova < start || iova > end))
		return 0;

	spin_lock_irqsave(&dom->pgtlock, flags);
	pa = *(dom->pgt_va + ((iova - start) >> SPRD_IOMMU_PAGE_SHIFT));
	pa = (pa << SPRD_IOMMU_PAGE_SHIFT) + ((iova - start) & (SPRD_IOMMU_PAGE_SIZE - 1));
	spin_unlock_irqrestore(&dom->pgtlock, flags);

	return pa;
}

static struct iommu_device *sprd_iommu_probe_device(struct device *dev)
{
	struct sprd_iommu_device *sdev = dev_iommu_priv_get(dev);

	return &sdev->iommu;
}

static int sprd_iommu_of_xlate(struct device *dev,
			       const struct of_phandle_args *args)
{
	struct platform_device *pdev;

	if (!dev_iommu_priv_get(dev)) {
		pdev = of_find_device_by_node(args->np);
		dev_iommu_priv_set(dev, platform_get_drvdata(pdev));
		platform_device_put(pdev);
	}

	return 0;
}


static const struct iommu_ops sprd_iommu_ops = {
	.domain_alloc_paging = sprd_iommu_domain_alloc_paging,
	.probe_device	= sprd_iommu_probe_device,
	.device_group	= generic_single_device_group,
	.of_xlate	= sprd_iommu_of_xlate,
	.owner		= THIS_MODULE,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev	= sprd_iommu_ops_attach_device,
		.map_pages	= sprd_iommu_ops_map,
		.unmap_pages	= sprd_iommu_ops_unmap,
		.iotlb_sync_map	= sprd_iommu_sync_map,
		.iotlb_sync	= sprd_iommu_sync,
		.iova_to_phys	= sprd_iommu_iova_to_phys,
		.free		= sprd_iommu_domain_free,
	}
};

static const struct of_device_id sprd_iommu_of_match[] = {
	{ .compatible = "sprd,iommu-v1" },
	{ },
};
MODULE_DEVICE_TABLE(of, sprd_iommu_of_match);

/*
 * Clock is not required, access to some of IOMMUs is controlled by gate
 * clk, enabled clocks for that kind of IOMMUs before accessing.
 * Return 0 for success or no clocks found.
 */
static int sprd_iommu_clk_enable(struct sprd_iommu_device *sdev)
{
	struct clk *eb;

	eb = devm_clk_get_optional(sdev->dev, NULL);
	if (!eb)
		return 0;

	if (IS_ERR(eb))
		return PTR_ERR(eb);

	sdev->eb = eb;
	return clk_prepare_enable(eb);
}

static void sprd_iommu_clk_disable(struct sprd_iommu_device *sdev)
{
	if (sdev->eb)
		clk_disable_unprepare(sdev->eb);
}

static int sprd_iommu_probe(struct platform_device *pdev)
{
	struct sprd_iommu_device *sdev;
	struct device *dev = &pdev->dev;
	void __iomem *base;
	int ret;

	sdev = devm_kzalloc(dev, sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base)) {
		dev_err(dev, "Failed to get ioremap resource.\n");
		return PTR_ERR(base);
	}
	sdev->base = base;

	sdev->prot_page_va = dma_alloc_coherent(dev, SPRD_IOMMU_PAGE_SIZE,
						&sdev->prot_page_pa, GFP_KERNEL);
	if (!sdev->prot_page_va)
		return -ENOMEM;

	platform_set_drvdata(pdev, sdev);
	sdev->dev = dev;

	ret = iommu_device_sysfs_add(&sdev->iommu, dev, NULL, dev_name(dev));
	if (ret)
		goto free_page;

	ret = iommu_device_register(&sdev->iommu, &sprd_iommu_ops, dev);
	if (ret)
		goto remove_sysfs;

	ret = sprd_iommu_clk_enable(sdev);
	if (ret)
		goto unregister_iommu;

	ret = sprd_iommu_get_version(sdev);
	if (ret < 0) {
		dev_err(dev, "IOMMU version(%d) is invalid.\n", ret);
		goto disable_clk;
	}
	sdev->ver = ret;

	return 0;

disable_clk:
	sprd_iommu_clk_disable(sdev);
unregister_iommu:
	iommu_device_unregister(&sdev->iommu);
remove_sysfs:
	iommu_device_sysfs_remove(&sdev->iommu);
free_page:
	dma_free_coherent(sdev->dev, SPRD_IOMMU_PAGE_SIZE, sdev->prot_page_va, sdev->prot_page_pa);
	return ret;
}

static void sprd_iommu_remove(struct platform_device *pdev)
{
	struct sprd_iommu_device *sdev = platform_get_drvdata(pdev);

	dma_free_coherent(sdev->dev, SPRD_IOMMU_PAGE_SIZE, sdev->prot_page_va, sdev->prot_page_pa);

	platform_set_drvdata(pdev, NULL);
	iommu_device_sysfs_remove(&sdev->iommu);
	iommu_device_unregister(&sdev->iommu);
}

static struct platform_driver sprd_iommu_driver = {
	.driver	= {
		.name		= "sprd-iommu",
		.of_match_table	= sprd_iommu_of_match,
		.suppress_bind_attrs = true,
	},
	.probe	= sprd_iommu_probe,
	.remove = sprd_iommu_remove,
};
module_platform_driver(sprd_iommu_driver);

MODULE_DESCRIPTION("IOMMU driver for Unisoc SoCs");
MODULE_ALIAS("platform:sprd-iommu");
MODULE_LICENSE("GPL");
