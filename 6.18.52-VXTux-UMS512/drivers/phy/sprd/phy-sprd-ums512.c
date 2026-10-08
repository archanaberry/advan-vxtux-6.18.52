// SPDX-License-Identifier: GPL-2.0
/*
 * phy-sprd-ums512.c — USB2 High-Speed PHY Unisoc UMS512/T618 (sharkl5pro).
 *
 * Port ke mainline 6.18 dari blob vendor `phy-sprd-sharkl5Pro.ko` (5.4.210,
 * sandi sumber asli di .rodata: `drivers/usb/phy/phy-sprd-sharkl5Pro.c`).
 * Target papan: Advan Tab VX Lite (UMS512, DT `sprd,sharkl5ph5-phy`? -> lihat
 * of_match di bawah). Ini adalah **gerbang USB**: MUSB tidak akan probe tanpa
 * PHY yang bisa didaftarkan di `usb_add_phy_dev()`.
 *
 * Sumber bukti (semua angka register punya provenance, tidak ada yang ditebak):
 *   [R1] docs/usb_register_map.md §5 — blok AON APB 0x327D0000
 *        (header SoC: src/upstream/u-boot-ums512/arch/arm/include/asm/
 *         arch-sharkl5pro/chip_sharkl5pro/aon_apb.h)
 *   [R2] docs/usb_register_map.md §6 — blok ANLG PHY G2 0x323B0000
 *        (header SoC: …/anlg_phy_g2.h)
 *   [R3] Disassembly blob (dumped/re/phy-sprd-sharkl5Pro.ko.<fungsi>.dis.txt),
 *        dihasilkan `tools/re/re_dis.py func phy-sprd-sharkl5Pro.ko …`.
 *        Pasangan (offset, mask, value) di bawah dibaca dari immediate
 *        instruction di dalam `regmap_update_bits_base` — bukan tafsiran.
 *   [R4] DT stok `blobs/fdt_live.dtb`: /soc/aon/hsphy@323b0000
 *
 * DEVIASI yang disengaja dari blob (masing-masing beralasan, bukan diam-diam):
 *   1. Deteksi charger memakai API mainline `sprd_pmic_detect_charger_type()`
 *      (include/linux/mfd/sc27xx-pmic.h:5, ekspor di
 *      drivers/mfd/sprd-sc27xx-spi.c:70-105). Blob memakai jalur sendiri:
 *      polling register PMIC `0x1B9C` bit 11 (yang di mainline bernama
 *      `SPRD_PMIC_CHG_DET_DONE`, sprd-sc27xx-spi.c:23,31) lalu membaca ADC
 *      dp/dm lewat iio-channel dan mengklasifikasi dengan ambang tetap.
 *      Jalur mainline lebih pendek, sudah diuji hulu, dan tidak menuntut
 *      regmap PMIC mentah (yang di pohon ini tidak punya penyedia —
 *      `dev_get_regmap()` hanya melihat devres device itu sendiri,
 *      drivers/base/regmap/regmap.c:1517-1525).
 *      Ambang ADC blob (dp*3 cmp 0x962/0x4b0, dm*3 cmp 0x961) TIDAK dipakai;
 *      angkanya tetap tercatat di docs/usb_register_map.md §7 untuk rujukan.
 *   2. Register PMIC `0x1BA0` (mask 0x3, tulis 0x2 lalu 0x0) tidak dipakai —
 *      hanya bermakna untuk jalur ADC blob di atas.
 *   3. `sprd_hsphy_cali_mode()` (membaca /chosen + `androidboot.mode=cali`)
 *      tidak diport: tidak ada bootargs itu di papan ini, dan fungsinya hanya
 *      untuk mode kalibrasi pabrik.
 *   5. Regulator `vdd` diambil dengan `devm_regulator_get_optional()`, bukan
 *      `devm_regulator_get()`. Blob MENUNTUT rail ini ada (probe gagal bila
 *      tidak: `_dev_err` di `sprd_hsphy_probe+0x180/0x198`). Alasannya bukan
 *      kelalaian: rail ini berasal dari PMIC SC2730 lewat rantai perangkat
 *      yang panjang (sprd-adi → sprd-sc27xx-spi → regulator), dan PHY adalah
 *      GERBANG USB — kalau PHY menolak probe hanya karena PMIC belum siap,
 *      maka seluruh stack USB gagal karena sebab yang sama sekali bukan USB.
 *      Di papan nyata PMIC selalu ada, jadi perilakunya identik; di QEMU
 *      (tanpa PMIC) driver tetap probe dan itu justru membuat kegagalan
 *      berikutnya terlihat jelas di probe_map. Voltage tetap diset bila rail
 *      ada, dan nilainya diambil dari `sprd,vdd-voltage` DT (papan: 3.050.000).
 *   4. Urutan host-mode `sprd_hostphy_set()` SUDAH direkonstruksi penuh
 *      (2026-09-26): fungsinya `int sprd_hostphy_set(priv, bool on)` — TEPAT
 *      dua argumen, dan `on` satu-satunya yang menentukan nilai. Empat update
 *      register-nya: AON OTG_PHY_CTRL mask 0x8 value (`on ? 0 : 0x8`) = bit
 *      IDDIG; ANAG REG_SEL_CFG_0 mask 0x6 value 0x6 (selalu); ANAG UTMI_CTL2
 *      mask 0x18 value (`on ? 0 : 0x18`) = pulldown DP/DM; ANAG UTMI_CTL1 mask
 *      0xffff value (`on ? 0x200 : 0`). Bukti: blob `sprd_hostphy_set`
 *      0x9f8-0xae0 + alamatnya diambil (`R_AARCH64_JUMP26 .text+0x9f8` di
 *      0x1218, tabel lompat CFI), jadi dipanggil lewat pointer, bukan `bl`.
 *      Yang port ini tetap TIDAK panggil bukan karena belum tahu, melainkan
 *      karena tidak ada pemanggilnya: `struct usb_phy` mainline tidak punya
 *      hook set-mode, DT papan ini `dr-mode = "peripheral"`, dan tidak ada
 *      `id-gpio` di extcon (lihat docs/usb_register_map.md §8) sehingga host
 *      mode tidak pernah dilaporkan. Menambah fungsi statis tanpa pemanggil
 *      hanya akan memicu -Wunused-function di gerbang nol-peringatan; jadi
 *      urutannya didokumentasikan, bukan ditulis sebagai kode mati. Port
 *      U-Boot memakainya lewat PHY-uclass `set_mode` (dr-mode = host).
 *
 * Rollback: hapus berkas ini + 2 baris di drivers/phy/{Kconfig,Makefile}
 * (sumber `drivers/phy/sprd/Kconfig`, `obj-y += sprd/`) + direktori
 * drivers/phy/sprd/. Tidak ada berkas lain yang bergantung padanya.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mfd/sc27xx-pmic.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/phy.h>

/* ---------------------------------------------------------------------------
 * [R1] AON APB (syscon `sprd,syscon-enable`, base 0x327D0000, ukuran 0x3000)
 * ------------------------------------------------------------------------ */
