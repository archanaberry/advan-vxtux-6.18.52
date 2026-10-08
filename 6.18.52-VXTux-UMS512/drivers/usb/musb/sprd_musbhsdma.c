// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_musbhsdma.c — DMA engine vendor Unisoc untuk MUSB sharkl5pro,
 * port ke mainline 6.18.
 *
 * Sumber: GPL fork Samsung Tab A8 (T618/UMS512)
 * drivers/usb/musb/sprd_musbhsdma.c (Spreadtrum 2018, 1503 baris) +
 * sprd_musbhsdma.h. Register memakai blok vendor @ mregs+0x1000, 30
 * channel (1..15 TX, 16..30 RX = epnum+15), bukan DMA Inventra
 * (USB_INVENTRA_DMA hanya untuk omap2plus/mediatek/jz4740/polarfire).
 *
 * Pemetaan port (fork → 6.18), semua diverifikasi terhadap pohon:
 *   - musb->xceiv->otg->state          : ada utuh (musb_core.h, usb/otg.h)
 *   - musb->g.state (fork)             : musb->g.state (usb_gadget g
 *                                        embedded di struct musb) —
 *                                        musb_gadget.c:1716 dsb.
 *   - struct musb_request/usb_request
 *     num_mapped_sgs, short_not_ok, zero, dma, actual : ada utuh
 *     (include/linux/usb/gadget.h:43-116)
 *   - musb_ep->hw_ep, ep_in/ep_out, req_list, packet_sz, dma : ada utuh
 *   - musb_qh {segsize, iso_idx}, next_urb(), next_request(): ada utuh
 *   - musb_g_giveback: ada utuh (musb_gadget.h, non-static); musb_ep_select:
 *     ada utuh (static inline di header core)
 *   - musb_advance_schedule: **BUKAN API** — `static` di musb_host.c:297
 *     (fork memanggilnya karena ia menambal berkas itu). Klaim lama "ada
 *     utuh" di baris ini SALAH dan terbukti sebagai build blocker nyata,
 *     lihat DEV-H2 di bawah.
 *   - musb_rx_dma_sprd/musb_tx_dma_program (fork musb_host.c:706/794):
 *     TIDAK ADA di 6.18 (fork menambahkannya via patch musb_host.c)
 *     → path isoc host (sprd_musb_urb_completion) memakai
 *     musb_host_dma_sprd_prepare() pengganti, DEV-H1.
 *
 * DEVIASI tercatat (semua karena pohon, terukur):
 *   DEV-G1  isoc gadget TX: prepare() menolak (return -EINVAL) bila
 *           node terakhir tidak bulk-aligned — isoc TIDAK bulk-aligned.
 *           Fork menerima dengan sp=1/short-packet flag. Konsekuensi:
 *           isoc gadget TX fallback ke PIO core. (uji: bulk/int dulu)
 *   DEV-H1  isoc host per-packet ulang-program (fork memakai
 *           musb_rx_dma_sprd/musb_tx_dma_program — helper patch
 *           musb_host.c fork yang tidak ada di 6.18) tidak diport;
 *           URB isoc host diselesaikan seketika bila terjadi.
 *           B50 (2026-09-27, fase 2b): helper itu kini ADA (port fork
 *           ke musb_host.c 6.18, non-static + prototipe di musb_host.h)
 *           — ulang-program per paket isoc host diaktifkan di ekor
 *           sprd_musb_urb_completion.
 *   DEV-C1  CONFIG_USB_SPRD_LINKFIFO (queue node per-EP, alloc di
 *           patch musb_gadget.c fork:1939-1959) tidak diport: butuh
 *           patch core struct musb_ep + alloc/free per-EP. Jalur
 *           non-linkfifo terbukti cukup di fork (else-branch penuh).
 *   DEV-H2  completion host: fork memanggil musb_advance_schedule() — di 6.18
 *           fungsi itu STATIC (musb_host.c:297) sehingga panggilan itu membuat
 *           berkas ini GAGAL DIKOMPILASI (implicit declaration; kernel memakai
 *           -Werror=implicit-function-declaration). Ditemukan 2026-09-26 (B43)
 *           oleh tools/re/smoke/smoke_b40.bat SETELAH cacat kuotasi cmd-nya
 *           diperbaiki — sebelumnya 13 error kuotasi menyamarkan error ini.
 *           B50 (2026-09-27, fase 2b): opsi (a) dikerjakan — musb_host.c 6.18
 *           dipatch non-static (musb_advance_schedule, musb_tx_dma_program)
 *           + musb_rx_dma_sprd diport; prototipe di musb_host.h (pola extern
 *           fork musb_host.h:98/101/149). WARN_ONCE penanda fase 1 dihapus.
 *   DEV-C2  host-TX "RNDIS mode" (fork musb_host.c:837, situs
 *           musb_dma_sprd): B50 (2026-09-27, fase 2b) DIBUKA — fixup
 *           TXCSR dipindah verbatim ke musb_tx_dma_program() 6.18, cabang
 *           RX vendor (musb_rx_dma_sprd) terpasang di musb_ep_program.
 *           Gadget bulk/int tetap DMA; host bulk/int kini juga memakai
 *           DMA vendor.
 *   DEV-C3  fork mengalokasikan buffer linklist koheren per-EP di
 *           PATCH CORE (fork musb_gadget.c:1952-1959, alloc/free
 *           create/destroy EP); 6.18 tidak punya patch itu, jadi
 *           port mengalokasikan 30 buffer (channel 1..30) di
 *           sprd_musb_dma_controller_create() (konteks probe, boleh
 *           tidur) dan membebaskannya di destroy — pola memori sama
 *           (±1 MiB saat DMA aktif), tanpa menyentuh struct core.
 *
 * Fase 2b (terbuka, jujur): patch musb_host.c + musb_gadget.c 6.18
 * untuk jalur host penuh + LINKFIFO. Tahap ini = DMA gadget non-isoc
 * (f_mass_storage, f_rndis bulk) + IRQ DMA + completion gadget + abort.
 */
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "musb_core.h"
#include "musb_host.h"
#include "musb_gadget.h"   /* musb_g_giveback, next_request (fork: musb_core.h chain) */
#include "sprd_musbhsdma.h"

