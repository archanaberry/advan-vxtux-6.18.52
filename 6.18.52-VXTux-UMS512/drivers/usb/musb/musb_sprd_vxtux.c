// SPDX-License-Identifier: GPL-2.0
/*
 * musb_sprd_vxtux.c — Unisoc sharkl5pro MUSB glue, port ke mainline 6.18.
 *
 * Sumber: GPL fork Samsung Tab A8 (T618/UMS512) drivers/usb/musb/musb_sprd.c
 *         (Spreadtrum 2018) — diverifikasi ABI-nya identik dengan blob Advan
 *         T1030 (blobs/re_notes/musb_sprd.sym: 16 EP, compatible sharkl5pro).
 *
 * Lingkup v1 (PIO, device/gadget-first):
 *   - child "musb-hdrc" gaya omap2430 (resource mem+irq diwarisi)
 *   - fifo table ASLI vendor: 16 EP, ram_bits 13 (dari source GPL, bukan tebakan)
 *   - xceiv legacy (usb_phy_generic) + extcon VBUS → usb_phy_set_event
 *   - ops: init/exit/enable/disable/set_vbus (+ OTG_EXT_CSR 0x34b host force)
 *   - clk "core_clk" opsional
 * Lingkup v2 (RE blob musb_sprd, 2026-09-25):
 *   - sysfs "usb_control" di kelas "usb_notify" (usb_data_enabled,
 *     musb_hostenable, current_speed, maximum_speed) = port fungsi blob
 *     usb_data_enabled_show/store + musb_sprd_usb_notify_init/exit
 *     (provenance: dumped/re/musb_sprd.ko.usb_data_enabled_*.dis.txt;
 *     format string identik fork musb_sprd.c:1516-1558)
 *   - work mode versi v2: hanya langkah mode
 * Lingkup v3 (RE bedah penuh sprd_musb_work blob, 2026-09-25):
 *   - blob: 561 instruksi, 17 string .rodata, 22 API unik — SEMUA terpeta
 *     (sisanya persis fork musb_sprd.c:990-1320; sidik jari string 17/17)
 *   - port: wake-lock pd + musb (statis 6.18: wakeup_source_register),
 *     regulator vbus host enable/disable, gadget state, host connect via
 *     charger_detect SDP/CDP, fifo reset per-EP mainline (deviasi D5),
 *     pm_runtime_suspend/resume kunci fifo, debounce ktime (D6), dedup
 *     pre_mode/pre_vbus_active, is_suspend guard.
 *   - DEVIASI tercatat (blob → versi ini, semua karena pohon):
 *     D1 otg_notify/send_otg_notify   = layer Android, tidak ada di 6.18
 *     D2 CONFIG_USB_NOTIFY_LAYER      = sama (kedua cabang tidak aktif)
 *     D3 pdhub-c2c + sc27xx_get_dr_swap_flag + switch_dpdm_to_usb
 *                                     = chain charger vendor (fase 2)
 *     D4 usb_phy_post_init            = tidak ada di mainline
 *     D5 musb_reset_all_fifo_2_default→ep_config_from_table (statis di core)
 *       → diganti reset per-EP mainline: clear TXCSL/RXCSL + flush fifo
 *         (musb_h_tx_flush_fifo/musb_h_rx_flush_fifo di fork punya pola
 *         yang sama — lihat fungsi reset_ep_fifo)
 *     D6 ktime_get_mono_fast_ns vs ktime_get: keduanya mono-nanosecond
 *     D7 musb_host_start (fork musb_host.c:3436) → musb_start mainline
 *     D9 musb->shutdowning tidak ada di core mainline → loop tunggu
 *        sebelum suspend dibuang (fork musb_sprd.c:1236-1240)
 * Fase 2 (belum di port ini): DMA engine vendor (sprd_musbhsdma @+0x1000),
 *   charger detect hsphy asli (charger_detect pengganti dijalankan bila PHY
 *   menyediakannya), syscon chip_id singlefifo, babble recovery, audio offload.
 * Catatan: fixup_ep0fifo milik core fork tidak diport (field tidak ada di
 *   core mainline); EP0 pakai fifo default core — uji di bench.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/extcon.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/of.h>
#include <linux/usb/phy.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>

#include "musb_core.h"
#include "musb_gadget.h"
#ifdef CONFIG_USB_SPRD_DMA
#include "sprd_musbhsdma.h"   /* B40: sprd_dma_interrupt + dma_init/exit */
#endif
/*
 * B45-fix3 (2026-09-27): KEDUA flavor config wajib bisa DIBANGUN.
 *
 * Keputusan B40 = jalur DMA (`CONFIG_USB_SPRD_DMA=y`, `MUSB_PIO_ONLY` mati),
 * tetapi fragmennya sendiri menyatakan rollback resmi: "nyalakan kembali
 * MUSB_PIO_ONLY=y". Sebelum perbaikan ini rollback itu MUSTAHIL dibangun:
 * tiga rujukan di bawah (MUSB_DMA_INTR_MASK_STATUS, sprd_dma_interrupt,
 * ops.dma_init/dma_exit) tidak dijaga `#ifdef`, sementara header penyedianya
 * dijaga `#ifdef`. Akibatnya build flavor PIO berhenti dengan identifier tak
 * dikenal, bukan dengan pesan yang berguna.
 *
 * Penjagaan ini juga yang benar secara semantik, bukan sekadar agar lolos
 * kompilasi: musb_core.c hanya menuntut ops->dma_init/dma_exit ketika
 * MUSB_PIO_ONLY tidak diset (musb_core.c:2421-2427). Flavor PIO karena itu
 * memang TIDAK boleh memasang kedua ops itu, dan tidak membaca register DMA
 * vendor sama sekali.
 */

#define DRV_NAME		"musb-sprd-vxtux"

/* --- register/bit vendor sharkl5pro (dari fork musb_regs.h) --- */
#define MUSB_OTG_EXT_CSR	0x34b
#define MUSB_HOST_FORCE_EN	0x01
#define MUSB_CLEAR_TXBUFF	0x10
#define MUSB_CLEAR_RXBUFF	0x20
#define MUSB_TX_CMPL_MODE	0x40

/* --- fifo config ASLI vendor (fork sprd_musb_hdrc_config) --- */
#define SPRD_MUSB_MAX_EP_NUM	16
#define SPRD_MUSB_RAM_BITS	13