#define SPRD_AON_APB_EB1			0x0004
#define SPRD_AON_APB_EB1_OTG_UTMI_EB		BIT(8)	/* aon_apb.h:252 */
#define SPRD_AON_APB_RST1			0x0010
#define SPRD_AON_APB_RST1_OTG_UTMI_SOFT_RST	BIT(8)	/* aon_apb.h:326 */
#define SPRD_AON_APB_RST1_OTG_PHY_SOFT_RST	BIT(9)	/* aon_apb.h:325 */
#define SPRD_AON_APB_CGM_REG1			0x0138
#define SPRD_AON_APB_CGM_DPHY_REF_EN		BIT(10)	/* aon_apb.h:639 */
#define SPRD_AON_APB_CGM_OTG_REF_EN		BIT(12)	/* aon_apb.h:637 */
#define SPRD_AON_APB_OTG_PHY_TEST		0x0204
#define SPRD_AON_APB_OTG_VBUS_VALID_PHYREG	BIT(24)	/* aon_apb.h:832 */
#define SPRD_AON_APB_OTG_PHY_CTRL		0x0208
#define SPRD_AON_APB_OTG_USB2_PHY_IDDIG		BIT(3)	/* aon_apb.h:846 */
/*
 * Dulu ditulis sebagai "BIT(30) tanpa nama" (dokumen register §5/§9 R1).
 * KOREKSI 2026-09-26: header MENAMAI bit ini — `BIT_AON_APB_UTMI_WIDTH_SEL`
 * (blok bits definitions untuk REG_AON_APB_OTG_PHY_CTRL, [0x327D0208]) — dan
 * artinya dikonfirmasi sumber kedua: glue MUSB vendor U-Boot menulis bit yang
 * sama dengan komentar "PHY width: 16 bit"
 * (u-boot-ums512/drivers/usb/musb-new/sharkl5pro_usb_phy.c `usb_phy_init`).
 * Blob init menulisnya lewat regmap (mask=value=0x40000000).
 */
