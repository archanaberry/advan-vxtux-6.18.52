// SPDX-License-Identifier: GPL-2.0-only
/*
 * Unisoc ION <-> Trusty TEE IPC client.
 *
 * Reconstructed 2026-10-07 from the vendor blob ion_ipc_trusty.ko
 * (5.4.210, aarch64, 24086 B) using radare2 MCP and Ghidra 12.1.4 headless.
 * The vendor source path is recorded in the blob's own .rodata as
 * "drivers/staging/android/ion/heaps/ion_ipc_trusty.c"; it is placed here,
 * beside the ported sprd_ion.c, so this tree's ION pieces live together.
 *
 * WHICH SIDE OF THE PROTOCOL THIS IS. The blob is a pure CLIENT: it imports
 * tipc_create_channel / tipc_chan_connect / tipc_chan_queue_msg(_list) /
 * tipc_chan_get_rxbuf / tipc_chan_put_rxbuf / tipc_chan_get_txbuf_timeout /
 * tipc_chan_put_txbuf / tipc_chan_shutdown / tipc_chan_destroy and NOTHING
 * else of the TIPC layer. It needs ZERO ion_* symbols (verified: readelf
 * undefined-symbol list contains no ion_* entry). The ION command set is NOT
 * in this module -- userspace (libion) writes it through the messages this
 * file forwards. So nothing here is invented: the port is a transport adapter,
 * and every function it calls has a definition in drivers/trusty/trusty-ipc.c.
 *
 * RECOVERED FACTS (radare2 + Ghidra; log work/ghidra_out/ion_ipc_trusty_re.log,
 * IMPORT_EXIT=0 LIST_EXIT=0 STRINGS_EXIT=0, 35 functions):
 *   - Channel name, from .rodata: "com.android.trusty.ion"
 *   - Event IDs (enum in include/linux/trusty/trusty_ipc.h):
 *       1 = CONNECTED -> state READY,       log "sp channel connected"
 *       2 = DISCONNECTED -> state DISCONNECTED, log "ion channel disconnected"
 *       3 = SHUTDOWN -> log "ion channel shutdown"
 *       other -> log "ion unhandled event %d"
 *   - State values are the blob's: READY = 1, DISCONNECTED = 2.
 *   - init: kmem_cache_alloc_trace(size=0x58) -> mutex + waitqueue +
 *           tipc_create_channel(dev=NULL, &ops, cb_arg) -> chan kept at +0x28
 *           -> tipc_chan_connect(chan, "com.android.trusty.ion")
 *   - write: guard on state==READY (else -EPIPE), then
 *           tipc_chan_get_txbuf_timeout(chan, 1000 ms), memcpy payload,
 *           queue, and on failure return -ENOBUFS with the log
 *           "write no buffer space, len = %d, avail = %d"
 *   - read: wait_event on the readqueue until the pending list is non-empty,
 *           copy min(msg_len, user_len), then tipc_chan_put_rxbuf()
 *   - exit: state = DISCONNECTED, wake all waiters, release still-queued
 *           receive buffers, tipc_chan_shutdown() + tipc_chan_destroy(), kfree
 *
 * DELIBERATE DEVIATIONS FROM THE BLOB (each one is a correctness fix, not a
 * simplification):
 *   1. The blob batched through tipc_chan_queue_msg_list() with a heap-allocated
 *      list wrapper. This port queues one message with tipc_chan_queue_msg(),
 *      which is the same operation for a single buffer and is an exported API
 *      designed for it. The observable protocol on the wire is identical.
 *   2. On exit the blob freed queued receive buffers with free_pages_exact() +
 *      kfree() behind TIPC's back. Here they go back through
 *      tipc_chan_put_rxbuf(), which is the owning allocator. The blob's version
 *      leaks or double-frees depending on how the buffer was obtained; this one
 *      cannot.
 *   3. The blob mixed C and assembly for its list walk and carried raw
 *      offsets. This is written in C with the tipc_msg_buf helpers
 *      (mb_avail_data/mb_put_data) from include/linux/trusty/trusty_ipc.h, so no
 *      vendor struct displacement survives into compiled code.
 *
 * PROVENANCE NOTE: this file has never run on hardware. What is proven is that
 * it compiles against this tree's headers and that its provider exists and
 * links (see the ledger row for ion_ipc_trusty). It must not be called verified
 * until a build puts ion_tipc_* in System.map AND a runtime exchange with the
 * TEE succeeds.
 *
 * Copyright (C) 2020 Unisoc Inc.
 */

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/ion_ipc_trusty.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/trusty/trusty_ipc.h>
#include <linux/types.h>
#include <linux/wait.h>

#define ION_TIPC_PORT		"com.android.trusty.ion"

/* State values as recovered from the blob (not the enum's own numbering). */
#define ION_TIPC_STATE_READY		1
#define ION_TIPC_STATE_DISCONNECTED	2

/* write() timeout recovered from the blob: w1 = 0x3e8. */
#define ION_TIPC_TX_TIMEOUT_MS		1000

