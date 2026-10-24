// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_iq.c - Unisoc IQ (modem I/Q capture and playback) character device
 *
 * Port of the vendor 5.4 module `sprd_iq.ko`.  The reverse engineering notes
 * live in docs/RE-SPRD-IQ.md and the per-function disassembly (with every
 * .bss/.data reference resolved through the ELF relocations) in
 * docs/evidence-20261023/sprd_iq.objdump.txt.
 *
 * The AP and the protocol stack processor share a reserved RAM region named
 * by the "sprd,region" phandle of the "sprd,iq" node.  The region is mapped
 * non-cached through SIPC and laid out as:
 *
 *   0x000  struct iq_pkt   head descriptor (16 bytes)
 *   0x010  status block    140 bytes (I/Q playback mailbox)
 *   ...    payload
 *   size-0x10  struct iq_pkt  tail descriptor
 *
 * A descriptor is a 32-bit "magic", the absolute physical address of its
 * payload (low 32 bits, as the firmware writes it) and a length:
 *
 *   IQ_MAGIC_FREE   descriptor free, the AP may fill it
 *   IQ_MAGIC_READY  filled by the modem, the thread has to consume it
 *   IQ_MAGIC_INIT   0x5a5a5a5a, probe marks both descriptors with it
 *
 * The playback block has a handshake of its own: the modem sets
 * IQ_MAGIC_DATA_MOVING, iq_thread acknowledges with IQ_MAGIC_DATA_MOVED and
 * wakes pollers; userspace then reads it with the GET_PLAYBACK ioctl and
 * writes a new request with PUT_PLAYBACK, which tags the head descriptor
 * with IQ_MAGIC_STATUS_KCLB.
 *
 * The driver only binds when the kernel command line carries
 * "androidboot.mode=iq", so it stays completely inert on a normal boot.
 */

#define pr_fmt(fmt) "sprd-iq: " fmt

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <linux/slab.h>
#include <linux/soc/sprd/sipc.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

/*
 * The vser bypass path is provided by drivers/usb/gadget/function/f_vserial.c
 * (module usb_f_vser), which exports these without a header of its own; the
 * vendor blob declared them locally as well.
 */
void kernel_vser_register_callback(void (*cb)(void *buf, unsigned int len,
					      unsigned long data),
				   unsigned long data);
int vser_pass_user_write(void *buf, int len);

/*
 * sipc.h only declares the _ex() flavours under CONFIG_SPRD_SIPC_V2, but
 * smem.c always implements and GPL-exports them in this tree.
 */
#ifndef CONFIG_SPRD_SIPC_V2
void *shmem_ram_vmap_nocache_ex(u8 dst, u16 smem, phys_addr_t start,
				size_t size);
void shmem_ram_unmap_ex(u8 dst, u16 smem, const void *mem);
#endif

/* The blob maps the region as belonging to the protocol stack processor. */
#define IQ_SIPC_DST		SIPC_ID_PSCP

/* Descriptor magics, as found in the blob. */
#define IQ_MAGIC_FREE		0x81005a00u	/* descriptor is free */
#define IQ_MAGIC_READY		0x81815a5au	/* filled, consume it */
#define IQ_MAGIC_INIT		0x5a5a5a5au	/* probe marks both slots */
#define IQ_MAGIC_PUSH		0x424f504eu	/* "NPOB", SET_MODE 2 */
#define IQ_MAGIC_STATUS_IPCD	0x44435049u	/* "IPCD" playback request */
#define IQ_MAGIC_STATUS_KCLB	0x424c434bu	/* "KCLB" playback ack */

/*
 * Playback status handshake.  The modem moves data and sets DATA_MOVING
 * (0x4441504d, bytes "MPAD"); iq_thread replaces it with DATA_MOVED
 * (0x504d4441, bytes "ADMP"), which is what poll() waits for.
 */
#define IQ_MAGIC_DATA_MOVING	0x4441504du
#define IQ_MAGIC_DATA_MOVED	0x504d4441u

