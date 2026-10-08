// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * PORT 6.18 (2026-09-30)
 * Sumber: work/donors_20260930/bocchi_unisoc54/drivers/soc/sprd/modem/sipc/
 *         sipx.c  (GPL-2.0-only, Unisoc UMS512 5.4, 2115 baris)
 * Donor dicocokkan ke device: blob UMS512 `sipx.ko` meng-`undef` 12 simbol
 * yang DIEKSPOR berkas ini (docs/sipc_modem_bedah_20260930.md 6.2).
 *
 * Delta API 5.4 -> 6.18 yang diterapkan (nilai lihat di tiap tempat):
 *  1) <linux/sipc.h> -> <linux/soc/sprd/sipc.h>. Header t618 yang di-port
 *     dipindah ke include/linux/soc/sprd/sipc.h (diff -q = IDENTICAL).
 *  2) smem_alloc(dst,size) -> smem_alloc(size); smem_free(dst,addr,size)
 *     -> smem_free(addr,size); shmem_ram_vmap_nocache(dst,start,size)
 *     -> (start,size); shmem_ram_vmap_cache(dst,...) -> (...);
 *     shmem_ram_unmap(dst,mem) -> (mem).
 *     Bukti: bentuk non-V2 di include/linux/soc/sprd/sipc.h:370-444,
 *     definisi smem.c:423 / :354 / :331. Bentuk bertakdst hanya ada
 *     di balik #ifdef CONFIG_SPRD_SIPC_V2 yang tidak aktif di port.
 *  3) ipc->smem_base / ipc->dst_smem_base TIDAK ADA di struct smsg_ipc
 *     port (donor bocchi sipc_priv.h:102,105-107). Nilai setara ada di
 *     sisi pdata: sipc.c:203-224 membaca "sprd,smem-info" lalu mengisi
 *     pdata->smem_base (triplet ke-1) dan pdata->mapped_smem_base
 *     (triplet ke-2) - triplet ke-2 itu yang donor sebut dst_smem_base.
 *     sblock.c:341 di port sudah memakai rumus yang sama.
 *  4) CONFIG_PHYS_ADDR_T_64BIT / ipc->high_offset DIHAPUS: port tidak
 *     punya high_offset sama sekali (0 hit di sipc_priv.h/smsg.c).
 *     Blok offset 64-bit donor mati di device ini. Dinyatakan.
 *  5) hrtimer_init(&t,clk,mode); t.function = fn -> hrtimer_setup(&t,fn,
 *     clk,mode).
 *  6) struct platform_driver .remove: int -> void.
 *  7) struct file_operations TIDAK diganti struct proc_ops di debugfs:
 *     debugfs_create_file() (include/linux/debugfs.h:122-130) memakai
 *     _Generic() yang hanya menerima struct file_operations *.
 *  8) sched_setscheduler(task,policy,param) versi 3-arg (dengan task)
 *     masih diekspor 6.18 - BUKAN blocker yang tercatat di
 *     docs/PORTING_RE_API.tsv:29 (itu versi 2-arg tanpa task).
 */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "sprd-sipx: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/log2.h>
#include <linux/platform_device.h>
#include <linux/if_ether.h>
#include <linux/cache.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/soc/sprd/sipc.h>

/* debug{ */
#include <linux/delay.h>
/* }debug */

#include <uapi/linux/sched/types.h>

#include "sipc_debugfs.h"
#include "sipx.h"
#include "sblock.h"
#include "sipc_priv.h"

#define SIPX_BLOCK_PENDING      (32)

#define SIPX_UL_OFFSET          (16)

#define NORMAL_BLOCK_SIZE	(ETH_HLEN + ETH_DATA_LEN + NET_IP_ALIGN)
#define ACK_BLOCK_SIZE		(96)

#define TIME_TRIGGER_CP_NS      (500 * 1000)

#define IS_DL_BLK_ACK(sipx, blk) ((blk)->addr >= (sipx)->dl_ack_start)

#define IS_UL_BLK_ACK(sipx, blk) ((blk)->addr >= (sipx)->ul_ack_start)

/* #define UL_TEST */

#ifdef UL_TEST
int g_flow_on = 1;
#endif

static struct sipx_mgr *sipxs[SIPC_ID_NR];

#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
static struct device *sipx_dev;
#endif

static struct sipx_channel *sipx_chan_record[SMSG_CH_NR + 1];

/*
 * Basis smem sisi peer (CP) untuk sebuah alokasi.
 *
 * Donor memakai ipc->dst_smem_base; port tidak punya field itu. Yang
 * setara ada di pdata (catatan delta no.3): pdata->smem_base = triplet
 * ke-1 "sprd,smem-info", triplet ke-2 = pdata->mapped_smem_base.
 * Rumus identik dengan sblock.c:341 di pohon port.
 */
static u32 sipx_ap_smem_base(u8 dst)
{
	struct sipc_device *sdev = sipc_ap.sipc_dev[dst];

	if (!sdev || !sdev->pdata)
		return 0;

	return sdev->pdata->smem_base;
}

static u32 sipx_dst_smem_base(u8 dst)
{
	struct sipc_device *sdev = sipc_ap.sipc_dev[dst];

	if (!sdev || !sdev->pdata)
		return 0;

	return sdev->pdata->mapped_smem_base;
}

#if defined(CONFIG_DEBUG_FS)