static void sprd_dma_channel_release(struct dma_channel *channel);

static void sprd_dma_controller_stop(struct sprd_musb_dma_controller
				*controller)
{
	struct musb *musb = controller->private_data;
	struct dma_channel *channel;
	u32 bit;

	if (controller->used_channels != 0) {
		dev_info(musb->controller,
			"Stopping DMA controller while channel active\n");

		for (bit = 0; bit < MUSB_DMA_CHANNELS; bit++) {
			if (controller->used_channels & (1 << bit)) {
				channel = &controller->channel[bit].channel;
				sprd_dma_channel_release(channel);

				if (!controller->used_channels)
					break;
			}
		}
	}
}

/*alloc dma channel number*/

static void sprd_dma_channel_release(struct dma_channel *channel)
{
	struct sprd_musb_dma_channel *musb_channel;
	struct sprd_musb_dma_controller *controller;

	if (!channel)
		return;
	musb_channel = channel->private_data;
	controller = musb_channel->controller;

	channel->actual_len = 0;

	musb_channel->controller->used_channels &=
		~(1 << musb_channel->channel_num);

	if (controller->used_channels == 0)
		wake_up(&controller->wait);

	channel->status = MUSB_DMA_STATUS_UNKNOWN;
}

static struct dma_channel *sprd_dma_channel_allocate(struct dma_controller *c,
				struct musb_hw_ep *hw_ep, u8 transmit)
{
	struct sprd_musb_dma_controller *controller = container_of(c,
			struct sprd_musb_dma_controller, controller);
	struct sprd_musb_dma_channel *musb_channel = NULL;
	struct dma_channel *channel = NULL;
	u8 bit;
	u16 csr;
	struct musb *musb;

	bit = hw_ep->epnum;
	musb = controller->private_data;

	if (transmit) {
		musb_channel = &(controller->channel[bit]);
		controller->used_channels |= (1 << bit);
		musb_channel->channel_num = bit;
		if (musb->xceiv->otg->state != OTG_STATE_A_HOST) {
			/* CONFIG DMA MODE */
			csr = musb_readw(hw_ep->regs, MUSB_TXCSR);
			csr |= MUSB_TXCSR_DMAMODE | MUSB_TXCSR_DMAENAB |
				MUSB_TXCSR_AUTOSET;
			musb_writew(hw_ep->regs, MUSB_TXCSR, csr);
		}
		/* dma_linklist/list_dma_addr SUDAH dialokasikan di
		 * sprd_musb_dma_controller_create() (DEV-C3) — tidak diubah di
		 * sini agar buffer koheren persisten antar alloc/release. */
	} else {
		musb_channel = &(controller->channel[bit + 15]);
		controller->used_channels |= (1 << (bit + 15));
		musb_channel->channel_num = bit + 15;
		if (musb->xceiv->otg->state != OTG_STATE_A_HOST) {
			/* CONFIG DMA MODE */
			csr = musb_readw(hw_ep->regs, MUSB_RXCSR);
			csr |= MUSB_RXCSR_DMAMODE | MUSB_RXCSR_DMAENAB |
				MUSB_RXCSR_AUTOCLEAR;
			musb_writew(hw_ep->regs, MUSB_RXCSR, csr);
		}
	}

	/* Wait 9 more cycles for ensuring DMA can get USB request length */
	musb_writel(controller->base, MUSB_DMA_FRAG_WAIT, 0x8);

	musb_channel->controller = controller;
	musb_channel->ep_num = bit;
	musb_channel->transmit = transmit;
	musb_channel->node_num = 0;
	musb_channel->busy_slot = 0;
	musb_channel->free_slot = 0;
	channel = &(musb_channel->channel);
	channel->private_data = musb_channel;
	channel->status = MUSB_DMA_STATUS_FREE;
	channel->max_len = 0xffff;
	/* Tx => mode 1; Rx => mode 0 */
	channel->desired_mode = transmit;
	channel->actual_len = 0;
	INIT_LIST_HEAD(&musb_channel->req_queued);

	return channel;
}

static void sprd_configure_channel(struct dma_channel *channel,
				u8 transmit)
{
	struct sprd_musb_dma_channel *musb_channel = channel->private_data;
	struct sprd_musb_dma_controller *controller = musb_channel->controller;
	void __iomem *mbase = controller->base;
	u8 bchannel = musb_channel->channel_num;
	u32 csr = 0;
	u32 haddr4;

	haddr4 = (u32)((u64)musb_channel->list_dma_addr >> 32);
	haddr4 <<= 4;
	if (transmit) {
		/* enable linklist end interrupt */
		csr = musb_readl(mbase, MUSB_DMA_CHN_INTR(bchannel));
		csr |= CHN_LLIST_INT_EN | CHN_CLEAR_INT_EN;
		musb_writel(mbase, MUSB_DMA_CHN_INTR(bchannel), csr);

		/* set linklist pointer */
		musb_writel(mbase, MUSB_DMA_CHN_LLIST_PTR(bchannel),
					(u32)musb_channel->list_dma_addr);

		musb_writel(mbase, MUSB_DMA_CHN_ADDR_H(bchannel), haddr4);
		/* enable channel and trigger tx dma transfer */
		csr = musb_readl(mbase, MUSB_DMA_CHN_PAUSE(bchannel));
		if (csr & CHN_CLR)
			musb_writel(mbase, MUSB_DMA_CHN_PAUSE(bchannel), 0);
		csr = musb_readl(mbase, MUSB_DMA_CHN_CFG(bchannel));
		csr |= CHN_EN;
		musb_writel(mbase, MUSB_DMA_CHN_CFG(bchannel), csr);
	} else {
		/* enable linklist end and rx last interrupt */
		csr = musb_readl(mbase, MUSB_DMA_CHN_INTR(bchannel));
		csr |= CHN_USBRX_INT_EN | CHN_LLIST_INT_EN | CHN_CLEAR_INT_EN;
		musb_writel(mbase, MUSB_DMA_CHN_INTR(bchannel), csr);

		/* set linklist pointer */
		musb_writel(mbase, MUSB_DMA_CHN_LLIST_PTR(bchannel),
					(u32)musb_channel->list_dma_addr);

		musb_writel(mbase, MUSB_DMA_CHN_ADDR_H(bchannel), haddr4);

		/* enable channel and trigger rx dma transfer */
		csr = musb_readl(mbase, MUSB_DMA_CHN_CFG(bchannel));
		csr |= CHN_EN;
		musb_writel(mbase, MUSB_DMA_CHN_CFG(bchannel), csr);
	}
}

