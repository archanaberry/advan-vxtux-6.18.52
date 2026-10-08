// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_usb_f_rndis.c — port PENYEDIA ABI `sprd_usb_f_rndis` (fork Unisoc UMS512
 * dari drivers/usb/gadget/function/{f_rndis.c,rndis.c} 5.4.210) ke 6.18.52.
 *
 * === BENTUK BLOB VENDOR (hasil bedah, bukan dugaan) ===
 * blobs/vendor_dlkm/sprd_usb_f_rndis.ko (97.358 B) BUKAN cuma library RNDIS:
 * ia MENGGABUNG f_rndis.c + rndis.c dalam satu modul, dan mengganti awalan
 * `rndis_` -> `sprd_rndis_` (16 T-simbol). Bukti dari symtab blob
 * (blobs/re_notes/sprd_usb_f_rndis.sym):
 *   - sisi fungsi (f_rndis.c): t rndis_alloc_inst, t rndis_bind,
 *     t rndis_set_alt, t rndis_setup, t rndis_disable, t rndis_free_inst,
 *     rndis_attrs, rndis_item_ops, rndis_opts_attr_{qmult,dev_addr,host_addr,
 *     ifname,class,subclass,protocol}, eth_{fs,hs,ss}_function,
 *     rndis_iad_descriptor, rndis_ida, rndisusb_func;
 *   - sisi library (rndis.c): oid_supported_list, gen_ndis_query_resp,
 *     gen_ndis_set_resp, rndis_add_header, rndis_response_available,
 *     rndis_command_complete, rndis_response_complete;
 *   - UND-nya 18 simbol penyedia `sprd_u_ether` — lihat sprd_u_ether.c.
 * Modinfo blob: `alias=usbfunc:rndis`, `depends=sprd_u_ether`, `license=GPL`.
 *
 * === NAMA CONFIGFS: "rndis" — SAMA PERSIS DENGAN MAINLINE ===
 * Alias `usbfunc:rndis` dihasilkan makro DECLARE_USB_FUNCTION
 * (include/linux/usb/composite.h: MODULE_ALIAS("usbfunc:"__stringify(_name))),
 * jadi vendor memakai INSTANCE(configfs) bernama `rndis` — identik dengan
 * `DECLARE_USB_FUNCTION_INIT(rndis, ...)` di f_rndis.c mainline. Konsekuensinya:
 * userspace Android/vendor TIDAK perlu diubah; nama fungsi tetap "rndis".
 *
 * === KENAPA BERKAS INI TIDAK MENDAFTARKAN FUNGSI CONFIGFS SENDIRI ===
 * Dua pendaftar dengan nama sama akan gagal: usb_function_register() menolak
 * duplikat (nama + modul), jadi mendaftarkan "rndis" dari sini akan membuat
 * salah satu modul gagal probe saat dimuat. Di papan ini jalur RNDIS sudah
 * dilayani f_rndis.c mainline (PASS di docs/usb_driver_inventory.md, sym
 * USB_CONFIGFS_RNDIS=y pada config r9) — jadi berkas ini mengambil peran
 * PENYEDIA ABI + penampung delta vendor, bukan pendaftar kedua. Bila kelak
 * f_rndis mainline diganti versi vendor, yang perlu DITAMBAHKAN di sisi fungsi
 * hanya tiga hal (semuanya ada di disasm blob, work/rndis.asm):
 *   - rndis_set_alt: gether_enable_sg(&port, 1)          (0xa9c)
 *   - rndis_command_complete: gether_is_sg_enabled()     (0x11a4)
 *     lalu gether_update_dl_max_xfer_size(&port, msg+0x14) (0x11b8) dan
 *     gether_update_dl_max_pkts_per_xfer(&port, 10)        (0x11cc)
 *   - atribut baru `max_pkt_xfer` -> sprd_rndis_set_max_pkt_xfer() (0x2adc)
 *
 * === SUMBER: TIDAK ADA TEKS GPL DI REPO INI ===
 * Sama seperti sprd_u_ether.c: ledger menyebut "open-source (fork GPL)", tapi
 * tarball fork yang ada di repo tidak memuat drivers/usb/gadget. Karena itu 15
 * dari 16 fungsi di bawah adalah delegasi 1:1 ke library rndis.c mainline yang
 * SUDAH ada di pohon (bukan stub kosong), dan satu sisanya — `max_pkt_xfer`,
 * yang di blob hanya `strb w1,[x0,#0x34]` — diimplementasikan sebagai state
 * per-params. Flavors export disamakan: blob memakai EXPORT_SYMBOL non-GPL.
 *
 * Catatan pemakaian: `sprd_rndis_set_max_pkt_xfer()` TIDAK dipanggil siapa pun
 * di himpunan blob r11 (satu-satunya acuan adalah thunk CFI JUMP26 di 0x3f40,
 * bukan call-site) — jadi ia adalah API publik vendor untuk pemakai luar/turunan
 * seperti halnya posisinya sekarang: disediakan untuk paritas, bukan untuk
 * menutup pemanggil di pohon ini. Untuk alasan yang sama dengan sprd_u_ether.c,
 * prototipe publiknya ditaruh di sprd_usb_f_rndis.h (-Wmissing-prototypes).
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "sprd_usb_f_rndis.h"