static int sipx_debug_show(struct seq_file *m, void *private)
{
	volatile struct sipx_fifo_info *fifo_info = NULL;
	struct sipx_mgr *sipx = NULL;
	struct sipx_channel *sipx_chan = NULL;
	int i, j;

	for (i = 0; i < SIPC_ID_NR; i++) {
		sipx = sipxs[i];
		if (!sipx)
			continue;

		/*sipx*/
		sipc_debug_putline(m, '*', 180);
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
		seq_printf(m, "sipx dst 0x%0x, state: 0x%0x, recovery: %d, ",
			   sipx->dst, sipx->state, sipx->recovery);
		seq_printf(m, "smem_virt: 0x%p, smem_cached_virt: 0x%p, ",
			   sipx->smem_virt, sipx->smem_cached_virt);
		seq_printf(m, "smem_addr: 0x%0x, mapped_smem_addr:",
			   sipx->smem_addr);
		seq_printf(m, "0x%0x\n, smem_size: 0x%0x",
			   sipx->dst_smem_addr,
			   sipx->smem_size);
#else
		seq_printf(m, "sipx dst %d, state: 0x%0x, recovery: %d, ",
			   sipx->dst, sipx->state, sipx->recovery);
		seq_printf(m, "smem_virt: 0x%p, smem_addr: 0x%0x, smem_size: 0x%0x\n",
			   sipx->smem_virt,
			   sipx->smem_addr,
			   sipx->smem_size);
#endif
		seq_printf(m, "dl_pool_size: %d, dl_ack_pool_size: %d,",
			   sipx->dl_pool_size,
			   sipx->dl_ack_pool_size);
		seq_printf(m, "ul_pool_size: %d, ul_ack_pool_size: %d,",
			   sipx->ul_pool_size,
			   sipx->ul_ack_pool_size);
		seq_printf(m, "blk_size: %d, ack_blk_size: %d\n",
			   sipx->blk_size,
			   sipx->ack_blk_size);

		fifo_info = sipx->dl_pool->fifo_info;
		seq_printf(m, "dl_pool: blks_addr :0x%0x, blk_size :%d,",
			   (u32)fifo_info->blks_addr,
			   (u32)fifo_info->blk_size);
		seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d, fifo_rdptr",
			   (u32)fifo_info->fifo_addr,
			   (u32)fifo_info->fifo_size);
		seq_printf(m, " :0x%0x, fifo_wrptr: 0x%0x\n",
			   (u32)fifo_info->fifo_rdptr,
			   (u32)fifo_info->fifo_wrptr);

		fifo_info = sipx->dl_ack_pool->fifo_info;
		seq_printf(m, "dl_ack_pool: blks_addr :0x%0x, blk_size :%d,",
			   (u32)fifo_info->blks_addr,
			   (u32)fifo_info->blk_size);
		seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d,",
			   (u32)fifo_info->fifo_addr,
			   (u32)fifo_info->fifo_size);
		seq_printf(m, " fifo_rdptr :0x%0x, fifo_wrptr: 0x%0x\n",
			   (u32)fifo_info->fifo_rdptr,
			   (u32)fifo_info->fifo_wrptr);

		fifo_info = sipx->ul_pool->fifo_info;
		seq_printf(m, "ul_pool: blks_addr :0x%0x, blk_size :%d,",
			   (u32)fifo_info->blks_addr,
			   (u32)fifo_info->blk_size);
		seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d,",
			   (u32)fifo_info->fifo_addr,
			   (u32)fifo_info->fifo_size);
		seq_printf(m, " fifo_rdptr :0x%0x, fifo_wrptr: 0x%0x\n",
			   (u32)fifo_info->fifo_rdptr,
			   (u32)fifo_info->fifo_wrptr);

		fifo_info = sipx->ul_ack_pool->fifo_info;
		seq_printf(m, "ul_ack_pool: blks_addr :0x%0x, blk_size :%d,",
			   (u32)fifo_info->blks_addr,
			   (u32)fifo_info->blk_size);
		seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d,",
			   (u32)fifo_info->fifo_addr,
			   (u32)fifo_info->fifo_size);
		seq_printf(m, " fifo_rdptr :0x%0x, fifo_wrptr: 0x%0x\n",
			   (u32)fifo_info->fifo_rdptr,
			   (u32)fifo_info->fifo_wrptr);

		sipc_debug_putline(m, '*', 180);

		for (j = 0; j < SMSG_VALID_CH_NR; j++) {
			sipx_chan = sipx->channels[j];
			if (!sipx_chan)
				continue;

			/* sipx channel */
			seq_printf(m, "sipx_chan dst %d, channel: %3d, state: %d, ",
				   sipx_chan->dst,
				   sipx_chan->channel,
				   sipx_chan->state);
			seq_printf(m, "smem_virt: 0x%p, smem_addr: 0x%0x, ",
				   sipx_chan->smem_virt,
				   sipx_chan->smem_addr);
			seq_printf(m, "dst_smem_addr: 0x%0x, smem_size: 0x%0x, ",
				   sipx_chan->dst_smem_addr,
				   sipx_chan->smem_size);
			seq_printf(m, "dl_record: 0x%0x,ul_record: 0x%0x\n",
				   sipx_chan->dl_record_flag,
				   sipx_chan->ul_record_flag);

			fifo_info = sipx_chan->dl_ring->fifo_info;
			seq_printf(m, "dl_ring: blks_addr :0x%0x, blk_size :%d,",
				   (u32)fifo_info->blks_addr,
				   (u32)fifo_info->blk_size);
			seq_printf(m,
				   "fifo_addr :0x%0x, fifo_size :%d, fifo_rdptr :0x%0x,:",
				   (u32)fifo_info->fifo_addr,
				   (u32)fifo_info->fifo_size,
				   (u32)fifo_info->fifo_rdptr);
			seq_printf(m, "fifo_wrptr: 0x%0x\n",
				   (u32)fifo_info->fifo_wrptr);

			fifo_info = sipx_chan->dl_ack_ring->fifo_info;
			seq_printf(m, "dl_ack_ring: blks_addr :0x%0x, blk_size :%d,",
				   (u32)fifo_info->blks_addr,
				   (u32)fifo_info->blk_size);
			seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d, ",
				   (u32)fifo_info->fifo_addr,
				   (u32)fifo_info->fifo_size);
			seq_printf(m, "fifo_rdptr :0x%0x, fifo_wrptr: 0x%0x\n",
				   (u32)fifo_info->fifo_rdptr,
				   (u32)fifo_info->fifo_wrptr);

			fifo_info = sipx_chan->ul_ring->fifo_info;
			seq_printf(m, "ul_ring: blks_addr :0x%0x, blk_size :%d, ",
				   (u32)fifo_info->blks_addr,
				   (u32)fifo_info->blk_size);
			seq_printf(m, "fifo_addr :0x%0x, fifo_size :%d, ",
				   (u32)fifo_info->fifo_addr,
				   (u32)fifo_info->fifo_size);
			seq_printf(m, "fifo_rdptr :0x%0x, fifo_wrptr: 0x%0x\n",
				   (u32)fifo_info->fifo_rdptr,
				   (u32)fifo_info->fifo_wrptr);

			fifo_info = sipx_chan->ul_ack_ring->fifo_info;
			seq_printf(m, "ul_ack_ring: blks_addr :0x%0x, blk_size :%d,",
				   (u32)fifo_info->blks_addr,
				   (u32)fifo_info->blk_size);
			seq_printf(m,
				   " fifo_addr :0x%0x, fifo_size :%d, fifo_rdptr :0x%0x,",
				   (u32)fifo_info->fifo_addr,
				   (u32)fifo_info->fifo_size,
				   (u32)fifo_info->fifo_rdptr);
			seq_printf(m, " fifo_wrptr: 0x%0x\n",
				   (u32)fifo_info->fifo_wrptr);

			sipc_debug_putline(m, '*', 180);
		}
	}

	return 0;
}

static int sipx_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipx_debug_show, inode->i_private);
}

/*
 * 6.18 - KOREKSI ASUMSI: debugfs masih memakai struct file_operations.
 * Makro debugfs_create_file() (include/linux/debugfs.h:122-130) memakai
 * _Generic() yang hanya menerima `struct file_operations *` atau
 * `struct debugfs_short_fops *`; struct proc_ops ditolak. Penggantian ke
 * proc_ops hanya berlaku untuk procfs (proc_create()).
 */
static const struct file_operations sipx_debug_fops = {
	.open		= sipx_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/* Bukan static: sipc_debugfs.h:20 mendeklarasikannya non-static di
 * bawah CONFIG_SPRD_SIPC_ZERO_COPY_SIPX supaya sipc_debugfs.c bisa
 * memanggilnya. Tanpa itu: "static declaration follows non-static
 * declaration".
 */
int  sipx_init_debugfs(void *root)
{
	if (!root)
		return -ENXIO;

	debugfs_create_file("sipx", 0444,
			    (struct dentry *)root,
			    NULL,
			    &sipx_debug_fops);
	return 0;
}

#endif /* CONFIG_DEBUG_FS */

static inline u16 SIPX_GEN_DESC(u32 length, u16 offset)
{
	return ((length & 0x7ff) | ((offset & 0x1F) << 11));
}

static inline void SIPX_PARSE_DESC(u16 desc, u32 *length, u16 *offset)
{
	*length = (desc & 0x7ff);
	*offset = (desc >> 11) & 0x1F;
}

static int sipx_get_item_from_pool(struct sipx_pool *pool, struct sblock *blk)
{
	unsigned long flags;
	int pos;
	int ret = 0;

	/* multi-gotter may cause got failure */
	spin_lock_irqsave(&pool->lock, flags);
	if (pool->fifo_info->fifo_rdptr != pool->fifo_info->fifo_wrptr) {
		pos = sblock_get_ringpos(pool->fifo_info->fifo_rdptr,
					 pool->fifo_size);
		blk->index = pool->fifo_buf[pos].index;
		blk->addr = pool->blks_buf +  blk->index * pool->blk_size;
		blk->length = pool->blk_size;
		pool->fifo_info->fifo_rdptr++;
	} else {
		ret = -EAGAIN;
	}

	spin_unlock_irqrestore(&pool->lock, flags);

	return ret;
}

static int sipx_put_item_back_to_pool(struct sipx_pool *pool,
				      struct sblock *blk)
{
	unsigned long flags;
	int pos;
	int ret = 0;
	u32 last_rdptr;

	spin_lock_irqsave(&pool->lock, flags);
	if ((pool->fifo_info->fifo_wrptr - pool->fifo_info->fifo_rdptr)
			< pool->fifo_size) {
		last_rdptr = pool->fifo_info->fifo_rdptr - 1;
		pos = sblock_get_ringpos(last_rdptr, pool->fifo_size);
		pool->fifo_buf[pos].index = blk->index;
		pool->fifo_info->fifo_rdptr = last_rdptr;
	} else {
		ret = -1;
	}

	spin_unlock_irqrestore(&pool->lock, flags);

	return ret;
}

static int sipx_put_item_into_pool(struct sipx_pool *pool, struct sblock *blk)
{
	unsigned long flags;
	int pos;
	int ret = 0;
	u32 wr, rd;
	u32 item;

	spin_lock_irqsave(&pool->lock, flags);

	wr = pool->fifo_info->fifo_wrptr;
	rd = pool->fifo_info->fifo_rdptr;
	if ((wr - rd) < pool->fifo_size) {
		pos = sblock_get_ringpos(wr, pool->fifo_size);
		item = blk->index;
		/* smp_wmb();
		 * pool->fifo_buf[pos].index = blk->index;
		 * (*(u32*)(&pool->fifo_buf[pos])) = *((u32*)&item);
		 */
		(*(u32 *)(&pool->fifo_buf[pos])) = item;
		pool->fifo_info->fifo_wrptr = wr + 1;
	} else {
		pr_err("fail: wr:0x%x, rd:0x%x, size:%d\n",
			 wr, rd, pool->fifo_size);
		ret = -1;
	}
	spin_unlock_irqrestore(&pool->lock, flags);

	return ret;
}

static int sipx_get_pool_free_count(struct sipx_pool *pool)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&pool->lock, flags);
	ret = pool->fifo_info->fifo_wrptr - pool->fifo_info->fifo_rdptr;
	spin_unlock_irqrestore(&pool->lock, flags);

	return ret;
}