u32 musb_linknode_full(struct musb *musb, u32 is_tx)
{
	/* DEV-C1: LINKFIFO tidak diport; jalur non-linkfifo tidak pernah
	 * penuh (satu node-chain per program). */
	return 0;
}
EXPORT_SYMBOL_GPL(musb_linknode_full);

static void musb_prepare_node(struct sprd_musb_dma_channel *musb_channel,
			dma_addr_t dma_addr, u32 len, u8 last, u8 sp, u32 index)
{
	musb_channel->free_slot++;

	musb_channel->dma_linklist[index].addr = (unsigned int)dma_addr;
	musb_channel->dma_linklist[index].data_addr =
		((u8)((u64)dma_addr >> 32)) & 0xf;
	musb_channel->dma_linklist[index].blk_len = len;
	musb_channel->dma_linklist[index].frag_len = 32;
	musb_channel->dma_linklist[index].ioc = last;
	musb_channel->dma_linklist[index].sp = sp;
	musb_channel->dma_linklist[index].list_end = last;
	/*
	 *   wmb below is used to make sure linklist CPU
	 *   initialized is really written to DDR before
	 *   USB DMA read the linklist.
	 */
	wmb();
}

static void musb_prepare_sg_lastnode(struct sprd_musb_dma_channel *musb_channel,
			u32 index)
{
	musb_channel->dma_linklist[index].ioc = 1;
	musb_channel->dma_linklist[index].list_end = 1;
	/*
	 *   wmb below is used to make sure linklist CPU
	 *   initialized is really written to DDR before
	 *   USB DMA read the linklist.
	 */
	wmb();
}

static void musb_prepare_nodes(struct sprd_musb_dma_channel *musb_channel,
	dma_addr_t dma_addr, u32 length, struct musb_request *musb_req,
	struct musb_ep *musb_ep, u32 node_num, u32 nodes_left)
{
	int i;
	u8 last_one = 0;
	u32 len;

	for (i = 0; i < node_num; i++) {
		nodes_left--;
		len = 0xfffc;
		if ((node_num - 1) == i) {
			len = length - (node_num
			- 1) * 0xfffc;
			last_one = 1;
		}
		musb_prepare_node(musb_channel,
		dma_addr, len, last_one, 0,
		musb_channel->node_num);
		dma_addr += len;
		musb_channel->node_num++;
	}
	list_del(&musb_req->list);
	list_add_tail(&musb_req->list,
		&musb_channel->req_queued);
}

static void musb_prepare_first_nodes(struct sprd_musb_dma_channel *musb_channel,
	dma_addr_t dma_addr, u32 length, dma_addr_t addr_last,
	u32 node_num, u32 nodes_left)
{
	int i;
	u8 last_one = 0;
	unsigned int len;

	for (i = 0; i < node_num; i++) {
		nodes_left--;
		len = 0xfffc;
		if ((node_num - 1) == i) {
			len = length - (node_num
			- 1) * 0xfffc;
			if (!addr_last)
				last_one = 1;
		}
		musb_prepare_node(musb_channel,
		dma_addr, len, last_one, 0,
		musb_channel->node_num);
		dma_addr += len;
		musb_channel->node_num++;
	}
}

static void musb_prepare_listnodes(struct sprd_musb_dma_channel *musb_channel,
			struct musb_ep *musb_ep, bool starting)
{
	struct musb_request *musb_req, *n;
	u8 last_one = 0;
	dma_addr_t addr, addr_cpr, addr_last;
	unsigned int length;
	dma_addr_t dma;
	u32 i;
	u32 nodes_left;
	u32 len;
	u8  sp = 0;
	u32 total_sgs = 0, len_sgs;

	/* the first request must not be queued */
	nodes_left = (musb_channel->busy_slot - musb_channel->free_slot)
					& LISTNODE_MASK;

	/*
	 * If busy & slot are equal than it is either full or empty. If we are
	 * starting to process requests then we are empty. Otherwise we are
	 * full and don't do anything
	 */
	if (!nodes_left) {
		if (!starting)
			return;
		nodes_left = LISTNODE_NUM;
		musb_channel->busy_slot = 0;
		musb_channel->free_slot = 0;
		musb_channel->node_num = 0;
	}

