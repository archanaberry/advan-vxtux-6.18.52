// SPDX-License-Identifier: GPL-2.0-only
/*
 * Unisoc SPRD ION glue + carveout heap, reconstructed from the vendor
 * sprd-ion.ko (5.4.210, aarch64).
 *
 * EVIDENCE. Every claim below is backed either by the module's
 * R_AARCH64_CALL26 relocations (which name each kernel API call exactly) or
 * by raw disassembly offsets quoted as 0x08001xxx, which are the virtual
 * addresses radare2 reports for this blob. Constants that could not be
 * decoded with confidence are called out as such instead of being guessed.
 *
 * EXPORTED API (from __ksymtab/__kstrtab in the blob):
 *   sprd_ion_get_buffer, sprd_ion_get_phys_addr,
 *   sprd_ion_map_kernel, sprd_ion_unmap_kernel
 * These are what the sprdwcn GSP driver imports, so they are the contract
 * this file must honour. Signatures come from include/linux/sprd_ion.h.
 *
 * TWO BLOCKERS, neither worked around.
 *
 * 1. There is no ION core on 6.18. include/linux/ion.h (donated from the
 *    marohinmark UMS512 donor) provides __ion_device_add_heap() as
 *    `static inline { return -ENODEV; }`. The blob's sprd_ion_probe()
 *    registers its heap through that call, so the carveout heap below can
 *    never bind on this kernel. It is compiled and its offsets are
 *    documented because it is the real allocation logic, but claiming it
 *    works at runtime here would be false.
 *
 * 2. sprd_ion_get_phys_addr()/get_buffer() read the scatterlist straight
 *    out of struct dma_buf at dmabuf+0x88. That member does not exist in
 *    6.18 -- struct dma_buf has no ->sg_table, and upstream deliberately
 *    removed the exporter-side dma_buf_map()/dma_buf_unmap() accessors
 *    that made it possible (this tree has neither symbol). Getting an
 *    sg_table from a bare dma_buf in 6.18 requires a dma_buf_attachment,
 *    i.e. an importer struct device, which these two entry points do not
 *    receive. So both return -EOPNOTSUPP here, which is the same contract
 *    the tree already documents in drivers/gpu/drm/sprd/gsp/gsp_ion_stub.c
 *    and in that driver's Kconfig.
 *
 *    The blob's final address transform was deliberately NOT reproduced.
 *    At 0x0800120c-0x08001234 it computes, from sgl->dma_address and
 *    memstart_addr:
 *        t = -(memstart_addr >> 12)
 *        v = (dma_address & 0x3fffffffffffffc) - (t << 6)
 *        *phys = v + 0x40_0000_0000
 *    The two `lsl 6` operands do not correspond to any address-arithmetic
 *    idiom I can verify (a bare __pa() is a single 64-bit subtract, and
 *    0x40_0000_0000 is a vendor window base, not PAGE_OFFSET). Shipping
 *    an unverified constant on a DMA path is exactly how you get silent
 *    memory corruption, so it is left out and documented instead.
 *
 *    If a real implementation is ever wanted here, the correct shape on
 *    6.18 is dma_buf_attach() + dma_buf_map_attachment() + read the sgt +
 *    dma_buf_unmap_attachment() + dma_buf_detach(), and the signature must
 *    grow a struct device *.
 *
 * Copyright (C) 2020 Unisoc Inc.
 */

#include <linux/dma-buf.h>
#include <linux/err.h>
/*
 * 6.18 moved the generic allocator out of mm/gen_pool.c into lib/genalloc.c
 * and folded the old include/linux/gen_pool.h into include/linux/genalloc.h,
 * so upstream consumers (ghes.c, sram.c, kernel/dma/pool.c) all include
 * <linux/genalloc.h> now.
 */
#include <linux/genalloc.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/sprd_ion.h>
#include <linux/vmalloc.h>

/*
 * Vendor heap table entry, 0x30 bytes. Stride proven by the parse loop in
 * sprd_ion_probe, which advances its cursor by 0x30 and indexes
 * base + i*0x30 with an smaddl of the same stride.
 *
 *   [0]  u32 id            read as a 1-element u32 array property, stored at
 *                          +0, and +0 is then printed as "heap id: %u"
 *   [4]  u32 order         the second property read, stored at +4, and +4 is
 *                          printed as "size: %lu"; reused as the gen_pool
 *                          order via 1 << order
 *   [8]  char *type         string property, stored at +8
 *   [16] u64 base           from a 4-element "reg" u32 array, packed into two
 *   [24] u64 size           u64s with two bfi #32 -- the standard DT 64-bit
 *                          reg split, written as one stp of two u64s
 *   [40] struct device *dev &pdev->dev, stored only when the parent has no
 *                          of_node (guarded by a cbnz on the parent node)
 */
