// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_mbox_vendor.c — API mailbox vendor Unisoc (mbox_raw_sent dkk) di atas
 * kerangka mailbox generic mainline.
 *
 * Masalah yang dipecahkan:
 *   Seluruh konsumen vendor (SIPC: sipc.c/smsg.c, audio AGDSP: agdsp_access.c,
 *   audio-smsg.c, WCN) memanggil API mailbox vendor yang di kernel 5.4 STOK
 *   disediakan oleh vmlinux (bukan oleh blob .ko mana pun — sudah diverifikasi
 *   lewat database RE `blob_audit_syms.csv`, tidak ada blob yang meng-ekspornya).
 *   Di mainline 6.18 hanya ada CONTROLLER mailbox generic
 *   (drivers/mailbox/sprd-mailbox.c, CONFIG_SPRD_MBOX) tanpa API vendor itu.
 *
 * Solusi: implementasi ulang 5 fungsi vendor di atas API generic:
 *   mbox_raw_sent(id, msg)        -> mbox_send_message(chan[id], data[2])
 *   mbox_just_sent(id, msg)       -> idem, tanpa menunggu (fire and forget)
 *   mbox_register_irq_handle()    -> simpan handler, dipanggil dari rx_callback
 *   mbox_unregister_irq_handle()  -> hapus handler
 *   mbox_core_fifo_full()         -> status FIFO (lihat catatan)
 *
 * KONTEKS PEMANGGILAN (penting, sudah diverifikasi di kode konsumen):
 *   `smsg_send()` (smsg.c) memanggil mbox lewat txirq_trigger() DI DALAM
 *   `spin_lock_irqsave()` — jadi `mbox_raw_sent()` harus aman dipanggil dengan
 *   IRQ mati. Karena itu jalur cepat (kanal sudah ada) sepenuhnya bebas lock,
 *   dan jalur lambat (membuat kanal, butuh tidur) menolak jalan di konteks
 *   atomik. Kanal sendiri dibuat lebih dulu oleh `mbox_register_irq_handle()`
 *   saat probe konsumen (konteks proses), jadi jalur lambat normalnya tidak
 *   pernah dipakai dari jalur data.
 *
 * Pemetaan DT (WAJIB, ditulis di board DTS VXTux):
 *   vxtux_mbox: mailbox-client {
 *           compatible = "sprd,vxtux-mbox-client";
 *           // indeks ke-N di daftar ini = target_id / core_id N
 *           mboxes = <&mailbox 0>, <&mailbox 1>, ...;
 *   };
 *   Index ke-N dipakai apa adanya sebagai argumen sel controller
 *   (sprd-mailbox xlate memakai sel itu sebagai tujuan/core id).
 *
 * Catatan kejujuran porting:
 *   - mbox_core_fifo_full() mengembalikan 0 ("tidak penuh"). Register status
 *     FIFO outbox tidak diekspos controller mainline. Pemakainya di build
 *     mailbox adalah `smsg_senddie()` (smsg.c:607): > 0 berarti "txbuf penuh"
 *     dan pesannya langsung -EBUSY. Mengembalikan 0 = optimistis: pesan tetap
 *     dikirim. Ini pilihan yang benar (1 akan memblokir smsg_senddie selamanya),
 *     tapi konsekuensinya: bila outbox benar-benar penuh, kegagalan akan terlihat
 *     sebagai mbox_send_message() = -EBUSY dan di-drop oleh mbox_just_sent()
 *     (API vendor memang void). Kalau nanti perlu presisi, tambahkan helper
 *     status FIFO di drivers/mailbox/sprd-mailbox.c.
 *   - Provider ini di-build built-in (bool) dan TIDAK menyediakan .remove:
 *     melepas kanal saat konsumen (yang juga built-in dan menyimpan kanalnya
 *     seumur hidup) masih memakainya hanya akan menciptakan use-after-free.
 *     Unbind tidak didukung; node DT selalu ada sehingga tidak ada jalur normal
 *     yang membutuhkannya.
 */

#define pr_fmt(fmt) "[vxtux-mbox] " fmt

#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mailbox_client.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/preempt.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/soc/sprd/sprd_mailbox.h>
#include "sipc_debugfs.h"	/* prototipe mbox_init_debugfs() */

/* Target/core SIPC maksimum di SoC Unisoc (SIPC_ID_NR < 16). */
#define VMB_MAX_CORE	16

/*
 * Satu struct mbox_client per core: mainline hanya memberi (client, mssg) pada
 * rx_callback, jadi routing "irq dari core mana" harus dari container_of().
 */
struct vmb_core {
	struct mbox_client cl;
	struct mbox_chan *chan;
	struct sprd_mbox_vendor *vm;
	u8 id;
	/* dipublikasikan ke ISR: store_release/load_acquire */
	bool has_handler;
	MBOX_FUNCALL handler;
	void *hpriv;
};