#define IQ_STATUS_OFF		0x10	/* playback block inside the region */
#define IQ_STATUS_LEN		0x8cu	/* 140 bytes, opaque to the driver */
#define IQ_TAIL_RESERVE		0x10	/* tail descriptor at size - 0x10 */
#define IQ_FIRST_PAYLOAD	0x100	/* head descriptor offset in SET_MODE 2 */
#define IQ_MAX_CHUNK		0xd0000	/* vser chunk size */

/* Modes accepted by the SET_MODE ioctl. */
#define IQ_MODE_PUSH		0	/* thread feeds descriptors to vser */
#define IQ_MODE_IDLE		1	/* only wake up pollers */
#define IQ_MODE_PLAYBACK	2	/* modem writes the playback block */

/* ioctl commands -- plain numbers, not _IO() encoded, as in the blob. */
#define IQ_IOC_GET_BUF		0x00	/* out: struct iq_buf */
#define IQ_IOC_GET_PLAYBACK	0x01	/* out: 140 byte status block */
#define IQ_IOC_SET_MODE		0x80	/* in: u32 mode */
#define IQ_IOC_ACK		0x82	/* FREE -> READY on head or tail */
#define IQ_IOC_PUT_PLAYBACK	0x83	/* in: 140 byte status block */

struct iq_pkt {
	u32 magic;
	u32 offset;	/* absolute physical address, low 32 bits */
	u32 size;
};

struct iq_buf {
	u32 offset;	/* byte offset relative to base + mapping-offs */
	u32 size;
};

/* 24 bytes, allocated with kzalloc() by the blob as well. */
struct iq_chan {
	struct iq_pkt *head;
	struct iq_pkt *tail;
	u32 *status;
};

struct sprd_iq {
	phys_addr_t base;
	size_t size;
	u32 offs;		/* DT "sprd,mapping-offs" */
	void *va;
	struct iq_chan *chan;
	struct task_struct *thread;
	wait_queue_head_t wait;
	u32 mode;
};

static struct sprd_iq *iq_dev;

static inline u32 iq_read_magic(const u32 *p)
{
	return READ_ONCE(*p);
}

static inline void iq_write_magic(u32 *p, u32 val)
{
	WRITE_ONCE(*p, val);
}

/*
 * Physical address of a descriptor payload: the field holds the absolute
 * address with the mapping offset folded in, iq_thread and sprd_iq_complete
 * both subtract iq->offs before turning it into a virtual address.
 */
static inline phys_addr_t iq_pkt_pa(struct sprd_iq *iq, u32 offset)
{
	return (phys_addr_t)offset - iq->offs;
}

/* End of the payload, as compared against the vser completion address. */
static inline void *iq_pkt_end(struct sprd_iq *iq, const struct iq_pkt *pkt)
{
	return __va(iq_pkt_pa(iq, READ_ONCE(pkt->offset)) +
		    READ_ONCE(pkt->size));
}

/*
 * Refuse descriptors that would make us transmit memory outside the reserved
 * region.  The blob has no such check: a corrupted descriptor there hands an
 * arbitrary kernel address to vser_pass_user_write().  A zero length slot is
 * accepted, exactly like the blob, which simply sends nothing.
 */
static bool iq_pkt_valid(struct sprd_iq *iq, phys_addr_t pa, u32 size)
{
	if (size > iq->size)
		return false;
	if (pa < iq->base)
		return false;
	if (pa + size > iq->base + iq->size)
		return false;
	return true;
}

/*
 * Push one descriptor to the host through the vser bypass path, in 0xd0000
 * byte chunks, retrying a failed chunk after 200 ms.
 */
static void iq_push(struct sprd_iq *iq, const struct iq_pkt *pkt)
{
	u32 offset = READ_ONCE(pkt->offset);
	u32 size = READ_ONCE(pkt->size);
	phys_addr_t pa = iq_pkt_pa(iq, offset);
	u32 sent = 0;

	pr_info("iq write 0x%x, 0x%x\n", offset - iq->offs, size);

	if (!iq_pkt_valid(iq, pa, size)) {
		pr_err("invalid iq descriptor offset 0x%x size 0x%x\n",
		       offset, size);
		return;
	}

	while (sent < size) {
		u32 chunk = min(size - sent, (u32)IQ_MAX_CHUNK);
		int ret = vser_pass_user_write(__va(pa + sent), chunk);

		if (ret < 0) {
			msleep(200);
			continue;
		}
		sent += ret;
	}
}