struct sprd_ion_heap {
	u32 id;
	u32 order;
	char *type;
	u64 base;
	u64 size;
	struct device *dev;
};

/*
 * Argument struct handed to the heap-create callback. The blob loads two
 * u64s at +16 and +24 of it: +16 becomes the gen_pool extent start and +24
 * supplies the page count.
 */
struct sprd_ion_carveout_info {
	u32 heap_id;
	u32 order;
	u32 flags;
	u32 reserved;
	phys_addr_t base;
	phys_addr_t size;
};

/*
 * Carveout heap. The blob allocates this with a 424-byte kmem_cache_alloc
 * and writes type=2 at +40, the ops table at +48, flags=1 at +208, order at
 * +216, base at +224, the gen_pool at +408 and pool_start at +416.
 *
 * Those offsets describe the blob's own struct ion_heap, whose prefix is a
 * different size from the one in this tree's include/linux/ion.h. So the
 * struct below embeds THIS tree's struct ion_heap and accesses every field
 * by name -- no hardcoded displacement survives from the blob into the
 * compiled code.
 */
struct sprd_ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	u64 pool_start;
};

static struct sprd_ion_heap *sprd_ion_heaps;
static int sprd_ion_num_heaps;
static DEFINE_MUTEX(sprd_ion_lock);

/*
 * ion_heap_sglist_zero (blob 0x08001dc8)
 *
 * Batches of 0x20 == 32 pages, gathered into a 32-entry stack array and
 * mapped with vmap(); a failed vmap returns -ENOMEM. The argument is already
 * a struct page **, so it is walked directly rather than through the 5.4
 * sg_page_iter API -- sg_page_iter_init()/sg_page_iter_next() no longer exist
 * in 6.18 (only the __sg_page_iter_* internals do), and the plain index loop
 * is the same walk.
 *
 * __maybe_unused because its only caller in the blob is the ION core's alloc
 * path, which does not exist on this tree (see the file header). It is kept
 * because it is real recovered logic and will be needed the moment an ION
 * core lands; deleting it would discard evidence for no benefit.
 */
static int __maybe_unused sprd_ion_sglist_zero(struct page **pages,
					       unsigned long nr_pages)
{
	struct page *batch[32];
	unsigned long i, len;
	void *addr;

	for (i = 0; i < nr_pages; ) {
		len = min_t(unsigned long, nr_pages - i, ARRAY_SIZE(batch));
		memcpy(batch, &pages[i], len * sizeof(*batch));

		addr = vmap(batch, len, VM_WRITE, PAGE_KERNEL);
		if (!addr)
			return -ENOMEM;
		memset(addr, 0, len << PAGE_SHIFT);
		vunmap(addr);

		i += len;
	}
	return 0;
}

/*
 * ion_carveout_heap_allocate (blob 0x08001f8c)
 *
 *  - sg_alloc_table(1, GFP_KERNEL); in 6.18 that takes the table by
 *    pointer and returns int, so the table is allocated separately.
 *  - gen_pool_alloc_algo_owner(pool, len, pool->algo, pool->data, NULL);
 *    the 5.4 third argument (a page out-pointer) is gone in 6.18, so this
 *    is gen_pool_alloc(pool, len).
 *  - the error test is an IS_ERR() pattern on the pool result.
 *  - success builds one scatterlist: dma_address = the raw pool address,
 *    offset 0, length = len, and stores the table into buffer->sg_table.
 */
static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long len, unsigned long flags)
{
	struct sprd_ion_carveout_heap *carveout =
		(struct sprd_ion_carveout_heap *)heap;
	struct sg_table *sgt;
	struct scatterlist *sgl;
	unsigned long addr;
	int ret;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return -ENOMEM;

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (ret < 0) {
		kfree(sgt);
		return ret;
	}

	addr = gen_pool_alloc(carveout->pool, len);
	if (!addr) {
		sg_free_table(sgt);
		kfree(sgt);
		return -ENOMEM;
	}

	sgl = sgt->sgl;
	sgl->dma_address = addr;
	sgl->offset = 0;
	sgl->length = len;
	buffer->sg_table = sgt;

	return 0;
}

/*
 * ion_carveout_heap_free (blob 0x080020c0)
 *
 *  - buffer->sg_table is released, and gen_pool_free_owner() is called with
 *    the physical address. 5.4's two-argument gen_pool_free() grew a size
 *    argument in 6.18, so the length is recovered from the scatterlist --
 *    which is where it came from in the first place.
 */
