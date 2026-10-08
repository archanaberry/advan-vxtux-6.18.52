/* SPDX-License-Identifier: GPL-2.0 */
/*
 * drivers/vxtux/sensor/sensorhub/sensorhub.h
 * ABI provider sensorhub/hardware-info (RE) — pengganti ekspor sensorhub.ko.
 *
 * Kenapa header ini ada (dua alasan, keduanya nyata):
 *  1. Berkas ini menyatakan dirinya dikonsumsi EMPAT driver lain
 *     (sc27xx_fuel_gauge, chipone-tddi, focaltech_tp, hwinfo). Tanpa header
 *     bersama, keempatnya tidak punya tempat untuk melihat layout struct/enum
 *     yang sama — padahal kompatibilitas ABI-lah tujuannya.
 *  2. get_hardware_info_data() sudah EXPORT_SYMBOL_GPL tetapi tanpa deklarasi
 *     publik => clang -Wmissing-prototypes. Warning itu baru terlihat setelah
 *     wiring sensor/ diperbaiki (2026-09-18); sebelum itu berkasnya tidak
 *     pernah dikompilasi sehingga cacatnya tersembunyi.
 *
 * Layout struct/enum = prototipe proven build v4 (vxtux_stubs.c); nilai enum
 * masih placeholder sampai decompile shub_get_hardware_info_data selesai.
 */
#ifndef _VXTUX_SENSORHUB_H
#define _VXTUX_SENSORHUB_H

#include <linux/types.h>

/* from: shub_core.c (fork vendor) — prototipe proven build v4 */
struct hardware_info {
	char		sensor_name[64];
	unsigned int	sensor_count;
	unsigned int	ic_type;
	/* layout DIISI dari decompile — konsumen compile dengan layout sama */
};

/* enum DIISI nilai asli dari decompile shub_get_hardware_info_data;
 * nilai 0..3 = placeholder agar gate syntax lulus sebelum decompile. */
enum hardware_info_type {
	HW_INFO_TYPE_PLACEHOLDER_0 = 0,
	HW_INFO_TYPE_PLACEHOLDER_1,
	HW_INFO_TYPE_PLACEHOLDER_2,
	HW_INFO_TYPE_PLACEHOLDER_3,
};

int get_hardware_info_data(enum hardware_info_type type, void *out);

#endif /* _VXTUX_SENSORHUB_H */