#define SPRD_AON_APB_OTG_UTMI_WIDTH_SEL		BIT(30)

/* Nilai init/shutdown AON yang dipakai blob (pasangan mask/value) */
#define SPRD_AON_INIT_EB1_MASK			SPRD_AON_APB_EB1_OTG_UTMI_EB
#define SPRD_AON_INIT_CGM_MASK			(SPRD_AON_APB_CGM_DPHY_REF_EN | \
						 SPRD_AON_APB_CGM_OTG_REF_EN)
#define SPRD_AON_INIT_RST_MASK			(SPRD_AON_APB_RST1_OTG_UTMI_SOFT_RST | \
						 SPRD_AON_APB_RST1_OTG_PHY_SOFT_RST)
/* Delay reset dari blob: usleep_range(20000, 30000) — `sprd_hsphy_init+0x1b0` */
#define SPRD_HS_RST_DELAY_MIN_US		20000
#define SPRD_HS_RST_DELAY_MAX_US		30000

/* ---------------------------------------------------------------------------
 * [R2] ANLG PHY G2 (syscon `sprd,syscon-anag2`, base 0x323B0000)
 * ------------------------------------------------------------------------ */
#define SPRD_ANAG2_USB20_UTMI_CTL1		0x0058	/* anlg_phy_g2.h:53 */
#define SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT	BIT(16)	/* :233 */
#define SPRD_ANAG2_USB20_UTMI_CTL1_DATABUS16_8	BIT(28)	/* :222 */
#define SPRD_ANAG2_USB20_BATTER_PLL		0x005C	/* :54 */
#define SPRD_ANAG2_USB20_BATTER_PLL_PS_PD_L	BIT(3)	/* :239 */
#define SPRD_ANAG2_USB20_BATTER_PLL_PS_PD_S	BIT(4)	/* :238 */
#define SPRD_ANAG2_USB20_UTMI_CTL2		0x0060	/* :55 */
#define SPRD_ANAG2_USB20_UTMI_CTL2_DMPULLDOWN	BIT(3)	/* :245 */
#define SPRD_ANAG2_USB20_UTMI_CTL2_DPPULLDOWN	BIT(4)	/* :244 */
#define SPRD_ANAG2_USB20_TRIMMING		0x0064	/* :56 */
#define SPRD_ANAG2_USB20_TRIMMING_TUNEHSAMP	(BIT(25) | BIT(26)) /* :252 */
#define SPRD_ANAG2_USB20_TRIMMING_TFREGRES	GENMASK(24, 19)	/* :253 */
#define SPRD_ANAG2_USB20_ISO_SW			0x0070	/* :59 */
#define SPRD_ANAG2_USB20_ISO_SW_EN		BIT(0)	/* :273 */
#define SPRD_ANAG2_REG_SEL_CFG_0		0x0074	/* :60 */

/* Nilai init/shutdown ANLG2 (pasangan mask/value dari blob) */
#define SPRD_ANAG2_BATTER_PLL_MASK		(SPRD_ANAG2_USB20_BATTER_PLL_PS_PD_L | \
						 SPRD_ANAG2_USB20_BATTER_PLL_PS_PD_S)
#define SPRD_ANAG2_BATTER_PLL_ON		0	/* lepas power-down */
#define SPRD_ANAG2_BATTER_PLL_OFF		SPRD_ANAG2_BATTER_PLL_MASK
#define SPRD_ANAG2_TRIMMING_HSAMP_VAL		SPRD_ANAG2_USB20_TRIMMING_TUNEHSAMP
#define SPRD_ANAG2_TRIMMING_TFREGRES_VAL	(0x14 << 19)	/* blob: 0x00A00000 */

/* Waktu tunggu setelah deteksi charger dimulai (blob: msleep(300)) */
#define SPRD_HS_CHG_DET_SETTLE_MS		300

struct sprd_hsphy {
	struct device		*dev;
	struct usb_phy		phy;

