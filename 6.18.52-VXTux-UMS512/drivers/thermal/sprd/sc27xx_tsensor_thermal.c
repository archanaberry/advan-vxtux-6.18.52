// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Spreadtrum Communications Inc.
 *
 * sc27xx_tsensor_thermal - PMIC (SC27xx/SC2730) temperature sensor bridge.
 *
 * Port vendor 5.4 -> 6.18.  Sumber GPL:
 * work/donors_20260930/bocchi_unisoc54/drivers/thermal/sc27xx_tsensor_thermal.c
 * (md5 identik dengan marohinmark). Bukti blob:
 * work/re_batt_20260930/out/sc27xx_tsensor_thermal.ko.*
 * Ringkasan RE: docs/baterai_termal_bedah_20260930.md bagian 2.
 *
 * PERINGATAN KALIBRASI (terukur, bukan asumsi): probe mewajibkan
 * "androidboot.mode=cali" di /chosen/bootargs. Tanpa itu driver keluar dengan
 * dev_warn dan mendaftarkan NOL zona termal - perilaku ini milik blob, bukan
 * stub yang kita karang. Jalur OUT (id=1) juga memakai v2t_table yang belum
 * terkalibrasi (136 entri pertama = plateau), lihat RE bagian 10 butir 5.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/string.h>
#include <linux/thermal.h>

#define SC27XX_XTL_WAIT_CTRL0		0x1b78
#define SC27XX_XTL_EN			BIT(8)
#define	SC27XX_TSEN_CTRL0		0x0
#define SC27XX_TSEN_CLK_SRC_SEL		BIT(4)
#define	SC27XX_TSEN_ADCLDO_EN		BIT(15)
#define	SC27XX_TSEN_CTRL1		0x4
#define SC27XX_TSEN_SDADC_EN		BIT(11)
#define	SC27XX_TSEN_UGBUF_EN		BIT(14)
#define	SC27XX_TSEN_CTRL3		0xc
#define SC27XX_TSEN_EN			BIT(0)
#define SC27XX_TSEN_SEL_EN		BIT(3)
#define SC27XX_TSEN_SEL_CH		BIT(4)
#define	SC27XX_TSEN_CTRL4		0x10
#define	SC27XX_TSEN_CTRL5		0x14
#define SC27XX_TSEN_TEMP_MASK		GENMASK(15, 0)
#define SC27XX_TSEN_INDEX_MASK		GENMASK(7, 0)
#define SC27XX_TSEN_INDEX_SHIFT		8
#define SC27XX_TSEN_FRAC_MASK		GENMASK(7, 0)

/*
 * Kemiringan jalur OSC, dari instruksi blob (bukan dari sumber vendor).
 *
 * Konstanta ajaib di .text = 0xAE147AE1 -> 17/800 per LSB, yaitu
 * 165,1125 mC/LSB. Sumber GPL vendor menulis "7770 * rawdata / 100"
 * (77,7 mC/LSB) - TIDAK sama. 165,1125 == 7770 * 17 / 800 persis, jadi offset
 * 3880400 tetap dan hanya pembagi yang berubah. Angka instruksi yang dipakai.
 * Bukti: docs/baterai_termal_bedah_20260930.md bagian 2.1 dan 10 butir 3.
 */
#define SC27XX_TSEN_OSC_OFFSET		3880400		/* mC */
#define SC27XX_TSEN_OSC_SLOPE_MC	1651125		/* mC/LSB * 1000 */