/*
 * Bulk-IN completion callback registered with the vser bypass path.  When the
 * last chunk of a descriptor has been handed to the host the slot is marked
 * ready again, which is what the blob does (only in push mode).
 */
static void sprd_iq_complete(void *buf, unsigned int len, unsigned long data)
{
	struct sprd_iq *iq = iq_dev;
	struct iq_chan *chan;
	void *end = (u8 *)buf + len;

	if (!iq || READ_ONCE(iq->mode) != IQ_MODE_PUSH)
		return;

	chan = iq->chan;
	if (!chan || !chan->head || !chan->tail)
		return;

	pr_info("complete 0x%p, 0x%x\n", buf, len);

	if (end == iq_pkt_end(iq, chan->head))
		iq_write_magic(&chan->head->magic, IQ_MAGIC_READY);
	if (end == iq_pkt_end(iq, chan->tail))
		iq_write_magic(&chan->tail->magic, IQ_MAGIC_READY);
}

static int sprd_iq_thread(void *data)
{
	struct sprd_iq *iq = data;
	struct iq_chan *chan = iq->chan;
	struct sched_param sp = { .sched_priority = 80 };

	if (!chan || !chan->head || !chan->tail)
		return -EPERM;

	/* Same policy and priority as the blob: SCHED_FIFO at 80. */
	sched_setscheduler(current, SCHED_FIFO, &sp);

	while (!kthread_should_stop()) {
		struct iq_pkt *pkt;

		/* Acknowledge playback data the modem moved to the AP. */
		if (READ_ONCE(iq->mode) == IQ_MODE_PLAYBACK &&
		    iq_read_magic(chan->status) == IQ_MAGIC_DATA_MOVING) {
			iq_write_magic(chan->status, IQ_MAGIC_DATA_MOVED);
			wake_up_interruptible(&iq->wait);
		}

		pkt = chan->head;
		if (iq_read_magic(&pkt->magic) == IQ_MAGIC_READY)
			goto consume;

		pkt = chan->tail;
		if (iq_read_magic(&pkt->magic) == IQ_MAGIC_READY)
			goto consume;

		pr_info("ch: %d, hand1: 0x%x, flag2: 0x%x\n",
			READ_ONCE(iq->mode),
			iq_read_magic(&chan->head->magic),
			iq_read_magic(&chan->tail->magic));
		msleep(1500);
		continue;

consume:
		iq_write_magic(&pkt->magic, IQ_MAGIC_FREE);

		if (READ_ONCE(iq->mode) == IQ_MODE_IDLE) {
			wake_up_interruptible(&iq->wait);
			continue;
		}

		if (READ_ONCE(iq->mode) == IQ_MODE_PUSH)
			iq_push(iq, pkt);
	}

	return 0;
}

static __poll_t iq_mem_poll(struct file *filp, poll_table *wait)
{
	struct sprd_iq *iq = iq_dev;
	struct iq_chan *chan;

	if (!iq || !iq->chan)
		return EPOLLERR;

	chan = iq->chan;
	if (!chan->head || !chan->tail)
		return EPOLLERR;

	poll_wait(filp, &iq->wait, wait);

	if (READ_ONCE(iq->mode) == IQ_MODE_PLAYBACK) {
		if (iq_read_magic(chan->status) == IQ_MAGIC_DATA_MOVED) {
			pr_err("I/Q playback status DATA_AP_MOVING\n");
			return EPOLLIN | EPOLLRDNORM;
		}
		return 0;
	}

	if (READ_ONCE(iq->mode) == IQ_MODE_IDLE) {
		if (iq_read_magic(&chan->head->magic) == IQ_MAGIC_FREE ||
		    iq_read_magic(&chan->tail->magic) == IQ_MAGIC_FREE)
			return EPOLLIN | EPOLLRDNORM;
	}

	return 0;
}

