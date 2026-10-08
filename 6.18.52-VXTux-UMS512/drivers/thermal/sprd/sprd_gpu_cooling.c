// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Unisoc Inc.
 *
 * sprd_gpu_cooling - model daya GPU (static leakage + dynamic) untuk
 * devfreq_cooling, direkonstruksi dari blob vendor `sprd_gpu_cooling.ko`.
 *
 * Sumber kebenaran hasil RE (semuanya ada di repo ini):
 *   tools/re/re_gpu_cooling_model.py   model bit-eksak + bukti C == assembly
 *   dumped/re/sprd_gpu_cooling.ko.dis.txt   disasm blob (llvm-objdump -dr)
 *   blobs/re_notes/sprd_gpu_cooling.sym     tabel simbol/ksymtab
 *   blobs/fdt_live.dtb -> /gpu-cooling-devices/gpu-cooling0
 *                                          koefisien power model yang asli
 *   tools/re/re_fdt_gpu_cooling.py     pembaca node DT di atas
 *
 * Bentuk ABI vendor dipertahankan apa adanya: init_module() milik blob hanya
 * `return 0` dan cleanup_module() kosong -- perangkat cooling dibuat atas
 * permintaan pemanggil luar lewat dua ekspor:
 *   int create_gpu_cooling_device(struct devfreq *, struct device *);
 *   int destroy_gpu_cooling_device(void);
 * (Nama ekspor sama dengan blob; dipakai kbase/DDK Mali yang tidak ada di
 * pohon ini, jadi driver ini provider tanpa konsumen in-tree.)
 *
 * Satuan (dibuktikan, bukan diasumsikan):
 *   tegangan dalam mV dan hasil dalam mW. Polinomial `sprd,voltage-scale`
 *   <801 -1712 1335 -324> hanya "seimbang" (ketiga sukunya seorde) pada
 *   V ~ 800-1000; dan dengan mV rumus dinamis menghasilkan ~607 mW pada
 *   850 MHz / 0,8 V, wajar untuk Mali-G57. Dengan uV hasilnya 10^5 kali
 *   lipat (ratusan kW) -- mustahil. Framework 6.18 memberi uV, jadi di
 *   sprd_gpu_get_real_power() tegangan dibagi 1000 lebih dulu.
 *
 * Adaptasi 5.4 -> 6.18 (tanpa shim, tanpa stub):
 *   - 5.4 punya devfreq_cooling_power.{get_static_power,get_dynamic_power};
 *     6.18 hanya menyisakan get_real_power(). Kedua callback vendor
 *     DIGABUNG di sini (static + dynamic), dan itu memang cara mainline
 *     6.18 memakai nilainya: dibandingkan langsung dengan em_power_mw.
 *   - strlcpy() dihapus di 6.8 -> strscpy().
 *   - tz->ops->get_temp(tz, &t) -> thermal_zone_get_temp(tz, &t) (API
 *     publik sejak 5.9; sekaligus mengambil lock zona yang tidak diambil
 *     versi vendor).
 *   - of_find_node_by_name()/of_get_next_child() di of_node_put() dengan
 *     benar; blob vendor membocorkan satu referensi setiap probe.
 *   - kmalloc() -> kcalloc(): blob mengindeks slot dengan alias-id DT dan
 *     bisa membaca memori tak-terinisialisasi bila id-nya berlubang.
 *   - Batas iterasi tabel slot pada unregister ditutup (blob memindai
 *     melewati akhir array sampai kebetulan cocok).
 *
 * Batasan yang JUSTRU dipertahankan dari blob (agar perilaku identik):
 *   - get_real_power selalu memakai slot 0 (`cluster_data[0]`), sama seperti
 *     blob; callback tidak menerima pointer data sehingga tidak ada cara
 *     memilih slot lain. Untuk gpu-cooling0 (satu-satunya node di board ini,
 *     alias gpu-cooling0) ini tepat.
 *   - `sprd,hotplug-period` hanya diperiksa, tidak menghentikan probe.
 *   - Default suhu 55000 mC bila zona termal tidak ada / gagal dibaca.
 *   - `sprd,cluster-base` dan `sprd,dynamic-cluster` dibaca dan disimpan
 *     tetapi tidak dipakai aritmetika mana pun di blob (hanya jalur
 *     core yang ada di dua callback ini).
 */

#include <linux/devfreq.h>
#include <linux/devfreq_cooling.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/thermal.h>

#include "sprd_gpu_cooling.h"

#define SPRD_GPU_COOLING_NAME		"sprd_gpu_cooling"
#define SPRD_GPU_COOLING_DEVS_NODE	"gpu-cooling-devices"
#define SPRD_GPU_COOLING_COMPAT		"sprd,mali-power-model"
#define SPRD_GPU_COOLING_TZ_NAME	"gpu-thmzone"
#define SPRD_GPU_COOLING_ID		"gpu-cooling"