	struct regmap		*aon_apb;
	struct regmap		*anag2;

	/* device PMIC (sprd,sc2730) untuk charger_detect via API mainline */
	struct device		*pmic_dev;

	struct regulator	*vdd;
	u32			vdd_voltage;

	bool			initialized;
	bool			regulator_enabled_by_us;
};

static const struct of_device_id sprd_hsphy_of_match[];

/* ---------------------------------------------------------------------------
 * init / shutdown — urutan PERSIS dari blob (lihat docs/usb_register_map.md
 * §5/§6). Setiap baris menyebut alamat fungsi blob sebagai provenance.
 * ------------------------------------------------------------------------ */
static int sprd_hsphy_init(struct usb_phy *phy)
{
	struct sprd_hsphy *hsphy = container_of(phy, struct sprd_hsphy, phy);
	struct regmap *aon = hsphy->aon_apb;
	struct regmap *anag = hsphy->anag2;
	int ret;

	if (hsphy->initialized)
		return 0;

	/* sprd_hsphy_init+0x30: regulator_set_voltage(vdd, vdd_voltage, ..) */
	if (hsphy->vdd) {
		ret = regulator_set_voltage(hsphy->vdd, hsphy->vdd_voltage,
					    hsphy->vdd_voltage);
		if (ret) {
			dev_err(hsphy->dev, "gagal set tegangan vdd %u uV: %d\n",
				hsphy->vdd_voltage, ret);
			return ret;
		}

		if (!regulator_is_enabled(hsphy->vdd)) {
			ret = regulator_enable(hsphy->vdd);
			if (ret) {
				dev_err(hsphy->dev, "gagal menyalakan vdd: %d\n", ret);
				return ret;
			}
			hsphy->regulator_enabled_by_us = true;
		}
	} else {
		dev_warn(hsphy->dev,
			 "rail vdd tidak ada (PMIC belum siap) — lanjut tanpa set tegangan\n");
	}

	/* +0x44: AON EB1.OTG_UTMI_EB = 1 */
	regmap_update_bits(aon, SPRD_AON_APB_EB1, SPRD_AON_INIT_EB1_MASK,
			   SPRD_AON_INIT_EB1_MASK);
	/* +0x64: AON CGM_REG1: DPHY_REF_EN + OTG_REF_EN = 1 */
	regmap_update_bits(aon, SPRD_AON_APB_CGM_REG1, SPRD_AON_INIT_CGM_MASK,
			   SPRD_AON_INIT_CGM_MASK);
	/* +0x84: ANLG2 USB20_ISO_SW.ISO_SW_EN = 0 */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_ISO_SW,
			   SPRD_ANAG2_USB20_ISO_SW_EN, 0);
	/* +0xa8: ANLG2 BATTER_PLL: PS_PD_L/PS_PD_S = 0 (keluar power-down) */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_BATTER_PLL,
			   SPRD_ANAG2_BATTER_PLL_MASK, SPRD_ANAG2_BATTER_PLL_ON);
	/* +0xc8: AON OTG_PHY_TEST.VBUS_VALID_PHYREG = 1 */
	regmap_update_bits(aon, SPRD_AON_APB_OTG_PHY_TEST,
			   SPRD_AON_APB_OTG_VBUS_VALID_PHYREG,
			   SPRD_AON_APB_OTG_VBUS_VALID_PHYREG);
	/* +0xe8: ANLG2 UTMI_CTL1.VBUSVLDEXT = 1 */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_UTMI_CTL1,
			   SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT,
			   SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT);
	/* +0x108: AON OTG_PHY_CTRL.UTMI_WIDTH_SEL = 1 (lebar UTMI 16 bit) */
	regmap_update_bits(aon, SPRD_AON_APB_OTG_PHY_CTRL,
			   SPRD_AON_APB_OTG_UTMI_WIDTH_SEL,
			   SPRD_AON_APB_OTG_UTMI_WIDTH_SEL);
	/* +0x128: ANLG2 UTMI_CTL1.DATABUS16_8 = 1 */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_UTMI_CTL1,
			   SPRD_ANAG2_USB20_UTMI_CTL1_DATABUS16_8,
			   SPRD_ANAG2_USB20_UTMI_CTL1_DATABUS16_8);
	/* +0x148: ANLG2 TRIMMING.TUNEHSAMP = 3 */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_TRIMMING,
			   SPRD_ANAG2_USB20_TRIMMING_TUNEHSAMP,
			   SPRD_ANAG2_TRIMMING_HSAMP_VAL);
	/* +0x168: ANLG2 TRIMMING.TFREGRES = 0x14 */
	regmap_update_bits(anag, SPRD_ANAG2_USB20_TRIMMING,
			   SPRD_ANAG2_USB20_TRIMMING_TFREGRES,
			   SPRD_ANAG2_TRIMMING_TFREGRES_VAL);
	/* +0x190: AON RST1: OTG UTMI + PHY soft reset = 1 */
	regmap_update_bits(aon, SPRD_AON_APB_RST1, SPRD_AON_INIT_RST_MASK,
			   SPRD_AON_INIT_RST_MASK);
	usleep_range(SPRD_HS_RST_DELAY_MIN_US, SPRD_HS_RST_DELAY_MAX_US);
	/* +0x1b8: AON RST1: lepas reset */
	regmap_update_bits(aon, SPRD_AON_APB_RST1, SPRD_AON_INIT_RST_MASK, 0);

	hsphy->initialized = true;
	dev_info(hsphy->dev, "sprd-hsphy init (vdd %u uV)\n",
		 hsphy->vdd_voltage);
	return 0;
}