/* Temperature query table according to integral index */
static const int v2t_table[256] = {
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119,
	298119, 298119, 298119, 298119, 298119, 298119, 298119, 298119, 280421, 264933, 251238, 238928, 227748, 217492, 208038, 199268,
	191078, 183383, 176104, 169240, 162710, 156500, 150570, 144893, 139428, 134191, 129138, 124253, 119525, 114943, 110500, 106187,
	101990, 97905, 93919, 90026, 86224, 82506, 78860, 75286, 71777, 68330, 64940, 61606, 58337, 55113, 51932, 48790,
	45696, 42642, 39620, 36627, 33675, 30750, 27848, 24974, 22128, 19300, 16490, 13707, 10937, 8180, 5444, 2717,
	0,      -2702, -5398, -8088, -10768, -13446, -16120, -18791, -21462, -24132, -26805, -29481, -32160, -34847, -37539, -40241,
	-42956, -45680, -48419, -51175, -53944, -56734, -59548, -62379, -65234, -68124, -71039, -73979, -76964, -79985, -83041, -86136,
	-89290, -92493, -95747, -99057, -102428, -105886, -109418, -113031, -116733, -120533, -124438,  -128458, -132609, -136903, -141357,
	-145991, -150823, -155881, -161216, -166830, -172781, -179140, -185969, -193354, -201441, -210423, -220528, -232181, -245984,
	-263191, -263191, -263191,
};

static bool cali_mode;
static DEFINE_MUTEX(tsen_mutex);

struct sc27xx_tsen {
	struct device *dev;
	struct thermal_zone_device *tz_dev;
	struct regmap *regmap;
	u32 base;
	int id;
};

static int sc27xx_tsensor_read_config(struct sc27xx_tsen *tsen)
{
	int ret;

	ret = regmap_update_bits(tsen->regmap, SC27XX_XTL_WAIT_CTRL0,
				 SC27XX_XTL_EN, SC27XX_XTL_EN);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL0,
				 SC27XX_TSEN_CLK_SRC_SEL, 0x0);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL0,
				 SC27XX_TSEN_ADCLDO_EN, SC27XX_TSEN_ADCLDO_EN);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL1,
				 SC27XX_TSEN_SDADC_EN, SC27XX_TSEN_SDADC_EN);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL1,
				 SC27XX_TSEN_UGBUF_EN, SC27XX_TSEN_UGBUF_EN);
	if (ret)
		return ret;

	return regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL3,
				 SC27XX_TSEN_SEL_EN, SC27XX_TSEN_SEL_EN);
}

static int sc27xx_tsensor_osc_rawdata_read(struct sc27xx_tsen *tsen, int *rawdata)
{
	unsigned int val;
	int ret;

	ret = sc27xx_tsensor_read_config(tsen);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL3,
				 SC27XX_TSEN_SEL_CH, SC27XX_TSEN_SEL_CH);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL3,
				 SC27XX_TSEN_EN, SC27XX_TSEN_EN);
	if (ret)
		return ret;

	/*
	 * According to the requirements of design document,
	 * trigger a data sample and wait data ready need wait 21ms
	 */
	msleep(21);

	ret = regmap_read(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL5, &val);
	if (ret)
		return ret;

	*rawdata = val & SC27XX_TSEN_TEMP_MASK;

	return ret;
}

static int sc27xx_tsensor_osc_temp_read(struct sc27xx_tsen *tsen, int *temp)
{
	int ret, rawdata;

	mutex_lock(&tsen_mutex);
	ret = sc27xx_tsensor_osc_rawdata_read(tsen, &rawdata);
	mutex_unlock(&tsen_mutex);
	if (ret)
		return ret;

	/*
	 * Offset 3880400 mC dan kemiringan 165,1125 mC/LSB (lihat define di
	 * atas). Dihitung dalam s64 supaya perkalian tidak melimpah.
	 */
	*temp = SC27XX_TSEN_OSC_OFFSET -
		(int)div_s64((s64)rawdata * SC27XX_TSEN_OSC_SLOPE_MC, 1000);

	return ret;
}

static int sc27xx_tsensor_out_rawdata_read(struct sc27xx_tsen *tsen, int *rawdata)
{
	unsigned int val;
	int ret;

	ret = sc27xx_tsensor_read_config(tsen);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL3,
				 SC27XX_TSEN_SEL_CH, 0x0);
	if (ret)
		return ret;

	ret = regmap_update_bits(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL3,
				 SC27XX_TSEN_EN, SC27XX_TSEN_EN);
	if (ret)
		return ret;

	/*
	 * According to the requirements of design document,
	 * trigger a data sample and wait data ready need wait 21ms
	 */
	msleep(21);

	ret = regmap_read(tsen->regmap, tsen->base + SC27XX_TSEN_CTRL4, &val);
	if (ret)
		return ret;

	*rawdata = val & SC27XX_TSEN_TEMP_MASK;

	return ret;
}