static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct sprd_ion_carveout_heap *carveout =
		(struct sprd_ion_carveout_heap *)buffer->heap;
	struct sg_table *sgt = buffer->sg_table;
	struct scatterlist *sgl;

	if (!sgt)
		return;

	sgl = sgt->sgl;
	gen_pool_free(carveout->pool, (unsigned long)sgl->dma_address,
		      sgl->length);

	sg_free_table(sgt);
	kfree(sgt);
	buffer->sg_table = NULL;
}

static struct ion_heap_ops ion_carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free	= ion_carveout_heap_free,
};

/*
 * ion_carveout_heap_create (blob 0x08001898)
 *
 *  - a 424-byte kmem_cache_alloc; kmem_cache_alloc_trace() was removed in
 *    6.9, so kmalloc() of the same size is used.
 *  - gen_pool_create(12, -1): 12 is a literal, -1 the min_id.
 *  - gen_pool_add_owner() with the info base and ~0UL for the size.
 *  - the type/ops/flags stores documented above.
 */
static struct ion_heap *ion_carveout_heap_create(struct sprd_ion_carveout_info *info)
{
	struct sprd_ion_carveout_heap *carveout;

	carveout = kmalloc(424, GFP_KERNEL);
	if (!carveout)
		return ERR_PTR(-ENOMEM);

	carveout->heap.type = ION_HEAP_TYPE_DMA;
	carveout->heap.ops = &ion_carveout_heap_ops;
	carveout->heap.flags = 1;	/* ION_HEAP_FLAG_DEFER_FREE */
	carveout->heap.id = info ? info->heap_id : 0;

	carveout->pool = gen_pool_create(12, -1);
	if (!carveout->pool) {
		kfree(carveout);
		return ERR_PTR(-ENOMEM);
	}

	carveout->pool_start = info ? info->base : 0;
	gen_pool_add_owner(carveout->pool, carveout->pool_start, ~0UL, -1,
			   -1, NULL);

	return &carveout->heap;
}

/*
 * sprd_ion_ioctl (blob 0x08001a38)
 *
 *  - _IOC_SIZE is extracted with an ubfx of 14 bits at bit 16 and required
 *    to be <= 0x18, else -EINVAL.
 *  - copy_from_user into that 0x18-byte stack struct, -EFAULT on failure.
 *  - the command is compared against the literal 0xc018490b, which is
 *    _IOWR(ION_IOC_MAGIC, 11, struct ion_phy_data) for a 24-byte struct --
 *    i.e. ION_IOC_PHY from include/uapi/linux/sprd_ion.h. Anything else is
 *    -ENOTTY.
 *  - the call passes the struct's first u32 (the fd) in w0, NULL in x1,
 *    and &struct+0x10 then &struct+8 -- so struct ion_phy_data is
 *    { u32 fd; u32 pad; u64 size; u64 phys; } with phys at +16 and size at
 *    +8, which is exactly the layout added to include/uapi/linux/ion.h.
 *  - the result is copied back, and -EFAULT wins over the driver return.
 */
static long sprd_ion_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	struct ion_phy_data data;
	unsigned long size = _IOC_SIZE(cmd);
	void __user *user = (void __user *)arg;
	int ret;

	if (size > sizeof(data))
		return -EINVAL;

	if (copy_from_user(&data, user, size))
		return -EFAULT;

	if (cmd != ION_IOC_PHY)
		return -ENOTTY;

	memset(&data, 0, sizeof(data));
	ret = sprd_ion_get_phys_addr(data.fd, NULL,
				     (unsigned long *)&data.phys,
				     (size_t *)&data.size);

	if (copy_to_user(user, &data, size))
		return -EFAULT;

	return ret;
}

static const struct file_operations sprd_ion_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= sprd_ion_ioctl,
	.compat_ioctl	= compat_ptr_ioctl,
};

/*
 * sprd_ion_map_kernel (blob 0x080010cc)
 *
 * A NULL dmabuf returns ERR_PTR(-EINVAL). Then an indirect call through
 * dmabuf->ops+0x30 with (dmabuf, NULL), followed by one through ops+0x58
 * with (dmabuf, <second arg>). The `cmp` against __cfi_check_fail is Clang's
 * CFI type check, not a NULL test, so there is no "skip if ops is NULL"
 * branch to reproduce.
 *
 * 6.18 DELTA: struct dma_buf_ops has no ->map at all. @map was folded into
 * the attachment-based map_dma_buf, and @vmap is now
 * int (*vmap)(struct dma_buf *, struct iosys_map *). So the whole buffer is
 * mapped through dma_buf_vmap(), which is the current spelling of exactly
 * this operation, and the result is an int with the pointer in the map.
 */