static struct musb_fifo_cfg sprd_musb_device_mode_cfg[] = {
	MUSB_EP_FIFO_DOUBLE(1, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(1, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(2, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(2, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(3, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(3, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(4, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(4, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(5, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(5, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(6, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(6, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(7, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(7, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(8, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(8, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(9, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(9, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(10, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(10, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(11, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(11, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(12, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(12, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(13, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(13, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(14, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(14, FIFO_RX, 512),
	MUSB_EP_FIFO_DOUBLE(15, FIFO_TX, 512),
	MUSB_EP_FIFO_DOUBLE(15, FIFO_RX, 512),
};

/* Catatan: fork punya host_fifo_cfg (ekstensi vendor, single-fifo host mode)
 * — field itu TIDAK ADA di musb core mainline, jadi tidak diport. */

static struct musb_hdrc_config sprd_musb_hdrc_config = {
	.fifo_cfg	= sprd_musb_device_mode_cfg,
	.fifo_cfg_size	= (unsigned)ARRAY_SIZE(sprd_musb_device_mode_cfg),
	/* B40: fork punya .multipoint = true (musb_sprd.c:1046) — wajib:
	 * musb_core.c:2366 mematikan seluruh DMA bila multipoint 0.
	 * Klaim "false, PIO v1" port lama keliru; FIFO config vendor
	 * tidak memakai CSR MUSB_CSR0_MULTIPROCEP — dinonaktifkan. */
	.multipoint	= true,
	.num_eps	= SPRD_MUSB_MAX_EP_NUM,
	.ram_bits	= SPRD_MUSB_RAM_BITS,
};

struct sprd_glue {
	struct device		*dev;
	struct platform_device	*musb;
	struct clk		*clk;
	struct usb_phy		*xceiv;
	struct extcon_dev	*edev;
	struct extcon_dev	*id_edev;
	struct notifier_block	vbus_nb;
	struct notifier_block	id_nb;
	struct notifier_block	audio_nb;
	bool			dr_mode_host;
	int			host_disabled;	/* fork bitmask (show/store "enabled") */
	/* --- RE blob musb_sprd (2026-09-25), v3 semantik penuh ---
	 * Urutan & ukuran field dipetakan ke offset struct blob:
	 *   +0x50 dr_mode (aktif)   +0x54 wq_mode      +0x60 lock
	 *   +0x70 work              +0x199 vbus_active +0x1b0 data_enabled
	 *   +0x1b4 last_mode
	 * (bukti: dumped/re/musb_sprd.ko.usb_data_enabled_store.dis.txt)
	 * Field mode-kerja fork yang diport v3 (semantik blob sprd_musb_work):
	 *   pre_mode/pre_vbus_active (dedup), charging_mode, is_suspend,
	 *   vbus (regulator), pd_wake_lock + wake_lock (6.18: pointer),
	 *   last_event_us (debounce ktime, pengganti fork otg_notify).
	 */
	spinlock_t		lock;
	enum usb_dr_mode	dr_mode_cur;	enum usb_dr_mode	wq_mode;
	enum usb_dr_mode	last_mode;
	bool			vbus_active;
	bool			is_data_disabled;
	bool			data_enabled;	/* blob [0x1b0] (fork: negasi dari
						 * is_data_disabled) */
	bool			charging_mode;
	bool			is_suspend;
	enum usb_dr_mode	pre_mode;
	bool			pre_vbus_active;
	struct regulator	*vbus;
	struct wakeup_source	*pd_wake_lock;
	struct wakeup_source	*wake_lock;
	u64			last_event_us;
	struct work_struct	work;
	/* Bidang berikut ditambahkan 2026-10-06 dari fork T618 (nama sama,
	 * musb_sprd.c:85-102) + RE blob musb_sprd.ko:
	 *   recover_work        fork:1741 INIT_DELAYED_WORK; blob
	 *                       sprd_musb_recover @0x104478 queue 25 jiffies
	 *   id_active/audio_active  flag sumber notifier: blob musb_sprd_id_
	 *                       notifier @0x103ef4 (+0x31), musb_sprd_audio_
	 *                       notifier @0x103ff8 (+0x19)
	 *   retry_charger_detect   fork musb_sprd.c:102, dinyalakan di
	 *                       musb_sprd_retry_charger_detect (fork:890) */
	struct delayed_work	recover_work;
	bool			id_active;
	bool			audio_active;
	bool			retry_charger_detect;
};

/* xceiv sintetis USB2 (v1): musb core wajib punya musb->xceiv.
 * Fase 2: ganti dengan usb-phy asli dari DT (fork: hsphy).
 *
 * PENTING (bukti probe QEMU 2026-09-25): `usb_add_phy()` menolak PHY yang
 * punya `x->type != USB_PHY_TYPE_UNDEFINED` (drivers/usb/phy/phy.c:638-646:
 * "not accepting initialized PHY" -> -EINVAL). Karena itu `x->type` HARUS
 * dibiarkan UNDEFINED saat mendaftar; kernel yang menyetel type ke USB2
 * (atau arg kedua `usb_add_phy(x, USB_PHY_TYPE_USB2)`) hanya menyimpan nilai
 * itu ke `x->last_type` -- nomor type PHY itu sendiri tetap dari DT/core.
 * Menyetel `x->type` sebelum `usb_add_phy()` membuat probe glue GAGAL
 * permanen dengan EINVAL -- kegagalan inilah yang terlihat di peta probe:
 *   musb-sprd-vxtux 5fff0000.usb: not accepting initialized PHY -22
 */
static struct usb_phy *sprd_musb_create_xceiv(struct device *dev)
{
	struct usb_phy *x;
	int ret;

	x = devm_kzalloc(dev, sizeof(*x), GFP_KERNEL);
	if (!x)
		return ERR_PTR(-ENOMEM);
	x->otg = devm_kzalloc(dev, sizeof(*x->otg), GFP_KERNEL);
	if (!x->otg)
		return ERR_PTR(-ENOMEM);

	x->label	= "musb-sprd-vxtux";
	x->dev		= dev;
	x->otg->usb_phy	= x;
	x->otg->state	= OTG_STATE_B_IDLE;
	/* type tetap USB_PHY_TYPE_UNDEFINED sampai SETELAH add berhasil */
	ret = usb_add_phy(x, USB_PHY_TYPE_USB2);
	if (ret)
		return ERR_PTR(ret);
	x->type = USB_PHY_TYPE_USB2;
	return x;
}

/* ---------------- musb platform ops (v1) ---------------- */

static void sprd_musb_enable(struct musb *musb)
{
	struct sprd_glue *glue = dev_get_drvdata(musb->controller->parent);
	u8 otgextcsr;
	u8 devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	unsigned long flags;

	spin_lock_irqsave(&glue->lock, flags);
	glue->dr_mode_cur = glue->dr_mode_host ? USB_DR_MODE_HOST
					       : USB_DR_MODE_PERIPHERAL;
	spin_unlock_irqrestore(&glue->lock, flags);

	if (glue->dr_mode_host) {
		/* host: session + force host di register vendor OTG_EXT_CSR */
		devctl |= MUSB_DEVCTL_SESSION;
		musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
		otgextcsr = musb_readb(musb->mregs, MUSB_OTG_EXT_CSR);
		otgextcsr |= MUSB_HOST_FORCE_EN;
		if (musb->is_multipoint)
			otgextcsr |= MUSB_TX_CMPL_MODE;
		musb_writeb(musb->mregs, MUSB_OTG_EXT_CSR, otgextcsr);
		dev_info(glue->dev, "host enable, devctl=%02x\n", devctl);
	} else {
		/* device: soft-connect hanya saat gadget driver ter-bind */
		u8 pwr = musb_readb(musb->mregs, MUSB_POWER);

		if (musb->gadget_driver) {
			pwr |= MUSB_POWER_SOFTCONN;
			dev_info(glue->dev, "soft-connect\n");
		} else {
			pwr &= ~MUSB_POWER_SOFTCONN;
			dev_info(glue->dev, "soft-disconnect\n");
		}
		musb_writeb(musb->mregs, MUSB_POWER, pwr);
	}
}

static void sprd_musb_disable(struct musb *musb)
{
	struct sprd_glue *glue = dev_get_drvdata(musb->controller->parent);
	unsigned long flags;

	spin_lock_irqsave(&glue->lock, flags);
	glue->dr_mode_cur = USB_DR_MODE_UNKNOWN;
	spin_unlock_irqrestore(&glue->lock, flags);

	/* test mode off saat plug out/in */
	musb_writeb(musb->mregs, MUSB_TESTMODE, 0x0);
}

static irqreturn_t sprd_musb_interrupt(int irq, void *__hci)
{
	irqreturn_t retval = IRQ_NONE;
	struct musb *musb = __hci;
	u8 mask8;
	u16 mask16;
#ifdef CONFIG_USB_SPRD_DMA
	u32 reg_dma;
#endif
	unsigned long flags;

	spin_lock_irqsave(&musb->lock, flags);
	mask8 = musb_readb(musb->mregs, MUSB_INTRUSBE);
	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB) & mask8;
	mask16 = musb_readw(musb->mregs, MUSB_INTRTXE);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX) & mask16;
	mask16 = musb_readw(musb->mregs, MUSB_INTRRXE);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX) & mask16;

#ifdef CONFIG_USB_SPRD_DMA
	/* B40: pemisahan IRQ DMA vendor (fork musb_sprd.c:240, 249-252):
	 * status DMA terbaca SEBELUM musb_interrupt, handler vendor
	 * dipanggil SETELAHNYA, urutan sama dengan fork. */
	reg_dma = musb_readl(musb->mregs, MUSB_DMA_INTR_MASK_STATUS);
#endif

	if (musb->int_usb || musb->int_tx || musb->int_rx)
		retval = musb_interrupt(musb);

#ifdef CONFIG_USB_SPRD_DMA
	if (reg_dma)
		retval = sprd_dma_interrupt(musb, reg_dma);
#endif

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;
}

static int sprd_musb_init(struct musb *musb)
{
	struct sprd_glue *glue = dev_get_drvdata(musb->controller->parent);

	musb->xceiv = glue->xceiv;
	musb->isr = sprd_musb_interrupt;
	sprd_musb_enable(musb);
	return 0;
}

static int sprd_musb_exit(struct musb *musb)
{
	return 0;
}

static void sprd_musb_set_vbus(struct musb *musb, int is_on)
{
	struct usb_otg *otg = musb->xceiv->otg;
	u8 devctl;
	unsigned long timeout = 0;

	if (pm_runtime_suspended(musb->controller))
		return;

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	if (is_on) {
		if (otg->state == OTG_STATE_A_IDLE) {
			devctl |= MUSB_DEVCTL_SESSION;
			musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
			while (musb_readb(musb->mregs, MUSB_DEVCTL) &
			       MUSB_DEVCTL_BDEVICE) {
				if (++timeout > 1000) {
					dev_err(musb->controller,
						"A-device timeout\n");
					break;
				}
			}
			otg_set_vbus(otg, 1);
		} else {
			musb->is_active = 1;
			otg->default_a = 1;
			otg->state = OTG_STATE_A_WAIT_VRISE;
			devctl |= MUSB_DEVCTL_SESSION;
		}
	} else {
		musb->is_active = 0;
		otg->default_a = 0;
		otg->state = OTG_STATE_B_IDLE;
		devctl &= ~MUSB_DEVCTL_SESSION;
	}
	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
}

static void sprd_musb_try_idle(struct musb *musb, unsigned long timeout)
{
	/* v1: no-op (flush fifo DMA versi fork = fase 2) */
}

static const struct musb_platform_ops sprd_musb_ops = {
	/* B50 (2026-09-27, fase 2b): quirk kembali ke bit ASLI vendor
	 * MUSB_DMA_SPRD BIT(10) — port fork musb_core.h:174; bit kini ada di
	 * musb_core.h 6.18 (B50) dan dipilih musb_dma_sprd() (musb_dma.h B50)
	 * di musb_tx_dma_program + musb_ep_program + musb_advance_schedule.
	 * Sebelumnya menumpang MUSB_DMA_UX500 BIT(6) karena 6.18 belum punya
	 * bitnya; itu membuat host-TX masuk jalur mode "mentor" yang keliru
	 * untuk engine vendor. Gadget tidak menyentuh quirk ini. */
	.quirks		= MUSB_DMA_SPRD,
	.init		= sprd_musb_init,
	.exit		= sprd_musb_exit,
	.enable		= sprd_musb_enable,
	.disable	= sprd_musb_disable,
	.set_vbus	= sprd_musb_set_vbus,
	.try_idle	= sprd_musb_try_idle,
#ifdef CONFIG_USB_SPRD_DMA
	/* B45-fix3: hanya flavor DMA yang boleh memasang kedua ops ini — lihat
	 * catatan penjagaan di kepala berkas. */
	.dma_init	= sprd_musb_dma_controller_create,
	.dma_exit	= sprd_musb_dma_controller_destroy,
#endif
};

/* ---------------- helper mode-kerja (v3, provenance fork/blob) ---------- */

/* Fork musb_sprd_is_connect_host (musb_sprd.c:918): tipe charger dari PHY;
 * SDP/CDP = host. Deviasi D10: xceiv sintetis kita tak punya charger_detect,
 * jadi fallback = ANGGAP SDP (data-first) — tanpa ini colok PC jatuh ke
 * charging-only dan ADB mati, justru melawan fokus kompatibilitas. Ketika
 * PHY asli hsphy mendarat (fase 2) dengan charger_detect, nilai balik
 * otomatis mengikuti tipe SDP/CDP seperti fork. */
enum usb_charger_type musb_sprd_retry_charger_detect(struct sprd_glue *glue);
/* DEV-D14: titik masuk pemulihan (fork: sprd_musb_ops.recover). Field
 * `.recover` itu tambahan vendor di struct musb_platform_ops dan tidak ada di
 * 6.18, jadi fungsinya ber-prototype sendiri di sini supaya -Wmissing-prototypes
 * tetap bersih dan pemanggilnya jelas (+- nama tersebut). */
int sprd_musb_recover(struct musb *musb);

static bool sprd_musb_is_connect_host(struct sprd_glue *glue)
{
	struct usb_phy *usb_phy = glue->xceiv;
	enum usb_charger_type type;

	if (!usb_phy || !usb_phy->charger_detect)
		return true;
	type = usb_phy->charger_detect(usb_phy);
	dev_info(glue->dev, "%s type = %d\n", __func__, type);

	/*
	 * DEV-D13: fork musb_sprd.c:919-934 memanggil deteksi kedua UniSoc bila
	 * hasil pertama UNKNOWN:
	 *     if (type == UNKNOWN_TYPE && usb_phy->retry_charger_detect)
	 *             type = musb_sprd_retry_charger_detect(glue);
	 * Cabang itu TIDAK dapat ditulis di sini: `retry_charger_detect`
	 * adalah metode TAMBAHAN di struct usb_phy fork, dan struct usb_phy
	 * mainline 6.18 tidak memilikinya (terbukti dari kegagalan kompilasi
	 * nyata: "no member named 'retry_charger_detect' in 'struct usb_phy'").
	 * Yang bisa dipanggil hanya ->charger_detect. Karena itu fungsinya
	 * tetap ada lengkap sebagai titik masuk global
	 * (musb_sprd_retry_charger_detect) dan pemanggilan otomatisnya
	 * menunggu hsphy fase 2 — bukan pointer yang dikarang di sini.
	 */
	return type == SDP_TYPE || type == CDP_TYPE;
}

/* Deviasi D5: musb_reset_all_fifo_2_default blob/fork = ep_config_from_table
 * (statis di core mainline). Padanan semantik mainline: kosongkan endpoint
 * control/status + flush fifo per-EP — pola musb_h_{tx,rx}_flush_fifo fork
 * (musb_host.c) dan ep_config_from_table mainline. */
static void sprd_musb_reset_ep_fifo(struct musb *musb)
{
	void __iomem *mbase = musb->mregs;
	u8 epnum;

	for (epnum = 1; epnum < musb->nr_endpoints; epnum++) {
		struct musb_hw_ep *hw_ep = &musb->endpoints[epnum];
		void __iomem *epio = hw_ep->regs;

		musb_ep_select(mbase, epnum);
		if (!epio)
			continue;
		musb_writew(epio, MUSB_TXCSR,
			    MUSB_TXCSR_FLUSHFIFO | MUSB_TXCSR_CLRDATATOG);
		musb_writew(epio, MUSB_RXCSR,
			    MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_CLRDATATOG);
		musb_writew(epio, MUSB_TXMAXP, hw_ep->max_packet_sz_tx);
		musb_writew(epio, MUSB_RXMAXP, hw_ep->max_packet_sz_rx);
	}
	musb_ep_select(mbase, 0);
}

/* Port persis fork musb_sprd_release_all_request (musb_sprd.c:1358-1385):
 * disable endpoint yang punya DMA channel sebelum controller suspend. */
static void sprd_musb_release_all_request(struct musb *musb)
{
	struct musb_ep *musb_ep_in;
	struct musb_ep *musb_ep_out;
	struct musb_hw_ep *endpoints;
	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	u32 i;

	for (i = 1; i < musb->config->num_eps; i++) {
		endpoints = &musb->endpoints[i];
		if (!endpoints)
			continue;
		musb_ep_in = &endpoints->ep_in;
		if (musb_ep_in && musb_ep_in->dma) {
			ep_in = &musb_ep_in->end_point;
			usb_ep_disable(ep_in);
		}
		musb_ep_out = &endpoints->ep_out;
		if (musb_ep_out && musb_ep_out->dma) {
			ep_out = &musb_ep_out->end_point;
			usb_ep_disable(ep_out);
		}
	}
}

/* ---------------- sysfs usb_control + usb_notify (RE blob musb_sprd) -----
 *
 * Mekanisme penuh dari blob `musb_sprd` (string .rodata + bedah disasm
 * 2026-09-25); fungsi blob yang diberi port di sini:
 *   usb_data_enabled_show/store (dumped/re/musb_sprd.ko.usb_data_enabled_*.dis.txt
 *   — string format-nya identik dengan fork musb_sprd.c:1516-1558),
 *   musb_sprd_usb_notify_init/exit (kelas "usb_notify" + dev "usb_control"),
 *   dan pekerja mode sprd_musb_work — v3 SEMANTIK PENUH dari bedah blob
 *   (561 insn, 17/17 string, 22/22 API terpeta).
 */

/* Vektor antrian blob = CPU 32 (NULL cpumask) pada system_unbound_wq —
 * queue_work_on(0x20, system_unbound_wq, &glue->work) di disasm store. */
#define SPRD_MUSB_WORK_CPU	32
/* fork: cnt=100 × msleep(200) menunggu suspend; cnt=250 × msleep(20) resume */
#define SPRD_MUSB_SUSP_TRIES	100
#define SPRD_MUSB_RESUME_TRIES	250

/* fork musb_sprd.c:115 + __setup("androidboot.mode=") :938-948 — 6.18 kita
 * tak ada Android init, jadi jadi parameter modul: vxtux_musb.boot_charging=1
 * memaksa jalur charging-only saat boot (mode "cas saja", stabil untuk
 * recovery). Default 0 = perilaku normal. */
static int boot_charging;
module_param(boot_charging, int, 0444);
MODULE_PARM_DESC(boot_charging, "1 = paksa charging-only di boot (fork: androidboot.mode=charger)");

static void sprd_musb_data_work(struct work_struct *work)
{
	struct sprd_glue *glue = container_of(work, struct sprd_glue, work);
	struct musb *musb = platform_get_drvdata(glue->musb);
	bool current_state, charging_only = false;
	enum usb_dr_mode current_mode;
	unsigned long flags;
	int ret, cnt;

	if (!musb)
		return;

	dev_info(glue->dev, "%s enter!\n", __func__);
	spin_lock_irqsave(&glue->lock, flags);
	current_mode = glue->wq_mode;
	current_state = glue->vbus_active && !glue->is_data_disabled;
	glue->wq_mode = USB_DR_MODE_UNKNOWN;
	spin_unlock_irqrestore(&glue->lock, flags);

	if (current_mode == USB_DR_MODE_UNKNOWN)
		return;		/* Fork: kecepatan plug-in bisa membuat interupsi banjir melebihi
		 * kapasitas mekanisme work; jika state sama dengan sebelumnya,
		 * pertahankan state lama (hindari tunggu-suspend yang gagal). */
	if (glue->pre_vbus_active == current_state) {
		dev_err(glue->dev, "Same vbus_active: mode(%d %d), state(%d %d)\n",
			glue->pre_mode, current_mode,
			glue->pre_vbus_active, current_state);
		return;
	}
	glue->pre_mode = current_mode;
	glue->pre_vbus_active = current_state;

	__pm_stay_awake(glue->pd_wake_lock);		/* fork: menunggu resume — regulator vbus lewat i2c mati saat suspend.
		 * Alasan dedup plug-in cepat lihat fork sprd_musb_work komentar
		 * atas (interupsi banjir vs mekanisme work). */
	while (glue->is_suspend)
		msleep(20);

	if (current_mode == USB_DR_MODE_HOST && !musb->gadget_driver &&
	    glue->dr_mode_cur == USB_DR_MODE_UNKNOWN) {
		/* fork musb_host_start() (musb_host.c:3436) — inti: start
		 * controller dengan role host. Deviasi D7: musb_start. */
		spin_lock_irqsave(&musb->lock, flags);
		musb->is_active = 1;
		otg_set_peripheral(musb->xceiv->otg, &musb->g);
		spin_unlock_irqrestore(&musb->lock, flags);
		musb_start(musb);
	}

	spin_lock_irqsave(&glue->lock, flags);
	glue->dr_mode_cur = current_mode;
	glue->last_mode = glue->dr_mode_cur;
	spin_unlock_irqrestore(&glue->lock, flags);

	dev_err(musb->controller, "%s enter: vbus = %d mode = %d\n",
		__func__, current_state, current_mode);

	if (current_state) {
		if ((musb->g.state != USB_STATE_NOTATTACHED) &&
		    pm_runtime_active(musb->controller)) {
			dev_info(glue->dev, "musb device is resumed!\n");
			/* fork: pm_runtime_get_noresume agar usage_count +1 */
			if (glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
				pm_runtime_get_noresume(musb->controller);
			goto end;
		}

		if (glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			usb_gadget_set_state(&musb->g, USB_STATE_ATTACHED);

		if ((glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL &&
		     !sprd_musb_is_connect_host(glue)) || boot_charging) {
			/* charging-only: jangan resume, cukup cas (fork:1093-1104;
			 * D3: tanpa pdhub-c2c; param boot_charging = fork
			 * androidboot.mode=charger) */
			spin_lock_irqsave(&glue->lock, flags);
			glue->charging_mode = true;
			spin_unlock_irqrestore(&glue->lock, flags);
			dev_info(glue->dev,
				 "Don't need resume musb device in charging mode!\n");
			goto end;
		}

		if (glue->dr_mode_cur == USB_DR_MODE_HOST)
			MUSB_HST_MODE(musb);

		if (glue->dr_mode_cur == USB_DR_MODE_HOST) {
			if (!glue->vbus) {
				glue->vbus = devm_regulator_get(glue->dev,
								"vbus");
				if (IS_ERR(glue->vbus)) {
					dev_err(glue->dev,
						"unable to get vbus supply\n");
					glue->vbus = NULL;
					goto end;
				}
			}
			if (!regulator_is_enabled(glue->vbus)) {
				ret = regulator_enable(glue->vbus);
				if (ret) {
					dev_err(glue->dev,
						"Failed to enable vbus: %d\n",
						ret);
					goto end;
				}
			}
		}

		/* blob: __pm_runtime_suspend(RPM_GET_PUT) lalu tunggu suspend
		 * (fork: cnt=100 × msleep(200)) sebelum menyentuh fifo */
		__pm_runtime_suspend(musb->controller, RPM_GET_PUT);
		cnt = SPRD_MUSB_SUSP_TRIES;
		while (!pm_runtime_suspended(musb->controller) && --cnt > 0)
			msleep(200);
		if (cnt <= 0) {
			spin_lock_irqsave(&glue->lock, flags);
			glue->dr_mode_cur = USB_DR_MODE_UNKNOWN;
			glue->vbus = NULL;
			spin_unlock_irqrestore(&glue->lock, flags);
			dev_err(musb->controller,
				"Wait for musb controller enter suspend failed!\n");
			goto end;
		}

		/* fork: pm_runtime_get_sync sebelum konfigurasi ulang fifo.
		 * Koreksi kecil: fork menulis `if (ret)` padahal get_sync bisa
		 * balik 1 saat sukses (status sudah suspended) — cek < 0. */
		ret = pm_runtime_get_sync(musb->controller);
		if (ret < 0) {
			pm_runtime_put_noidle(musb->controller);
			dev_err(musb->controller,
				"musb controller pm_runtime_get_sync failed with %d.\n",
				ret);
		}
		/* Deviasi D5: musb_reset_all_fifo_2_default blob/fork =
		 * ep_config_from_table (statis di core mainline) — padanan:
		 * reset csr/flush fifo per-EP (pola fork). Jalur ini tidak
		 * bisa gagal cara yang sama, jadi pesan error konfigurasi fifo
		 * milik blob sengaja TIDAK dibawa. */
		sprd_musb_reset_ep_fifo(musb);

		spin_lock_irqsave(&glue->lock, flags);
		glue->charging_mode = false;
		if (glue->dr_mode_cur == USB_DR_MODE_HOST)
			musb->xceiv->otg->state = OTG_STATE_A_HOST;
		spin_unlock_irqrestore(&glue->lock, flags);

		__pm_stay_awake(glue->wake_lock);

		dev_info(glue->dev, "is running as %s\n",
			 glue->dr_mode_cur == USB_DR_MODE_HOST ?
			 "HOST" : "DEVICE");
		goto end;
	} else {
		spin_lock_irqsave(&glue->lock, flags);
		charging_only = glue->charging_mode;
		spin_unlock_irqrestore(&glue->lock, flags);
		usb_gadget_set_state(&musb->g, USB_STATE_NOTATTACHED);
		if (charging_only || pm_runtime_suspended(musb->controller)) {
			spin_lock_irqsave(&glue->lock, flags);
			glue->dr_mode_cur = USB_DR_MODE_UNKNOWN;
			spin_unlock_irqrestore(&glue->lock, flags);
			dev_info(glue->dev,
				 "musb device had been in suspend status!\n");
			goto end;
		}
		if (glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL) {
			u8 devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

			/* fork: bersihkan request + disable ep sebelum
			 * controller suspend (musb_sprd_release_all_request,
			 * musb_sprd.c:1358-1385 — port persis; usb_phy_post_init
			 * dilewati: D4) */
			musb_writeb(musb->mregs, MUSB_DEVCTL,
				    devctl & ~MUSB_DEVCTL_SESSION);
			spin_lock_irqsave(&musb->lock, flags);
			sprd_musb_release_all_request(musb);
			spin_unlock_irqrestore(&musb->lock, flags);
		}

		if (glue->dr_mode_cur == USB_DR_MODE_HOST && glue->vbus &&
		    regulator_is_enabled(glue->vbus)) {
			ret = regulator_disable(glue->vbus);
			if (ret) {
				dev_err(glue->dev,
					"Failed to disable vbus: %d\n", ret);
				goto end;
			}
		}

		/* fork: pm_runtime_mark_last_busy + put_autosuspend lalu
		 * tunggu child suspend (250 × 20 ms) */
		pm_runtime_mark_last_busy(musb->controller);
		pm_runtime_put_autosuspend(musb->controller);
		cnt = SPRD_MUSB_RESUME_TRIES;
		while (!pm_runtime_suspended(musb->controller) && --cnt > 0)
			msleep(20);
		if (cnt <= 0) {
			dev_err(musb->controller,
				"musb child device enters suspend failed!!!\n");
			goto end;
		}

		__pm_relax(glue->wake_lock);

		spin_lock_irqsave(&glue->lock, flags);
		glue->charging_mode = false;
		musb->xceiv->otg->default_a = 0;
		musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		glue->last_mode = glue->dr_mode_cur;
		glue->dr_mode_cur = USB_DR_MODE_UNKNOWN;
		spin_unlock_irqrestore(&glue->lock, flags);

		MUSB_DEV_MODE(musb);

		dev_info(glue->dev, "is shut down\n");
		goto end;
	}
end:
	__pm_relax(glue->pd_wake_lock);
}

static ssize_t usb_data_enabled_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", glue->data_enabled);
}

static ssize_t usb_data_enabled_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	int value = 0;
	int ret;
	unsigned long flags;
	/* blob: ktime_get_mono_fast_ns di store — dipakai sebagai debounce
	 * antar-event (interpretasi D6; fork tidak punya padanannya) */
	u64 now = (u64)ktime_get_mono_fast_ns();
	struct sprd_glue *glue = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		dev_err(dev, "input err:%d\n", ret);
		return count;
	}
	spin_lock_irqsave(&glue->lock, flags);
	/* debounce 20 ms (blob: ktime_get_mono_fast_ns di store; D6) */
	if (now - glue->last_event_us < 20 * NSEC_PER_MSEC) {
		spin_unlock_irqrestore(&glue->lock, flags);
		return count;
	}
	glue->last_event_us = now;
	dev_info(dev, "input value:%d\n", value);
	if (glue->is_data_disabled == !!value) {
		if (glue->last_mode == USB_DR_MODE_UNKNOWN) {
			dev_warn(dev, "last_mode=:%d to %d\n",
					glue->last_mode, glue->dr_mode_cur);
			glue->last_mode = glue->dr_mode_cur;
		}
		glue->is_data_disabled = !value;
		glue->data_enabled = !!value;
		glue->wq_mode = glue->last_mode;
		queue_work_on(SPRD_MUSB_WORK_CPU, system_unbound_wq,
			      &glue->work);
	} else {
		dev_info(dev, "ingnored:%d %d\n",
				glue->is_data_disabled, value);
	}
	dev_dbg(dev, "enabled:%d mode:%d vbus:%d\n",
			glue->data_enabled, glue->wq_mode, glue->vbus_active);
	spin_unlock_irqrestore(&glue->lock, flags);

	return count;
}
static DEVICE_ATTR_RW(usb_data_enabled);

/* fork: show mem-print string "enabled"/"disabled" dari bitmask
 * host_disabled, bukan angka; store menerima "enable"/"disable" lewat
 * strncmp dan juga disable/enable usbid_irq (IRQ ID-pin vendor — fase 2,
 * jadi di sini hanya flag-nya; deviasi tercatat). */
static ssize_t musb_hostenable_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n",
		       (glue->host_disabled & 0x01) ? "disabled" : "enabled");
}

static ssize_t musb_hostenable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);

	if (strncmp(buf, "disable", 7) == 0) {
		glue->host_disabled |= 1;
	} else if (strncmp(buf, "enable", 6) == 0) {
		glue->host_disabled &= ~0x01;
	} else {
		return 0;
	}
	return count;
}
static DEVICE_ATTR_RW(musb_hostenable);

static ssize_t current_speed_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(glue->musb);

	if (!musb)
		return -ENODEV;
	return sprintf(buf, "%s\n", usb_speed_string(musb->g.speed));
}
static DEVICE_ATTR_RO(current_speed);

static ssize_t maximum_speed_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(glue->musb);

	if (!musb)
		return -ENODEV;
	return sprintf(buf, "%s\n", usb_speed_string(musb->g.max_speed));
}

static ssize_t maximum_speed_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(glue->musb);
	unsigned int max_speed;
	int ret;

	if (!musb)
		return -EINVAL;
	ret = kstrtouint(buf, 0, &max_speed);
	if (ret)
		return -EINVAL;
	if (max_speed > USB_SPEED_SUPER)
		return -EINVAL;
	/* fork juga menulis musb->config->maximum_speed, tapi `config` kini
	 * const di mainline (musb_core.h:412); efek fungsionalnya hanya
	 * musb->g.max_speed. */
	musb->g.max_speed = max_speed;
	return count;
}
static DEVICE_ATTR_RW(maximum_speed);

static struct attribute *usb_control_attrs[] = {
	&dev_attr_usb_data_enabled.attr,
	&dev_attr_musb_hostenable.attr,
	&dev_attr_current_speed.attr,
	&dev_attr_maximum_speed.attr,
	NULL,
};
static const struct attribute_group usb_control_group = {
	.attrs = usb_control_attrs,
};

static struct class *usb_notify_class;
static struct device *usb_notify_dev;

/* musb_sprd_usb_notify_init blob: class_create("usb_notify") +
 * device_create(usb_notify_class, "usb_control") + sysfs_create_group. */
static int sprd_musb_usb_notify_init(struct device *dev)
{
	int ret;

	usb_notify_class = class_create("usb_notify");
	if (IS_ERR(usb_notify_class))
		return PTR_ERR(usb_notify_class);

	usb_notify_dev = device_create(usb_notify_class, dev, MKDEV(0, 0),
				       NULL, "usb_control");
	if (IS_ERR(usb_notify_dev)) {
		ret = PTR_ERR(usb_notify_dev);
		usb_notify_dev = NULL;
		goto err_class;
	}
	dev_set_drvdata(usb_notify_dev, dev_get_drvdata(dev));

	ret = sysfs_create_group(&usb_notify_dev->kobj, &usb_control_group);
	if (ret)
		goto err_dev;
	return 0;

err_dev:
	device_destroy(usb_notify_class, MKDEV(0, 0));
	usb_notify_dev = NULL;
err_class:
	class_destroy(usb_notify_class);
	usb_notify_class = NULL;
	return ret;
}

static void sprd_musb_usb_notify_exit(void)
{
	if (usb_notify_dev) {
		sysfs_remove_group(&usb_notify_dev->kobj, &usb_control_group);
		device_destroy(usb_notify_class, MKDEV(0, 0));
		usb_notify_dev = NULL;
	}
	if (usb_notify_class) {
		class_destroy(usb_notify_class);
		usb_notify_class = NULL;
	}
}

/* ---------------- extcon/VBUS/ID/audio → mode kerja ----------------
 *
 * BAGIAN INI DITAMBAHKAN 2026-10-06 dari RE blob vendor musb_sprd.ko
 * (Ghidra 12.1.4 headless; log lengkap work/ghidra_out/musb_re.log,
 * skrip work/ghidra/_musb_re.sh). Sebelum ini port kita punya
 * sprd_musb_data_work() tetapi TIDAK ADA satu pun pemanggil queue_work di
 * jalur bukan-sysfs: ketiga notifier vendor tidak diport, sehingga seluruh
 * mesin mode (host/device/charging) adalah kode mati kecuali lewat sysfs
 * usb_data_enabled. Itu sebabnya baris ledger musb_sprd berstatus PARTIAL.
 *
 * Semantik ketiga notifier diambil apa adanya dari dekompilasi:
 *   musb_sprd_vbus_notifier  @0x103df4  → wq_mode = 2 (PERIPHERAL)
 *   musb_sprd_id_notifier    @0x103ef4  → wq_mode = 1 (HOST)
 *   musb_sprd_audio_notifier @0x103ff8  → wq_mode = 1 (HOST) di kedua arah
 * Ketiganya: ambil spinlock, tolak event bila state sudah sama atau bila
 * dr_mode_cur sudah sama dengan mode yang akan ditulis, set flag sumber +
 * wq_mode, lepas lock, queue_work_on(32, system_unbound_wq, &glue->work),
 * dev_info satu baris, return 0. Guard "tolak bila dr_mode_cur == mode"
 * itulah yang membuat dedup pre_mode di data_work bekerja.
 */

static void sprd_musb_queue_mode(struct sprd_glue *glue, enum usb_dr_mode mode)
{
	unsigned long flags;

	spin_lock_irqsave(&glue->lock, flags);
	glue->wq_mode = mode;
	spin_unlock_irqrestore(&glue->lock, flags);
	queue_work_on(SPRD_MUSB_WORK_CPU, system_unbound_wq, &glue->work);
}

static int sprd_musb_vbus_notifier(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct sprd_glue *glue = container_of(nb, struct sprd_glue, vbus_nb);
	unsigned long flags;
	bool conn = !!event;

	spin_lock_irqsave(&glue->lock, flags);
	if (!conn) {
		if (!glue->vbus_active ||
		    glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			goto ignore_disconnect;
		glue->vbus_active = false;
		glue->wq_mode = USB_DR_MODE_PERIPHERAL;
	} else {
		if (glue->vbus_active || glue->dr_mode_cur == USB_DR_MODE_HOST)
			goto ignore_connect;
		glue->vbus_active = true;
		glue->wq_mode = USB_DR_MODE_PERIPHERAL;
	}
	spin_unlock_irqrestore(&glue->lock, flags);
	queue_work_on(SPRD_MUSB_WORK_CPU, system_unbound_wq, &glue->work);
	usb_phy_set_event(glue->xceiv, conn ? USB_EVENT_VBUS
					     : USB_EVENT_NONE);
	dev_info(glue->dev, conn ?
		 "device connection detected from VBUS GPIO.\n" :
		 "device disconnect detected from VBUS GPIO.\n");
	return 0;

ignore_disconnect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore device disconnect detected from VBUS GPIO.\n");
	return 0;
ignore_connect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore device connection detected from VBUS GPIO.\n");
	return 0;
}

static int sprd_musb_id_notifier(struct notifier_block *nb,
				 unsigned long event, void *data)
{
	struct sprd_glue *glue = container_of(nb, struct sprd_glue, id_nb);
	unsigned long flags;
	bool conn = !!event;

	spin_lock_irqsave(&glue->lock, flags);
	if (!conn) {
		if (!glue->id_active ||
		    glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			goto ignore_disconnect;
		glue->id_active = false;
		glue->wq_mode = USB_DR_MODE_HOST;
	} else {
		if (glue->id_active ||
		    glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			goto ignore_connect;
		glue->id_active = true;
		glue->wq_mode = USB_DR_MODE_HOST;
	}
	spin_unlock_irqrestore(&glue->lock, flags);
	queue_work_on(SPRD_MUSB_WORK_CPU, system_unbound_wq, &glue->work);
	dev_info(glue->dev, conn ?
		 "host connection detected from ID GPIO.\n" :
		 "host disconnect detected from ID GPIO.\n");
	return 0;

ignore_disconnect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore host disconnect detected from ID GPIO.\n");
	return 0;
ignore_connect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore host connection detected from ID GPIO.\n");
	return 0;
}

/*
 * Audio-dock notifier.
 *
 * DEVIASI D10 (dicatat, bukan fitur): di blob fungsi ini terdaftar pada
 * rantai notifier audio vendor (sprd audio hub / offload), yang tidak ada
 * di mainline 6.18. Fungsinya tetap diimplementasikan penuh di sini supaya
 * semantiknya auditable dan bisa dipanggil dari pembungkus mana pun; ia
 * TIDAK didaftarkan otomatis. Saat rantai audio vendor diport (fase audio),
 * cukup panggil sprd_musb_audio_notifier(conn) dari sana — jangan tulis
 * ulang logikanya.
 */
static int sprd_musb_audio_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct sprd_glue *glue = container_of(nb, struct sprd_glue, audio_nb);
	unsigned long flags;
	bool conn = !!event;

	spin_lock_irqsave(&glue->lock, flags);
	if (!conn) {
		if (!glue->audio_active ||
		    glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			goto ignore_disconnect;
		glue->audio_active = false;
		glue->wq_mode = USB_DR_MODE_HOST;
	} else {
		if (glue->audio_active ||
		    glue->dr_mode_cur == USB_DR_MODE_PERIPHERAL)
			goto ignore_connect;
		glue->audio_active = true;
		glue->wq_mode = USB_DR_MODE_HOST;
	}
	spin_unlock_irqrestore(&glue->lock, flags);
	queue_work_on(SPRD_MUSB_WORK_CPU, system_unbound_wq, &glue->work);
	dev_info(glue->dev, conn ?
		 "host connection detected from audio.\n" :
		 "host disconnect detected from audio.\n");
	return 0;

ignore_disconnect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore host disconnect detected from audio.\n");
	return 0;
ignore_connect:
	spin_unlock_irqrestore(&glue->lock, flags);
	dev_info(glue->dev, "ignore host connection detected from audio.\n");
	return 0;
}

/*
 * musb_sprd_disable_all_interrupts @0x10303c (blob) — port penuh.
 *
 * Terjemahan byte-for-byte: tulis 0 ke INTRUSBE/INTRTXE/INTRRXE, baca
 * INTRUSB/INTRTX/INTRRX untuk membersihkan pending, lalu dua loop atas 24
 * blok register DMA vendor (stride 0x20 dari 0x1c08): loop pertama
 * membersihkan bit 0x3c (mask 0xffffffc3), loop kedua men-set 0x1f000000.
 * Dipanggil dari jalur runtime-suspend di bawah spinlock musb, sama seperti
 * blob (yang memanggilnya di dalam raw_spin_lock_irqsave(&glue->lock)).
 *
 * TIDAK diport: ekor blob yang menulis register vendor 0x142c lalu
 * membersihkan byte flag di musb+0x2569. Kedua alamat itu milik ekstensi
 * struct musb + peta register vendor yang tidak ada di 6.18; arti flag itu
 * tidak dapat ditetapkan dari RE yang ada, jadi tidak dituliskan.
 */
static void musb_sprd_disable_all_interrupts(struct musb *musb)
{
	u32 val;
	int i;

	musb_writeb(musb->mregs, MUSB_INTRUSBE, 0);
	musb_writew(musb->mregs, MUSB_INTRTXE, 0);
	musb_writew(musb->mregs, MUSB_INTRRXE, 0);
	musb_readb(musb->mregs, MUSB_INTRUSB);
	musb_readw(musb->mregs, MUSB_INTRTX);
	musb_readw(musb->mregs, MUSB_INTRRX);

	for (i = 0; i != 0x3c0; i += 0x20) {
		val = musb_readl(musb->mregs, i + 0x1c08);
		musb_writel(musb->mregs, i + 0x1c08, val & 0xffffffc3);
	}
	for (i = 0; i != 0x3c0; i += 0x20) {
		val = musb_readl(musb->mregs, i + 0x1c08);
		musb_writel(musb->mregs, i + 0x1c08, val | 0x1f000000);
	}
}

/* ---------------- PM ops (RE blob: musb_sprd_{suspend,resume,runtime_*}) ----------------
 *
 * Blob: musb_sprd_suspend @0x1029a0, musb_sprd_resume @0x102a78,
 * musb_sprd_runtime_idle @0x102df8, musb_sprd_runtime_suspend @0x102b18,
 * musb_sprd_runtime_resume @0x102cf8. Sebelum ini port kita mengaktifkan
 * pm_runtime di probe tetapi TIDAK memasang .driver.pm sama sekali,
 * sehingga pm_runtime_suspend/resume yang dipanggil data_work tidak punya
 * callback.
 *
 * DEVIASI D11 (dicatat): blob memanggil tiga slot vtable PHY vendor
 * (offset +0x118 enable, +0x120 disable, +0x128 mode-set) dan menunggu
 * field port-suspend musb+0xbe8 dengan batas 500 jiffies. Ketiga slot itu
 * milik hsphy vendor, dan field 0xbe8 tidak ada di struct musb 6.18;
 * keduanya TIDAK diport. Yang diport adalah efek yang terukur dari sisi
 * register/clock/regulator: clk prepare/enable (resume), clk
 * disable/unprepare (suspend), pemutusan seluruh interupsi + is_suspend,
 * regulator vbus host, dan pemanggilan ulang sprd_musb_enable() saat host.
 */
static int musb_sprd_suspend(struct device *dev)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	unsigned long flags;
	int ret = 0;

	if (glue->vbus) {
		ret = regulator_disable(glue->vbus);
		if (ret) {
			dev_err(dev, "Failed to disable vbus: %d\n", ret);
			ret = 0;
		}
	}
	spin_lock_irqsave(&glue->lock, flags);
	glue->is_suspend = true;
	spin_unlock_irqrestore(&glue->lock, flags);
	return ret;
}

static int musb_sprd_resume(struct device *dev)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	unsigned long flags;
	int ret = 0;

	if (glue->vbus) {
		ret = regulator_enable(glue->vbus);
		if (ret) {
			dev_err(dev, "Failed to enable vbus: %d\n", ret);
			ret = 0;
		}
	}
	spin_lock_irqsave(&glue->lock, flags);
	glue->is_suspend = false;
	spin_unlock_irqrestore(&glue->lock, flags);
	return ret;
}

static int musb_sprd_runtime_idle(struct device *dev)
{
	dev_info(dev, "enter into idle mode\n");
	return 0;
}

static int musb_sprd_runtime_suspend(struct device *dev)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(glue->musb);
	unsigned long flags;

	if (!musb)
		return 0;

	spin_lock_irqsave(&musb->lock, flags);
	musb_sprd_disable_all_interrupts(musb);
	glue->is_suspend = true;
	spin_unlock_irqrestore(&musb->lock, flags);

	clk_disable(glue->clk);
	clk_unprepare(glue->clk);

	dev_info(dev, "enter into suspend mode\n");
	return 0;
}

static int musb_sprd_runtime_resume(struct device *dev)
{
	struct sprd_glue *glue = dev_get_drvdata(dev);
	struct musb *musb = platform_get_drvdata(glue->musb);
	int ret;

	ret = clk_prepare(glue->clk);
	if (ret)
		return ret;
	ret = clk_enable(glue->clk);
	if (ret) {
		clk_unprepare(glue->clk);
		return ret;
	}

	glue->is_suspend = false;

	if (musb && glue->dr_mode_cur == USB_DR_MODE_HOST) {
		/* blob: msleep(0x96) = 150 ms lalu enable ulang controller */
		msleep(150);
		sprd_musb_enable(musb);
	}

	dev_info(dev, "enter into resume mode\n");
	return 0;
}

static const struct dev_pm_ops musb_sprd_pm_ops = {
	.suspend		= musb_sprd_suspend,
	.resume			= musb_sprd_resume,
	.runtime_idle		= musb_sprd_runtime_idle,
	.runtime_suspend	= musb_sprd_runtime_suspend,
	.runtime_resume		= musb_sprd_runtime_resume,
};

/* ---------------- recovery + charger-detect (RE blob) ----------------
 *
 * sprd_musb_recover @0x104478: bila controller masih hidup dan mode HOST,
 * queue_delayed_work_on(32, system_wq, &recover_work, 0x19 = 25 jiffies).
 * sprd_musb_recover_work @0x103d58: log "try to recover musb controller",
 * lalu bila flag aktif: matikan flag, set wq_mode = HOST, queue kerja data,
 * msleep(300), hidupkan flag lagi, queue lagi — satu siklus reset tapak
 * yang sama dengan yang dijalankan fork.
 *
 * DEVIASI D12: blob memakai field glue +0xf9 sebagai gerbang siklus ini;
 * artinya tidak dapat ditetapkan dari RE yang ada. Di sini gerbangnya
 * adalah keadaan yang TERUKUR: mode HOST dan tidak sedang suspend.
 */
static void sprd_musb_recover_work(struct work_struct *work)
{
	struct sprd_glue *glue = container_of(work, struct sprd_glue,
					      recover_work.work);
	struct musb *musb = platform_get_drvdata(glue->musb);

	dev_info(glue->dev, "try to recover musb controller\n");
	if (!musb || glue->is_suspend)
		return;

	glue->data_enabled = false;
	glue->wq_mode = USB_DR_MODE_HOST;
	queue_work_on(SPRD_MUSB_WORK_CPU, system_wq, &glue->work);
	msleep(300);
	glue->data_enabled = true;
	glue->wq_mode = USB_DR_MODE_HOST;
	queue_work_on(SPRD_MUSB_WORK_CPU, system_wq, &glue->work);
}

/*
 * Footprint sama dengan fork: sprd_musb_ops.recover = sprd_musb_recover
 * (fork musb_sprd.c:411). Field `.recover` itu TAMBAHAN vendor di struct
 * musb_platform_ops; mainline 6.18 TIDAK memilikinya, jadi fungsi ini
 * dipertahankan global + ber-prototype sebagai titik masuk yang siap
 * dipanggil begitu hook-nya ada (DEV-D14), bukan dipanggil dari tempat
 * yang tidak mewakili fork. Guard-nya di sini adalah keadaan terukur:
 * mode HOST dan tidak sedang suspend. Fork memakai is_host_active() +
 * dr_mode HOST; blob memakai flag musb+0x20e0 yang artinya tidak dapat
 * ditetapkan dari RE yang ada (DEV-D12).
 */
int sprd_musb_recover(struct musb *musb)
{
	struct sprd_glue *glue = dev_get_drvdata(musb->controller->parent);

	if (glue->dr_mode_cur == USB_DR_MODE_HOST && !glue->is_suspend)
		queue_delayed_work_on(SPRD_MUSB_WORK_CPU, system_wq,
				      &glue->recover_work, 25);
	return 0;
}

/*
 * musb_sprd_retry_charger_detect @0x104be8 (blob, 760 byte) — port penuh
 * mengikuti fork musb_sprd.c:879-912 (keduanya sepakat urutannya).
 *
 * Dicatat apa adanya supaya tidak ada yang “memperbaiki” tanpa bukti:
 *   - penulisan INTRTXE/INTRRXE di fungsi ini memakai writeb (8-bit),
 *     BUKAN writew seperti di musb_sprd_disable_all_interrupts. Itu bukan
 *     salah salin: blob juga memanggil _musb_writeb untuk register 6/8 di
 *     fungsi ini (dekompilasi musb_re.log baris retry_charger_detect).
 *   - usb_phy_init()/usb_phy_shutdown() dipertahankan walau xceiv port ini
 *     sintetis; keduanya API mainline dan menjadi no-op bila metode PHY
 *     tidak ada.
 *
 * DEVIASI D13: blob memanggil slot vtable PHY +0x158 (hsphy charger
 * detect) sambil men-set bit 0x80000000 di flag musb vendor. Struct usb_phy
 * mainline TIDAK punya metode retry_charger_detect, jadi langkah itu tidak
 * bisa diidentikkan; yang dipakai adalah ->charger_detect (metode mainline
 * yang memang ada) — deviasi yang dicatat, bukan diklaim setara. Fungsi ini
 * global dan BER-PROTOTIPE supaya bisa dipanggil dari jalur pemulihan
 * begitu hsphy fase 2 mendarat.
 */
enum usb_charger_type musb_sprd_retry_charger_detect(struct sprd_glue *glue)
{
	enum usb_charger_type type = UNKNOWN_TYPE;
	struct usb_phy *usb_phy = glue->xceiv;
	struct musb *musb = platform_get_drvdata(glue->musb);
	unsigned long flags;
	u8 pwr;

	if (!musb || !usb_phy)
		return type;

	dev_info(glue->dev, "%s enter\n", __func__);
	spin_lock_irqsave(&glue->lock, flags);
	glue->retry_charger_detect = true;
	spin_unlock_irqrestore(&glue->lock, flags);

	if (clk_prepare_enable(glue->clk))
		goto out;

	usb_phy_init(usb_phy);
	musb_writeb(musb->mregs, MUSB_INTRUSBE, 0);
	musb_writeb(musb->mregs, MUSB_INTRTXE, 0);
	musb_writeb(musb->mregs, MUSB_INTRRXE, 0);
	pwr = musb_readb(musb->mregs, MUSB_POWER);
	pwr |= MUSB_POWER_SOFTCONN;
	musb_writeb(musb->mregs, MUSB_POWER, pwr);

	/* DEV-D13: padanan mainline untuk deteksi ulang (lihat kepala fungsi) */
	if (usb_phy->charger_detect)
		type = usb_phy->charger_detect(usb_phy);

	pwr = musb_readb(musb->mregs, MUSB_POWER);
	pwr &= ~MUSB_POWER_SOFTCONN;
	musb_writeb(musb->mregs, MUSB_POWER, pwr);

	spin_lock_irqsave(&glue->lock, flags);
	glue->retry_charger_detect = false;
	spin_unlock_irqrestore(&glue->lock, flags);

	/* flush pending interrupts (fork) */
	musb_readb(musb->mregs, MUSB_INTRUSB);
	musb_readw(musb->mregs, MUSB_INTRTXE);

	usb_phy_shutdown(usb_phy);
	clk_disable_unprepare(glue->clk);
out:
	return type;
}

/*
 * dwc3_sprd_probe_finish @0x102030 (blob) — SELURUH isinya `return 1`
 * (8 byte). Diport apa adanya; TIDAK diekspor: pemeriksaan ulang tabel
 * simbol blob (blobs/re_notes/musb_sprd.sym) menunjukkan NOL entri
 * __ksymtab, jadi klaim lama di ledger bahwa fungsi ini EXPORT_SYMBOLed
 * TIDAK didukung bukti dan dikoreksi di ledger 2026-10-06.
 */
int dwc3_sprd_probe_finish(void)
{
	return 1;
}

/* ---------------- probe/remove ---------------- */

static int musb_sprd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct musb_hdrc_platform_data pdata;
	struct platform_device_info pinfo;
	struct sprd_glue *glue;
	int ret;

	glue = devm_kzalloc(dev, sizeof(*glue), GFP_KERNEL);
	if (!glue)
		return -ENOMEM;
	glue->dev = dev;
	glue->dr_mode_host = false;
	spin_lock_init(&glue->lock);
	INIT_WORK(&glue->work, sprd_musb_data_work);
	/* fork musb_sprd.c:1741 */
	INIT_DELAYED_WORK(&glue->recover_work, sprd_musb_recover_work);
	glue->data_enabled = true;	/* blob probe: [0x1b0]=0 ("enabled"=1) */
	glue->dr_mode_cur = USB_DR_MODE_UNKNOWN;
	glue->last_mode = USB_DR_MODE_UNKNOWN;
	/* fork: wakeup_source_init(&glue->wake_lock, "musb-sprd") /
	 * "musb-sprd-pd" — 6.18: pointer (register/unregister). */
	glue->wake_lock = wakeup_source_register(dev, "musb-sprd");
	glue->pd_wake_lock = wakeup_source_register(dev, "musb-sprd-pd");

	memset(&pdata, 0, sizeof(pdata));
	switch (usb_get_dr_mode(dev)) {
	case USB_DR_MODE_HOST:
		pdata.mode = MUSB_HOST;
		glue->dr_mode_host = true;
		break;
	case USB_DR_MODE_PERIPHERAL:
		pdata.mode = MUSB_PERIPHERAL;
		break;
	default:
		pdata.mode = MUSB_OTG;
		break;
	}

	glue->clk = devm_clk_get_optional(dev, "core_clk");
	if (IS_ERR(glue->clk))
		return dev_err_probe(dev, PTR_ERR(glue->clk), "core_clk\n");
	ret = clk_prepare_enable(glue->clk);
	if (ret)
		return ret;

	/* xceiv sintetis USB2 (fase 2: usb-phy asli dari DT) */
	glue->xceiv = sprd_musb_create_xceiv(dev);
	if (IS_ERR(glue->xceiv)) {
		ret = PTR_ERR(glue->xceiv);
		goto err_clk;
	}

	pdata.config = &sprd_musb_hdrc_config;
	pdata.platform_ops = &sprd_musb_ops;

	memset(&pinfo, 0, sizeof(pinfo));
	pinfo.name = "musb-hdrc";
	pinfo.id = PLATFORM_DEVID_AUTO;
	pinfo.parent = dev;
	pinfo.res = pdev->resource;
	pinfo.num_res = pdev->num_resources;
	pinfo.data = &pdata;
	pinfo.size_data = sizeof(pdata);
	pinfo.dma_mask = DMA_BIT_MASK(BITS_PER_LONG);

	glue->musb = platform_device_register_full(&pinfo);
	if (IS_ERR(glue->musb)) {
		ret = PTR_ERR(glue->musb);
		goto err_xceiv;
	}

	/* extcon opsional (sc27xx_typec fase 2); kalau ada → bridge VBUS */
	glue->edev = extcon_get_edev_by_phandle(dev, 0);
	if (IS_ERR(glue->edev)) {
		glue->edev = NULL;
		dev_info(dev, "no extcon phandle — VBUS manual\n");
	} else {
		glue->vbus_nb.notifier_call = sprd_musb_vbus_notifier;
		ret = extcon_register_notifier(glue->edev, EXTCON_USB,
					       &glue->vbus_nb);
		if (ret)
			goto err_pm;

		/* ID terpisah bila DT menyediakannya (fork musb_sprd.c:1806-1822:
		 * extcon phandle indeks 1; kalau tidak ada, pakai edev VBUS yang
		 * sama). Notifier-nya yang membuat mode HOST benar-benar antre ke
		 * sprd_musb_data_work — tanpa ini kerja mode hanya bisa dipicu
		 * dari sysfs. */
		glue->id_edev = extcon_get_edev_by_phandle(dev, 1);
		if (IS_ERR(glue->id_edev)) {
			glue->id_edev = NULL;
			dev_info(dev, "No separate ID extcon device.\n");
		}
		glue->id_nb.notifier_call = sprd_musb_id_notifier;
		ret = extcon_register_notifier(glue->id_edev ?: glue->edev,
					       EXTCON_USB_HOST, &glue->id_nb);
		if (ret) {
			dev_err(dev, "failed to register extcon USB HOST notifier.\n");
			goto err_id_notifier;
		}
		/* DEV-D10: notifier audio tidak didaftarkan di sini — rantai
		 * SPRD_USBM_EVENT_HOST_MUSB (fork musb_sprd.c:1840) tidak ada di
		 * mainline; fungsinya sudah diport dan siap dipanggil. */
	}

	platform_set_drvdata(pdev, glue);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, -1);
	pm_runtime_use_autosuspend(dev);

	/* blob: usb_notify + sysfs usb_control dibuat setelah drvdata siap */
	ret = sprd_musb_usb_notify_init(dev);
	if (ret)
		goto err_pm;

	dev_info(dev, "VXTux musb-sprd v3: PIO, %d EP, ram_bits=%d, mode=%s\n",
		 SPRD_MUSB_MAX_EP_NUM, SPRD_MUSB_RAM_BITS,
		 glue->dr_mode_host ? "host" :
		 (pdata.mode == MUSB_PERIPHERAL ? "peripheral" : "dual-role"));
	return 0;

err_id_notifier:
	extcon_unregister_notifier(glue->edev, EXTCON_USB, &glue->vbus_nb);
err_pm:
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_disable(dev);
	if (glue->edev)
		extcon_unregister_notifier(glue->edev, EXTCON_USB,
					   &glue->vbus_nb);
	platform_device_unregister(glue->musb);
err_xceiv:
	usb_remove_phy(glue->xceiv);
err_clk:
	clk_disable_unprepare(glue->clk);
	wakeup_source_unregister(glue->pd_wake_lock);
	wakeup_source_unregister(glue->wake_lock);
	return ret;
}

static void musb_sprd_remove(struct platform_device *pdev)
{
	struct sprd_glue *glue = platform_get_drvdata(pdev);

	sprd_musb_usb_notify_exit();
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	cancel_work_sync(&glue->work);
	cancel_delayed_work_sync(&glue->recover_work);
	if (glue->edev) {
		extcon_unregister_notifier(glue->edev, EXTCON_USB,
					   &glue->vbus_nb);
		extcon_unregister_notifier(glue->id_edev ?: glue->edev,
					   EXTCON_USB_HOST, &glue->id_nb);
	}
	platform_device_unregister(glue->musb);
	usb_remove_phy(glue->xceiv);
	clk_disable_unprepare(glue->clk);
	wakeup_source_unregister(glue->pd_wake_lock);
	wakeup_source_unregister(glue->wake_lock);
}

static const struct of_device_id musb_sprd_ids[] = {
	{ .compatible = "sprd,sharkl5pro-musb" },
	{ .compatible = "sprd,sharkl5-musb" },
	{ .compatible = "sprd,qogirn6pro-musb" },
	{ .compatible = "sprd,qogirl6-musb" },
	{ .compatible = "sprd,sharkl3-musb" },
	{ }
};
MODULE_DEVICE_TABLE(of, musb_sprd_ids);

static struct platform_driver musb_sprd_driver = {
	.probe		= musb_sprd_probe,
	.remove		= musb_sprd_remove,
	.driver = {
		.name		= DRV_NAME,
		.of_match_table	= musb_sprd_ids,
		/* 2026-10-06: sebelumnya tidak ada .pm sama sekali padahal probe
		 * menyalakan pm_runtime, sehingga pm_runtime_suspend/resume yang
		 * dipanggil data_work tidak punya callback (blob: musb_sprd_pm_ops
		 * @0x100e60, terdaftar di musb_sprd_driver). */
		.pm		= &musb_sprd_pm_ops,
	},
};
module_platform_driver(musb_sprd_driver);

MODULE_DESCRIPTION("VXTux: Unisoc sharkl5pro MUSB glue (port GPL fork, v3 PIO + work penuh)");
MODULE_AUTHOR("LinDroid/VXTux v1.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
