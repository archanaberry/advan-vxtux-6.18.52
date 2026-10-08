/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MUSB OTG driver host defines
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 */

#ifndef _MUSB_HOST_H
#define _MUSB_HOST_H

#include <linux/scatterlist.h>

/* stored in "usb_host_endpoint.hcpriv" for scheduled endpoints */
struct musb_qh {
	struct usb_host_endpoint *hep;		/* usbcore info */
	struct usb_device	*dev;
	struct musb_hw_ep	*hw_ep;		/* current binding */

	struct list_head	ring;		/* of musb_qh */
	/* struct musb_qh		*next; */	/* for periodic tree */
	u8			mux;		/* qh multiplexed to hw_ep */

	unsigned		offset;		/* in urb->transfer_buffer */
	unsigned		segsize;	/* current xfer fragment */

	u8			type_reg;	/* {rx,tx} type register */
	u8			intv_reg;	/* {rx,tx} interval register */
	u8			addr_reg;	/* device address register */
	u8			h_addr_reg;	/* hub address register */
	u8			h_port_reg;	/* hub port register */

	u8			is_ready;	/* safe to modify hw_ep */
	u8			type;		/* XFERTYPE_* */
	u8			epnum;
	u8			hb_mult;	/* high bandwidth pkts per uf */
	u16			maxpacket;
	u16			frame;		/* for periodic schedule */
	unsigned		iso_idx;	/* in urb->iso_frame_desc[] */
	struct sg_mapping_iter sg_miter;	/* for highmem in PIO mode */
	bool			use_sg;		/* to track urb using sglist */
};

/* map from control or bulk queue head to the first qh on that ring */
static inline struct musb_qh *first_qh(struct list_head *q)
{
	if (list_empty(q))
		return NULL;
	return list_entry(q->next, struct musb_qh, ring);
}


#if IS_ENABLED(CONFIG_USB_MUSB_HOST) || IS_ENABLED(CONFIG_USB_MUSB_DUAL_ROLE)
extern struct musb *hcd_to_musb(struct usb_hcd *);
extern irqreturn_t musb_h_ep0_irq(struct musb *);
extern int musb_host_alloc(struct musb *);
extern int musb_host_setup(struct musb *, int);
extern void musb_host_cleanup(struct musb *);
extern void musb_host_tx(struct musb *, u8);
extern void musb_host_rx(struct musb *, u8);
extern void musb_root_disconnect(struct musb *musb);
extern void musb_host_free(struct musb *);
extern void musb_host_resume_root_hub(struct musb *musb);
extern void musb_host_poke_root_hub(struct musb *musb);
extern int musb_port_suspend(struct musb *musb, bool do_suspend);
extern void musb_port_reset(struct musb *musb, bool do_reset);
extern void musb_host_finish_resume(struct work_struct *work);
#else
static inline struct musb *hcd_to_musb(struct usb_hcd *hcd)
{
	return NULL;
}

static inline irqreturn_t musb_h_ep0_irq(struct musb *musb)
{
	return 0;
}

static inline int musb_host_alloc(struct musb *musb)
{
	return 0;
}

static inline int musb_host_setup(struct musb *musb, int power_budget)
{
	return 0;
}

static inline void musb_host_cleanup(struct musb *musb)		{}
static inline void musb_host_free(struct musb *musb)		{}
static inline void musb_host_tx(struct musb *musb, u8 epnum)	{}
static inline void musb_host_rx(struct musb *musb, u8 epnum)	{}
static inline void musb_root_disconnect(struct musb *musb)	{}
static inline void musb_host_resume_root_hub(struct musb *musb)	{}
static inline void musb_host_poke_root_hub(struct musb *musb)	{}
static inline int musb_port_suspend(struct musb *musb, bool do_suspend)
{
	return 0;
}
static inline void musb_port_reset(struct musb *musb, bool do_reset) {}
static inline void musb_host_finish_resume(struct work_struct *work) {}
#endif

struct usb_hcd;

extern int musb_hub_status_data(struct usb_hcd *hcd, char *buf);extern int musb_hub_control(struct usb_hcd *hcd,
				u16 typeReq, u16 wValue, u16 wIndex,
				char *buf, u16 wLength);

/*
 * Helper host yang dipakai bersama engine DMA vendor (sprd_musbhsdma.c).
 *
 * 2026-10-06: ketiga fungsi ini SEMULA static di musb_host.c, sehingga
 * sprd_musbhsdma.c GAGAL dikompilasi (call to undeclared function; kernel
 * memakai -Werror=implicit-function-declaration) padahal komentarnya
 * mengklaim patch B50 sudah dikerjakan. Prototipe ditulis dengan pola
 * extern fork T618 musb_host.h:98/101/149; definisi musb_rx_dma_sprd
 * di-port dari fork musb_host.c:706.
 */
extern bool musb_tx_dma_program(struct dma_controller *dma,
		struct musb_hw_ep *hw_ep, struct musb_qh *qh,
		struct urb *urb, u32 offset, u32 length);
extern void musb_rx_dma_sprd(struct dma_channel *dma_channel,
		struct musb *musb, u8 epnum,
		struct musb_qh *qh,
		struct urb *urb,
		u32 offset, size_t len);
extern void musb_advance_schedule(struct musb *musb, struct urb *urb,
				  struct musb_hw_ep *hw_ep, int is_in);

static inline struct urb *next_urb(struct musb_qh *qh)
{
	struct list_head	*queue;

	if (!qh)
		return NULL;
	queue = &qh->hep->urb_list;
	if (list_empty(queue))
		return NULL;
	return list_entry(queue->next, struct urb, urb_list);
}

#endif				/* _MUSB_HOST_H */