/* suhu default (mC) saat zona termal tak tersedia -- sama dengan blob */
#define SPRD_GPU_DEFAULT_TEMP_MC	55000

/* panjang maksimum nama node yang disimpan; blob: stride 0x70 - offset 0x50 */
#define SPRD_GPU_NAME_LEN		32

/*
 * Tata letak struct DIREKONSTRUKSI dari offset yang benar-benar dibaca blob
 * (stride 0x70). Jangan ubah urutannya tanpa memperbarui
 * tools/re/re_gpu_cooling_model.py -- model itu menguji per-offset.
 */
struct sprd_gpu_cooling_data {
	u32 hotplug_period;	/* +0x00 sprd,hotplug-period      */
	u32 core_base;		/* +0x04 sprd,core-base           */
	u32 cluster_base;	/* +0x08 sprd,cluster-base (tak dipakai) */
	s32 temp_scale[4];	/* +0x0c sprd,temp-scale          */
	s32 volt_scale[4];	/* +0x1c sprd,voltage-scale       */
	s32 dyn_core[3];	/* +0x2c sprd,dynamic-core        */
	s32 dyn_cluster[3];	/* +0x38 sprd,dynamic-cluster (tak dipakai) */
	struct thermal_cooling_device *cdev;	/* +0x48 */
	char name[SPRD_GPU_NAME_LEN];		/* +0x50 */
};

static struct sprd_gpu_cooling_data *cluster_data;
static int cluster_num;
static struct thermal_zone_device *gpu_tz;

/*
 * Seluruh aritmetika 64-bit di bawah memakai semantik wrap mod 2^64 (itulah
 * yang dilakukan mul/madd AArch64 di blob) dan pembagian UNSIGNED. Pembagian
 * oleh konstanta (125/3125/10000/1e8) dikompilasi clang kembali menjadi
 * umulh+shift yang persis sama dengan blob, sehingga hasilnya bit-identik.
 * Bukti: tools/re/re_gpu_cooling_model.py (0 beda atas 408 kombinasi).
 *
 * Nama `get_static_power()`/`get_dynamic_power()` SENGAJA dipertahankan sama
 * dengan blob (keduanya `static` di sana, jadi tidak ada polusi namespace).
 * Selain setia pada sumber yang di-RE, ini yang membuat paritas tingkat-fungsi
 * di dumped/re/RE_INDEX.tsv (pencocokan berbasis NAMA) bisa diaudit: ganti nama
 * helper ini dan modul akan salah terbaca sebagai "belum di-RE".
 */
static u64 get_static_power(const struct sprd_gpu_cooling_data *cd,
			    unsigned long voltage_mv, int temp_mc)
{
	s64 t = div_s64((s64)temp_mc, 1000);	/* blob: smull 0x10624dd3 >> 38 */
	u64 v = (u64)voltage_mv;
	u64 x10, x12, x9, x7, x6;
	s64 temp_poly;
	s32 w14;

	/* x10 = V^3 / 1000                                  (V^3 >> 3, lalu /125) */
	x10 = ((v * v) * v) >> 3;
	x10 = x10 / 125;
	/* 0x5dc: mul x10, x10, x14   (volt_scale[0]) */
	x10 *= (u64)(s64)cd->volt_scale[0];
	/* blob menghitung volt_scale[3]*1000000 dalam 32 bit (mul w14,w14,w15) */
	w14 = (s32)((u32)cd->volt_scale[3] * 1000000u);
	x10 += (u64)(s64)w14;
	/* x12 = volt_scale[2]*1000 + volt_scale[1]*V */
	x12 = (u64)((s64)cd->volt_scale[2] * 1000 + (s64)cd->volt_scale[1] * (s64)v);
	/* x9 = x12*V + x10 ; >>5 lalu /3125  =>  / 100000 */
	x9 = (x12 * v + x10) >> 5;
	x7 = x9 / 3125;

	/* polinomial suhu: temp_scale[0]*t^3 + [1]*t^2 + [2]*t + [3] */
	temp_poly = ((s64)cd->temp_scale[0] * t + cd->temp_scale[1]) * t +
		    cd->temp_scale[2];
	temp_poly = temp_poly * t + cd->temp_scale[3];
	x6 = (u64)temp_poly / 10000;

	/* hasil akhir: umulh(M, 0xabcc77118461cefd) >> 26  =>  / 1e8 */
	return (x6 * (u64)cd->core_base * x7) / 100000000ULL;
}

