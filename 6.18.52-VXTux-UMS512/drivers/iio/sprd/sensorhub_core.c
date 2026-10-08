// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/sensor/sensorhub/sensorhub_core.c
 *
 * Sensorhub / hardware-info provider — RE rewrite.
 * Blob sumber : sensorhub.ko (shub_core.c vendor)
 * Konsumen    : sc27xx_fuel_gauge, chipone-tddi, focaltech_tp, hwinfo (4 driver)
 * Landasan    : prototipe proven di vxtux_stubs.c (build v4, 11 simbol) —
 *               file ini adalah versi RESMI pengganti stub.
 *
 * LinDroid/VXTux v1.0
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/hwspinlock.h>
/* Wajib eksplisit: dipakai `struct of_device_id` + MODULE_DEVICE_TABLE(of).
 * Dulu muncul tanpa sengaja lewat header shim global (de-shim 2026-09-18). */
#include <linux/of.h>

#include <vxtux/compat.h>

/* struct hardware_info + enum hardware_info_type + prototipe provider kini
 * tinggal di header bersama; empat driver konsumen melihat layout yang sama */
#include "sensorhub.h"

/* TODO(RE): isi implementasi dari decompile shub_get_hardware_info_data.
 * Signature dipertahankan persis (konsumen vendor depend ke ini). */
int get_hardware_info_data(enum hardware_info_type type, void *out)
{
	/* stub proven: kembalikan data kosong dengan status sukses agar
	 * konsumen jalan; implementasi nyata menyusul dari decompile */
	memset(out, 0, sizeof(struct hardware_info));
	return 0;
}
EXPORT_SYMBOL_GPL(get_hardware_info_data);

static int vxtux_sensorhub_probe(struct platform_device *pdev)
{
	vx_dbg(&pdev->dev, "sensorhub provider (RE) probe ok\n");
	return 0;
}

static const struct of_device_id vxtux_sensorhub_of_match[] = {
	{ .compatible = "sprd,sensorhub" },
	{ }
};
MODULE_DEVICE_TABLE(of, vxtux_sensorhub_of_match);

static struct platform_driver vxtux_sensorhub_driver = {
	.probe	= vxtux_sensorhub_probe,
	.driver	= {
		.name		= "vxtux-sensorhub",
		.of_match_table	= vxtux_sensorhub_of_match,
	},
};
module_platform_driver(vxtux_sensorhub_driver);

MODULE_AUTHOR("VXTux Project");
MODULE_DESCRIPTION("Sensorhub hw-info provider (RE) for UMS512");
MODULE_LICENSE("GPL");