struct sprd_mbox_vendor {
	struct device *dev;
	struct mutex lock;	/* hanya untuk jalur pembuatan kanal */
	struct vmb_core *core[VMB_MAX_CORE];
	atomic_t tx_count;
	atomic_t rx_count;
};

static struct sprd_mbox_vendor *g_vmb;

/*
 * Konteks: dipanggil SINKRON dari ISR controller (mbox_chan_received_data()),
 * jadi `mssg` menunjuk ke buffer 8 byte milik ISR dan hanya sah selama callback
 * ini berjalan — konsumen (smsg_process) memakainya langsung tanpa menyimpan.
 */
static void vmb_rx_callback(struct mbox_client *cl, void *mssg)
{
	struct vmb_core *c = container_of(cl, struct vmb_core, cl);

	atomic_inc(&c->vm->rx_count);
	pr_debug("rx dari core %u\n", c->id);

	/*
	 * Pasangan acquire dari smp_store_release() di mbox_register_irq_handle():
	 * tanpa ini, di ARM64 ISR bisa melihat has_handler==true tapi pointer
	 * handler/hpriv masih basi (belum ter-publish) -> lompat ke alamat sampah.
	 */
	if (!smp_load_acquire(&c->has_handler))
		return;

	/* API vendor: irqreturn_t (*)(void *ptr, void *private) */
	c->handler(mssg, c->hpriv);
}

/*
 * Buat kanal untuk core_id. Pemanggil memegang vm->lock; boleh tidur.
 */
static struct vmb_core *vmb_create_locked(struct sprd_mbox_vendor *vm, u8 id)
{
	struct vmb_core *c;
	int ret;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	c->vm = vm;
	c->id = id;
	c->cl.dev = vm->dev;
	c->cl.rx_callback = vmb_rx_callback;
	c->cl.tx_block = false;	/* kirim non-blocking, seperti mbox_raw_sent vendor */
	c->cl.knows_txdone = false;

	c->chan = mbox_request_channel(&c->cl, id);
	if (IS_ERR(c->chan)) {
		ret = PTR_ERR(c->chan);
		/*
		 * -EPROBE_DEFER = controller mailbox belum siap; itu kondisi normal
		 * (driver core akan mengulang probe), jadi jangan berisik. Error lain
		 * (mis. 'mboxes' salah/kanal tidak ada) baru layak diperingatkan.
		 */
		if (ret != -EPROBE_DEFER)
			pr_warn("kanal core %u tidak tersedia: %d (cek 'mboxes' di DTS)\n",
				id, ret);
		kfree(c);
		return ERR_PTR(ret);
	}

	/* Publikasikan ke jalur data yang bebas lock (release). */
	smp_store_release(&vm->core[id], c);
	dev_dbg(vm->dev, "kanal core %u aktif\n", id);

	return c;
}

/*
 * Ambil kanal untuk core_id.
 *   - jalur cepat (kanal sudah ada): bebas lock, aman di konteks atomik/IRQ;
 *   - jalur lambat (buat kanal): butuh tidur, jadi DITOLAK bila IRQ mati atau
 *     di konteks atomik — pemanggil menerima -EAGAIN, bukan hang.
 */
static struct vmb_core *vmb_lookup(u8 id)
{
	struct sprd_mbox_vendor *vm;
	struct vmb_core *c;

	if (id >= VMB_MAX_CORE)
		return ERR_PTR(-EINVAL);

	vm = READ_ONCE(g_vmb);
	if (!vm)
		return ERR_PTR(-EPROBE_DEFER);

	c = smp_load_acquire(&vm->core[id]);
	if (c)
		return c;

	if (in_atomic() || irqs_disabled())
		return ERR_PTR(-EAGAIN);

	mutex_lock(&vm->lock);
	c = vm->core[id];
	if (!c)
		c = vmb_create_locked(vm, id);
	mutex_unlock(&vm->lock);