/*
 * ==== STATE `max_pkt_xfer` (field vendor +0x34 di struct rndis_params) ====
 *
 * Struct rndis_params 6.18 tidak punya field ini (rndis.h:154-176) dan berkas
 * ini tidak boleh mengubah header mainline hanya demi satu field vendor, jadi
 * nilainya disimpan per-params di sini. Daur hidup: dibuat saat pertama di-set,
 * dihapus di sprd_rndis_deregister() — jadi tidak tumbuh tanpa batas.
 */
#define VXTUX_SPRD_RNDIS_SLOTS	4

struct vxtux_sprd_rndis_state {
	struct rndis_params	*params;
	u8			max_pkt_xfer;
};

static struct vxtux_sprd_rndis_state vxtux_sprd_rndis[VXTUX_SPRD_RNDIS_SLOTS];
static DEFINE_SPINLOCK(vxtux_sprd_rndis_lock);

/* `params == NULL` menandai slot kosong; params nyata tidak pernah NULL. */
static struct vxtux_sprd_rndis_state *
vxtux_sprd_rndis_find(struct rndis_params *params, bool create)
{
	struct vxtux_sprd_rndis_state *st = NULL, *free_slot = NULL;
	unsigned long flags;
	int i;

	if (!params)
		return NULL;

	spin_lock_irqsave(&vxtux_sprd_rndis_lock, flags);
	for (i = 0; i < VXTUX_SPRD_RNDIS_SLOTS; i++) {
		if (vxtux_sprd_rndis[i].params == params) {
			st = &vxtux_sprd_rndis[i];
			break;
		}
		if (!vxtux_sprd_rndis[i].params && !free_slot)
			free_slot = &vxtux_sprd_rndis[i];
	}
	if (!st && create && free_slot) {
		memset(free_slot, 0, sizeof(*free_slot));
		free_slot->params = params;
		st = free_slot;
	}
	spin_unlock_irqrestore(&vxtux_sprd_rndis_lock, flags);

	return st;
}

/* ==================== 16 x ABI vendor -> library mainline ================ */

int sprd_rndis_msg_parser(struct rndis_params *params, u8 *buf)
{
	return rndis_msg_parser(params, buf);
}
EXPORT_SYMBOL(sprd_rndis_msg_parser);

struct rndis_params *sprd_rndis_register(void (*resp_avail)(void *v), void *v)
{
	return rndis_register(resp_avail, v);
}
EXPORT_SYMBOL(sprd_rndis_register);

void sprd_rndis_deregister(struct rndis_params *params)
{
	struct vxtux_sprd_rndis_state *st;

	rndis_deregister(params);

	st = vxtux_sprd_rndis_find(params, false);
	if (st)
		memset(st, 0, sizeof(*st));
}
EXPORT_SYMBOL(sprd_rndis_deregister);

int sprd_rndis_set_param_dev(struct rndis_params *params, struct net_device *dev,
			     u16 *cdc_filter)
{
	return rndis_set_param_dev(params, dev, cdc_filter);
}
EXPORT_SYMBOL(sprd_rndis_set_param_dev);

int sprd_rndis_set_param_vendor(struct rndis_params *params, u32 vendorID,
				const char *vendorDescr)
{
	return rndis_set_param_vendor(params, vendorID, vendorDescr);
}
EXPORT_SYMBOL(sprd_rndis_set_param_vendor);