static u64 get_dynamic_power(const struct sprd_gpu_cooling_data *cd,
			     unsigned long freq_hz,
			     unsigned long voltage_mv)
{
	/* blob: (freq & 0xffffffff) * 0x431bde83 >> 50  ==  freq / 1e6 */
	u64 freq_mhz = (u64)(u32)freq_hz / 1000000;
	u64 v = (u64)(u32)voltage_mv;
	u32 c2 = (u32)cd->dyn_core[2];
	u32 den32 = c2 * c2;			/* blob: mul w9, w5, w5 (32-bit) */
	u64 num;

	if (!den32)
		return 0;

	/* umull x11, w4, w4 ; mul x8, x11, x3 ; mul x8, x8, x10 */
	num = v * v * freq_mhz * (u64)(s64)cd->dyn_core[0];
	/* udiv x8, x8, x9 lalu /10000 */
	return (num / (u64)den32) / 10000;
}

static int sprd_gpu_read_temperature(void)
{
	int temp;

	if (!gpu_tz)
		return SPRD_GPU_DEFAULT_TEMP_MC;

	if (thermal_zone_get_temp(gpu_tz, &temp)) {
		pr_err(SPRD_GPU_COOLING_NAME ": Error reading temperature:%d\n",
		       temp);
		return SPRD_GPU_DEFAULT_TEMP_MC;
	}

	return temp;
}

/*
 * Penggabungan dua callback 5.4 menjadi satu get_real_power 6.18.
 * `voltage` dari mainline dalam uV; model vendor bekerja dalam mV.
 * `*power` harus dalam mW (6.18: dibandingkan dengan em_power_mw).
 */
static int sprd_gpu_get_real_power(struct devfreq *devfreq, u32 *power,
				   unsigned long freq, unsigned long voltage)
{
	const struct sprd_gpu_cooling_data *cd = cluster_data;
	unsigned long voltage_mv;
	int temp_mc;
	u64 static_mw, dynamic_mw;

	if (!cd)
		return -ENODEV;

	temp_mc = sprd_gpu_read_temperature();
	voltage_mv = voltage / 1000;

	static_mw = get_static_power(cd, voltage_mv, temp_mc);
	dynamic_mw = get_dynamic_power(cd, freq, voltage_mv);

	pr_debug(SPRD_GPU_COOLING_NAME
		 ": temp:%d m_volt:%lu static_power:%llu\n",
		 temp_mc, voltage_mv, static_mw);

	*power = (u32)(static_mw + dynamic_mw);

	return 0;
}

static struct devfreq_cooling_power sprd_gpu_power_model_ops = {
	.get_real_power = sprd_gpu_get_real_power,
};

static int sprd_gpu_parse_cooling_params(struct device_node *child,
					 struct sprd_gpu_cooling_data *cd)
{
	int ret;

	ret = of_property_read_u32(child, "sprd,core-base", &cd->core_base);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get def cool-dev leak-core-base\n");
		return ret;
	}

	ret = of_property_read_u32(child, "sprd,cluster-base",
				   &cd->cluster_base);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get def cool-dev leak-cluster-base\n");
		return ret;
	}

	ret = of_property_read_u32_array(child, "sprd,temp-scale",
					 (u32 *)cd->temp_scale, 4);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get cooling devices temp-scale\n");
		return ret;
	}

	ret = of_property_read_u32_array(child, "sprd,voltage-scale",
					 (u32 *)cd->volt_scale, 4);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get cooling devices voltage-scale\n");
		return ret;
	}

	ret = of_property_read_u32_array(child, "sprd,dynamic-core",
					 (u32 *)cd->dyn_core, 3);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get cooling devices dynamic-core-coeff\n");
		return ret;
	}

	ret = of_property_read_u32_array(child, "sprd,dynamic-cluster",
					 (u32 *)cd->dyn_cluster, 3);
	if (ret) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": fail to get cooling devices dynamic-cluster-coeff\n");
		return ret;
	}

	/*
	 * Opsional. Blob memeriksa properti ini tetapi TIDAK membatalkan
	 * probe (di sana pesan errornya salah kutip: "efuse_block");
	 * di sini perilakunya sama, hanya pesannya yang benar.
	 */
	if (of_property_read_u32(child, "sprd,hotplug-period",
				 &cd->hotplug_period))
		pr_info(SPRD_GPU_COOLING_NAME
			": optional sprd,hotplug-period missing\n");

	return 0;
}

/*
 * Ekspor ABI (nama & aritas harus sama dengan blob; keduanya
 * EXPORT_SYMBOL biasa, bukan _GPL -- lihat __ksymtab di .sym).
 * Parameter kedua divalidasi non-NULL oleh blob lalu TIDAK dipakai sama
 * sekali (disasm 0x168: cbz x1, ... ; 0x16c x1 langsung ditimpa). Di sini
 * pun hanya dijadikan syarat awal, demi kompatibilitas pemanggil.
 */
