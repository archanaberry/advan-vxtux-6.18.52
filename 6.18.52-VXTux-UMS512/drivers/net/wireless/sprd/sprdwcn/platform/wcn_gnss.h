/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _WCN_GNSS_H
#define _WCN_GNSS_H

/*
 * SATU bentuk untuk kedua jalur (2026-09-23).
 *
 * Vendor memisahkannya lewat `IS_ENABLED(CONFIG_WCN_INTEG)`, tetapi build kita
 * menyalakan WCN_INTEG + WCN_BOOT sekaligus (keputusan port: gnss_common_ctl
 * memerlukan wcn_is_power_busy dari lapisan integrate). Akibatnya berkas
 * `boot/` (memakai `file_judge`) dan `platform/` (memakai empat anggota SDIO)
 * berada di SATU build, dan bentuk "satu anggota saja" membuat salah satu
 * berkas tidak bisa dikompilasi: 12 error build #8 di platform/wcn_boot.c
 * (set_file_path/write_data/backup_data/wait_gnss_boot).
 *
 * Superset ini AMAN karena SETIAP pemakai memakai designated initializer
 * (platform/gnss/gnss_common_ctl.c:982-987 `{.backup_data = ...}`), jadi urutan
 * anggota tidak menentukan apa pun; anggota yang tak diisi tetap NULL dan
 * pemanggilnya sudah menjaga (jalur SDIO mengisi empat, jalur integrate tidak).
 *
 * Bukti kedua jalur memang butuh anggota masing-masing (llvm-nm blob perangkat
 * blobs/vendor_dlkm/wcn_bsp.ko): `gnss_backup_data`, `gnss_write_data`,
 * `wcn_wait_gnss_boot` terdefinisi di modul SDIO itu; `file_judge` dipakai
 * boot/wcn_integrate_boot.c:301-302.
 */
struct sprdwcn_gnss_ops {
	int (*file_judge)(char *buff, int type);
	int (*backup_data)(void);
	int (*write_data)(void);
	void (*set_file_path)(char *buf);
	int (*wait_gnss_boot)(void);
};

int wcn_gnss_ops_register(struct sprdwcn_gnss_ops *ops);
void wcn_gnss_ops_unregister(void);

#endif