/* Report the first free descriptor; all zeroes when both are busy. */
static int iq_get_buf(struct sprd_iq *iq, struct iq_chan *chan,
		      unsigned long arg)
{
	struct iq_buf buf = { 0 };
	struct iq_pkt *pkt;

	pkt = chan->head;
	if (iq_read_magic(&pkt->magic) != IQ_MAGIC_FREE) {
		pkt = chan->tail;
		if (iq_read_magic(&pkt->magic) != IQ_MAGIC_FREE)
			goto out;
	}

	buf.offset = READ_ONCE(pkt->offset) - lower_32_bits(iq->base) -
		     iq->offs;
	buf.size = READ_ONCE(pkt->size);

out:
	if (copy_to_user((void __user *)arg, &buf, sizeof(buf))) {
		pr_err("copy iq buf info to user space failed.\n");
		return -EFAULT;
	}

	return 0;
}

static long iq_mem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct sprd_iq *iq = iq_dev;
	struct iq_chan *chan;

	pr_info("enter ioctl cmd = 0x%x\n", cmd);

	if (!iq || !iq->chan)
		return -EPERM;

	chan = iq->chan;
	if (!chan->head || !chan->tail)
		return -EPERM;

	switch (cmd) {
	case IQ_IOC_GET_BUF:
		return iq_get_buf(iq, chan, arg);

	case IQ_IOC_GET_PLAYBACK:
		if (READ_ONCE(iq->mode) != IQ_MODE_PLAYBACK) {
			pr_err("current mode is not playback mode\n");
			return -EINVAL;
		}
		if (copy_to_user((void __user *)arg, chan->status,
				 IQ_STATUS_LEN)) {
			pr_err("copy iq playback data info to user space failed.\n");
			return -EFAULT;
		}
		return 0;

	case IQ_IOC_SET_MODE:
		if (arg > (unsigned long)IQ_MODE_PLAYBACK) {
			pr_err("iq ch type invalid\n");
			return -EINVAL;
		}
		WRITE_ONCE(iq->mode, (u32)arg);

		if (arg == IQ_MODE_IDLE) {
			if (iq_read_magic(&chan->head->magic) == IQ_MAGIC_FREE ||
			    iq_read_magic(&chan->tail->magic) == IQ_MAGIC_FREE)
				wake_up_interruptible(&iq->wait);
		} else if (arg == IQ_MODE_PLAYBACK) {
			chan->status = (u32 *)((u8 *)iq->va + IQ_STATUS_OFF);
			WRITE_ONCE(chan->head->offset,
				   lower_32_bits(iq->base) + IQ_FIRST_PAYLOAD);
			iq_write_magic(&chan->head->magic, IQ_MAGIC_PUSH);
		}
		return 0;

	case IQ_IOC_ACK:
		if (iq_read_magic(&chan->head->magic) == IQ_MAGIC_FREE)
			iq_write_magic(&chan->head->magic, IQ_MAGIC_READY);
		else if (iq_read_magic(&chan->tail->magic) == IQ_MAGIC_FREE)
			iq_write_magic(&chan->tail->magic, IQ_MAGIC_READY);
		return 0;

	case IQ_IOC_PUT_PLAYBACK:
		if (READ_ONCE(iq->mode) != IQ_MODE_PLAYBACK) {
			pr_err("current mode is not playback mode\n");
			return -EINVAL;
		}
		if (copy_from_user(chan->status, (void __user *)arg,
				   IQ_STATUS_LEN)) {
			pr_err("copy iq playback data info from user space failed.\n");
			return -EFAULT;
		}
		WRITE_ONCE(chan->head->size, READ_ONCE(chan->status[2]));
		if (READ_ONCE(chan->status[0]) == IQ_MAGIC_STATUS_IPCD)
			iq_write_magic(&chan->head->magic,
				       IQ_MAGIC_STATUS_KCLB);
		return 0;

	default:
		/*
		 * The blob ignores every other command (including 2..0x7f and
		 * anything above 0x83) and returns success.
		 */
		return 0;
	}
}