static void sprd_hsphy_shutdown(struct usb_phy *phy)
{
	struct sprd_hsphy *hsphy = container_of(phy, struct sprd_hsphy, phy);
	struct regmap *aon = hsphy->aon_apb;
	struct regmap *anag = hsphy->anag2;

	if (!hsphy->initialized)
		return;

	/* Kebalikan dari init, urutan PERSIS `sprd_hsphy_shutdown` 0x900-0x99c */
	regmap_update_bits(aon, SPRD_AON_APB_OTG_PHY_TEST,
			   SPRD_AON_APB_OTG_VBUS_VALID_PHYREG, 0);
	regmap_update_bits(anag, SPRD_ANAG2_USB20_UTMI_CTL1,
			   SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT, 0);
	regmap_update_bits(anag, SPRD_ANAG2_USB20_BATTER_PLL,
			   SPRD_ANAG2_BATTER_PLL_MASK, SPRD_ANAG2_BATTER_PLL_OFF);
	regmap_update_bits(anag, SPRD_ANAG2_USB20_ISO_SW,
			   SPRD_ANAG2_USB20_ISO_SW_EN, SPRD_ANAG2_USB20_ISO_SW_EN);
	regmap_update_bits(aon, SPRD_AON_APB_CGM_REG1,
			   SPRD_AON_INIT_CGM_MASK, 0);

	if (hsphy->vdd && hsphy->regulator_enabled_by_us &&
	    regulator_is_enabled(hsphy->vdd)) {
		regulator_disable(hsphy->vdd);
		hsphy->regulator_enabled_by_us = false;
	}
	hsphy->initialized = false;
	dev_info(hsphy->dev, "sprd-hsphy shutdown\n");
}

/*
 * VBUS — pasangan bit yang TERBUKTI dari `sprd_hsphy_vbus_notify`
 * (blob 0xb2c-0xb64): AON OTG_PHY_TEST.VBUS_VALID_PHYREG dan
 * ANLG2 UTMI_CTL1.VBUSVLDEXT, keduanya diset/dibersihkan sesuai state.
 */
static void sprd_hsphy_vbus_set(struct sprd_hsphy *hsphy, bool on)
{
	regmap_update_bits(hsphy->aon_apb, SPRD_AON_APB_OTG_PHY_TEST,
			   SPRD_AON_APB_OTG_VBUS_VALID_PHYREG,
			   on ? SPRD_AON_APB_OTG_VBUS_VALID_PHYREG : 0);
	regmap_update_bits(hsphy->anag2, SPRD_ANAG2_USB20_UTMI_CTL1,
			   SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT,
			   on ? SPRD_ANAG2_USB20_UTMI_CTL1_VBUSVLDEXT : 0);
}

static int sprd_hsphy_set_vbus(struct usb_phy *phy, int on)
{
	struct sprd_hsphy *hsphy = container_of(phy, struct sprd_hsphy, phy);

	sprd_hsphy_vbus_set(hsphy, !!on);
	return 0;
}