	list_for_each_entry_safe(musb_req, n, &musb_ep->req_list, list) {
		if (musb_req->request.num_mapped_sgs > 0) {
			struct usb_request *request = &musb_req->request;
			struct scatterlist *sg = request->sg;
			struct scatterlist *s;

			/*
			 * total req's num_sgs should not exceed LISTNODE_NUM,
			 * or else set last_one flag
			 */
			total_sgs += musb_req->request.num_mapped_sgs;
			if (total_sgs > LISTNODE_NUM &&
				musb_channel->node_num > 1) {
				musb_prepare_sg_lastnode(musb_channel,
					musb_channel->node_num - 1);
				break;
			}

			list_del(&musb_req->list);
			list_add_tail(&musb_req->list,
			&musb_channel->req_queued);

			len_sgs = 0;
			for_each_sg(sg, s, request->num_mapped_sgs, i) {
				length = sg_dma_len(s);
				dma = sg_dma_address(s);
				addr = (dma + length) & ADDR_FLAG;
				addr_cpr = dma & ADDR_FLAG;
				addr_last = (dma + length) & 0xfffffff;
				sp = 0;
				len_sgs += length;

				if (i == (request->num_mapped_sgs - 1) ||
						sg_is_last(s)) {
					if (list_empty(&musb_ep->req_list) ||
					(len_sgs % musb_ep->end_point.maxpacket
							== 0)) {
						last_one = 1;
						sp = 0;
					} else {
						last_one = 0;
						sp = 1;
					}
				}

				nodes_left--;
				if (!nodes_left) {
					last_one = 1;
					sp = 0;
				}

				if (addr == addr_cpr || (!addr_last)) {
					musb_prepare_node(musb_channel,
					dma, length, last_one, sp,
					musb_channel->node_num);
					musb_channel->node_num++;
				} else {
					/*
					 *If nodes_left is 0,need config list
					 *node next time,but if request is
					 *empty,it canconfig this time.
					 */
					if (last_one && (!nodes_left))
						break;
					len = (u32)(addr - dma);
					musb_prepare_node(musb_channel,
					dma, len, 0, sp,
					musb_channel->node_num);
					musb_channel->node_num++;
					nodes_left--;

					if (!nodes_left)
						last_one = 1;
					len = length - (addr - dma);
					musb_prepare_node(musb_channel,
					addr, len, last_one, sp,
					musb_channel->node_num);
					musb_channel->node_num++;
				}

				if (last_one)
					break;
			}

			if (last_one)
				break;
		} else {
			nodes_left = LISTNODE_NUM;
			musb_channel->busy_slot = 0;
			musb_channel->free_slot = 0;
			musb_channel->node_num = 0;

			length = musb_req->request.length;
			dma = musb_req->request.dma;

			addr = (dma + length) & ADDR_FLAG;
			addr_cpr = dma & ADDR_FLAG;
			addr_last = (dma + length) & 0xfffffff;

			if (length < 0xffff) {
				if (addr == addr_cpr) {
					nodes_left--;

					list_del(&musb_req->list);
					list_add_tail(&musb_req->list,
					&musb_channel->req_queued);
					last_one = 1;
					if (musb_req->request.zero && length &&
					(length % musb_ep->packet_sz == 0)) {
						musb_prepare_node(
						musb_channel,
						dma, length, 0, sp,
						musb_channel->node_num);
						musb_channel->node_num++;

						musb_prepare_node(
						musb_channel, dma, 0, 1, sp,
						musb_channel->node_num);
						musb_channel->node_num++;
					} else {
						musb_prepare_node(
						musb_channel, dma, length,
						last_one, sp,
						musb_channel->node_num);
						musb_channel->node_num++;
					}
					if (last_one)
						break;
				} else {
					nodes_left--;
					len = (u32)(addr - dma);
					if (!addr_last)
						last_one = 1;
					musb_prepare_node(musb_channel,
					dma, len, last_one, sp,
					musb_channel->node_num);
					musb_channel->node_num++;

					if (addr_last) {
						nodes_left--;
						len = length - (addr - dma);
						last_one = 1;
						musb_prepare_node(musb_channel,
						addr, len, last_one, sp,
						musb_channel->node_num);
						musb_channel->node_num++;
					}
					list_del(&musb_req->list);
					list_add_tail(&musb_req->list,
					&musb_channel->req_queued);
					if (last_one)
						break;
				}
			} else {
				last_one = 1;

				if (addr == addr_cpr) {
					u32 node_num;

					if (length % 0xfffc)
						node_num = length / 0xfffc + 1;
					else
						node_num = length / 0xfffc;

					musb_prepare_nodes(musb_channel,
					dma, length, musb_req, musb_ep,
					node_num, nodes_left);
				} else {
					u32 node_num1, node_num2;

					if ((addr_cpr + 0x10000000
						- dma) % 0xfffc)
						node_num1 = (addr_cpr +
						0x10000000 - dma) / 0xfffc + 1;
					else
						node_num1 = (addr_cpr +
						0x10000000 - dma) / 0xfffc;

					if ((dma + length - addr) % 0xfffc)
						node_num2 = (dma + length
						- addr) / 0xfffc + 1;
					else
						node_num2 = (dma + length
						- addr) / 0xfffc;

					len = addr - dma;
					musb_prepare_first_nodes(musb_channel,
						dma, len, addr_last,
						node_num1, nodes_left);

					if (addr_last) {
						len = dma + length - addr;
						musb_prepare_nodes(musb_channel,
						addr, len, musb_req, musb_ep,
						node_num2, nodes_left);
					} else {
						list_del(&musb_req->list);
						list_add_tail(&musb_req->list,
						&musb_channel->req_queued);
					}
				}
				if (last_one)
					break;
			}
		}
	}
}

static void musb_host_prepare_nodes(struct sprd_musb_dma_channel *musb_channel,
	dma_addr_t dma_addr, u32 length,
	u32 node_num, u32 nodes_left)
{
	int i;
	u8 last_one = 0;
	unsigned int len;

	for (i = 0; i < node_num; i++) {
		nodes_left--;
		len = 0xfffc;
		if ((node_num - 1) == i) {
			len = length - (node_num
			- 1) * 0xfffc;
			last_one = 1;
		}
		musb_prepare_node(musb_channel,
		dma_addr, len, last_one, 0,
		musb_channel->node_num);
		dma_addr += len;
		musb_channel->node_num++;
	}
}