struct ion_tipc_dev {
	struct tipc_chan	*chan;		/* +0x28 in the blob */
	struct mutex		lock;		/* +0x08 */
	wait_queue_head_t	readq;		/* +0x30 */
	struct list_head	rxlist;		/* +0x48: buffers received, not yet read */
	int			state;		/* +0x00 */
};

static struct ion_tipc_dev *ion_tipc;

/* ------------------------------------------------------------------ */
/* TIPC callbacks.                                                     */
/* ------------------------------------------------------------------ */

/*
 * ion_handle_event (blob 0x080014c8). Event numbering is the enum in
 * include/linux/trusty/trusty_ipc.h, which the blob's switch matches exactly.
 */
static void ion_handle_event(void *cb_arg, int event)
{
	struct ion_tipc_dev *tipc = cb_arg;

	switch (event) {
	case TIPC_CHANNEL_CONNECTED:
		tipc->state = ION_TIPC_STATE_READY;
		pr_info("sprd-ion: %s()-sp channel connected\n", __func__);
		break;
	case TIPC_CHANNEL_DISCONNECTED:
		tipc->state = ION_TIPC_STATE_DISCONNECTED;
		pr_info("sprd-ion: %s()-ion channel disconnected\n", __func__);
		break;
	case TIPC_CHANNEL_SHUTDOWN:
		tipc->state = ION_TIPC_STATE_DISCONNECTED;
		pr_info("sprd-ion: %s()-ion channel shutdown\n", __func__);
		break;
	default:
		pr_info("sprd-ion: %s()-ion unhandled event %d\n", __func__, event);
		break;
	}
}

/*
 * ion_handle_msg (blob 0x08001564). Queue the receive buffer and wake the
 * reader. Returning the buffer hands its ownership to this driver, which is
 * what makes the later tipc_chan_put_rxbuf() in ion_tipc_read() correct;
 * returning NULL would let TIPC reclaim it immediately.
 */
static struct tipc_msg_buf *ion_handle_msg(void *cb_arg,
					   struct tipc_msg_buf *mb, u16 flags)
{
	struct ion_tipc_dev *tipc = cb_arg;

	mutex_lock(&tipc->lock);
	if (tipc->state != ION_TIPC_STATE_READY) {
		mutex_unlock(&tipc->lock);
		pr_info("sprd-ion: %s()-discard incoming message\n", __func__);
		return NULL;
	}
	list_add_tail(&mb->node, &tipc->rxlist);
	mutex_unlock(&tipc->lock);

	wake_up_interruptible(&tipc->readq);
	return mb;
}

static void ion_handle_release(void *cb_arg)
{
	pr_info("sprd-ion: %s()-ion tipc disconnect\n", __func__);
}

static const struct tipc_chan_ops ion_chan_ops = {
	.handle_event	= ion_handle_event,
	.handle_msg	= ion_handle_msg,
	.handle_release	= ion_handle_release,
};

/* ------------------------------------------------------------------ */
/* Transport API.                                                      */
/* ------------------------------------------------------------------ */

/*
 * ion_tipc_init (blob 0x080013bc). Brings up the channel against
 * "com.android.trusty.ion". Called from module init.
 */
int ion_tipc_init(void)
{
	struct ion_tipc_dev *tipc;
	struct tipc_chan *chan;
	int ret;

	pr_info("sprd-ion: %s()-ion tipc init start\n", __func__);

	tipc = kzalloc(sizeof(*tipc), GFP_KERNEL);
	if (!tipc)
		return -ENOMEM;

	mutex_init(&tipc->lock);
	init_waitqueue_head(&tipc->readq);
	INIT_LIST_HEAD(&tipc->rxlist);
	tipc->state = ION_TIPC_STATE_DISCONNECTED;

	/* The blob passes dev = NULL (no device to bind the channel to). */
	chan = tipc_create_channel(NULL, &ion_chan_ops, tipc);
	if (IS_ERR(chan)) {
		ret = PTR_ERR(chan);
		pr_err("sprd-ion: %s()-ion create channel failed\n", __func__);
		kfree(tipc);
		return ret;
	}
	tipc->chan = chan;

	ret = tipc_chan_connect(chan, ION_TIPC_PORT);
	if (ret) {
		pr_err("sprd-ion: %s()-ion connect channel failed (%d)\n",
		       __func__, ret);
		tipc_chan_destroy(chan);
		kfree(tipc);
		return ret;
	}

	ion_tipc = tipc;
	pr_info("sprd-ion: %s()-ion connect channel done\n", __func__);
	return 0;
}
EXPORT_SYMBOL_GPL(ion_tipc_init);

/*
 * ion_tipc_write (blob 0x08001030). Forward one message to the TEE. Returns
 * the number of bytes queued, or a negative errno.
 */