static int sc27xx_tsensor_out_temp_read(struct sc27xx_tsen *tsen, int *temp)
{
	int ret, rawdata, index, frac, t;

	mutex_lock(&tsen_mutex);
	ret = sc27xx_tsensor_out_rawdata_read(tsen, &rawdata);
	mutex_unlock(&tsen_mutex);
	if (ret)
		return ret;

	index = (rawdata >> SC27XX_TSEN_INDEX_SHIFT) & SC27XX_TSEN_INDEX_MASK;
	frac = rawdata & SC27XX_TSEN_FRAC_MASK;

	/*
	 * According to the requirements of design document,get the index accrding
	 * to the integral result, and query the current temperature value
	 * according to the index.
	 * t = (v2t[index] * (0x100-frac) + v2t[index+1] * frac + 0x800) / 256;
	 */
	if (index != (int)(sizeof(v2t_table) / 4) - 1)
		t = (v2t_table[index] * (0x100 - frac) +
		     v2t_table[index + 1] * frac + 0x800) / 256;
	else
		t = v2t_table[index];

	/*
	 * According to the requirements of design document,
	 * tsensor out temp = (t * 1000) / 4096 + 25000
	 */
	*temp = (t * 1000) / 4096 + 25000;

	return ret;
}

static int sc27xx_tsensor_disable(struct regmap *regmap, u32 base)
{
	int ret;

	ret = regmap_update_bits(regmap, base + SC27XX_TSEN_CTRL0,
				SC27XX_TSEN_CLK_SRC_SEL, SC27XX_TSEN_CLK_SRC_SEL);
	if (ret)
		return ret;

	ret = regmap_update_bits(regmap, base + SC27XX_TSEN_CTRL3,
				SC27XX_TSEN_EN | SC27XX_TSEN_SEL_CH |
				SC27XX_TSEN_SEL_EN, 0);
	if (ret)
		return ret;

	ret = regmap_update_bits(regmap, base + SC27XX_TSEN_CTRL1,
				SC27XX_TSEN_SDADC_EN | SC27XX_TSEN_UGBUF_EN, 0);
	if (ret)
		return ret;

	return regmap_update_bits(regmap, base + SC27XX_TSEN_CTRL0,
				SC27XX_TSEN_ADCLDO_EN, 0);
}

static int get_boot_mode(void)
{
	struct device_node *cmdline_node;
	const char *cmd_line;
	int ret;

	/*
	 * Vendor 5.4 di sini bocorin node /chosen (tanpa of_node_put). Kernels
	 * baru makin paranoid soal itu, jadi dilepas di sini.
	 */
	cmdline_node = of_find_node_by_path("/chosen");
	if (!cmdline_node)
		return -EINVAL;

	ret = of_property_read_string(cmdline_node, "bootargs", &cmd_line);
	of_node_put(cmdline_node);
	if (ret)
		return ret;

	cali_mode = strstr(cmd_line, "androidboot.mode=cali") != NULL;

	return 0;
}

/*
 * Migrasi API: vendor 5.4 memakai struct thermal_zone_of_device_ops
 * (get_temp(void *devdata, int *)); 6.18 memakai
 * struct thermal_zone_device_ops
 * (get_temp(struct thermal_zone_device *, int *)), dan devdata diambil lewat
 * accessor publik thermal_zone_device_priv().
 *
 * Deviasi yang dinyatakan: blob mengembalikan -524 untuk id >= 2, sumber GPL
 * menulis -ENOTSUPP (-95). Yang dipakai di sini -ENOTSUPP karena -524 tidak
 * punya arti errno di 6.18; angka biner dicatat di
 * docs/baterai_termal_bedah_20260930.md bagian 10 butir 4.
 */