static void musb_host_listnodes(struct sprd_musb_dma_channel *musb_channel,
	dma_addr_t dma_addr, u32 len)
{
	dma_addr_t addr, addr_cpr, addr_last;
	u32 i;
	u32 nodes_left;
	u32 length;
	int last_one = 0;

	nodes_left = LISTNODE_NUM;
	addr = (dma_addr + len) & ADDR_FLAG;
	addr_cpr = dma_addr & ADDR_FLAG;

	if (len < 0xffff) {
		if (addr == addr_cpr) {
			nodes_left--;
			musb_prepare_node(musb_channel,
				dma_addr, len, 1, 0,
				musb_channel->node_num);
			musb_channel->node_num++;
		} else {
			nodes_left--;
			length = (u32)(addr - dma_addr);
			addr_last = (dma_addr + len) & 0xfffffff;
			if (!addr_last)
				last_one = 1;
			musb_prepare_node(musb_channel,
				dma_addr, length, last_one, 0,
				musb_channel->node_num);
			musb_channel->node_num++;
			if (addr_last) {
				nodes_left--;
				length = len - (addr - dma_addr);
				musb_prepare_node(musb_channel,
					addr, length, 1, 0,
					musb_channel->node_num);
				musb_channel->node_num++;
			}
		}
	} else {
		if (addr == addr_cpr) {
			u32 node_num;

			if (len % 0xfffc)
				node_num = len / 0xfffc + 1;
			else
				node_num = len / 0xfffc;

			musb_host_prepare_nodes(musb_channel,
				dma_addr, len,
				node_num, nodes_left);
		} else {
			u32 node_num1, node_num2;
			u32 len2;

			addr_last = (dma_addr + len) & 0xfffffff;

			if ((addr_cpr + 0x10000000
				- dma_addr) % 0xfffc)
				node_num1 = (addr_cpr +
				0x10000000 - dma_addr) / 0xfffc + 1;
			else
				node_num1 = (addr_cpr +
				0x10000000 - dma_addr) / 0xfffc;

			if (addr_last) {
				if ((dma_addr + len - addr) % 0xfffc)
					node_num2 = (dma_addr + len
						- addr) / 0xfffc + 1;
				else
					node_num2 = (dma_addr + len
						- addr) / 0xfffc;
			} else
				node_num2 = 0;

			len2 = dma_addr + len - addr;

			for (i = 0; i < node_num1; i++) {
				nodes_left--;
				length = 0xfffc;
				if ((node_num1 - 1) == i) {
					length = addr_cpr +
						0x10000000 - dma_addr;
					if (!addr_last)
						last_one = 1;
				}
				musb_prepare_node(musb_channel,
					dma_addr, length, last_one, 0,
					musb_channel->node_num);
				dma_addr += length;
				musb_channel->node_num++;
			}

			if (addr_last) {
				musb_host_prepare_nodes(musb_channel,
					addr, len2,
					node_num2, nodes_left);
			}
		}
	}
}

static int sprd_dma_channel_program(struct dma_channel *channel,
				u16 packet_sz, u8 mode,
				dma_addr_t dma_addr, u32 len)
{
	struct sprd_musb_dma_channel *musb_channel = channel->private_data;
	struct sprd_musb_dma_controller *controller = musb_channel->controller;
	struct musb *musb = controller->private_data;
	struct musb_ep *musb_ep;
	struct musb_hw_ep *hw_ep;

	if (channel->status == MUSB_DMA_STATUS_UNKNOWN ||
		channel->status == MUSB_DMA_STATUS_BUSY)
		return -EINVAL;

	/* (ops->adjust_channel_params tidak ada di 6.18 — dipangkas;
	 * tak ada koreksi argumen vendor di ops glue ini.) */

	hw_ep = &musb->endpoints[musb_channel->ep_num];
	if (musb_channel->transmit)
		musb_ep = &hw_ep->ep_in;
	else
		musb_ep = &hw_ep->ep_out;
	/*
	 * The DMA engine in RTL1.8 and above cannot handle
	 * DMA addresses that are not aligned to a 4 byte boundary.
	 * It ends up masking the last two bits of the address
	 * programmed in DMA_ADDR.
	 *
	 * Fail such DMA transfers, so that the backup PIO mode
	 * can carry out the transfer
	 */
	if ((musb->hwvers >= MUSB_HWVERS_1800) && (dma_addr % 4))
		return -EINVAL;

	/* DEV-C1: LINKFIFO tidak diport (end_point.linkfifo tak ada di
	 * struct usb_ep 6.18) → jalur non-linkfifo fork selalu. */
	musb_channel->used_queue = 0;

	channel->actual_len = 0;
	musb_channel->max_packet_sz = packet_sz;
	channel->status = MUSB_DMA_STATUS_BUSY;

	/*
	 * DMA transfer length and address has some restriction
	 */

	if (musb->xceiv->otg->state == OTG_STATE_B_PERIPHERAL) {
		void __iomem *epio = hw_ep->regs;
		u16 csr, dma_setting;

		if (!musb_channel->transmit) {
			dma_setting = MUSB_RXCSR_AUTOCLEAR |
				      MUSB_RXCSR_DMAMODE |
				      MUSB_RXCSR_DMAENAB;
			csr = musb_readw(epio, MUSB_RXCSR);
			if ((csr & dma_setting) != dma_setting)
				musb_writew(epio, MUSB_RXCSR, dma_setting);
		}
		musb_prepare_listnodes(musb_channel, musb_ep, true);
	} else {
		musb_channel->busy_slot = 0;
		musb_channel->free_slot = 0;
		musb_channel->node_num = 0;
		musb_host_listnodes(musb_channel, dma_addr, len);
	}
	dev_dbg(musb->controller,
		"ep%d-%s  dma_addr 0x%08x length %d\n",
		musb_channel->ep_num,
		musb_channel->transmit ? "Tx" : "Rx",
		(unsigned int)dma_addr, (int)len);

	sprd_configure_channel(channel, musb_channel->transmit);

	return 1;
}static inline struct musb_request *channel_get_next_request
						(struct list_head *list)
{
	if (list_empty(list))
		return NULL;

	return list_first_entry(list, struct musb_request, list);
}

static inline struct device *musb_controller_dma_dev(
	struct sprd_musb_dma_controller *controller)
{
	struct musb *musb = controller->private_data;

	return musb->controller;
}