void *sprd_ion_map_kernel(struct dma_buf *dmabuf, unsigned long offset)
{
	struct iosys_map map = { };
	int ret;

	if (IS_ERR_OR_NULL(dmabuf))
		return ERR_PTR(-EINVAL);

	ret = dma_buf_vmap(dmabuf, &map);
	if (ret)
		return ERR_PTR(ret);

	return map.vaddr;
}
EXPORT_SYMBOL(sprd_ion_map_kernel);

/*
 * sprd_ion_unmap_kernel (blob 0x08001030)
 *
 * A NULL dmabuf returns -EINVAL. Then an indirect call through ops+0x60 with
 * (dmabuf, offset), followed by one through ops+0x40 with a literal 0.
 * Returns 0 unconditionally.
 *
 * 6.18 DELTA: same collapse as above -- the vendor @vunmap/@unmap pair is
 * dma_buf_vunmap() here. Order (vunmap then unmap) is preserved.
 */
int sprd_ion_unmap_kernel(struct dma_buf *dmabuf, unsigned long offset)
{
	struct iosys_map map = { };

	if (IS_ERR_OR_NULL(dmabuf))
		return -EINVAL;

	dma_buf_vunmap(dmabuf, &map);

	return 0;
}
EXPORT_SYMBOL(sprd_ion_unmap_kernel);

/*
 * sprd_ion_get_phys_addr (blob 0x0800117c)
 *
 *  - fd >= 0: dma_buf_get(fd); NULL or ERR_PTR is -EBADF. The scatterlist is
 *    read at dmabuf+0x88 before dma_buf_put(), so the blob uses the table
 *    after dropping its reference -- kept only as a note; it is not
 *    reproduced, because there is no member to read.
 *  - fd < 0: the passed dmabuf is used as-is.
 *  - sanity check: sgl->flags at sgl+0x28, and if it is zero then
 *    sgt->orig_sgl at sgt+0x28 must equal PAGE_SIZE (the literal compare is
 *    `cmp x8, 1, lsl 12`), else -EINVAL.
 *  - *phys_addr gets the window-transformed address and *size gets
 *    sgt->orig_sgl.
 *
 * See the file header: every one of those reads is of a struct dma_buf /
 * struct sg_table member that 6.18 does not expose to an exporter-side
 * caller, and the address transform could not be decoded with confidence.
 * Returns -EOPNOTSUPP rather than guessing. This matches the contract the
 * tree already states in drivers/gpu/drm/sprd/gsp/gsp_ion_stub.c.
 */
int sprd_ion_get_phys_addr(int fd, struct dma_buf *dmabuf,
			    unsigned long *phys_addr, size_t *size)
{
	pr_err("sprd_ion: no ION core on 6.18, physical address unavailable\n");
	return -EOPNOTSUPP;
}
EXPORT_SYMBOL(sprd_ion_get_phys_addr);

/*
 * sprd_ion_get_buffer (blob 0x080012dc)
 *
 * Structurally identical to sprd_ion_get_phys_addr: same dma_buf_get/put
 * pair, same -EBADF, but it never touches dma_address -- it stores
 * sgt->orig_sgl into *buf and sgl->length into *size. It therefore has the
 * same blocker and the same answer.
 */
int sprd_ion_get_buffer(int fd, struct dma_buf *dmabuf,
			 void **buf, size_t *size)
{
	return -EOPNOTSUPP;
}
EXPORT_SYMBOL(sprd_ion_get_buffer);

/*
 * sprd_ion_probe (blob 0x080013a8)
 *
 * Walks the DT child nodes of the ION controller, one heap description per
 * child, filling the 0x30-byte table described above. Each heap is then
 * created and offered to the ION core.
 *
 * The register step cannot succeed here: __ion_device_add_heap() is an
 * -ENODEV stub on this tree (see the file header). The probe reports that
 * and fails rather than pretending to have registered anything, which is
 * why the carveout heap is unreachable at runtime on this kernel.
 */
