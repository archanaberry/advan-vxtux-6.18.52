/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sprd_u_ether.h — deklarasi ABI fork `sprd_u_ether` (implementasi + bukti
 * disasm + batas yang disengaja ada di sprd_u_ether.c).
 *
 * Kenapa header ini ada: sejak 6.2 kernel dibangun dengan
 * -Wmissing-prototypes, jadi fungsi non-static yang diekspor tanpa prototipe
 * menghasilkan warning (kelas yang sudah pernah muncul di pohon ini; lihat
 * tools/re/audit_provider_proto.py). Header ini juga yang dipakai konsumen
 * (mis. sisi fungsi RNDIS kita sendiri) untuk melihat tipe argumennya.
 */
#ifndef __VXTUX_SPRD_U_ETHER_H
#define __VXTUX_SPRD_U_ETHER_H

#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>

#include "../u_ether.h"

/* --- 16 nama yang di blob punya padanan 1:1 di u_ether.c 6.18 ----------- */
void sprd_gether_cleanup(struct eth_dev *dev);
struct net_device *sprd_gether_connect(struct gether *port);
void sprd_gether_disconnect(struct gether *port);
int sprd_gether_get_dev_addr(struct net_device *net, char *dev_addr, int len);
int sprd_gether_get_host_addr(struct net_device *net, char *host_addr, int len);
int sprd_gether_get_host_addr_cdc(struct net_device *net, char *host_addr,
				  int len);
void sprd_gether_get_host_addr_u8(struct net_device *net,
				  u8 host_mac[ETH_ALEN]);
int sprd_gether_get_ifname(struct net_device *net, char *name, int len);
unsigned sprd_gether_get_qmult(struct net_device *net);
int sprd_gether_register_netdev(struct net_device *net);
int sprd_gether_set_dev_addr(struct net_device *net, const char *dev_addr);
void sprd_gether_set_gadget(struct net_device *net, struct usb_gadget *g);
int sprd_gether_set_host_addr(struct net_device *net, const char *host_addr);
void sprd_gether_set_qmult(struct net_device *net, unsigned qmult);
struct eth_dev *sprd_gether_setup_name(struct usb_gadget *g,
				       const char *dev_addr,
				       const char *host_addr,
				       u8 ethaddr[ETH_ALEN], unsigned qmult,
				       const char *netname);
struct net_device *sprd_gether_setup_name_default(const char *netname);

/* --- 4 helper SG/limit multi-paket: tidak ada padanannya di 6.18 -------- */
void gether_enable_sg(struct gether *port, bool on);
bool gether_is_sg_enabled(struct gether *port);
void gether_update_dl_max_xfer_size(struct gether *port, u32 size);
void gether_update_dl_max_pkts_per_xfer(struct gether *port, u32 pkts);

#endif /* __VXTUX_SPRD_U_ETHER_H */