static int sipx_put_item_to_ring(struct sipx_ring *ring, struct sblock *blk,
				 int *yell, int *full)
{
	unsigned long flags;
	int pos;
	int cnt;
	struct sipx_blk_item item;

#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	SKB_DATA_TO_SIPC_CACHE_FLUSH(sipx_dev, (blk->addr - sipxs[SIPC_ID_PSCP]->smem_cached_virt
				     + sipxs[SIPC_ID_PSCP]->smem_addr), blk->length);
#endif
	/* multi-gotter may cause got failure */
	spin_lock_irqsave(&ring->lock, flags);
	pos = sblock_get_ringpos(ring->fifo_info->fifo_wrptr,
				 ring->fifo_size);
	item.index = blk->index;
	item.desc = SIPX_GEN_DESC(blk->length, blk->offset);
	/* force compiler do one 4-bytes operation */
	/* ring->fifo_buf[pos] = item; */
	(*(u32 *)(&ring->fifo_buf[pos])) = (*(u32 *)&item);
	ring->fifo_info->fifo_wrptr++;

	cnt = (int)(ring->fifo_info->fifo_wrptr - ring->fifo_info->fifo_rdptr);
	if (cnt == 1)
		*yell = 1;
	else if (cnt >= ring->fifo_info->fifo_size)
		*full = 1;

	spin_unlock_irqrestore(&ring->lock, flags);

	return 0;
}

static int sipx_get_item_from_ring(struct sipx_ring *ring, struct sblock *blk)
{
	unsigned long flags;
	int pos;
	int ret = 0;
	u32 wr, rd;
	struct sipx_blk_item item;

	spin_lock_irqsave(&ring->lock, flags);

	wr = ring->fifo_info->fifo_wrptr;
	rd = ring->fifo_info->fifo_rdptr;

	if (wr == rd) {
		ret = -ENODATA;
	} else {
		pos = sblock_get_ringpos(rd, ring->fifo_size);
		item = ring->fifo_buf[pos];
		/* force compiler do one 4-bytes operation */
		/* item = ring->fifo_buf[pos]; */
		(*(u32 *)&item) = (*(u32 *)(&ring->fifo_buf[pos]));

		SIPX_PARSE_DESC(item.desc, &blk->length, &blk->offset);
		blk->index = item.index;
		blk->addr = ring->blks_buf +  (blk->index * ring->blk_size) +
			blk->offset;

		ring->fifo_info->fifo_rdptr = rd + 1;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
		SIPC_DATA_TO_SKB_CACHE_INV(sipx_dev, (blk->addr - sipxs[SIPC_ID_PSCP]->smem_cached_virt
					   + sipxs[SIPC_ID_PSCP]->smem_addr), blk->length);
#endif
	}

	spin_unlock_irqrestore(&ring->lock, flags);

	return ret;
}

static int sipx_get_ring_item_cnt(struct sipx_ring *ring)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&ring->lock, flags);
	ret = ring->fifo_info->fifo_wrptr - ring->fifo_info->fifo_rdptr;
	spin_unlock_irqrestore(&ring->lock, flags);

	return ret;
}

static int sipx_recover_channel(struct sipx_mgr *sipx,
				struct sipx_channel *sipx_chan)
{
	volatile struct sipx_fifo_info *fifo_info;
	unsigned long flags;
	int pos;
	struct sblock blk;

	sipx_chan->state = SBLOCK_STATE_IDLE;

	/* clean dl record blk */
	if (sipx_chan->dl_record_flag) {
		pr_info("dl_record_flag = 1\n");

		blk = sipx_chan->dl_record_blk;
		if (IS_DL_BLK_ACK(sipx, &blk))
			sipx_put_item_into_pool(sipx->dl_pool, &blk);
		else
			sipx_put_item_into_pool(sipx->dl_ack_pool, &blk);

		sipx_chan->dl_record_flag = 0;
	}

	/* clean dl ring blks */
	fifo_info = sipx_chan->dl_ring->fifo_info;

	spin_lock_irqsave(&sipx_chan->dl_ring->lock, flags);

	while (fifo_info->fifo_wrptr != fifo_info->fifo_rdptr) {
		pr_info("%s 0x%p, wrptr: 0x%x, rdptr:0x%x\n",
			  "dl_ring fifo_info:",
			  (void *)fifo_info,
			  (unsigned int)fifo_info->fifo_wrptr,
			  (unsigned int)fifo_info->fifo_rdptr);

		pos = sblock_get_ringpos(fifo_info->fifo_rdptr,
					 sipx_chan->dl_ring->fifo_size);
		blk.index = sipx_chan->dl_ring->fifo_buf[pos].index;

		fifo_info->fifo_rdptr++;

		sipx_put_item_into_pool(sipx->dl_pool, &blk);
	}
	spin_unlock_irqrestore(&sipx_chan->dl_ring->lock, flags);

	/* clean dl ack ring blks */
	fifo_info = sipx_chan->dl_ack_ring->fifo_info;

	spin_lock_irqsave(&sipx_chan->dl_ack_ring->lock, flags);

	while (fifo_info->fifo_wrptr != fifo_info->fifo_rdptr) {
		pr_info("%s0x%p, wrptr:0x%x, rdptr:0x%x\n",
			  "dl_ack_ring fifo_info:",
			  (void *)fifo_info,
			  (unsigned int)fifo_info->fifo_wrptr,
			  (unsigned int)fifo_info->fifo_rdptr);

		pos = sblock_get_ringpos(fifo_info->fifo_rdptr,
					 sipx_chan->dl_ack_ring->fifo_size);
		blk.index = sipx_chan->dl_ack_ring->fifo_buf[pos].index;

		fifo_info->fifo_rdptr++;

		sipx_put_item_into_pool(sipx->dl_ack_pool, &blk);
	}
	spin_unlock_irqrestore(&sipx_chan->dl_ack_ring->lock, flags);

	/* clean ul ring blks */
	fifo_info = sipx_chan->ul_ring->fifo_info;

	spin_lock_irqsave(&sipx_chan->ul_ring->lock, flags);
	while (fifo_info->fifo_wrptr != fifo_info->fifo_rdptr) {
		pr_info("%s0x%p, wrptr:0x%x, rdptr:0x%x\n",
			  "ul_ring fifo_info:",
			  (void *)fifo_info,
			  (unsigned int)fifo_info->fifo_wrptr,
			  (unsigned int)fifo_info->fifo_rdptr);

		pos = sblock_get_ringpos(fifo_info->fifo_rdptr,
					 sipx_chan->ul_ring->fifo_size);
		blk.index = sipx_chan->ul_ring->fifo_buf[pos].index;

		fifo_info->fifo_rdptr++;

		sipx_put_item_into_pool(sipx->ul_pool, &blk);
	}
	spin_unlock_irqrestore(&sipx_chan->ul_ring->lock, flags);

	/* clean ul ack ring blks */
	fifo_info = sipx_chan->ul_ack_ring->fifo_info;

	spin_lock_irqsave(&sipx_chan->ul_ack_ring->lock, flags);
	while (fifo_info->fifo_wrptr != fifo_info->fifo_rdptr) {
		pr_info("%s0x%p, wrptr:0x%x, rdptr:0x%x\n",
			  "ul_ack_ring fifo_info:",
			  (void *)fifo_info,
			  (unsigned int)fifo_info->fifo_wrptr,
			  (unsigned int)fifo_info->fifo_rdptr);

		pos = sblock_get_ringpos(fifo_info->fifo_rdptr,
					 sipx_chan->ul_ack_ring->fifo_size);
		blk.index = sipx_chan->ul_ack_ring->fifo_buf[pos].index;

		fifo_info->fifo_rdptr++;

		sipx_put_item_into_pool(sipx->ul_ack_pool, &blk);
	}
	spin_unlock_irqrestore(&sipx_chan->ul_ack_ring->lock, flags);