int create_gpu_cooling_device(struct devfreq *devfreq, struct device *dev)
{
	struct device_node *np, *child;
	struct sprd_gpu_cooling_data *cd;
	struct thermal_cooling_device *cdev;
	int ret = 0, id, num;

	if (!devfreq || !dev)
		return -ENODEV;

	np = of_find_node_by_name(NULL, SPRD_GPU_COOLING_DEVS_NODE);
	if (!np) {
		pr_err(SPRD_GPU_COOLING_NAME
		       ": unable to find thermal zones\n");
		return -ENODEV;
	}

	num = of_get_child_count(np);
	if (!num) {
		pr_err(SPRD_GPU_COOLING_NAME ": params is not complete!\n");
		ret = -ENODEV;
		goto out_put_np;
	}

	/* kcalloc, bukan kmalloc seperti blob: id alias bisa berlubang */
	cluster_data = kcalloc(num, sizeof(*cluster_data), GFP_KERNEL);
	if (!cluster_data) {
		ret = -ENOMEM;
		goto out_put_np;
	}
	cluster_num = num;

	for_each_child_of_node(np, child) {
		if (!of_device_is_compatible(child, SPRD_GPU_COOLING_COMPAT))
			continue;
		if (!of_device_is_available(child))
			continue;

		id = of_alias_get_id(child, SPRD_GPU_COOLING_ID);
		if (id < 0 || id >= cluster_num) {
			pr_err(SPRD_GPU_COOLING_NAME
			       ": fail to get cooling devices id\n");
			ret = -ENODEV;
			of_node_put(child);
			goto out_free;
		}

		cd = &cluster_data[id];

		ret = sprd_gpu_parse_cooling_params(child, cd);
		if (ret) {
			pr_err(SPRD_GPU_COOLING_NAME
			       ": fail to get power model coeff !\n");
			ret = -EINVAL;
			of_node_put(child);
			goto out_free;
		}

		cdev = of_devfreq_cooling_register_power(child, devfreq,
							 &sprd_gpu_power_model_ops);
		if (IS_ERR_OR_NULL(cdev)) {
			pr_err(SPRD_GPU_COOLING_NAME
			       ": fail to register cool-dev (%d)\n",
			       (int)PTR_ERR_OR_ZERO(cdev));
			ret = -EINVAL;
			of_node_put(child);
			goto out_free;
		}
		cd->cdev = cdev;
		strscpy(cd->name, child->name, sizeof(cd->name));

		gpu_tz = thermal_zone_get_zone_by_name(SPRD_GPU_COOLING_TZ_NAME);
	}

	of_node_put(np);
	return 0;

out_free:
	kfree(cluster_data);
	cluster_data = NULL;
	cluster_num = 0;
out_put_np:
	of_node_put(np);
	return ret;
}
EXPORT_SYMBOL(create_gpu_cooling_device);

int destroy_gpu_cooling_device(void)
{
	struct device_node *np, *child;
	int i;

	if (!cluster_data)
		return 0;

	np = of_find_node_by_name(NULL, SPRD_GPU_COOLING_DEVS_NODE);
	if (!np) {
		pr_err(SPRD_GPU_COOLING_NAME ": unable to find thermal zones\n");
		return -ENODEV;
	}

	for_each_child_of_node(np, child) {
		/*
		 * Blob mencocokkan nama node (strncmp sepanjang nama child);
		 * batas i < cluster_num ditambahkan -- blob memindai melewati
		 * akhir array.
		 */
		for (i = 0; i < cluster_num; i++) {
			if (!cluster_data[i].name[0])
				continue;
			if (strncmp(child->name, cluster_data[i].name,
				    strlen(child->name)))
				continue;
			if (!IS_ERR_OR_NULL(cluster_data[i].cdev))
				devfreq_cooling_unregister(cluster_data[i].cdev);
			cluster_data[i].cdev = NULL;
			break;
		}
	}

	of_node_put(np);

	kfree(cluster_data);
	cluster_data = NULL;
	cluster_num = 0;

	return 0;
}
EXPORT_SYMBOL(destroy_gpu_cooling_device);

static int __init sprd_gpu_cooling_init(void)
{
	/* blob: init_module() = { return 0; } -- tidak ada yang didaftarkan */
	return 0;
}
module_init(sprd_gpu_cooling_init);

MODULE_DESCRIPTION("Spreadtrum GPU devfreq cooling power model");
MODULE_LICENSE("GPL v2");
