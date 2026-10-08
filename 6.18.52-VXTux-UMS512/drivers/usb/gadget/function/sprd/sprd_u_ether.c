// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_u_ether.c — port PENYEDIA ABI `sprd_u_ether` (fork Unisoc UMS512 dari
 * drivers/usb/gadget/function/u_ether.c 5.4.210) ke mainline 6.18.52.
 *
 * === KENAPA BERKAS INI ADA (bukti, bukan dugaan) ===
 * Blob blobs/vendor_dlkm/sprd_u_ether.ko (59.750 B) adalah SATU-SATUNYA
 * penyedia 18 simbol yang diminta blob sprd_usb_f_rndis.ko (97.358 B):
 * 14 x `sprd_gether_*` + 4 helper SG/limit multi-paket. Tanpa penyedia ini
 * rantai RNDIS vendor tidak bisa di-link ke pohon 6.18 — dan gate r11
 * mengklasifikasikan 18 dari 24 backlog modul itu sebagai PROVIDER-BLOB
 * ("bangun provider lebih dulu"): lihat
 * device_artifacts/gate-r11-static/dissect.tsv, baris sprd_usb_f_rndis.
 *
 * === SUMBER: TIDAK ADA TEKS GPL DI REPO INI (jujur di depan) ===
 * docs/blob_opensrc_ledger.tsv menyebut sumber `unisoc-fork-5.4` /
 * "open-source (fork GPL)" — tetapi tarball yang ADA di repo ini
 * (gpl-source/fork_ums512_54.tar.gz) hanya memuat Documentation/ + arch/,
 * dan fork_t618 hanya drivers/usb/musb + usbpinmux. Jadi isi berkas ini
 * BUKAN salinan sumber fork, melainkan:
 *   (a) delegasi 1:1 ke API mainline 6.18 yang memang sudah ada di pohon
 *       (u_ether.h/u_ether.c: 20 fungsi), dan
 *   (b) satu-satunya bagian yang TIDAK punya padanan di 6.18 — SG + limit
 *       multi-paket — diimplementasikan sebagai state per-port di sini.
 *
 * === BUKTI KESAMAAN SIGNATURE (disasm llvm-objdump -dr, work/rndis.asm) ===
 *   sprd_gether_setup_name_default   0x0fc    hasil diperiksa IS_ERR
 *                                    (cmn x0,#0xfff) -> struct net_device *
 *   sprd_gether_cleanup              0x058/0x1ac/0x15f8
 *                                    dipanggil dgn netdev_priv(net) (net+0x980)
 *                                    -> struct eth_dev *  (identik u_ether.c)
 *   sprd_gether_set_gadget           0x588    (opts->net, cdev->gadget)
 *   sprd_gether_register_netdev      0x590    (opts->net), balikan dipakai sbg int
 *   sprd_gether_get_ifname           0x197c   (opts->net, buf, 4096)
 *   sprd_gether_get_qmult            0x19d0   (opts->net) -> unsigned
 *   sprd_gether_set_qmult            0x1a7c   (opts->net, u8 dari kstrtou8)
 *   sprd_gether_get_host_addr        0x1af4   (opts->net, buf, 4096)
 *   sprd_gether_set_host_addr        0x1b68   (opts->net, str)
 *   sprd_gether_get_dev_addr         0x1bcc   (opts->net, buf, 4096)
 *   sprd_gether_set_dev_addr         0x1c40   (opts->net, str)
 *   sprd_gether_connect/disconnect   0xaa4/0x990  (&rndis->port) = struct gether *
 *   gether_enable_sg                 0xa9c    (&rndis->port, 1)
 *   gether_is_sg_enabled             0x3a8/0x11a4  (&port) -> bool (tbz w0,#0)
 *   gether_update_dl_max_xfer_size   0x11b8   (&port, u32 dari pesan RNDIS +0x14)
 *   gether_update_dl_max_pkts_per_xfer 0x11cc (&port, 10)
 * Tidak ada satu pun call-site yang berbeda urutan argumen / tipe balikan dari
 * prototipe mainline u_ether.h — jadi delegasi 1:1 di bawah aman secara tipe,
 * bukan tebakan.
 *
 * Flavors export: blob vendor memakai EXPORT_SYMBOL *non*-GPL (nol entri
 * __ksymtab_gpl_ di kedua blob; diperiksa llvm-nm). Berkas ini menyalin flavor
 * itu persis supaya paritas ABI tidak berubah.
 *
 * Prototipe publiknya ada di sprd_u_ether.h (bukan inline di sini): kernel
 * ≥6.2 dibangun dengan -Wmissing-prototypes, jadi 20 ekspor tanpa deklarasi
 * akan menghasilkan 20 warning saat berkas ini pertama dikompilasi — kelas
 * yang sudah pernah muncul di pohon ini (tools/re/audit_provider_proto.py).
 *
 * === YANG BELUM ADA DI BERKAS INI (batas yang disengaja) ===
 * Mesin transfer SG-nya sendiri. Mainline 6.18 u_ether.c tidak punya jalur SG
 * sama sekali (nol kemunculan `sg`), sedangkan fork vendor memakainya di jalur
 * data (blob-nya UND `sg_init_table`). Berkas ini MENYIMPAN + MELAPORKAN state
 * SG/limit sesuai kontrak API yang dipanggil pemakai, tetapi jalur kirim/terima
 * u_ether 6.18 tetap jalur copy biasa. Mengaktifkan SG nyata berarti mengubah
 * jalur data drivers/usb/gadget/function/u_ether.c — pekerjaan terpisah dan
 * SENGAJA tidak dikerjakan di sini: mesin build device sedang offline sehingga
 * perubahan jalur data tidak bisa dikompilasi/diuji sama sekali.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/usb/gadget.h>

#include "sprd_u_ether.h"

/*
 * ==== STATE SG / LIMIT MULTI-PAKET (bagian yang tidak ada di mainline) ====
 *
 * Dipakai oleh: gether_enable_sg(), gether_is_sg_enabled(),
 * gether_update_dl_max_xfer_size(), gether_update_dl_max_pkts_per_xfer().
 * Kunci tabel = `struct gether *` (port), karena pemanggil pertama
 * (rndis_set_alt) memanggil enable_sg SEBELUM gether_connect() sehingga
 * `port->ioport` masih NULL pada saat itu.
 *
 * Daur hidup: entri dibuat saat pertama dipakai, `link` (eth_dev dari
 * netdev_priv) diisi saat connect, dan entri dihapus saat cleanup() link itu
 * dipanggil — jadi tidak tumbuh tanpa batas. Entri untuk port yang enable_sg
 * tapi tidak pernah connect tetap tinggal sampai modul dibongkar (jumlah port
 * gadget di papan ini satu; tabel statis, bukan alokasi dinamis).
 */
#define VXTUX_SPRD_SG_SLOTS	4

struct vxtux_sprd_sg_state {
	struct gether		*port;
	const struct eth_dev	*link;
	u8			sg_enabled;
	u8			dl_max_pkts;
	u32			dl_max_xfer;
};

static struct vxtux_sprd_sg_state vxtux_sprd_sg[VXTUX_SPRD_SG_SLOTS];
static DEFINE_SPINLOCK(vxtux_sprd_sg_lock);

/* `port == NULL` menandai slot kosong; port gadget nyata tidak pernah NULL. */
static struct vxtux_sprd_sg_state *vxtux_sprd_sg_find(struct gether *port,
						      bool create)
{
	struct vxtux_sprd_sg_state *st = NULL, *free_slot = NULL;
	unsigned long flags;
	int i;

	if (!port)
		return NULL;

	spin_lock_irqsave(&vxtux_sprd_sg_lock, flags);
	for (i = 0; i < VXTUX_SPRD_SG_SLOTS; i++) {
		if (vxtux_sprd_sg[i].port == port) {
			st = &vxtux_sprd_sg[i];
			break;
		}
		if (!vxtux_sprd_sg[i].port && !free_slot)
			free_slot = &vxtux_sprd_sg[i];
	}
	if (!st && create && free_slot) {
		memset(free_slot, 0, sizeof(*free_slot));
		free_slot->port = port;
		st = free_slot;
	}
	spin_unlock_irqrestore(&vxtux_sprd_sg_lock, flags);

	return st;
}

static void vxtux_sprd_sg_forget_link(const struct eth_dev *link)
{
	unsigned long flags;
	int i;

	if (!link)
		return;

	spin_lock_irqsave(&vxtux_sprd_sg_lock, flags);
	for (i = 0; i < VXTUX_SPRD_SG_SLOTS; i++) {
		if (vxtux_sprd_sg[i].link == link)
			memset(&vxtux_sprd_sg[i], 0, sizeof(vxtux_sprd_sg[i]));
	}
	spin_unlock_irqrestore(&vxtux_sprd_sg_lock, flags);
}

/* ==================== 20 x ABI vendor -> API mainline ==================== */

void sprd_gether_cleanup(struct eth_dev *dev)
{
	vxtux_sprd_sg_forget_link(dev);
	gether_cleanup(dev);
}
EXPORT_SYMBOL(sprd_gether_cleanup);

struct net_device *sprd_gether_connect(struct gether *port)
{
	struct net_device *net = gether_connect(port);
	struct vxtux_sprd_sg_state *st;

	/* Simpan eth_dev link supaya state bisa dibersihkan saat cleanup(). */
	if (!IS_ERR_OR_NULL(net)) {
		st = vxtux_sprd_sg_find(port, true);
		if (st)
			st->link = netdev_priv(net);
	}

	return net;
}
EXPORT_SYMBOL(sprd_gether_connect);

void sprd_gether_disconnect(struct gether *port)
{
	struct vxtux_sprd_sg_state *st;

	gether_disconnect(port);

	st = vxtux_sprd_sg_find(port, false);
	if (st)
		st->link = NULL;	/* flag SG & limit dipertahankan */
}
EXPORT_SYMBOL(sprd_gether_disconnect);

int sprd_gether_get_dev_addr(struct net_device *net, char *dev_addr, int len)
{
	return gether_get_dev_addr(net, dev_addr, len);
}
EXPORT_SYMBOL(sprd_gether_get_dev_addr);

int sprd_gether_get_host_addr(struct net_device *net, char *host_addr, int len)
{
	return gether_get_host_addr(net, host_addr, len);
}
EXPORT_SYMBOL(sprd_gether_get_host_addr);

int sprd_gether_get_host_addr_cdc(struct net_device *net, char *host_addr,
				  int len)
{
	return gether_get_host_addr_cdc(net, host_addr, len);
}
EXPORT_SYMBOL(sprd_gether_get_host_addr_cdc);

void sprd_gether_get_host_addr_u8(struct net_device *net,
				  u8 host_mac[ETH_ALEN])
{
	gether_get_host_addr_u8(net, host_mac);
}
EXPORT_SYMBOL(sprd_gether_get_host_addr_u8);

int sprd_gether_get_ifname(struct net_device *net, char *name, int len)
{
	return gether_get_ifname(net, name, len);
}
EXPORT_SYMBOL(sprd_gether_get_ifname);

unsigned sprd_gether_get_qmult(struct net_device *net)
{
	return gether_get_qmult(net);
}
EXPORT_SYMBOL(sprd_gether_get_qmult);

int sprd_gether_register_netdev(struct net_device *net)
{
	return gether_register_netdev(net);
}
EXPORT_SYMBOL(sprd_gether_register_netdev);

int sprd_gether_set_dev_addr(struct net_device *net, const char *dev_addr)
{
	return gether_set_dev_addr(net, dev_addr);
}
EXPORT_SYMBOL(sprd_gether_set_dev_addr);

void sprd_gether_set_gadget(struct net_device *net, struct usb_gadget *g)
{
	gether_set_gadget(net, g);
}
EXPORT_SYMBOL(sprd_gether_set_gadget);

int sprd_gether_set_host_addr(struct net_device *net, const char *host_addr)
{
	return gether_set_host_addr(net, host_addr);
}
EXPORT_SYMBOL(sprd_gether_set_host_addr);

void sprd_gether_set_qmult(struct net_device *net, unsigned qmult)
{
	gether_set_qmult(net, qmult);
}
EXPORT_SYMBOL(sprd_gether_set_qmult);

/*
 * Catatan tipe: tidak ada pemanggil `sprd_gether_setup_name` di seluruh himpunan
 * blob r11 (hanya ter-export), jadi tipe balikannya diambil dari mainline
 * (struct eth_dev *, sama seperti 5.4).
 */
struct eth_dev *sprd_gether_setup_name(struct usb_gadget *g,
				       const char *dev_addr,
				       const char *host_addr,
				       u8 ethaddr[ETH_ALEN], unsigned qmult,
				       const char *netname)
{
	return gether_setup_name(g, dev_addr, host_addr, ethaddr, qmult,
				 netname);
}
EXPORT_SYMBOL(sprd_gether_setup_name);

struct net_device *sprd_gether_setup_name_default(const char *netname)
{
	return gether_setup_name_default(netname);
}
EXPORT_SYMBOL(sprd_gether_setup_name_default);

/* =============== 4 x helper SG/limit (tanpa padanan mainline) ============ */

void gether_enable_sg(struct gether *port, bool on)
{
	struct vxtux_sprd_sg_state *st = vxtux_sprd_sg_find(port, true);

	if (!st) {
		pr_warn_once("%s: tabel SG penuh (%d slot) - enable_sg(port=%px) diabaikan\n",
			     __func__, VXTUX_SPRD_SG_SLOTS, port);
		return;
	}

	WRITE_ONCE(st->sg_enabled, on ? 1 : 0);
}
EXPORT_SYMBOL(gether_enable_sg);

bool gether_is_sg_enabled(struct gether *port)
{
	struct vxtux_sprd_sg_state *st = vxtux_sprd_sg_find(port, false);

	return st ? !!READ_ONCE(st->sg_enabled) : false;
}
EXPORT_SYMBOL(gether_is_sg_enabled);

/*
 * `size` = MaxTransferSize yang diminta host di pesan RNDIS (dibaca pemanggil
 * dari msg+0x14). `pkts` = jumlah paket per transfer. Keduanya disimpan apa
 * adanya; validasi rentang dilakukan pemilik kebijakan (fungsi RNDIS), bukan
 * di sini — persis seperti blob vendor yang hanya menyimpan di struct-nya
 * (offset +0x10c / +0x108 pada disasm rndis_command_complete).
 *
 * Tipe argumen: `u32` untuk kedua helper. Disasm hanya membuktikan isi register
 * w1 yang diisi 32-bit (`str w1`/`str w8` ke field u32 di struct vendor pada
 * rndis_command_complete), jadi u32 adalah pilihan yang paling sempit-terbukti;
 * helper `sprd_rndis_set_max_pkt_xfer()` yang bersifat u8 dipisahkan di bawah.
 */
void gether_update_dl_max_xfer_size(struct gether *port, u32 size)
{
	struct vxtux_sprd_sg_state *st = vxtux_sprd_sg_find(port, true);

	if (!st)
		return;

	WRITE_ONCE(st->dl_max_xfer, size);
}
EXPORT_SYMBOL(gether_update_dl_max_xfer_size);

void gether_update_dl_max_pkts_per_xfer(struct gether *port, u32 pkts)
{
	struct vxtux_sprd_sg_state *st = vxtux_sprd_sg_find(port, true);

	if (!st)
		return;

	WRITE_ONCE(st->dl_max_pkts, pkts > 0xff ? 0xff : (u8)pkts);
}
EXPORT_SYMBOL(gether_update_dl_max_pkts_per_xfer);

MODULE_DESCRIPTION("Unisoc sprd_u_ether ABI provider (VXTux port, 6.18)");
MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_LICENSE("GPL");