	/* clean ul record blk */
	if (sipx_chan->ul_record_flag) {
		pr_info("ul_record_flag = 1\n");

		blk = sipx_chan->ul_record_blk;
		if (IS_UL_BLK_ACK(sipx, &blk))
			sipx_put_item_into_pool(sipx->ul_ack_pool, &blk);
		else
			sipx_put_item_into_pool(sipx->ul_pool, &blk);

		sipx_chan->ul_record_flag = 0;
	}

	return 0;
}

/* recover all channels here */
static int sipx_recover(u8 dst)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int i;

	if (!sipx)
		return -ENODEV;

	for (i = 0; i < SMSG_VALID_CH_NR; i++) {
		sipx_chan = sipx->channels[i];
		if (sipx_chan && (sipx_chan->state == SBLOCK_STATE_READY))
			sipx_recover_channel(sipx, sipx_chan);
	}

	return 0;
}

static int sipx_thread(void *data)
{
	struct sipx_channel *sipx_chan = data;
	struct smsg mcmd, mrecv;
	int rval;

	/* since the channel open may hang, we call it in the seblock thread */
	rval = smsg_ch_open(sipx_chan->dst, sipx_chan->channel, -1);
	if (rval != 0) {
		pr_err("Failed to open channel %d\n", sipx_chan->channel);
		/* assign NULL to thread poniter as failed to open channel */
		sipx_chan->thread = NULL;
		return rval;
	}
	/* msleep(30 * 1000); */
	/* handle the seblock events */
	while (!kthread_should_stop()) {
		/* monitor seblock recv smsg */
		smsg_set(&mrecv, sipx_chan->channel, 0, 0, 0);
		rval = smsg_recv(sipx_chan->dst, &mrecv, -1);
		if (rval == -EIO || rval == -ENODEV) {
			/* channel state is FREE */
			usleep_range(5000, 10000);
			continue;
		}

		pr_debug("%s recv msg: dst=%d, channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
			   __func__,
			   sipx_chan->dst,
			   sipx_chan->channel,
			   mrecv.type,
			   mrecv.flag,
			   mrecv.value);

		switch (mrecv.type) {
		case SMSG_TYPE_OPEN:
			/* just ack open */
			smsg_open_ack(sipx_chan->dst, sipx_chan->channel);
			break;
		case SMSG_TYPE_CLOSE:
			/* handle channel close */
			smsg_close_ack(sipx_chan->dst, sipx_chan->channel);
			if (sipx_chan->handler)
				sipx_chan->handler(SBLOCK_NOTIFY_CLOSE,
						   sipx_chan->data);

			sipx_chan->state = SBLOCK_STATE_IDLE;
			break;
		case SMSG_TYPE_CMD:
			/* respond cmd done for seblock init */
			WARN_ON(mrecv.flag != SMSG_CMD_SBLOCK_INIT);

			/* handle channel recovery */
			if (sipx_chan->sipx->recovery && (mrecv.value == 0)) {
				pr_info("sipx_chan start recover! recv msg: dst=%d, channel=%d, type=%d, flag=0x%04x, value=0x%08x\n",
					  sipx_chan->dst,
					  sipx_chan->channel,
					  mrecv.type,
					  mrecv.flag,
					  mrecv.value);
				if (sipx_chan->handler)
					sipx_chan->handler(SBLOCK_NOTIFY_CLOSE,
							   sipx_chan->data);

				/*no channel of CP is opened yet*/
				sipx_recover(sipx_chan->dst);
			}

			/* give smem address to cp side */
			smsg_set(&mcmd, sipx_chan->channel,
				 SMSG_TYPE_DONE,
				 SMSG_DONE_SBLOCK_INIT,
				 sipx_chan->sipx->dst_smem_addr);
			smsg_send(sipx_chan->dst, &mcmd, -1);
			sipx_chan->state = SBLOCK_STATE_READY;
			sipx_chan->sipx->recovery = 1;
			if (sipx_chan->handler)
				sipx_chan->handler(SBLOCK_NOTIFY_OPEN,
						   sipx_chan->data);
			break;
		case SMSG_TYPE_EVENT:
			/* handle seblock send/release events */
			switch (mrecv.flag) {
			case SMSG_EVENT_SBLOCK_SEND:
				break;
			case SMSG_EVENT_SBLOCK_RELEASE:
#ifdef UL_TEST
				if (sipx_chan->channel == 7) {
					g_flow_on = 1;
					pr_info("channel 7 leave flow ctrl\n");
				}
#endif
				if (sipx_chan->handler)
					sipx_chan->handler(
							   SBLOCK_NOTIFY_GET,
							   sipx_chan->data);

				break;
			default:
				rval = 1;
				break;
			}
			break;
		default:
			rval = 1;
			break;
		};

		if (rval) {
			pr_info("%s: %d-%d, %d, %d, %d\n",
				  "non-handled sipx_chan msg",
				  sipx_chan->dst, sipx_chan->channel,
				  mrecv.type, mrecv.flag, mrecv.value);
			rval = 0;
		}
	}

	pr_err("sipx_chan %d-%d thread stop",
		 sipx_chan->dst, sipx_chan->channel);

	return rval;
}

static int create_sipx_pool_ctrl(struct sipx_pool **out,
				 volatile struct sipx_fifo_info *fifo_info,
				 void *fifo_virt,
				 void *blk_virt,
				 struct sipx_mgr *sipx)
{
	struct sipx_pool *pool = NULL;
	int i;

	pool = kzalloc(sizeof(*pool), GFP_KERNEL);
	if (!pool)
		return -ENOMEM;

	pool->fifo_info = fifo_info;
	pool->fifo_buf = fifo_virt;
	pool->fifo_size = fifo_info->fifo_size;
	pool->blks_buf = blk_virt;
	pool->blk_size = fifo_info->blk_size;
	pool->sipx = sipx;

	if (blk_virt) {
		for (i = 0; i < pool->fifo_size; i++) {
			pool->fifo_buf[i].index = i;
			pool->fifo_buf[i].desc = 0;
			pool->fifo_info->fifo_wrptr++;
		}
	}
	spin_lock_init(&pool->lock);

	*out = pool;
	return 0;
}

static int destroy_spix_mgr(struct sipx_mgr *sipx)
{
	kfree(sipx->dl_pool);
	kfree(sipx->dl_ack_pool);
	kfree(sipx->ul_pool);
	kfree(sipx->ul_ack_pool);

	if (sipx->smem_virt)
		shmem_ram_unmap(sipx->smem_virt);

#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	if (sipx->smem_cached_virt)
		shmem_ram_unmap(sipx->smem_cached_virt);
#endif

	if (sipx->smem_addr)
		smem_free(sipx->smem_addr, sipx->smem_size);

	kfree(sipx);

	return 0;
}