/*
 * Map the reserved region.  The blob passes base >> PAGE_SHIFT to
 * remap_pfn_range() and asks for a non-cached mapping by hand; both are done
 * with the current helpers here.  mmap() of an I/O region has to stay out of
 * the page cache, hence VM_IO | VM_PFNMAP.
 */
static int iq_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct sprd_iq *iq = iq_dev;
	unsigned long size = vma->vm_end - vma->vm_start;

	if (!iq || !iq->base) {
		pr_err("get iq mem info failed\n");
		return -EAGAIN;
	}

	if (size > iq->size) {
		pr_err("map size to big, exceed maxsize.\n");
		return -EAGAIN;
	}

	vm_flags_set(vma, VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start, iq->base >> PAGE_SHIFT, size,
			    vma->vm_page_prot)) {
		pr_err("remap_pfn_range failed\n");
		return -EAGAIN;
	}

	pr_info("iq mmap %lx,%lx,%lx\n", vma->vm_start, vma->vm_end, size);

	return 0;
}

static int iq_mem_open(struct inode *inode, struct file *filp)
{
	pr_info("%s called\n", __func__);
	return 0;
}

static int iq_mem_release(struct inode *inode, struct file *filp)
{
	pr_info("%s called\n", __func__);
	return 0;
}

static const struct file_operations iq_mem_fops = {
	.owner		= THIS_MODULE,
	.open		= iq_mem_open,
	.release	= iq_mem_release,
	.poll		= iq_mem_poll,
	.unlocked_ioctl	= iq_mem_ioctl,
	.mmap		= iq_mem_mmap,
};

static struct miscdevice iq_mmap_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "iq_mem",
	.fops	= &iq_mem_fops,
};

/*
 * Factory-mode gate: the whole driver is inert unless the kernel command
 * line carries androidboot.mode=iq.  The blob has a separate static
 * in_iqmode() helper with the same logic, but it is dead code -- the copy
 * that runs is the one inlined at the top of probe, which is what is kept
 * here (it also has the error message belonging to each branch).
 */