static int sprd_hsphy_vbus_notify(struct notifier_block *nb,
				  unsigned long event, void *ptr)
{
	struct usb_phy *phy = container_of(nb, struct usb_phy, vbus_nb);
	struct sprd_hsphy *hsphy = container_of(phy, struct sprd_hsphy, phy);

	dev_dbg(hsphy->dev, "vbUS notify event=%lu\n", event);
	sprd_hsphy_vbus_set(hsphy, event != 0);
	return NOTIFY_DONE;
}

/*
 * charger_detect — memakai API mainline (lihat DEVIASI #1 di kepala berkas).
 * `sprd_pmic_detect_charger_type()` membaca register PMIC 0x1B9C
 * (SPRD_SC2730_CHG_DET) dan menunggu SPRD_PMIC_CHG_DET_DONE = BIT(11) —
 * bit yang SAMA dengan yang di-poll blob (disasm `tbnz w8,#0xb`),
 * dibuktikan silang di drivers/mfd/sprd-sc27xx-spi.c:23,31.
 */
static enum usb_charger_type sprd_hsphy_charger_detect(struct usb_phy *phy)
{
	struct sprd_hsphy *hsphy = container_of(phy, struct sprd_hsphy, phy);

	if (!hsphy->pmic_dev) {
		dev_dbg(hsphy->dev, "charger_detect tidak tersedia (tanpa PMIC)\n");
		return UNKNOWN_TYPE;
	}
	return sprd_pmic_detect_charger_type(hsphy->pmic_dev);
}

/* --- sysfs: vdd_voltage (show/store), meniru atribut blob --- */
static ssize_t vdd_voltage_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sprd_hsphy *hsphy = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", hsphy->vdd_voltage);
}

static ssize_t vdd_voltage_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct sprd_hsphy *hsphy = dev_get_drvdata(dev);
	u32 val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	if (!hsphy->vdd)
		return -ENODEV;

	ret = regulator_set_voltage(hsphy->vdd, val, val);
	if (ret) {
		dev_err(dev, "nilai tegangan %u tidak diterima: %d\n", val, ret);
		return ret;
	}
	hsphy->vdd_voltage = val;
	return count;
}
static DEVICE_ATTR_RW(vdd_voltage);

static struct attribute *sprd_hsphy_attrs[] = {
	&dev_attr_vdd_voltage.attr,
	NULL,
};

static const struct attribute_group sprd_hsphy_attr_group = {
	.attrs = sprd_hsphy_attrs,
};

static const struct attribute_group *sprd_hsphy_attr_groups[] = {
	&sprd_hsphy_attr_group,
	NULL,
};

/* ---------------------------------------------------------------------------
 * probe / remove
 * ------------------------------------------------------------------------ */
static void sprd_hsphy_put_pmic(void *data)
{
	struct sprd_hsphy *hsphy = data;

	if (hsphy->pmic_dev)
		put_device(hsphy->pmic_dev);
}