static int sc27xx_tsensor_get_temp(struct thermal_zone_device *tz, int *temp)
{
	struct sc27xx_tsen *tsen = thermal_zone_device_priv(tz);
	int ret;

	if (!tsen || !temp)
		return -EINVAL;

	if (tsen->id == 0)
		ret = sc27xx_tsensor_osc_temp_read(tsen, temp);
	else if (tsen->id == 1)
		ret = sc27xx_tsensor_out_temp_read(tsen, temp);
	else
		ret = -ENOTSUPP;

	return ret;
}

static const struct thermal_zone_device_ops tsensor_thermal_ops = {
	.get_temp = sc27xx_tsensor_get_temp,
};

static int sc27xx_tsen_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *sen_child = NULL;
	u32 id;
	struct sc27xx_tsen *tsen;
	struct regmap *regmap;
	u32 base;
	int ret;

	ret = get_boot_mode();
	if (ret) {
		pr_err("boot_mode can't not parse bootargs property\n");
		return ret;
	}

	if (!cali_mode) {
		dev_warn(&pdev->dev,
			 "no calibration mode, don't register sc27xx tsen");
		return 0;
	}

	/* Pola identik drivers/nvmem/sc27xx-efuse.c:214 */
	regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!regmap) {
		dev_err(&pdev->dev, "failed to get efuse regmap\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "reg", &base);
	if (ret) {
		dev_err(&pdev->dev, "failed to get efuse base address\n");
		return ret;
	}

	for_each_child_of_node(np, sen_child) {
		tsen = devm_kzalloc(&pdev->dev, sizeof(*tsen), GFP_KERNEL);
		if (!tsen) {
			ret = -ENOMEM;
			goto err_node_put;
		}

		tsen->dev = &pdev->dev;
		tsen->regmap = regmap;
		tsen->base = base;

		ret = of_property_read_u32(sen_child, "reg", &id);
		if (ret) {
			dev_err(&pdev->dev, "get sensor reg failed");
			goto err_node_put;
		}
		tsen->id = id;

		/*
		 * thermal_zone_of_sensor_register() (4 argumen di vendor 5.4)
		 * sudah DIHAPUS di 6.18; penggantinya
		 * devm_thermal_of_zone_register(). Catatan: fungsi itu mencari
		 * node zona lewat of_thermal_zone_find(sensor, id), jadi node
		 * sensor WAJIB punya #thermal-sensor-cells DAN dirujuk property
		 * thermal-sensors dari sebuah node thermal-zones.
		 */
		tsen->tz_dev = devm_thermal_of_zone_register(&pdev->dev,
							     tsen->id,
							     tsen,
							     &tsensor_thermal_ops);
		if (IS_ERR(tsen->tz_dev)) {
			ret = PTR_ERR(tsen->tz_dev);
			dev_err(&pdev->dev, "Thermal zone register failed\n");
			goto err_node_put;
		}
	}

	of_node_put(sen_child);

	return sc27xx_tsensor_disable(regmap, base);

err_node_put:
	of_node_put(sen_child);
	return ret;
}

static const struct of_device_id sc27xx_tsen_of_match[] = {
	{ .compatible = "sprd,sc2730-tsensor",},
	/* FDT stok memakai kedua nama itu; scr-otp yang generik sengaja
	 * dipertahankan agar DTS vendor tidak butuh suntingan. */
	{ .compatible = "sprd,sc27xx-tsensor",},
	{ }
};
MODULE_DEVICE_TABLE(of, sc27xx_tsen_of_match);

static struct platform_driver sc27xx_tsen_driver = {
	.probe = sc27xx_tsen_probe,
	.driver = {
		.name = "sc27xx-tsensor",
		.of_match_table = sc27xx_tsen_of_match,
	},
};
module_platform_driver(sc27xx_tsen_driver);

MODULE_DESCRIPTION("Spreadtrum SC27xx tsensor driver");
MODULE_LICENSE("GPL v2");
