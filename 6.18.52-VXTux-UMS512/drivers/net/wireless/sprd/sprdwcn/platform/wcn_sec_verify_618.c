/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sprdwcn/platform/wcn_sec_verify_618.c — stub verifikasi citra WCN/GNSS
 *
 * LATAR (jujur, ini deviasi keamanan yang disengaja):
 *   Di 5.4, wcn_firmware_sec_verify() dikerjakan oleh TEE lewat
 *   trusty-ipc (platform/wcn_ca_trusty.c, ketergantungan modul
 *   "trusty-ipc" pada blob wcn_bsp). Kernel 6.18 VXTux TIDAK memuat
 *   driver trusty, jadi jalur verifikasi kriptografis tidak tersedia.
 *
 *   Call-site (platform/wcn_boot.c:500/694, boot/wcn_integrate_boot.c:341/481)
 *   memakai nilai balik negatif untuk MEMBATALKAN pemuatan firmware.
 *   Kalau fungsi ini tidak ada, modul tidak bisa di-link sama sekali.
 *
 *   Jadi stub ini: mengembalikan 0 (lolos) tetapi WAJIB berisik —
 *   satu pr_warn hanya sekali per boot (WARN_ON_ONCE + pr_warn_once),
 *   supaya deviasi ini tidak pernah senyap di log.
 *
 *   Pemeriksaan yang MASIH berjalan di call-site sebelum sampai ke sini
 *   (tidak dilewati): magic SEC_IMAGE_MAGIC, konsistensi
 *   SEC_IMAGE_HDR_SIZE + img_real_size < img_signed_size, dan
 *   ketersediaan ruang di TCM (maxsz_*). Yang hilang hanya tanda tangan
 *   kriptografis TEE.
 *
 * KONFIG: CONFIG_WCN_SEC_VERIFY_STUB (default y). Bila di-n, fungsi ini
 * mengembalikan -EOPNOTSUPP sehingga firmware bertanda tangan tidak akan
 * dimuat — perilaku aman bila trusty akhirnya di-port.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/io.h>

#include "wcn_ca_trusty.h"	/* prototipe wcn_firmware_sec_verify() */
#include <wcn-618-compat.h>	/* 5.4 -> 6.18 shim */

#define WCN_SEC_TAG	"[wcn_bsp/sec] "

#if defined(CONFIG_WCN_SEC_VERIFY_STUB)
int wcn_firmware_sec_verify(u32 wcn_or_gnss_bin,
			    phys_addr_t bin_base_addr, u32 bin_length)
{
	static bool warned;

	if (!warned) {
		warned = true;
		pr_warn(WCN_SEC_TAG
			"verifikasi TEE (trusty-ipc) tidak tersedia di 6.18; "
			"citra %s diturunkan TANPA cek tanda tangan "
			"(addr=%pa len=%u). Header/magic/size tetap diperiksa "
			"oleh pemanggil.\n",
			(wcn_or_gnss_bin == 1) ? "BTWF" :
			(wcn_or_gnss_bin == 2) ? "GNSS" : "tidak-dikenal",
			&bin_base_addr, bin_length);
	}

	if (wcn_or_gnss_bin != 1 && wcn_or_gnss_bin != 2) {
		pr_err(WCN_SEC_TAG "indeks citra tidak sah: %u\n",
		       wcn_or_gnss_bin);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(wcn_firmware_sec_verify);
#else /* !CONFIG_WCN_SEC_VERIFY_STUB */
int wcn_firmware_sec_verify(u32 wcn_or_gnss_bin,
			    phys_addr_t bin_base_addr, u32 bin_length)
{
	pr_err(WCN_SEC_TAG
	       "tanpa trusty dan tanpa stub: citra %u (addr=%pa len=%u) "
	       "tidak bisa diverifikasi -> ditolak.\n",
	       wcn_or_gnss_bin, &bin_base_addr, bin_length);
	return -EOPNOTSUPP;
}
EXPORT_SYMBOL_GPL(wcn_firmware_sec_verify);
#endif

MODULE_DESCRIPTION("Unisoc WCN firmware secure-verify shim (VXTux 6.18 port)");
MODULE_LICENSE("GPL");