#if IS_ENABLED(CONFIG_USB_MUSB_HOST) || IS_ENABLED(CONFIG_USB_MUSB_DUAL_ROLE)
static void musb_host_channel_abort(struct musb *musb,
	struct sprd_musb_dma_channel *musb_channel)
{
	struct urb *urb;
	struct musb_qh *qh;
	struct musb_hw_ep *hw_ep;
	struct musb_ep *musb_ep;
	struct dma_channel *channel;

	if (musb_channel->transmit) {
		musb_ep = &musb->endpoints[musb_channel->ep_num].ep_out;
		hw_ep = musb_ep->hw_ep;
		qh = hw_ep->out_qh;
	} else {
		musb_ep = &musb->endpoints[musb_channel->ep_num].ep_in;
		hw_ep = musb_ep->hw_ep;
		qh = hw_ep->in_qh;
	}
	if (qh) {
		urb = next_urb(qh);
		urb->status = -ECONNRESET;
		channel = &musb_channel->channel;
		if (list_empty(&qh->hep->urb_list))
			channel->status = MUSB_DMA_STATUS_FREE;
	}
}
#else
static void musb_host_channel_abort(struct musb *musb,
	struct sprd_musb_dma_channel *musb_channel)
{
}
#endif

static int sprd_dma_channel_abort(struct dma_channel *channel)
{
	struct sprd_musb_dma_channel *musb_channel;
	struct sprd_musb_dma_controller *controller;
	void __iomem *mbase;
	struct musb *musb;
	struct musb_request *musb_req;
	struct usb_request *request;
	struct musb_ep *musb_ep;
	u8 bchannel;
	void __iomem *epio;
	u16 csr;
	u32 pause;

	if (!channel)
		return 0;

	musb_channel = channel->private_data;
	if (!musb_channel)
		return 0;

	controller = musb_channel->controller;
	mbase = controller->base;
	musb = controller->private_data;
	bchannel = musb_channel->channel_num;

	if (channel->status == MUSB_DMA_STATUS_BUSY) {
		pause = musb_readl(mbase, MUSB_DMA_CHN_PAUSE(bchannel));
		pause |= CHN_CLR;
		musb_writel(mbase, MUSB_DMA_CHN_PAUSE(bchannel), pause);
		if (musb->g.state != USB_STATE_NOTATTACHED) {
			epio = musb->endpoints[musb_channel->ep_num].regs;

			if (musb_channel->transmit) {
				/*
				 * The programming guide says that we must clear
				 * the DMAENAB bit before the DMAMODE bit...
				 */
				csr = musb_readw(epio, MUSB_TXCSR);
				csr &= ~(MUSB_TXCSR_AUTOSET |
					 MUSB_TXCSR_DMAENAB);
				musb_writew(epio, MUSB_TXCSR, csr);
				csr &= ~MUSB_TXCSR_DMAMODE;
				musb_writew(epio, MUSB_TXCSR, csr);
			} else {
				csr = musb_readw(epio, MUSB_RXCSR);
				csr &= ~(MUSB_RXCSR_AUTOCLEAR |
					 MUSB_RXCSR_DMAENAB |
					 MUSB_RXCSR_DMAMODE);
				musb_writew(epio, MUSB_RXCSR, csr);
			}

			musb_writel(mbase, MUSB_DMA_CHN_LLIST_PTR(bchannel), 0);
			musb_write_dma_addr(mbase, bchannel, 0);
		}
		if (!is_host_active(musb) &&
		    (musb->xceiv->otg->state != OTG_STATE_A_HOST) &&
		    (musb->xceiv->otg->state != OTG_STATE_A_WAIT_BCON)) {
			/*release request*/
			if (musb_channel->transmit)
				musb_ep =
				&musb->endpoints[musb_channel->ep_num].ep_in;
			else
				musb_ep =
				&musb->endpoints[musb_channel->ep_num].ep_out;
			while (!list_empty(&musb_channel->req_queued)) {
				musb_req =
					channel_get_next_request
					(&musb_channel->req_queued);
				request = &musb_req->request;
				if (request->num_mapped_sgs)
					musb_channel->busy_slot +=
						request->num_mapped_sgs;
				else
					musb_channel->busy_slot++;

				musb_g_giveback(musb_ep, request, -ESHUTDOWN);
				channel->status = MUSB_DMA_STATUS_FREE;
			}
		} else
			musb_host_channel_abort(musb, musb_channel);
	} else if (channel->status == MUSB_DMA_STATUS_CORE_ABORT) {
		pause = musb_readl(mbase, MUSB_DMA_CHN_PAUSE(bchannel));
		pause |= CHN_CLR;
		musb_writel(mbase, MUSB_DMA_CHN_PAUSE(bchannel), pause);
	}

	return 0;
}

static void sprd_musb_dma_completion(struct musb *musb, u8 epnum, u8 transmit)
{
	struct musb_ep *musb_ep;
	struct musb_request *musb_req;
	struct usb_request *request;
	struct dma_channel *channel;
	struct sprd_musb_dma_channel *musb_channel;
	u32 blk_len	=	0;

	if (transmit)
		musb_ep = &musb->endpoints[epnum].ep_in;
	else
		musb_ep = &musb->endpoints[epnum].ep_out;

	channel = musb_ep->dma;
	if (!channel)
		return;

	musb_channel = channel->private_data;

	do {
		musb_req = channel_get_next_request(&musb_channel->req_queued);
		if (!musb_req) {
			WARN_ON_ONCE(1);
			break;
		}
		request = &musb_req->request;

		if (!transmit) {
			blk_len = musb_readl(musb->mregs,
				MUSB_DMA_CHN_LEN(epnum + 15));
			blk_len = (blk_len & 0xffff0000) >> 16;
			request->actual = request->length - blk_len;
		} else
			request->actual = request->length;

		if (request->num_mapped_sgs)
			musb_channel->busy_slot += request->num_mapped_sgs;
		else
			musb_channel->busy_slot++;

		musb_g_giveback(musb_ep, request, 0);
	} while (!list_empty(&musb_channel->req_queued));

	channel->status = MUSB_DMA_STATUS_FREE;
	musb_req = musb_ep->desc ? next_request(musb_ep) : NULL;
	if (!musb_req) {
	dev_dbg(musb->controller, "%s idle now\n",
		musb_ep->end_point.name);
		return;
	}
	musb_ep_select(musb->mregs, epnum);

	if (channel->status == MUSB_DMA_STATUS_BUSY) {
		dev_info(musb->controller, "dma pending...\n");
		return;
	}

	request = &musb_req->request;
	sprd_dma_channel_program(channel, musb_ep->packet_sz, musb_req->tx,
			request->dma + request->actual,
			request->length - request->actual);
}