static int create_sipx_mgr(struct sipx_mgr **out, struct sipx_init_data *pdata)
{
	struct sipx_mgr *sipx = NULL;
	volatile struct sipx_fifo_info *fifo_info;
	volatile u32 *chan_cnt;
	u32 offset_chan_cnt;
	u32 offset_dl_blk;
	u32 offset_dl_fifo;
	u32 offset_dl_ack_blk;
	u32 offset_dl_ack_fifo;
	u32 offset_ul_blk;
	u32 offset_ul_fifo;
	u32 offset_ul_ack_blk;
	u32 offset_ul_ack_fifo;
	u32 total_size = 0;
	void *p = NULL;
	int ret = 0;

	/* donor: sipc = smsg_ipcs[pdata->dst]; if (!sipc) ... -ENODEV.
	 * Port tidak punya ipc->smem_base/dst_smem_base, jadi cek
	 * keberadaan sisi pdata lewat sipc_ap.sipc_dev (lihat helper
	 * sipx_ap_smem_base() di atas).
	 */
	if (!sipc_ap.sipc_dev[pdata->dst] ||
	    !sipc_ap.sipc_dev[pdata->dst]->pdata) {
		pr_err("sipc is null, dst = %d\n", pdata->dst);
		return -ENODEV;
	}

	sipx = kzalloc(sizeof(*sipx), GFP_KERNEL);
	if (!sipx)
		return -ENOMEM;

	memset(sipx, 0, sizeof(struct sipx_mgr));
	sipx->pdata = pdata;
	sipx->dst = pdata->dst;
	sipx->dl_pool_size = pdata->dl_pool_size;
	sipx->dl_ack_pool_size = pdata->dl_ack_pool_size;
	sipx->ul_pool_size = pdata->ul_pool_size;
	sipx->ul_ack_pool_size = pdata->ul_ack_pool_size;
	sipx->blk_size = ALIGN(NORMAL_BLOCK_SIZE + SIPX_BLOCK_PENDING,
			       SMP_CACHE_BYTES);
	sipx->ack_blk_size = ALIGN(ACK_BLOCK_SIZE + SIPX_BLOCK_PENDING,
				   SMP_CACHE_BYTES);

	/* allocated smem based on seblock_main_mgr structure , dl & ul mixed */

	/* dl pool info */
	total_size = sizeof(struct sipx_fifo_info);
	/* dl ack pool info */
	total_size += sizeof(struct sipx_fifo_info);
	/* ul pool info */
	total_size += sizeof(struct sipx_fifo_info);
	/* ul ack pool info */
	total_size += sizeof(struct sipx_fifo_info);
	offset_chan_cnt = total_size;

	/* channels smem ptr */
	total_size += sizeof(u32) * (SMSG_CH_NR + 1);
	offset_dl_blk = total_size;

	/* dl blk mem */
	total_size += sipx->dl_pool_size * sipx->blk_size;
	offset_dl_ack_blk = total_size;

	/* dl ack blk mem */
	total_size += sipx->dl_ack_pool_size * sipx->ack_blk_size;
	offset_ul_blk = total_size;

	/* ul blk mem */
	total_size += sipx->ul_pool_size * sipx->blk_size;
	offset_ul_ack_blk = total_size;

	/* ul ack blk mem */
	total_size += sipx->ul_ack_pool_size * sipx->ack_blk_size;
	offset_dl_fifo = total_size;

	/* dl pool fifo mem */
	total_size += sizeof(struct sipx_blk_item) * sipx->dl_pool_size;
	offset_dl_ack_fifo = total_size;

	/* dl ack pool fifo mem */
	total_size += sizeof(struct sipx_blk_item) * sipx->dl_ack_pool_size;
	offset_ul_fifo = total_size;

	/* ul pool fifo mem */
	total_size += sizeof(struct sipx_blk_item) * sipx->ul_pool_size;
	offset_ul_ack_fifo = total_size;

	/* ul ack pool fifo mem */
	total_size += sizeof(struct sipx_blk_item) * sipx->ul_ack_pool_size;

	sipx->smem_size = ALIGN(total_size, SBLOCK_ALIGN_BYTES);
	sipx->smem_addr = smem_alloc(sipx->smem_size);

	if (!sipx->smem_addr) {
		ret = -ENOMEM;
		goto fail;
	}
	sipx->dst_smem_addr = sipx->smem_addr -
		sipx_ap_smem_base(sipx->dst) + sipx_dst_smem_base(sipx->dst);
	sipx->smem_virt = shmem_ram_vmap_nocache(sipx->smem_addr,
						 sipx->smem_size);
	if (!sipx->smem_virt) {
		ret = -EFAULT;
		goto fail;
	}

#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	sipx->smem_cached_virt = shmem_ram_vmap_cache(sipx->smem_addr,
						      sipx->smem_size);

	if (!sipx->smem_cached_virt) {
		ret = -EFAULT;
		goto fail;
	}
#endif

	/* memset(sipx->smem_virt, 0, sipx->smem_size); */
	for (p = sipx->smem_virt; p < sipx->smem_virt + sipx->smem_size;) {
#ifdef CONFIG_64BIT
		*(uint64_t *)p = 0x0;
		p += sizeof(uint64_t);
#else
		*(u32 *)p = 0x0;
		p += sizeof(u32);
#endif
	}

	/* dl pool */
	fifo_info = (volatile struct sipx_fifo_info *)(sipx->smem_virt);
	/* offset of smem_addr */
	fifo_info->blks_addr = sipx->dst_smem_addr + offset_dl_blk;
	fifo_info->fifo_size = sipx->dl_pool_size;
	fifo_info->blk_size = sipx->blk_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	/* offset of smem_addr */
	fifo_info->fifo_addr = sipx->dst_smem_addr + offset_dl_fifo;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	ret = create_sipx_pool_ctrl(&sipx->dl_pool,
				    fifo_info,
				    sipx->smem_virt + offset_dl_fifo,
				    sipx->smem_cached_virt + offset_dl_blk,
				    sipx);
#else
	ret = create_sipx_pool_ctrl(&sipx->dl_pool,
				    fifo_info,
				    sipx->smem_virt + offset_dl_fifo,
				    sipx->smem_virt + offset_dl_blk,
				    sipx);
#endif

	if (ret)
		goto fail;

	/* dl ack pool */
	fifo_info = (volatile struct sipx_fifo_info *)((sipx->smem_virt) +
			sizeof(struct sipx_fifo_info));
	fifo_info->blks_addr = sipx->dst_smem_addr + offset_dl_ack_blk;
	fifo_info->fifo_size = sipx->dl_ack_pool_size;
	fifo_info->blk_size = sipx->ack_blk_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	fifo_info->fifo_addr = sipx->dst_smem_addr + offset_dl_ack_fifo;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	ret = create_sipx_pool_ctrl(&sipx->dl_ack_pool,
				    fifo_info,
				    sipx->smem_virt + offset_dl_ack_fifo,
				    sipx->smem_cached_virt + offset_dl_ack_blk,
				    sipx);
#else
	ret = create_sipx_pool_ctrl(&sipx->dl_ack_pool,
				    fifo_info,
				    sipx->smem_virt + offset_dl_ack_fifo,
				    sipx->smem_virt + offset_dl_ack_blk,
				    sipx);
#endif

	if (ret)
		goto fail;

	/* ul pool */
	fifo_info = (volatile struct sipx_fifo_info *)((sipx->smem_virt) +
			sizeof(struct sipx_fifo_info) * 2);
	/* offset of smem_addr */
	fifo_info->blks_addr = sipx->dst_smem_addr + offset_ul_blk;
	fifo_info->fifo_size = sipx->ul_pool_size;
	fifo_info->blk_size = sipx->blk_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;

	/* offset of smem_addr */
	fifo_info->fifo_addr = sipx->dst_smem_addr + offset_ul_fifo;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	ret = create_sipx_pool_ctrl(&sipx->ul_pool,
				    fifo_info,
				    sipx->smem_virt + offset_ul_fifo,
				    sipx->smem_cached_virt + offset_ul_blk,
				    sipx);
#else
	ret = create_sipx_pool_ctrl(&sipx->ul_pool,
				    fifo_info,
				    sipx->smem_virt + offset_ul_fifo,
				    sipx->smem_virt + offset_ul_blk,
				    sipx);
#endif

	if (ret)
		goto fail;

	/* ul ack pool */
	fifo_info = (volatile struct sipx_fifo_info *)((sipx->smem_virt) +
			sizeof(struct sipx_fifo_info) * 3);
	fifo_info->blks_addr = sipx->dst_smem_addr + offset_ul_ack_blk;
	fifo_info->fifo_size = sipx->ul_ack_pool_size;
	fifo_info->blk_size = sipx->ack_blk_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	fifo_info->fifo_addr = sipx->dst_smem_addr + offset_ul_ack_fifo;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	ret = create_sipx_pool_ctrl(&sipx->ul_ack_pool,
				    fifo_info,
				    sipx->smem_virt + offset_ul_ack_fifo,
				    sipx->smem_cached_virt + offset_ul_ack_blk,
				    sipx);
#else
	ret = create_sipx_pool_ctrl(&sipx->ul_ack_pool,
				    fifo_info,
				    sipx->smem_virt + offset_ul_ack_fifo,
				    sipx->smem_virt + offset_ul_ack_blk,
				    sipx);
#endif

	if (ret)
		goto fail;

	/* channel count */
	chan_cnt = (volatile u32 *)((sipx->smem_virt) +
			offset_chan_cnt);
	*chan_cnt = SMSG_CH_NR;
#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	/* save pool start pos */
	sipx->dl_ack_start = sipx->smem_cached_virt + offset_dl_ack_blk;
	sipx->ul_ack_start = sipx->smem_cached_virt + offset_ul_ack_blk;
#else
	/* save pool start pos */
	sipx->dl_ack_start = sipx->smem_virt + offset_dl_ack_blk;
	sipx->ul_ack_start = sipx->smem_virt + offset_ul_ack_blk;