int sprd_rndis_set_param_medium(struct rndis_params *params, u32 medium,
				u32 speed)
{
	return rndis_set_param_medium(params, medium, speed);
}
EXPORT_SYMBOL(sprd_rndis_set_param_medium);

void sprd_rndis_add_hdr(struct sk_buff *skb)
{
	rndis_add_hdr(skb);
}
EXPORT_SYMBOL(sprd_rndis_add_hdr);

int sprd_rndis_rm_hdr(struct gether *port, struct sk_buff *skb,
		      struct sk_buff_head *list)
{
	return rndis_rm_hdr(port, skb, list);
}
EXPORT_SYMBOL(sprd_rndis_rm_hdr);

u8 *sprd_rndis_get_next_response(struct rndis_params *params, u32 *length)
{
	return rndis_get_next_response(params, length);
}
EXPORT_SYMBOL(sprd_rndis_get_next_response);

void sprd_rndis_free_response(struct rndis_params *params, u8 *buf)
{
	rndis_free_response(params, buf);
}
EXPORT_SYMBOL(sprd_rndis_free_response);

void sprd_rndis_uninit(struct rndis_params *params)
{
	rndis_uninit(params);
}
EXPORT_SYMBOL(sprd_rndis_uninit);

int sprd_rndis_signal_connect(struct rndis_params *params)
{
	return rndis_signal_connect(params);
}
EXPORT_SYMBOL(sprd_rndis_signal_connect);

int sprd_rndis_signal_disconnect(struct rndis_params *params)
{
	return rndis_signal_disconnect(params);
}
EXPORT_SYMBOL(sprd_rndis_signal_disconnect);

void sprd_rndis_set_host_mac(struct rndis_params *params, const u8 *addr)
{
	rndis_set_host_mac(params, addr);
}
EXPORT_SYMBOL(sprd_rndis_set_host_mac);

/*
 * `borrow_net` di blob ada di sisi f_rndis (opts->net di +0xc8, flag bound di
 * +0xd0, lalu netdev_priv(net)+0x980 untuk cleanup) — sama dengan
 * rndis_borrow_net() mainline, yang diekspor usb_f_rndis. Delegasi ini karena
 * itu membuat modul kita bergantung pada USB_F_RNDIS (lihat Kconfig).
 */
void sprd_rndis_borrow_net(struct usb_function_instance *f, struct net_device *net)
{
	rndis_borrow_net(f, net);
}
EXPORT_SYMBOL(sprd_rndis_borrow_net);

/* =============== helper khusus vendor (tanpa padanan mainline) =========== */

/*
 * Blob 0x2adc: `strb w1, [x0, #0x34]` + pr_debug -> setter sederhana untuk
 * field u8 di struct rndis_params versi fork. Implementasi di sini menyimpan
 * nilai yang sama (state per-params di atas) supaya kontraknya tetap utuh.
 */
void sprd_rndis_set_max_pkt_xfer(struct rndis_params *params, u8 max_pkt_xfer)
{
	struct vxtux_sprd_rndis_state *st = vxtux_sprd_rndis_find(params, true);

	if (!st) {
		pr_warn_once("%s: tabel max_pkt_xfer penuh (%d slot) — params=%px\n",
			     __func__, VXTUX_SPRD_RNDIS_SLOTS, params);
		return;
	}

	WRITE_ONCE(st->max_pkt_xfer, max_pkt_xfer);
}
EXPORT_SYMBOL(sprd_rndis_set_max_pkt_xfer);

/*
 * Bukan ABI vendor (tidak ada di ksymtab blob): pembaca pasangan setter di atas,
 * dipakai sisi fungsi RNDIS yang ingin menghormati nilai itu (mis. saat menyusun
 * jawaban OID_GEN_MAXIMUM_TOTAL_SIZE / init-cmplt MaxPacketsPerTransfer).
 */
u8 sprd_rndis_get_max_pkt_xfer(struct rndis_params *params)
{
	struct vxtux_sprd_rndis_state *st = vxtux_sprd_rndis_find(params, false);

	return st ? READ_ONCE(st->max_pkt_xfer) : 0;
}
EXPORT_SYMBOL_GPL(sprd_rndis_get_max_pkt_xfer);

MODULE_DESCRIPTION("Unisoc sprd_usb_f_rndis ABI provider (VXTux port, 6.18)");
MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_LICENSE("GPL");