static int sprd_ion_probe(struct platform_device *pdev)
{
	struct device_node *child, *node = pdev->dev.of_node;
	struct sprd_ion_carveout_info info;
	struct sprd_ion_heap *heap;
	struct ion_heap *ih;
	int count = 0, i, ret;

	if (!node)
		return -EINVAL;

	mutex_lock(&sprd_ion_lock);
	kfree(sprd_ion_heaps);
	sprd_ion_heaps = NULL;
	sprd_ion_num_heaps = 0;
	mutex_unlock(&sprd_ion_lock);

	/*
 * 6.18 removed for_each_child()/for_each_available_child() from the OF
 * headers; of_get_next_child() is the supported spelling.
 */
	for (child = of_get_next_child(node, NULL);
	     child;
	     child = of_get_next_child(node, child)) {
		u32 reg[4] = { 0, 0, 0, 0 };
		int n;

		n = of_property_read_u32_array(child, "reg", reg,
					       ARRAY_SIZE(reg));
		if (n < 0)
			continue;

		mutex_lock(&sprd_ion_lock);
		heap = krealloc_array(sprd_ion_heaps, count + 1,
				      sizeof(*heap), GFP_KERNEL);
		if (!heap) {
			mutex_unlock(&sprd_ion_lock);
			ret = -ENOMEM;
			goto err_heaps;
		}
		sprd_ion_heaps = heap;
		mutex_unlock(&sprd_ion_lock);

		heap = &sprd_ion_heaps[count];

		heap->id = count;
		if (of_property_read_u32(child, "heap-id", &heap->id))
			heap->id = count;

		heap->order = 0;
		of_property_read_u32(child, "order", &heap->order);

		heap->type = "carveout";
		of_property_read_string(child, "heap-type",
					 (const char **)&heap->type);

		heap->base  = (u64)reg[0] | ((u64)reg[1] << 32);
		heap->size  = (u64)reg[2] | ((u64)reg[3] << 32);
		heap->dev   = &pdev->dev;

		count++;
	}

	mutex_lock(&sprd_ion_lock);
	sprd_ion_num_heaps = count;
	mutex_unlock(&sprd_ion_lock);

	if (!count)
		return 0;

	for (i = 0; i < count; i++) {
		memset(&info, 0, sizeof(info));
		info.heap_id = sprd_ion_heaps[i].id;
		info.order   = sprd_ion_heaps[i].order;
		info.base    = sprd_ion_heaps[i].base;
		info.size    = sprd_ion_heaps[i].size;

		ih = ion_carveout_heap_create(&info);
		if (IS_ERR(ih))
			return PTR_ERR(ih);

		ret = __ion_device_add_heap(ih, THIS_MODULE);
		if (ret) {
			pr_err("sprd_ion: ION core absent (%d), carveout heaps cannot register\n",
			       ret);
			return ret;
		}
	}

	return 0;

err_heaps:
	mutex_lock(&sprd_ion_lock);
	kfree(sprd_ion_heaps);
	sprd_ion_heaps = NULL;
	sprd_ion_num_heaps = 0;
	mutex_unlock(&sprd_ion_lock);
	return ret;
}

/*
 * sprd_ion_remove (blob 0x08001870)
 *
 * The blob's remove() is a single kfree of the global heap table followed by
 * a 0 return, with no misc_deregister and no platform-device teardown -- it
 * leaks the misc device on unbind. Rebuilt to unregister cleanly, which is a
 * deliberate divergence and is noted as such.
 */
static void sprd_ion_remove(struct platform_device *pdev)
{
	mutex_lock(&sprd_ion_lock);
	kfree(sprd_ion_heaps);
	sprd_ion_heaps = NULL;
	sprd_ion_num_heaps = 0;
	mutex_unlock(&sprd_ion_lock);
}

static const struct of_device_id sprd_ion_match[] = {
	{ .compatible = "sprd,ion" },
	{ }
};

static struct platform_driver sprd_ion_driver = {
	.probe	= sprd_ion_probe,
	.remove	= sprd_ion_remove,
	.driver	= {
		.name	= "sprd-ion",
		.of_match_table = sprd_ion_match,
	},
};

static int __init sprd_ion_init(void)
{
	return platform_driver_register(&sprd_ion_driver);
}
fs_initcall(sprd_ion_init);

static void __exit sprd_ion_exit(void)
{
	platform_driver_unregister(&sprd_ion_driver);
}
module_exit(sprd_ion_exit);

MODULE_LICENSE("GPL");
/* dma_buf_vmap/dma_buf_vunmap diekspor di namespace DMA_BUF (kbuild 6.18):
 * tanpa IMPORT_NS, modpost menolak modul ini ("uses symbol dma_buf_vmap
 * from namespace DMA_BUF, but does not import it") -- ditemukan build
 * device 2026-10-09. */
MODULE_IMPORT_NS("DMA_BUF");
MODULE_DESCRIPTION("Unisoc ION allocator (UMS512/T618)");