#endif

	sipx->state = SIPX_STATE_READY;
	*out = sipx;

	return ret;
fail:
	if (sipx)
		destroy_spix_mgr(sipx);

	return ret;
}


static int sipx_parse_dt(struct sipx_init_data **init,
			 struct device_node *np, struct device *dev)
{
	struct device_node *parent_np;
	struct sipx_init_data *pdata = NULL;
	int ret;
	u32 data;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	/* Get name */
	ret = of_property_read_string(np, "label", &pdata->name);
	if (ret)
		goto error;
	dev_info(dev, "label  =%s\n", pdata->name);

	/* Get dst, sipx dst is share sipc dsti
	 * Get the parent node
	 */
	parent_np = of_get_parent(np);
	if (parent_np) {
		ret = of_property_read_u32(parent_np, "reg", (u32 *)&data);
		if (ret)
			goto error;
		pdata->dst = (u8)data;
		dev_info(dev, "dst    =%d\n", pdata->dst);
	}
	of_node_put(parent_np);

	ret = of_property_read_u32(np, "sprd,dl-pool",
				   (u32 *)&pdata->dl_pool_size);
	if (ret)
		goto error;
	dev_info(dev, "sprd,dl-pool =%d\n", pdata->dl_pool_size);

	ret = of_property_read_u32(np, "sprd,dl-ack-pool",
				   (u32 *)&pdata->dl_ack_pool_size);
	if (ret)
		goto error;

	ret = of_property_read_u32(np, "sprd,ul-pool",
				   (u32 *)&pdata->ul_pool_size);
	if (ret)
		goto error;

	ret = of_property_read_u32(np, "sprd,ul-ack-pool",
				   (u32 *)&pdata->ul_ack_pool_size);
	if (ret)
		goto error;

	*init = pdata;
	return 0;
error:
	devm_kfree(dev, pdata);
	*init = NULL;
	return ret;
}

static inline void sipx_destroy_pdata(struct sipx_init_data **init,
				      struct device *dev)
{
	struct sipx_init_data *pdata = *init;

	devm_kfree(dev, pdata);
	*init = NULL;
}

u32 sipx_get_ack_blk_len(u8 dst)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];

	return (sipx->ack_blk_size - SIPX_BLOCK_PENDING);
}
EXPORT_SYMBOL_GPL(sipx_get_ack_blk_len);

int sipx_get(u8 dst, u8 channel, struct sblock *blk, int is_ack)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int ret = 0;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	/* ack packet, get from ack pool first */
	if (is_ack) {
		ret = sipx_get_item_from_pool(sipx->ul_ack_pool, blk);
		if (ret) {
			pr_info("%d-%d ack pool is empty!\n",
				  dst, channel);
			/* ack packet, when  ack pool is empty,
			 * try normal pool
			 */
			ret = sipx_get_item_from_pool(sipx->ul_pool, blk);
		}
	} else {
		/* normal packet just normal pool */
		ret = sipx_get_item_from_pool(sipx->ul_pool, blk);
	}

	if (ret) {
		pr_info("%d-%d ack:%d is empty!\n",
			  dst, channel, is_ack);
		return -EAGAIN;
	}

	/* save to records */
	sipx_chan->ul_record_flag = 1;
	sipx_chan->ul_record_blk = *blk;

	/* do offset */
	blk->offset = SIPX_UL_OFFSET;
	blk->addr += SIPX_UL_OFFSET;
	blk->length -= SIPX_UL_OFFSET;

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_get);

/* put back */
int sipx_put(u8 dst, u8 channel, struct sblock *blk)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	if (IS_UL_BLK_ACK(sipx, blk))
		sipx_put_item_back_to_pool(sipx->ul_ack_pool, blk);
	else
		sipx_put_item_back_to_pool(sipx->ul_pool, blk);

	/* pop from records */
	sipx_chan->ul_record_flag = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_put);

static enum hrtimer_restart sipx_ul_trigger_timer_handler(struct hrtimer
*timer)
{
	struct sipx_channel *sipx_chan =
		container_of(timer,
			     struct sipx_channel,
			     ul_timer);
	struct smsg mevt;
	unsigned long flags;

	spin_lock_irqsave(&sipx_chan->lock, flags);

	sipx_chan->ul_timer_active = 0;
	smsg_set(&mevt, sipx_chan->channel,
		 SMSG_TYPE_EVENT,
		 SMSG_EVENT_SBLOCK_SEND,
		 0);
	smsg_send(sipx_chan->dst, &mevt, 0);

	spin_unlock_irqrestore(&sipx_chan->lock, flags);

	pr_debug("sipx trigger cp in timer func\n");

	return HRTIMER_NORESTART;
}

static void sipx_force_ul_trigger(struct sipx_channel *sipx_chan)
{
	struct smsg mevt;
	unsigned long flags;

	spin_lock_irqsave(&sipx_chan->lock, flags);

	if (sipx_chan->ul_timer_active) {
		hrtimer_try_to_cancel(&sipx_chan->ul_timer);
		sipx_chan->ul_timer_active = 0;
	}
	smsg_set(&mevt, sipx_chan->channel,
		 SMSG_TYPE_EVENT,
		 SMSG_EVENT_SBLOCK_SEND, 0);
	smsg_send(sipx_chan->dst, &mevt, 0);

	spin_unlock_irqrestore(&sipx_chan->lock, flags);

	pr_debug("sipx trigger cp force\n");
}

static void sipx_start_trigger_timer(struct sipx_channel *sipx_chan)
{
	sipx_chan->ul_timer_active = 1;
	hrtimer_start(&sipx_chan->ul_timer, sipx_chan->ul_timer_val,
		      HRTIMER_MODE_REL);
}

static void sipx_try_trigger_cp(struct sipx_channel *sipx_chan)
{
	unsigned long flags;

	spin_lock_irqsave(&sipx_chan->lock, flags);
	if (!sipx_chan->ul_timer_active)
		sipx_start_trigger_timer(sipx_chan);

	spin_unlock_irqrestore(&sipx_chan->lock, flags);
}

int sipx_flush(u8 dst, u8 channel)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int rval = 0;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	/* sipx_try_trigger_cp(sipx_chan); */

	return rval;
}
EXPORT_SYMBOL_GPL(sipx_flush);

int sipx_send(u8 dst, u8 channel, struct sblock *blk)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int full = 0;
	int yell = 0;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	pr_debug("dst=%d, channel=%d, addr=%p, len=%d\n",
		   dst, channel, blk->addr, blk->length);

	if (IS_UL_BLK_ACK(sipx, blk)) {
		sipx_put_item_to_ring(sipx_chan->ul_ack_ring, blk, &yell,
				      &full);
	} else {
		sipx_put_item_to_ring(sipx_chan->ul_ring, blk, &yell, &full);
	}

	/* pop from records */
	sipx_chan->ul_record_flag = 0;

	/* check should flush */
	if (full)
		sipx_force_ul_trigger(sipx_chan);
	else if (yell)
		sipx_try_trigger_cp(sipx_chan);

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_send);

int sipx_receive(u8 dst, u8 channel, struct sblock *blk)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int ret;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	/* check ack ring first */
	ret = sipx_get_item_from_ring(sipx_chan->dl_ack_ring, blk);
	if (ret)
		/* then normal ring */
		ret = sipx_get_item_from_ring(sipx_chan->dl_ring, blk);

	/* all ring empty */
	if (ret)
		return ret;

	/* save record */
	sipx_chan->dl_record_flag = 1;
	sipx_chan->dl_record_blk = *blk;

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_receive);

int sipx_release(u8 dst, u8 channel, struct sblock *blk)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	pr_debug("dst=%d, channel=%d, addr=%p, len=%d, offset=%d\n",
		   dst, channel,
		   blk->addr, blk->length, blk->offset);

	if (IS_DL_BLK_ACK(sipx, blk))
		sipx_put_item_into_pool(sipx->dl_ack_pool, blk);
	else
		sipx_put_item_into_pool(sipx->dl_pool, blk);

	/* pop record */
	sipx_chan->dl_record_flag = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(sipx_release);

int sipx_get_arrived_count(u8 dst, u8 channel)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int blk_count = 0;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	blk_count = sipx_get_ring_item_cnt(sipx_chan->dl_ring);
	blk_count += sipx_get_ring_item_cnt(sipx_chan->dl_ack_ring);

	return blk_count;
}
EXPORT_SYMBOL_GPL(sipx_get_arrived_count);