static int sprd_iq_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device_node *region;
	struct device_node *chosen;
	const char *bootargs;
	struct resource res;
	struct sprd_iq *iq;
	u32 offs;
	int ret;

	chosen = of_find_node_opts_by_path("/chosen", NULL);
	if (!chosen) {
		pr_err("Can't not parse bootargs\n");
		return -ENODEV;
	}

	if (of_property_read_string(chosen, "bootargs", &bootargs)) {
		pr_err("Can't not parse bootargs\n");
		of_node_put(chosen);
		return -ENODEV;
	}

	ret = strstr(bootargs, "androidboot.mode=iq") ? 0 : -ENODEV;
	of_node_put(chosen);

	if (ret) {
		pr_info("Not in iq mode\n");
		return ret;
	}

	pr_info("In iq mode\n");

	if (iq_dev)
		return -EBUSY;

	if (!node) {
		dev_err(&pdev->dev, "param error\n");
		return -ENODEV;
	}

	region = of_parse_phandle(node, "sprd,region", 0);
	if (!region) {
		dev_err(&pdev->dev, "get iq mem info failed\n");
		return -ENOMEM;
	}

	ret = of_address_to_resource(region, 0, &res);
	of_node_put(region);
	if (ret) {
		dev_err(&pdev->dev, "get iq mem info failed\n");
		return -ENODEV;
	}

	iq = kzalloc(sizeof(*iq), GFP_KERNEL);
	if (!iq)
		return -ENOMEM;

	iq->base = res.start;
	iq->size = resource_size(&res);

	dev_info(&pdev->dev, "iq base: 0x%lx, iq size: 0x%lx\n",
		 (unsigned long)iq->base, (unsigned long)iq->size);

	/*
	 * The blob prints this message but never checks: a region smaller than
	 * the two descriptors would put tail below va and corrupt memory.
	 */
	if (!iq->base || iq->size <= IQ_TAIL_RESERVE + sizeof(struct iq_pkt)) {
		dev_err(&pdev->dev, "invalid iq_base(0x%llx).\n",
			(unsigned long long)iq->base);
		ret = -ENODEV;
		goto err_free;
	}

	if (!of_property_read_u32(node, "sprd,mapping-offs", &offs))
		iq->offs = offs;

	dev_info(&pdev->dev, "iq mapping_offs: 0x%x\n", iq->offs);

	init_waitqueue_head(&iq->wait);

	iq->va = shmem_ram_vmap_nocache_ex(IQ_SIPC_DST, 0, iq->base, iq->size);
	if (!iq->va) {
		dev_err(&pdev->dev, "get iq mem info failed\n");
		ret = -ENOMEM;
		goto err_free;
	}

	iq->chan = kzalloc(sizeof(*iq->chan), GFP_KERNEL);
	if (!iq->chan) {
		ret = -ENOMEM;
		goto err_unmap;
	}

	iq->chan->head = (struct iq_pkt *)iq->va;
	iq->chan->tail = (struct iq_pkt *)((u8 *)iq->va + iq->size -
					   IQ_TAIL_RESERVE);
	/*
	 * The blob only fills chan->status in SET_MODE 2; pointing it at the
	 * playback block right away is the same address and keeps poll() from
	 * ever dereferencing a NULL pointer.
	 */
	iq->chan->status = (u32 *)((u8 *)iq->va + IQ_STATUS_OFF);

	iq_write_magic(&iq->chan->head->magic, IQ_MAGIC_INIT);
	iq_write_magic(&iq->chan->tail->magic, IQ_MAGIC_INIT);

	iq_dev = iq;
	platform_set_drvdata(pdev, iq);

	ret = misc_register(&iq_mmap_dev);
	if (ret) {
		dev_err(&pdev->dev, "cannot register iq_mmap_dev ret = (%d)\n",
			ret);
		goto err_clear;
	}

	kernel_vser_register_callback(sprd_iq_complete, 0);

	iq->thread = kthread_run(sprd_iq_thread, iq, "iq_thread");
	if (IS_ERR(iq->thread)) {
		ret = PTR_ERR(iq->thread);
		iq->thread = NULL;
		dev_err(&pdev->dev, "create iq_thread error!\n");
		goto err_deregister;
	}

	return 0;

err_deregister:
	kernel_vser_register_callback(NULL, 0);
	misc_deregister(&iq_mmap_dev);
err_clear:
	platform_set_drvdata(pdev, NULL);
	iq_dev = NULL;
	kfree(iq->chan);
	shmem_ram_unmap_ex(IQ_SIPC_DST, 0, iq->va);
	kfree(iq);
	return ret;
}

static void sprd_iq_remove(struct platform_device *pdev)
{
	struct sprd_iq *iq = platform_get_drvdata(pdev);

	if (!iq)
		return;

	/* The blob leaked the mapping and the thread; stop both first. */
	if (iq->thread) {
		kthread_stop(iq->thread);
		iq->thread = NULL;
	}

	kernel_vser_register_callback(NULL, 0);
	misc_deregister(&iq_mmap_dev);

	if (iq->va)
		shmem_ram_unmap_ex(IQ_SIPC_DST, 0, iq->va);

	kfree(iq->chan);
	iq_dev = NULL;
	kfree(iq);
}

static const struct of_device_id iq_of_match[] = {
	{ .compatible = "sprd,iq" },
	{ }
};
MODULE_DEVICE_TABLE(of, iq_of_match);

static struct platform_driver iq_driver = {
	.probe	= sprd_iq_probe,
	.remove	= sprd_iq_remove,
	.driver	= {
		.name		= "sprd-iq",
		.of_match_table	= iq_of_match,
	},
};
module_platform_driver(iq_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Unisoc IQ playback / I-Q data device");
MODULE_ALIAS("platform:sprd-iq");
