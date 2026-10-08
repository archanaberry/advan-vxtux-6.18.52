// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Unisoc Inc.
 *
 * sprd_soc_thm - "virtual thermal": zona termal yang melaporkan MAKSIMUM
 * dari beberapa zona termal nyata yang dirujuk by-name lewat "thmzone-cells".
 *
 * Port vendor 5.4 -> 6.18.  Sumber GPL:
 * work/donors_20260930/bocchi_unisoc54/drivers/thermal/sprd_soc_thm.c
 * Bukti blob: work/re_batt_20260930/out/sprd_soc_thm.ko.*
 * Ringkasan RE: docs/baterai_termal_bedah_20260930.md bagian 3.
 *
 * Tidak ada MMIO dan tidak ada polling: satu thermal_zone_device_update()
 * dipanggil di probe. Driver ini hanya berguna bersama sprd_cpu_cooling
 * (yang memakai zona "virtual" sebagai input model dayanya).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/thermal.h>

struct virtual_thm {
	int id;
	struct device *dev;
	struct thermal_zone_device *thm_dev;
};

struct real_tz_list {
	int temp;
	struct thermal_zone_device *tz_dev;
};

struct virtual_thm_data {
	int nr_thm;
	struct real_tz_list *tz_list;
	struct virtual_thm *vir_thm;
};

/*
 * Migrasi API 5.4 -> 6.18:
 *  - struct thermal_zone_of_device_ops (get_temp(void *, int *)) DIHAPUS;
 *    digantikan thermal_zone_device_ops (get_temp(struct thermal_zone_device
 *    *, int *)), dan devdata diambil lewat thermal_zone_device_priv().
 *  - definition struct thermal_zone_device ada di thermal_core.h yang tidak
 *    diekspor ke modul, jadi tz->ops->get_temp() tidak bisa dipakai; diganti
 *    accessor publik thermal_zone_get_temp(). Deviasi: accessor itu memicu
 *    thermal_zone_device_update() lebih dulu, sedangkan 5.4 membaca cache.
 */
static int virtual_thm_get_temp(struct thermal_zone_device *tz, int *temp)
{
	int i, ret = 0;
	int max_temp = 0;
	struct real_tz_list *tz_list = NULL;
	struct virtual_thm_data *thm_data = thermal_zone_device_priv(tz);
	struct device *dev;

	if (!thm_data || !temp)
		return -EINVAL;

	dev = thm_data->vir_thm->dev;
	for (i = 0; i < thm_data->nr_thm; i++) {
		tz_list = &thm_data->tz_list[i];
		if (IS_ERR_OR_NULL(tz_list->tz_dev))
			return -EINVAL;
		ret = thermal_zone_get_temp(tz_list->tz_dev, &tz_list->temp);
		if (ret) {
			dev_err(dev, "fail to get temp\n");
			return ret;
		}
		max_temp = max(max_temp, tz_list->temp);
	}
	*temp = max_temp;

	return ret;
}

static const struct thermal_zone_device_ops virtual_thm_ops = {
	.get_temp = virtual_thm_get_temp,
};

static int virtual_thm_register(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct virtual_thm_data *data = platform_get_drvdata(pdev);
	struct virtual_thm *vir_thm = data->vir_thm;

	/*
	 * devm_thermal_zone_of_sensor_register() (4 argumen, 5.4 vendor)
	 * DIHAPUS di 6.18 -> devm_thermal_of_zone_register(). Konsekuensi yang
	 * disengaja: panggilan devm_thermal_zone_of_sensor_unregister() di
	 * .remove vendor tidak lagi diperlukan, karena zona sekarang terikat
	 * umur hidup device (devres) dan dilepas otomatis.
	 */
	vir_thm->thm_dev = devm_thermal_of_zone_register(dev, vir_thm->id,
							 data, &virtual_thm_ops);
	if (IS_ERR_OR_NULL(vir_thm->thm_dev))
		return -ENODEV;

	thermal_zone_device_update(vir_thm->thm_dev, THERMAL_EVENT_UNSPECIFIED);

	return 0;
}

static int get_thm_zone_counts(struct device *dev)
{
	int len;
	struct device_node *np = dev->of_node;
	int count;

	if (!np) {
		dev_err(dev, "device node not found\n");
		return -EINVAL;
	}

	/* of_find_property() di 6.18 menulis ke unsigned int, bukan int */
	if (!of_find_property(np, "thmzone-cells", &len)) {
		dev_err(dev, "thmzone-cells not found\n");
		return -EINVAL;
	}
	count = len / sizeof(u32);

	return count;
}

static int get_thm_zone_device(struct platform_device *pdev)
{
	int i;
	const char *name;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node, *node = NULL;
	struct real_tz_list *tz_list;
	struct virtual_thm_data *data = platform_get_drvdata(pdev);

	for (i = 0; i < data->nr_thm; i++) {
		node = of_parse_phandle(np, "thmzone-cells", i);
		if (!node) {
			dev_err(dev, "thmzone-cell%d not found\n", i);
			return -EINVAL;
		}
		name = node->name;
		of_node_put(node);
		node = NULL;
		tz_list = &data->tz_list[i];
		tz_list->tz_dev = thermal_zone_get_zone_by_name(name);
		if (IS_ERR(tz_list->tz_dev)) {
			dev_err(dev, "failed to get thermal zone by name\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int virtual_thm_probe(struct platform_device *pdev)
{
	int count = 0, ret = 0, id;
	struct virtual_thm_data *data;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	if (!np) {
		dev_err(dev, "device node not found\n");
		return -EINVAL;
	}
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	count = get_thm_zone_counts(dev);
	if (count < 0) {
		dev_err(dev, "failed to get thmzone count\n");
		return -EINVAL;
	}
	if (!count) {
		dev_err(dev, "thmzone-cells is empty\n");
		return -EINVAL;
	}
	data->nr_thm = count;
	data->tz_list = devm_kzalloc(dev, sizeof(*data->tz_list) * data->nr_thm,
				     GFP_KERNEL);
	if (!data->tz_list)
		return -ENOMEM;

	data->vir_thm = devm_kzalloc(dev, sizeof(*data->vir_thm), GFP_KERNEL);
	if (!data->vir_thm)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	ret = get_thm_zone_device(pdev);
	if (ret) {
		dev_err(dev, "failed to get thmzone device\n");
		return -EINVAL;
	}
	id = of_alias_get_id(np, "thm-sensor");
	if (id < 0) {
		dev_err(dev, "failed to get id\n");
		return -ENODEV;
	}
	data->vir_thm->id = id;
	data->vir_thm->dev = dev;
	ret = virtual_thm_register(pdev);
	if (ret) {
		dev_err(dev, "failed to register virtual thermal\n");
		return -ENODEV;
	}

	return 0;
}

static const struct of_device_id virtual_thermal_of_match[] = {
	{ .compatible = "virtual-thermal" },
	{},
};
MODULE_DEVICE_TABLE(of, virtual_thermal_of_match);

static struct platform_driver virtual_thermal_driver = {
	.probe = virtual_thm_probe,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "virtual_thermal",
		   .of_match_table = virtual_thermal_of_match,
	},
};
module_platform_driver(virtual_thermal_driver);

MODULE_AUTHOR("Jeson Gao <jeson.gao@unisoc.com>");
MODULE_DESCRIPTION("Unisoc virtual thermal driver");
MODULE_LICENSE("GPL v2");