int sipx_get_free_count(u8 dst, u8 channel)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	int blk_count = 0;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d sipx_get not exist\n",
			 dst, channel);
		return -ENODEV;
	}

	if (sipx_chan->state != SBLOCK_STATE_READY) {
		pr_err("sipx-%d-%d not ready!\n",
			 dst, channel);
		return -EIO;
	}

	blk_count = sipx_get_pool_free_count(sipx->ul_pool);

	return blk_count;
}
EXPORT_SYMBOL_GPL(sipx_get_free_count);

static int create_sipx_ring_ctrl(struct sipx_ring **out,
				 volatile struct sipx_fifo_info *fifo_info,
				 void *fifo_virt,
				 void *blk_virt,
				 struct sipx_mgr *sipx)
{
	struct sipx_ring *ring = NULL;

	ring = kzalloc(sizeof(*ring), GFP_KERNEL);
	if (!ring)
		return -ENOMEM;

	ring->fifo_info = fifo_info;
	ring->fifo_buf = fifo_virt;
	ring->fifo_size = fifo_info->fifo_size;
	ring->blks_buf = blk_virt;
	ring->blk_size = fifo_info->blk_size;
	ring->sipx = sipx;

	spin_lock_init(&ring->lock);

	*out = ring;

	return 0;
}

static int init_sipx_channel_fifos(struct sipx_mgr *sipx,
				   struct sipx_channel *sipx_chan)
{
	volatile struct sipx_fifo_info *fifo_info;
	u32 offset;
	int ret = 0;

	/* dl ring fifo info */
	fifo_info = (volatile struct sipx_fifo_info *)sipx_chan->smem_virt;
	fifo_info->blks_addr = 0;
	fifo_info->blk_size = sipx->blk_size;
	fifo_info->fifo_size = sipx->dl_pool_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	/* all ring fifo info */
	offset = sizeof(struct sipx_fifo_info) * 4;
	fifo_info->fifo_addr = sipx_chan->dst_smem_addr + offset;
	ret = create_sipx_ring_ctrl(&sipx_chan->dl_ring, fifo_info,
				    sipx_chan->smem_virt + offset,
				    sipx->dl_pool->blks_buf,
				    sipx);
	if (ret)
		return ret;

	/* dl ack ring fifo info */
	fifo_info = (volatile struct sipx_fifo_info *)
			(sipx_chan->smem_virt +
			/* dl ring info */
			sizeof(struct sipx_fifo_info));
	fifo_info->blks_addr = 0;
	fifo_info->blk_size = sipx->ack_blk_size;
	fifo_info->fifo_size = sipx->dl_ack_pool_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	offset =
		/* dl ul ring pool info */
		(sizeof(struct sipx_fifo_info) * 4) +
		/* dl ring fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->dl_pool_size);
	fifo_info->fifo_addr = sipx_chan->dst_smem_addr + offset;
	ret = create_sipx_ring_ctrl(&sipx_chan->dl_ack_ring, fifo_info,
				    sipx_chan->smem_virt + offset,
				    sipx->dl_ack_pool->blks_buf,
				    sipx);
	if (ret)
		return ret;

	/* ul ring fifo info */
	fifo_info = (volatile struct sipx_fifo_info *)(sipx_chan->smem_virt +
			/* dl ring info */
			sizeof(struct sipx_fifo_info) +
			/* ul ring info */
			sizeof(struct sipx_fifo_info));
	fifo_info->blks_addr = 0;
	fifo_info->blk_size = sipx->blk_size;
	fifo_info->fifo_size = sipx->ul_pool_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	offset =
		/* dl ul ring pool info */
		(sizeof(struct sipx_fifo_info) * 4) +
		/* dl ring fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->dl_pool_size) +
		/* ul ring fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->dl_ack_pool_size);
	fifo_info->fifo_addr = sipx_chan->dst_smem_addr + offset;
	ret = create_sipx_ring_ctrl(&sipx_chan->ul_ring,
				    fifo_info,
				    sipx_chan->smem_virt + offset,
				    sipx->ul_pool->blks_buf,
				    sipx);
	if (ret)
		return ret;

	/* ul ack ring fifo info */
	fifo_info = (volatile struct sipx_fifo_info *)
			(sipx_chan->smem_virt +
			/* dl ring info */
			sizeof(struct sipx_fifo_info) +
			/* dl ack ring info */
			sizeof(struct sipx_fifo_info) +
			/* ul ring info */
			sizeof(struct sipx_fifo_info));
	fifo_info->blks_addr = 0;
	fifo_info->blk_size = sipx->ack_blk_size;
	fifo_info->fifo_size = sipx->ul_ack_pool_size;
	fifo_info->fifo_rdptr = 0;
	fifo_info->fifo_wrptr = 0;
	offset =
		/* dl ul ring pool info */
		(sizeof(struct sipx_fifo_info) * 4) +
		/* dl ring fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->dl_pool_size) +
		/* ul ring fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->dl_ack_pool_size) +
		/* dl pool fifo mem */
		(sizeof(struct sipx_blk_item) * sipx->ul_pool_size);

	fifo_info->fifo_addr = sipx_chan->dst_smem_addr + offset;
	ret = create_sipx_ring_ctrl(&sipx_chan->ul_ack_ring,
				    fifo_info,
				    sipx_chan->smem_virt + offset,
				    sipx->ul_ack_pool->blks_buf,
				    sipx);

	return ret;
}

static int destroy_sipx_channel_ctrl(struct sipx_channel *sipx_chan)
{
	kfree(sipx_chan->dl_ring);

	kfree(sipx_chan->dl_ack_ring);

	kfree(sipx_chan->ul_ring);

	kfree(sipx_chan->ul_ack_ring);

	if (sipx_chan->smem_virt)
		shmem_ram_unmap(sipx_chan->smem_virt);

	if (sipx_chan->smem_addr)
		smem_free(sipx_chan->smem_addr,
		sipx_chan->smem_size);

	kfree(sipx_chan);

	return 0;
}

static int update_sipx_channel_ptr(struct sipx_mgr *sipx,
				   struct sipx_channel *sipx_chan)
{
	u32 offset;
	volatile u32 *chan_ptr_mem_ptr;

	/* all pool info */
	offset = sizeof(struct sipx_fifo_info) * 4;
	offset += sizeof(u32);
	offset += sizeof(u32) * sipx_chan->channel;

	chan_ptr_mem_ptr = (volatile u32 *)(sipx->smem_virt + offset);
	*chan_ptr_mem_ptr = sipx_chan->dst_smem_addr;

	return 0;
}

static int create_sipx_channel_ctrl(struct sipx_mgr *sipx, u8 channel,
				    struct sipx_channel **out)
{
	struct sipx_channel *sipx_chan;
	int ret;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipc_ap.sipc_dev[sipx->dst] ||
	    !sipc_ap.sipc_dev[sipx->dst]->pdata) {
		pr_err("sipc is null, dst = %d\n", sipx->dst);
		return -ENODEV;
	}

	/* create channel ctrl */
	sipx_chan = kzalloc(sizeof(*sipx_chan), GFP_KERNEL);
	if (!sipx_chan)
		return -ENOMEM;

	memset(sipx_chan, 0, sizeof(struct sipx_channel));

	sipx_chan->state = SBLOCK_STATE_IDLE;
	sipx_chan->dst = sipx->dst;
	sipx_chan->channel = channel;
	sipx_chan->sipx = sipx;

    /* dl ul ring pool info */
	sipx_chan->smem_size = sizeof(struct sipx_fifo_info) * 4;

	/* dl ring fifo mem */
	sipx_chan->smem_size +=
		sizeof(struct sipx_blk_item) * sipx->dl_pool_size;

	/* dl ack ring fifo mem */
	sipx_chan->smem_size +=
		sizeof(struct sipx_blk_item) * sipx->dl_ack_pool_size;
	/* ul ring fifo mem */

	sipx_chan->smem_size +=
		sizeof(struct sipx_blk_item) * sipx->ul_pool_size;

	/* ul ack ring fifo mem */
	sipx_chan->smem_size +=
		sizeof(struct sipx_blk_item) * sipx->ul_ack_pool_size;

	pr_debug("sipx_chan->smem_size = %d\n", sipx_chan->smem_size);

	/*single channel in smem*/
	sipx_chan->smem_addr = smem_alloc(sipx_chan->smem_size);
	if (!sipx_chan->smem_addr) {
		kfree(sipx_chan);
		return -ENOMEM;
	}

	sipx_chan->dst_smem_addr = sipx_chan->smem_addr -
		sipx_ap_smem_base(sipx->dst) +
		sipx_dst_smem_base(sipx->dst);
	sipx_chan->smem_virt = shmem_ram_vmap_nocache(
				sipx_chan->smem_addr,
				sipx_chan->smem_size);

	if (!sipx_chan->smem_virt) {
		kfree(sipx_chan);
		return -EFAULT;
	}
	/* init ring/pool fifos in channel ctrl */
	ret = init_sipx_channel_fifos(sipx, sipx_chan);
	if (ret) {
		pr_err("Failed init_sipx_channel_fifos!\n");
		kfree(sipx_chan);
		return ret;
	}

	/* trigger cp interrupt timer init.
	 * 6.18: hrtimer_init(&t,clk,mode); t.function = fn;  ->
	 *        hrtimer_setup(&t, fn, clk, mode).
	 */
	hrtimer_setup(&sipx_chan->ul_timer, sipx_ul_trigger_timer_handler,
		      CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	spin_lock_init(&sipx_chan->lock);
	sipx_chan->ul_timer_active = 0;
	sipx_chan->ul_timer_val = ns_to_ktime(TIME_TRIGGER_CP_NS);

	/* update channel ptr in main share mem */
	update_sipx_channel_ptr(sipx, sipx_chan);
	sipx->channels[ch_index] = sipx_chan;

	*out = sipx_chan;

	return 0;
}