#if IS_ENABLED(CONFIG_USB_MUSB_HOST) || IS_ENABLED(CONFIG_USB_MUSB_DUAL_ROLE)
static void sprd_calc_urb_actual_length(struct musb *musb,
	struct urb *urb,
	struct musb_qh *qh,
	struct dma_channel *channel,
	u32 dma_addr,
	u32 blk_len)
{
	u32 i = 0, j = 0;
	u32 node_num = 0;
	u32 unused_length = 0;
	struct sprd_musb_dma_channel *musb_channel = channel->private_data;

	node_num = musb_channel->node_num;
	for (i = 0; i < node_num; i++) {
		if ((musb_channel->dma_linklist[i].addr +
			musb_channel->dma_linklist[i].blk_len -
			blk_len) == dma_addr)
			break;
	}

	if (i == node_num) {
		dev_err(musb->controller, "cannot find the dma_addr\n");
		urb->actual_length = 0;
		return;
	}

	if (i + 1 == node_num) {
		urb->actual_length += qh->segsize - blk_len;
		return;
	}

	for (j = i + 1; j < node_num; j++) {
		unused_length += musb_channel->dma_linklist[j].blk_len;
	}
	urb->actual_length += qh->segsize - unused_length - blk_len;

	return;
}

static void sprd_musb_urb_completion(struct musb *musb, u8 epnum, u8 is_in)
{
	struct musb_hw_ep *hw_ep;
	struct musb_qh *qh;
	struct dma_channel *channel;
	struct urb *urb;
	u32 blk_len = 0;
	u32 dma_addr = 0;

	hw_ep = &musb->endpoints[epnum];
	if (is_in) {
		channel = hw_ep->rx_channel;
		if (!channel)
			return;
		qh = hw_ep->in_qh;
	} else {
		channel = hw_ep->tx_channel;
		if (!channel)
			return;
		qh = hw_ep->out_qh;
	}
	if (!qh)
		return;

	urb = next_urb(qh);
	if (!urb)
		return;
	if (is_in) {
		blk_len = musb_readl(musb->mregs,
			MUSB_DMA_CHN_LEN(epnum + 15));
		dma_addr = musb_readl(musb->mregs,
			MUSB_DMA_CHN_ADDR(epnum + 15));
	} else {
		blk_len = musb_readl(musb->mregs,
			MUSB_DMA_CHN_LEN(epnum));
		dma_addr = musb_readl(musb->mregs,
			MUSB_DMA_CHN_ADDR(epnum));
	}

	blk_len = (blk_len & 0xffff0000) >> 16;
	sprd_calc_urb_actual_length(musb, urb, qh, channel, dma_addr, blk_len);
	if (usb_pipeisoc(urb->pipe)) {
		struct usb_iso_packet_descriptor *d;

		d = urb->iso_frame_desc + qh->iso_idx;
		channel->actual_len = qh->segsize - blk_len;
		d->status = 0;
		d->actual_length = channel->actual_len;
		if (++qh->iso_idx < urb->number_of_packets) {
			/* B50 (fase 2b): DEV-H1 ditutup — helper B50 ada di
			 * musb_host.c; ulang-program paket berikutnya dilakukan
			 * di ekor fungsi (pola fork :1355-1371). */
			channel->status = MUSB_DMA_STATUS_FREE;
		}
	}

	dev_dbg(musb->controller, "%s epnum=%d actual=%d urb %p\n",
		__func__, epnum, (int)urb->actual_length, urb);
	channel->status = MUSB_DMA_STATUS_FREE;

	/*
	 * B50 (2026-09-27, fase 2b): DEV-H2 ditutup — jalur fork dipulihkan.
	 * musb_advance_schedule kini non-static di musb_host.c (patch B50,
	 * prototipe musb_host.h — pola fork musb_host.h:98/101/149); isoc host
	 * memprogram ulang per paket lewat helper B50 (DEV-H1). fork
	 * sprd_musb_urb_completion :1362-1373.
	 */
	if (usb_pipeisoc(urb->pipe) &&
	    qh->iso_idx < urb->number_of_packets) {
		struct usb_iso_packet_descriptor *d =
			urb->iso_frame_desc + qh->iso_idx;

		if (is_in)
			musb_rx_dma_sprd(channel, musb, epnum, qh, urb,
					 d->offset, d->length);
		else
			musb_tx_dma_program(musb->dma_controller, hw_ep,
					    qh, urb, d->offset, d->length);
		return;
	}

	musb_advance_schedule(musb, urb, hw_ep, is_in);
}
#else
static void sprd_musb_urb_completion(struct musb *musb, u8 epnum, u8 is_in)
{
}
#endif