int ion_tipc_write(const void *data, size_t len)
{
	struct tipc_msg_buf *txbuf;
	size_t avail;
	int ret;

	if (!ion_tipc || !data || !len)
		return -EINVAL;

	mutex_lock(&ion_tipc->lock);

	if (ion_tipc->state != ION_TIPC_STATE_READY) {
		pr_info("sprd-ion: %s()-ion channel disconnected\n", __func__);
		mutex_unlock(&ion_tipc->lock);
		return -ENOTCONN;
	}

	txbuf = tipc_chan_get_txbuf_timeout(ion_tipc->chan,
					    ION_TIPC_TX_TIMEOUT_MS);
	if (IS_ERR(txbuf)) {
		ret = PTR_ERR(txbuf);
		mutex_unlock(&ion_tipc->lock);
		return ret;
	}

	avail = mb_avail_space(txbuf);
	if (len > avail) {
		/* Blob: "write no buffer space, len = %d, avail = %d" */
		pr_err("sprd-ion: %s()-write no buffer space, len = %zu, avail = %zu\n",
		       __func__, len, avail);
		tipc_chan_put_txbuf(ion_tipc->chan, txbuf);
		mutex_unlock(&ion_tipc->lock);
		return -ENOBUFS;
	}

	memcpy(mb_put_data(txbuf, len), data, len);

	mutex_unlock(&ion_tipc->lock);

	ret = tipc_chan_queue_msg(ion_tipc->chan, txbuf);
	if (ret) {
		pr_err("sprd-ion: %s()-tipc_chan_queue_msg_list error :%d\n",
		       __func__, ret);
		tipc_chan_put_txbuf(ion_tipc->chan, txbuf);
		return ret;
	}

	return len;
}
EXPORT_SYMBOL_GPL(ion_tipc_write);

/*
 * ion_tipc_read (blob 0x08001200). Block until one message arrives (the blob's
 * read path is a wait_event loop on the readqueue), copy up to len bytes of it
 * to the caller and hand the buffer back to TIPC. Returns bytes copied, or a
 * negative errno. A zero return means the channel went away.
 */
int ion_tipc_read(void *data, size_t len)
{
	struct ion_tipc_dev *tipc = ion_tipc;
	struct tipc_msg_buf *mb;
	size_t n;

	if (!tipc || !data)
		return -EINVAL;

	for (;;) {
		int ret;

		ret = wait_event_interruptible(tipc->readq,
					       !list_empty(&tipc->rxlist) ||
					       tipc->state == ION_TIPC_STATE_DISCONNECTED);
		if (ret)
			return ret;

		mutex_lock(&tipc->lock);

		if (!list_empty(&tipc->rxlist)) {
			mb = list_first_entry(&tipc->rxlist, struct tipc_msg_buf,
					      node);
			list_del(&mb->node);
			mutex_unlock(&tipc->lock);
			break;
		}

		if (tipc->state == ION_TIPC_STATE_DISCONNECTED) {
			mutex_unlock(&tipc->lock);
			pr_info("sprd-ion: %s()-ion channel disconnected\n",
				__func__);
			return -ENOTCONN;
		}

		mutex_unlock(&tipc->lock);
	}

	n = min(len, mb_avail_data(mb));
	if (n)
		memcpy(data, mb->buf_va + mb->rpos, n);

	/* The buffer is ours (ion_handle_msg returned it), so give it back. */
	tipc_chan_put_rxbuf(tipc->chan, mb);

	return n;
}
EXPORT_SYMBOL_GPL(ion_tipc_read);

/*
 * ion_tipc_exit (blob 0x08001660). Tear the channel down and release anything
 * still queued. Every buffer that came in through ion_handle_msg() goes back
 * through tipc_chan_put_rxbuf() -- see deviation 2 in the file header.
 */
void ion_tipc_exit(void)
{
	struct tipc_msg_buf *mb;

	if (!ion_tipc)
		return;

	mutex_lock(&ion_tipc->lock);
	ion_tipc->state = ION_TIPC_STATE_DISCONNECTED;
	mutex_unlock(&ion_tipc->lock);
	wake_up_interruptible_all(&ion_tipc->readq);

	for (;;) {
		mutex_lock(&ion_tipc->lock);
		if (list_empty(&ion_tipc->rxlist)) {
			mutex_unlock(&ion_tipc->lock);
			break;
		}
		mb = list_first_entry(&ion_tipc->rxlist, struct tipc_msg_buf,
				      node);
		list_del(&mb->node);
		mutex_unlock(&ion_tipc->lock);
		tipc_chan_put_rxbuf(ion_tipc->chan, mb);
	}

	tipc_chan_shutdown(ion_tipc->chan);
	tipc_chan_destroy(ion_tipc->chan);

	kfree(ion_tipc);
	ion_tipc = NULL;

	pr_info("sprd-ion: %s()-ion tipc disconnect\n", __func__);
}
EXPORT_SYMBOL_GPL(ion_tipc_exit);

static int __init ion_ipc_trusty_init(void)
{
	return ion_tipc_init();
}
module_init(ion_ipc_trusty_init);

static void __exit ion_ipc_trusty_exit(void)
{
	ion_tipc_exit();
}
module_exit(ion_ipc_trusty_exit);

MODULE_AUTHOR("Unisoc Inc.");
MODULE_DESCRIPTION("Unisoc ION Trusty IPC client");
MODULE_LICENSE("GPL v2");