int sipx_chan_destroy(u8 dst, u8 channel)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx)
		return -ENODEV;

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan)
		return -ENODEV;

	hrtimer_cancel(&sipx_chan->ul_timer);

	sipx_recover_channel(sipx, sipx_chan);
	smsg_ch_close(dst, channel, -1);

	/* stop seblock thread if it's created successfully and still alive */
	if (!IS_ERR_OR_NULL(sipx_chan->thread))
		kthread_stop(sipx_chan->thread);

	destroy_sipx_channel_ctrl(sipx_chan);

	sipxs[dst] = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_chan_destroy);

int sipx_chan_register_notifier(u8 dst, u8 channel,
				void (*handler)(int event, void *data),
				void *data)
{
	struct sipx_mgr *sipx = (struct sipx_mgr *)sipxs[dst];
	struct sipx_channel *sipx_chan;
	u8 ch_index;

	ch_index = sipc_channel2index(channel);
	if (ch_index == INVALID_CHANEL_INDEX) {
		pr_err("channel %d invalid!\n", channel);
		return -EINVAL;
	}

	if (!sipx) {
		pr_err("sipx-%d-%d sipx is not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan = sipx->channels[ch_index];

	if (!sipx_chan) {
		pr_err("sipx-%d-%d se_chanl is not exist!\n",
			 dst, channel);
		return -ENODEV;
	}

	sipx_chan->handler = handler;
	sipx_chan->data = data;

	return 0;
}
EXPORT_SYMBOL_GPL(sipx_chan_register_notifier);

#ifdef UL_TEST
struct task_struct *s_test_thrd_hdl;

static int send_test_pkt(int i)
{
	int ret;
	struct sblock blk;
	int is_ack = (i & 1);

	ret = sipx_get(5, 7, &blk, is_ack);
	if (ret) {
		g_flow_on = 0;
		pr_info("channel 7 enter flow ctrl\n");
		return -1;
	}
	blk.length = is_ack ? 40 : 1400;
	sipx_send(5, 7, &blk);
	return 0;
}

static int sipx_ul_test_thread(void *data)
{
	msleep(30000);
	/* handle the sblock events */
	while (!kthread_should_stop()) {
		int i, j;

		if (g_flow_on) {
			for (j = 0; j < 40; j++) {
				for (i = 0; i < 5; i++) {
					if (g_flow_on)
						send_test_pkt(i);
				}
				sipx_flush(5, 7);
			}

			pr_err("sent %d pkts\n", 40 * 5);
			usleep_range(1000, 1500);
			pr_err("sent start\n");
		} else {
			msleep(100);
		}
	}
	return 0;
}
#endif

int sipx_chan_create(u8 dst, u8 channel)
{
	struct sipx_mgr *sipx = NULL;
	struct sipx_channel *sipx_chan = NULL;
	int ret = 0;
	struct sched_param param = {.sched_priority = 88};

	pr_info("%s\n", __func__);

	if (dst >= SIPC_ID_NR) {
		pr_err("Input Param Error: dst = %d\n", dst);
		return -EINVAL;
	}
	/* check and create main ctrl */
	sipx = sipxs[dst];

	if (!sipx) {
		pr_err("sipx == NULL: sipx-%d-%d\n", dst, channel);
		return -EPROBE_DEFER;
	}

	ret = create_sipx_channel_ctrl(sipx, channel, &sipx_chan);

	if (ret) {
		pr_err("Failed to create channel, ret = %d", ret);
		goto fail;
	}

	/* create channel thread for this seblock channel */
	sipx_chan->thread = kthread_create(sipx_thread, sipx_chan,
					   "sipx-%d-%d", dst, channel);
	if (IS_ERR(sipx_chan->thread)) {
		pr_err("Failed to create kthread: sipx-%d-%d\n",
			 dst, channel);
		ret = PTR_ERR(sipx_chan->thread);
		goto fail;
	}
	sipx_chan_record[channel] = sipx_chan;

	/* set the thread as a real time thread, and its priority is 11 */
	sched_setscheduler(sipx_chan->thread, SCHED_RR, &param);
	wake_up_process(sipx_chan->thread);
#ifdef UL_TEST
	if (5 == dst && 7 == channel) {
		s_test_thrd_hdl = kthread_create(sipx_ul_test_thread, sipx,
						 "sipx-test");
		wake_up_process(s_test_thrd_hdl);
	}
#endif
	return 0;
fail:
	if (sipx_chan)
		destroy_sipx_channel_ctrl(sipx_chan);

	return ret;
}
EXPORT_SYMBOL_GPL(sipx_chan_create);

static int sipx_probe(struct platform_device *pdev)
{
	struct sipx_init_data *pdata = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np;
	struct sipx_mgr *sipx = NULL;
	int ret;
#if defined(CONFIG_DEBUG_FS)
	struct dentry *root = debugfs_create_dir("sipx", NULL);
#endif

	np = pdev->dev.of_node;
	ret = sipx_parse_dt(&pdata, np, &pdev->dev);
	if (ret) {
		pr_err("failed to parse sipx device tree, ret=%d\n", ret);
		return ret;
	}

	dev_info(dev, "parse dt, name=%s, dst=%u, pool size=%d, %d, %d, %d\n",
		  pdata->name, pdata->dst,
		  pdata->dl_pool_size,
		  pdata->dl_ack_pool_size,
		  pdata->ul_pool_size,
		  pdata->ul_ack_pool_size);

	sipx = sipxs[pdata->dst];

	if (!sipx) {
		ret = create_sipx_mgr(&sipx, pdata);
		if (ret) {
			dev_err(dev, "failed to create_sipx_mgr, ret=%d\n", ret);
			sipx_destroy_pdata(&pdata, &pdev->dev);
			return ret;
		}

		sipxs[pdata->dst] = sipx;
	}

	platform_set_drvdata(pdev, sipx);

#ifdef CONFIG_SPRD_SIPC_MEM_CACHE_EN
	sipx_dev = dev;
#endif

#if defined(CONFIG_DEBUG_FS)
	if (!root)
		return -ENXIO;

	sipx_init_debugfs(root);
#endif

	return 0;
}

/*
 * Cleanup sipx device driver.
 *
 * 6.18: struct platform_driver.remove bertipe void (sebelumnya int).
 */
static void sipx_remove(struct platform_device *pdev)
{
	struct sipx_mgr *sipx = platform_get_drvdata(pdev);
	struct sipx_init_data *pdata;

	if (sipx) {
		pdata = sipx->pdata;

		destroy_spix_mgr(sipx);
		sipx_destroy_pdata(&pdata, &pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
}

static const struct of_device_id sipx_match_table[] = {
	{ .compatible = "sprd,sipx", },
	{ },
};

static struct platform_driver sprd_ipx_driver = {
	.probe = sipx_probe,
	.remove = sipx_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sprd-sipx",
		.of_match_table = sipx_match_table,
	}
};

module_platform_driver(sprd_ipx_driver);

MODULE_AUTHOR("Wu Zhiwei");
MODULE_DESCRIPTION("SIPC/SIPX driver");
MODULE_LICENSE("GPL v2");
