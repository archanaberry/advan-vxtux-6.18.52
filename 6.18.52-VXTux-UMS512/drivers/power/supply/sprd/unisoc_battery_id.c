// SPDX-License-Identifier: GPL-2.0-only
/*
 * unisoc_battery_id - port jalur deteksi battery-ID dari donor unisoc_battery.c.
 *
 * Sumber: work/donors_20260930/{bocchi_unisoc54,marohinmark_ums512}/
 *         drivers/power/supply/unisoc_battery.c (632 baris, md5 identik
 *         antar kedua donor). Yang di-port HANYA jalur
 *         fgauge_get_profile_id() -> battery_type_check() (donor baris
 *         128-253). Sisanya (OPLUS gauge subsystem: oplus_gauge_chip,
 *         netlink, sysfs, gauge_get_property) tidak punya padanan atau
 *         konsumen di pohon ini, jadi tidak disalin.
 *
 * Kontrak dengan sprd_battery_info.c: nilai balik battery_id adalah INDEX
 * ke phandle list "monitored-battery" node fuel gauge (semantik donor).
 *
 * Kontrak DT (setia pada donor):
 *   - io-channel "batt_id" pada node konsumen -> resistor ID baterai via ADC
 *   - "ntc_switch_gpio" -> GPIO switch NTC (hanya dipakai donor saat
 *     is_batt_id_check()==1; DIHILANGKAN di sini karena DT kita tidak punya
 *     GPIO tersebut dan jalur itu mati total di perangkat ini)
 *   - "FUELGAGUE_APPLY" -> flag fuelgauge_apply (default donor 1)
 *
 * ums512-1h10-vxtux.dts tidak punya io-channel batt_id maupun gpio, jadi
 * fallback DONOR sendiri yang berlaku: batt_id == NULL -> battery_id 0
 * (profil baterai tunggal). Itu jalur kode donor (unisoc_battery.c:135-139),
 * BUKAN stub karang di sini - dengan satu profil baterai di DT, id 0 memang
 * profil yang benar.
 *
 * Yang di-drop dari donor (tidak ada padanan di pohon 6.18):
 *   - get_hardware_info_data()/HWID_* (bookkeeping hardware-info OPLUS)
 *   - is_batt_id_check()==1 NTC-switch path (butuh ntc_switch_gpio)
 *   - is_batt_id_check()==2 direct-ADC path (butuh batt_id_fast_chcek,
 *     varian DT lain; default donor 0)
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/iio/consumer.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* donor unisoc_battery.c:71-88 */
#define BAT_LIWEI_BATT_ID	1
#define BAT_GUANYU_BATT_ID	1
#define BAT_ATL_BATT_ID		0

#define BAT_TYPE__LIWEI_4450mV_NTC_MIN	70
#define BAT_TYPE__LIWEI_4450mV_NTC_MAX	270
#define BAT_TYPE__GUANYU_4450mV_NTC_MIN	310
#define BAT_TYPE__GUANYU_4450mV_NTC_MAX	510
#define BAT_TYPE__ATL_4450mV_NTC_MIN	630
#define BAT_TYPE__ATL_4450mV_NTC_MAX	810

static struct device *batt_id_dev;
static struct iio_channel *batt_id_chan;
static int fuelgauge_apply = 1;	/* donor: fg_read_dts_val(..., 1) */

/*
 * Dipanggil sprd_battery_info.c sebelum fgauge_get_profile_id() dengan
 * &psy->dev, supaya lookup io-channel "batt_id" pakai node DT yang benar.
 */
void unisoc_battery_id_set_dev(struct device *dev)
{
	batt_id_dev = dev;
}
EXPORT_SYMBOL_GPL(unisoc_battery_id_set_dev);

static bool is_fuelgauge_apply(void)
{
	return fuelgauge_apply != 0;
}

/*
 * Port donor unisoc_battery.c:244-253 + battery_type_check() jalur default
 * (is_batt_id_check()==0, donor baris 205-238). Nilai ADC dibandingkan
 * rentang NTC donor; di luar semua rentang -> BAT_TYPE__UNKNOWN -> id 0.
 */
int fgauge_get_profile_id(void)
{
	int ret_value = 0;
	int ret, value, battery_id = 0;

	/*
	 * Fallback donor (unisoc_battery.c:135-139): tanpa channel batt-id
	 * (atau flag mati) -> battery_id 0. Di DT satu-profil ini hasil yang
	 * benar; kalau nanti DT dapat channel, jalur ADC di bawah yang dipakai.
	 */
	if (!is_fuelgauge_apply() || !batt_id_dev)
		return 0;

	if (!batt_id_chan) {
		batt_id_chan = devm_iio_channel_get(batt_id_dev, "batt_id");
		if (IS_ERR_OR_NULL(batt_id_chan)) {
			dev_warn(batt_id_dev,
				 "battery ID channel err, fallback profile 0\n");
			batt_id_chan = NULL;
			return 0;
		}
	}

	ret = iio_read_channel_processed(batt_id_chan, &ret_value);
	if (ret < 0) {
		dev_warn(batt_id_dev,
			 "battery ID read err %d, fallback profile 0\n", ret);
		return 0;
	}
	value = ret_value;

	if (value >= BAT_TYPE__LIWEI_4450mV_NTC_MIN &&
	    value <= BAT_TYPE__LIWEI_4450mV_NTC_MAX)
		battery_id = BAT_LIWEI_BATT_ID;
	else if (value >= BAT_TYPE__GUANYU_4450mV_NTC_MIN &&
		 value < BAT_TYPE__GUANYU_4450mV_NTC_MAX)
		battery_id = BAT_GUANYU_BATT_ID;
	else if (value >= BAT_TYPE__ATL_4450mV_NTC_MIN &&
		 value < BAT_TYPE__ATL_4450mV_NTC_MAX)
		battery_id = BAT_ATL_BATT_ID;
	else
		battery_id = 0;

	pr_info("[battery_id]: adc_value[%d], battery_id[%d]\n", value,
		battery_id);

	return battery_id;
}
EXPORT_SYMBOL_GPL(fgauge_get_profile_id);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Unisoc battery ID detection (donor unisoc_battery.c port)");