	return c;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int mbox_raw_sent(u8 target_id, u64 msg);
int mbox_raw_sent(u8 target_id, u64 msg)
{
	struct vmb_core *c = vmb_lookup(target_id);
	u32 data[2];

	if (IS_ERR(c))
		return PTR_ERR(c);

	data[0] = lower_32_bits(msg);
	data[1] = upper_32_bits(msg);

	atomic_inc(&c->vm->tx_count);

	return mbox_send_message(c->chan, data);
}
EXPORT_SYMBOL(mbox_raw_sent);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void mbox_just_sent(u8 core_id, u64 msg);
void mbox_just_sent(u8 core_id, u64 msg)
{
	int ret = mbox_raw_sent(core_id, msg);

	if (ret < 0 && ret != -EPROBE_DEFER)
		pr_debug("kirim ke core %u gagal: %d\n", core_id, ret);
}
EXPORT_SYMBOL(mbox_just_sent);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int mbox_register_irq_handle(u8 target_id, MBOX_FUNCALL irq_handler, void *priv_data);
int mbox_register_irq_handle(u8 target_id, MBOX_FUNCALL irq_handler, void *priv_data)
{
	struct vmb_core *c;

	if (!irq_handler)
		return -EINVAL;

	c = vmb_lookup(target_id);
	if (IS_ERR(c))
		return PTR_ERR(c);

	c->handler = irq_handler;
	c->hpriv = priv_data;
	/*
	 * Publikasikan ke ISR dengan release: pastikan handler & hpriv sudah
	 * terlihat sebelum has_handler=true (dibaca dengan acquire di atas).
	 */
	smp_store_release(&c->has_handler, true);

	pr_info("handler terdaftar untuk core %u\n", target_id);

	return 0;
}
EXPORT_SYMBOL(mbox_register_irq_handle);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int mbox_unregister_irq_handle(u8 target_id);
int mbox_unregister_irq_handle(u8 target_id)
{
	struct sprd_mbox_vendor *vm = READ_ONCE(g_vmb);
	struct vmb_core *c;

	if (target_id >= VMB_MAX_CORE || !vm)
		return -EINVAL;

	c = READ_ONCE(vm->core[target_id]);
	if (!c)
		return 0;

	/* Cabut dulu penandanya (release), baru pointer-nya: urutan
	 * kebalikan dari saat publikasi di register. */
	smp_store_release(&c->has_handler, false);
	c->handler = NULL;
	c->hpriv = NULL;

	return 0;
}
EXPORT_SYMBOL(mbox_unregister_irq_handle);

/*
 * Status FIFO outbox tidak diekspos controller mainline: 0 = "tidak penuh".
 * Dipakai sipc.c (sipc_rxirq_status) dan, lebih penting, smsg_senddie() di
 * smsg.c:607 — lihat catatan di kepala berkas.
 */
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
u32 mbox_core_fifo_full(int core_id);
u32 mbox_core_fifo_full(int core_id)
{
	return 0;
}
EXPORT_SYMBOL(mbox_core_fifo_full);

/* Dipanggil sipc_debugfs.c (deklarasi di sipc_debugfs.h). */
int mbox_init_debugfs(void *root)
{
	struct sprd_mbox_vendor *vm = READ_ONCE(g_vmb);
	struct dentry *d;

	if (!root || !vm)
		return -ENODEV;

	d = debugfs_create_dir("mbox_vendor", root);
	if (IS_ERR_OR_NULL(d))
		return d ? PTR_ERR(d) : -ENOMEM;

	debugfs_create_atomic_t("tx_count", 0444, d, &vm->tx_count);
	debugfs_create_atomic_t("rx_count", 0444, d, &vm->rx_count);

	return 0;
}

static int sprd_mbox_vendor_probe(struct platform_device *pdev)
{
	struct sprd_mbox_vendor *vm;

	if (!pdev->dev.of_node)
		return -ENODEV;

	/* Cek dulu sebelum mengalokasi: hanya boleh ada satu provider. */
	if (READ_ONCE(g_vmb))
		return -EBUSY;

	vm = devm_kzalloc(&pdev->dev, sizeof(*vm), GFP_KERNEL);
	if (!vm)
		return -ENOMEM;

	vm->dev = &pdev->dev;
	mutex_init(&vm->lock);
	atomic_set(&vm->tx_count, 0);
	atomic_set(&vm->rx_count, 0);

	platform_set_drvdata(pdev, vm);
	WRITE_ONCE(g_vmb, vm);

	pr_info("provider mailbox vendor siap (%s)\n", dev_name(&pdev->dev));

	return 0;
}

static const struct of_device_id sprd_mbox_vendor_of_match[] = {
	{ .compatible = "sprd,vxtux-mbox-client" },
	{ }
};
MODULE_DEVICE_TABLE(of, sprd_mbox_vendor_of_match);

/*
 * Tanpa .remove: kanal dilepas hanya kalau provider dibongkar, sedangkan
 * konsumen (SIPC/audio, built-in) menyimpan kanalnya seumur hidup — melepasnya
 * di sini akan jadi use-after-free. Unbind provider tidak didukung
 * (lihat catatan kepala berkas).
 */
static struct platform_driver sprd_mbox_vendor_driver = {
	.probe = sprd_mbox_vendor_probe,
	.driver = {
		.name = "sprd-vxtux-mbox-client",
		.of_match_table = sprd_mbox_vendor_of_match,
	},
};

/*
 * Level init tinggi: SIPC/audio memanggil API ini saat probe mereka, dan
 * provider harus sudah terdaftar lebih dulu (kalau belum, mereka -EPROBE_DEFER).
 */
static int __init sprd_mbox_vendor_init(void)
{
	return platform_driver_register(&sprd_mbox_vendor_driver);
}
postcore_initcall(sprd_mbox_vendor_init);

static void __exit sprd_mbox_vendor_exit(void)
{
	platform_driver_unregister(&sprd_mbox_vendor_driver);
}
module_exit(sprd_mbox_vendor_exit);

MODULE_DESCRIPTION("Unisoc vendor mailbox client API over mainline sprd-mailbox");
MODULE_AUTHOR("VXTux port");
MODULE_LICENSE("GPL v2");