irqreturn_t sprd_dma_interrupt(struct musb *musb, u32 int_hsdma)
{
	void __iomem *mbase = musb->mregs;
	struct musb_ep *musb_ep;
	u8 bchannel = 0, epnum;
	u32 intr, int_dma;
	int i;
	int is_tx;

	int_dma = musb_readl(musb->mregs, MUSB_DMA_INTR_MASK_STATUS);
	for (i = 0; i < MUSB_DMA_CHANNELS; i++) {
		bchannel++;
		if ((int_hsdma & BIT(0)) != BIT(0)) {
			int_hsdma = int_hsdma >> 1;
			continue;
		}
		int_hsdma = int_hsdma >> 1;

		intr = musb_readl(mbase, MUSB_DMA_CHN_INTR(bchannel));
		dev_dbg(musb->controller, "%s is 0x%x, %d , %d\n",
				__func__, intr, (int)bchannel, (int)int_dma);

		if (intr & CHN_START_INT_MASK_STATUS) {
			dev_info(musb->controller, "DMA request is NULL\n");

			/* clear interrupt */
			intr |= CHN_START_INT_CLR;
			musb_writel(mbase, MUSB_DMA_CHN_INTR(bchannel), intr);
		}
		if (intr & CHN_CLEAR_INT_MASK_STATUS) {
			musb_writel(mbase, MUSB_DMA_CHN_PAUSE(bchannel), 0x0);
			musb_writel(mbase, MUSB_DMA_CHN_CFG(bchannel), 0x0);

		if (bchannel > 15) {
			is_tx = 0;
			epnum = bchannel - 15;
			musb_ep = &musb->endpoints[epnum].ep_out;
		} else {
			is_tx = 1;
			epnum = bchannel;
			musb_ep = &musb->endpoints[epnum].ep_in;
		}
		/* DEV-C1: fork memanggil musb_linknode_clear() bila
		 * musb_ep->end_point.linkfifo — field tak ada di 6.18, cabang
		 * tidak pernah hidup tanpa patch core (stub DEV-C1 dihapus). */

			dev_info(musb->controller, "dma interrupt clear channel\n");
		}

		if (bchannel > 15) {
			if (intr & CHN_LLIST_INT_MASK_STATUS) {
				/* clear interrupt */
				intr |= CHN_LLIST_INT_CLR | CHN_START_INT_CLR |
					CHN_FRAG_INT_CLR | CHN_BLK_INT_CLR |
					CHN_USBRX_LAST_INT_CLR;
				musb_writel(mbase, MUSB_DMA_CHN_INTR(bchannel),
					intr);
				/* callback to give complete */
				if (musb->xceiv->otg->state
					== OTG_STATE_B_PERIPHERAL)
					sprd_musb_dma_completion(musb,
					(bchannel - 15), 0);
				else
					sprd_musb_urb_completion(musb,
					(bchannel - 15), 1);
			}
		} else {
			if (intr & CHN_LLIST_INT_MASK_STATUS) {
				/* clear interrupt */
				intr |= CHN_LLIST_INT_CLR | CHN_START_INT_CLR |
					CHN_FRAG_INT_CLR | CHN_BLK_INT_CLR;
				musb_writel(mbase, MUSB_DMA_CHN_INTR(bchannel),
					intr);
				/* callback to give complete */
				if (musb->xceiv->otg->state
					== OTG_STATE_B_PERIPHERAL)
					sprd_musb_dma_completion(musb,
					bchannel, 1);
				else
					sprd_musb_urb_completion(musb,
							bchannel, 0);
			}
		}
	}

	return IRQ_HANDLED;
}

void sprd_musb_dma_controller_destroy(struct dma_controller *c)
{
	struct sprd_musb_dma_controller *controller = container_of(c,
			struct sprd_musb_dma_controller, controller);
	int i;

	sprd_dma_controller_stop(controller);

	/* DEV-C3: bebaskan 30 buffer linklist koheren milik controller. */
	for (i = 1; i <= MUSB_DMA_CHANNELS; i++) {
		if (controller->channel[i].dma_linklist)
			dma_free_coherent(musb_controller_dma_dev(controller),
				sizeof(struct linklist_node_s) * LISTNODE_NUM,
				controller->channel[i].dma_linklist,
				controller->channel[i].list_dma_addr);
		controller->channel[i].dma_linklist = NULL;
		controller->channel[i].list_dma_addr = 0;
	}

	kfree(controller);
}

struct dma_controller *sprd_musb_dma_controller_create(struct musb *musb,
						    void __iomem *base)
{
	struct sprd_musb_dma_controller *controller;
	int i;

	controller = kzalloc(sizeof(*controller), GFP_KERNEL);
	if (!controller)
		return NULL;

	controller->private_data = musb;
	controller->base = base;

	/* DEV-C3: fork mengalokasikan buffer linklist koheren per-EP di
	 * patch core (fork musb_gadget.c:1952, 32 KiB = 2048 node x 16 B
	 * per arah EP); patch core itu tidak ada di 6.18, jadi port ini
	 * mengalokasikan 30 buffer (channel 1..30) SEKALIGUS di sini —
	 * konteks probe (tidur boleh), bukan di channel_alloc yang
	 * dipanggil core dengan musb->lock terkunci. Pola memori sama
	 * dengan fork (±1 MiB saat DMA aktif), bebas race, dan gagal-
	 * alloc ditangani tertib (unwind + return NULL → core jatuh ke
	 * jalur tanpa DMA). */
	for (i = 1; i <= MUSB_DMA_CHANNELS; i++) {
		controller->channel[i].dma_linklist =
			dma_alloc_coherent(musb->controller,
				sizeof(struct linklist_node_s) * LISTNODE_NUM,
				&controller->channel[i].list_dma_addr,
				GFP_KERNEL);
		if (!controller->channel[i].dma_linklist) {
			int j;

			for (j = 1; j < i; j++)
				dma_free_coherent(musb->controller,
					sizeof(struct linklist_node_s) *
					LISTNODE_NUM,
					controller->channel[j].dma_linklist,
					controller->channel[j].list_dma_addr);
			kfree(controller);
			return NULL;
		}
	}

	controller->controller.channel_alloc = sprd_dma_channel_allocate;
	controller->controller.channel_release = sprd_dma_channel_release;
	controller->controller.channel_program = sprd_dma_channel_program;
	controller->controller.channel_abort = sprd_dma_channel_abort;
	init_waitqueue_head(&controller->wait);

	return &controller->controller;
}
EXPORT_SYMBOL_GPL(sprd_musb_dma_controller_create);

MODULE_DESCRIPTION("Unisoc MUSB high-speed DMA controller (VXTux port)");
MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_LICENSE("GPL");