static int sprd_hsphy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sprd_hsphy *hsphy;
	struct device_node *pmic_np;
	struct platform_device *pmic_pdev;
	int ret;

	hsphy = devm_kzalloc(dev, sizeof(*hsphy), GFP_KERNEL);
	if (!hsphy)
		return -ENOMEM;

	hsphy->dev = dev;
	hsphy->phy.dev = dev;
	hsphy->phy.label = "sprd-hsphy";
	hsphy->phy.init = sprd_hsphy_init;
	hsphy->phy.shutdown = sprd_hsphy_shutdown;
	hsphy->phy.set_vbus = sprd_hsphy_set_vbus;
	hsphy->phy.charger_detect = sprd_hsphy_charger_detect;
	hsphy->phy.vbus_nb.notifier_call = sprd_hsphy_vbus_notify;

	if (of_property_read_u32(np, "sprd,vdd-voltage", &hsphy->vdd_voltage)) {
		/* Papan ini menulis 3.050.000 uV (FDT stok). Kalau properti
		 * hilang, JANGAN menebak: pakai rentang minimum regulator. */
		dev_warn(dev, "sprd,vdd-voltage tidak ada di DT\n");
		hsphy->vdd_voltage = 0;
	}

	hsphy->vdd = devm_regulator_get_optional(dev, "vdd");
	if (IS_ERR(hsphy->vdd)) {
		int err = PTR_ERR(hsphy->vdd);

		if (err != -ENODEV)
			return dev_err_probe(dev, err, "gagal mengambil regulator vdd\n");
		hsphy->vdd = NULL;
		dev_warn(dev, "vdd-supply tidak ada di DT: lanjut tanpa rail\n");
	}

	hsphy->aon_apb = syscon_regmap_lookup_by_phandle(np,
							 "sprd,syscon-enable");
	if (IS_ERR(hsphy->aon_apb))
		return dev_err_probe(dev, PTR_ERR(hsphy->aon_apb),
				     "gagal mengambil regmap AON APB\n");

	hsphy->anag2 = syscon_regmap_lookup_by_phandle(np,
						       "sprd,syscon-anag2");
	if (IS_ERR(hsphy->anag2))
		return dev_err_probe(dev, PTR_ERR(hsphy->anag2),
				     "gagal mengambil regmap ANLG PHY G2\n");

	/*
	 * Device PMIC untuk charger detect. Blob MEWAJIBKAN ini (probe gagal
	 * bila tidak dapat: `_dev_err` di sprd_hsphy_probe+0x198/0x180), tetapi
	 * di pohon ini regmap PMIC mentah tidak punya penyedia, sedangkan
	 * `sprd_pmic_detect_charger_type()` hanya butuh struct device PMIC.
	 * Jadi di sini kekurangan PMIC = charger-detect mati, BUKAN PHY mati:
	 * PHY adalah gerbang USB, dan register PMIC tidak dipakai di init.
	 */
	pmic_np = of_find_compatible_node(NULL, NULL, "sprd,sc2730");
	if (pmic_np) {
		pmic_pdev = of_find_device_by_node(pmic_np);
		of_node_put(pmic_np);
		if (pmic_pdev) {
			hsphy->pmic_dev = &pmic_pdev->dev;
			ret = devm_add_action_or_reset(dev, sprd_hsphy_put_pmic,
						       hsphy);
			if (ret)
				return ret;
		}
	}
	if (!hsphy->pmic_dev)
		dev_warn(dev, "PMIC sc2730 tidak ditemukan: charger-detect nonaktif\n");

	platform_set_drvdata(pdev, hsphy);
	device_init_wakeup(dev, true);

	/* CATATAN penting: `type` TIDAK boleh diset sebelum pendaftaran —
	 * usb_add_phy() menolak PHY yang type-nya sudah terisi
	 * (drivers/usb/phy/phy.c:638-641). Kesalahan yang sama pernah menahan
	 * glue MUSB (EINVAL -22). */
	ret = usb_add_phy_dev(&hsphy->phy);
	if (ret) {
		dev_err(dev, "gagal mendaftarkan usb_phy: %d\n", ret);
		return ret;
	}

	ret = device_add_groups(dev, sprd_hsphy_attr_groups);
	if (ret)
		dev_warn(dev, "gagal membuat atribut sysfs: %d\n", ret);

	dev_info(dev, "sprd-hsphy siap (vdd %u uV)\n", hsphy->vdd_voltage);
	return 0;
}

static void sprd_hsphy_remove(struct platform_device *pdev)
{
	struct sprd_hsphy *hsphy = platform_get_drvdata(pdev);

	device_remove_groups(&pdev->dev, sprd_hsphy_attr_groups);
	usb_remove_phy(&hsphy->phy);
	if (hsphy->initialized)
		sprd_hsphy_shutdown(&hsphy->phy);
}

static const struct of_device_id sprd_hsphy_of_match[] = {
	/* Urutan mengikuti DT vendor (kompatibel terlemah dulu). Node stok
	 * memakai dua-duanya: `sprd,sharkl5-phy;sprd,sharkl5pro-phy`. */
	{ .compatible = "sprd,sharkl5-phy" },
	{ .compatible = "sprd,sharkl5pro-phy" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sprd_hsphy_of_match);

static struct platform_driver sprd_hsphy_driver = {
	.probe	= sprd_hsphy_probe,
	.remove	= sprd_hsphy_remove,
	.driver	= {
		.name		= "sprd-hsphy",
		.of_match_table	= sprd_hsphy_of_match,
	},
};
module_platform_driver(sprd_hsphy_driver);

MODULE_AUTHOR("VXTux port (RE dari blob vendor phy-sprd-sharkl5Pro.ko)");
MODULE_DESCRIPTION("Unisoc UMS512/T618 USB2 HS PHY (sharkl5pro/sharkl5)");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sprd-hsphy");
