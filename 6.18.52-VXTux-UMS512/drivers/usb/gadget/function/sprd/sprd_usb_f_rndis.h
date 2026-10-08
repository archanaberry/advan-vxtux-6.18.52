/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sprd_usb_f_rndis.h — deklarasi ABI fork `sprd_usb_f_rndis` (implementasi +
 * bedah blob + keputusan bentuk ada di sprd_usb_f_rndis.c).
 *
 * Kenapa header ini ada: fungsi non-static yang diekspor tanpa prototipe
 * menghasilkan -Wmissing-prototypes pada build kernel modern
 * (tools/re/audit_provider_proto.py). Tipe argumen mengikuti library rndis
 * mainline 6.18 karena 15 dari 16 fungsi memang delegasi 1:1 (bukti call-site
 * di sprd_usb_f_rndis.c).
 */
#ifndef __VXTUX_SPRD_USB_F_RNDIS_H
#define __VXTUX_SPRD_USB_F_RNDIS_H

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/usb/composite.h>

#include "../rndis.h"
#include "../u_rndis.h"

/* --- 16 nama ABI vendor ------------------------------------------------- */
int sprd_rndis_msg_parser(struct rndis_params *params, u8 *buf);
struct rndis_params *sprd_rndis_register(void (*resp_avail)(void *v), void *v);
void sprd_rndis_deregister(struct rndis_params *params);
int sprd_rndis_set_param_dev(struct rndis_params *params, struct net_device *dev,
			     u16 *cdc_filter);
int sprd_rndis_set_param_vendor(struct rndis_params *params, u32 vendorID,
				const char *vendorDescr);
int sprd_rndis_set_param_medium(struct rndis_params *params, u32 medium,
				u32 speed);
void sprd_rndis_add_hdr(struct sk_buff *skb);
int sprd_rndis_rm_hdr(struct gether *port, struct sk_buff *skb,
		      struct sk_buff_head *list);
u8 *sprd_rndis_get_next_response(struct rndis_params *params, u32 *length);
void sprd_rndis_free_response(struct rndis_params *params, u8 *buf);
void sprd_rndis_uninit(struct rndis_params *params);
int sprd_rndis_signal_connect(struct rndis_params *params);
int sprd_rndis_signal_disconnect(struct rndis_params *params);
void sprd_rndis_set_host_mac(struct rndis_params *params, const u8 *addr);
void sprd_rndis_borrow_net(struct usb_function_instance *f,
			   struct net_device *net);
void sprd_rndis_set_max_pkt_xfer(struct rndis_params *params, u8 max_pkt_xfer);

/* --- ekstensi kita (bukan ABI vendor): pembaca pasangan setter di atas --- */
u8 sprd_rndis_get_max_pkt_xfer(struct rndis_params *params);

#endif /* __VXTUX_SPRD_USB_F_RNDIS_H */
