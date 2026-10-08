// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/vendor_abi/vendor_abi_cfg80211.c
 *
 * cfg80211_get_bss — simbol 5.4 yang dibutuhkan modul blob sprd_wlan_combo.
 *
 * MASALAH (gerbang ronde-11, kelas IN-TREE-NOT-EXPORTED, 1 modul)
 *   Di 5.4 cfg80211_get_bss adalah FUNGSI TEREKSPOR:
 *     [W1] net/wireless/scan.c:708-755 (oracle 5.4) — definisi + EXPORT_SYMBOL
 *   Di 6.18 ia menjadi *static inline* yang membungkus __cfg80211_get_bss
 *   dengan argumen ke-8 NL80211_BSS_USE_FOR_NORMAL:
 *     [W2] include/net/cfg80211.h:7892-7901
 *   Akibatnya blob yang mengimpor `cfg80211_get_bss` tidak menemukan ekspor
 *   (`__cfg80211_get_bss` yang diekspor, scan.c:1660, namanya berbeda).
 *
 * CARA KERJA
 *   Definisi fungsi bernama sama TIDAK bisa ditulis di berkas yang
 *   meng-include <net/cfg80211.h>, karena inline 6.18 akan bentrok. Karena
 *   itu inline-nya di-rename dulu lewat #define, baru didefinisikan fungsi
 *   kita — semantiknya menyalin inline 6.18 apa adanya [W2], bukan menebak.
 *
 * BATAS JUJUR
 *   Ini shim ABI untuk blob; sumber mainline tetap tidak diubah. Bila kelak
 *   sprd_wlan_combo dibangun ulang dari sumber, shim ini tidak diperlukan
 *   lagi (modul akan memakai inline 6.18 langsung).
 *
 * B45-fix (2026-09-27) — kenapa berkas ini TIDAK selalu built-in
 *   Penyedia ini =y sementara CFG80211=m di r9 MAUPUN r10-qemu, sehingga
 *   vmlinux gagal link:
 *     ld.lld: error: undefined symbol: __cfg80211_get_bss
 *     >>> referenced by vendor_abi_cfg80211.c:53
 *   (relink2 device 2026-09-27 00:04). Simbol itu milik cfg80211.ko — objek
 *   built-in tidak bisa mereferensikannya. Karena itu simbol konfigurasinya
 *   kini tristate MENGIKUTI CFG80211 (VXTUX_VENDOR_ABI_CFG80211): =y saat
 *   CFG80211=y, dan =m (modul vendor_abi_cfg80211.ko) saat CFG80211=m.
 *   Konsekuensi jujur: sebagai modul, simbol hanya tersedia SETELAH modul ini
 *   dimuat, jadi modul blob yang mengimpornya harus dimuat sesudahnya
 *   (softdep/modules-load.d belum dibuat; butuh bukti blob diuji di device).
 */

#define pr_fmt(fmt) "vendor_abi: " fmt

#include <linux/errno.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>

#if IS_ENABLED(CONFIG_CFG80211)
/*
 * Rename static inline 6.18 (dan pemanggil internal header yang memakainya)
 * supaya nama `cfg80211_get_bss` bebas untuk definisi kita. Wajib dilakukan
 * SEBELUM <net/cfg80211.h> di-include (termasuk lewat header kita).
 */
#define cfg80211_get_bss		vxtux_abi_shadow__cfg80211_get_bss
#include <net/cfg80211.h>
#undef cfg80211_get_bss

#include "vendor_abi_cfg80211.h"

struct cfg80211_bss *
cfg80211_get_bss(struct wiphy *wiphy, struct ieee80211_channel *channel,
		 const u8 *bssid, const u8 *ssid, size_t ssid_len,
		 enum ieee80211_bss_type bss_type,
		 enum ieee80211_privacy privacy)
{
	/* Sama persis dengan inline 6.18 [W2]. */
	return __cfg80211_get_bss(wiphy, channel, bssid, ssid, ssid_len,
				  bss_type, privacy,
				  NL80211_BSS_USE_FOR_NORMAL);
}
EXPORT_SYMBOL(cfg80211_get_bss);
#endif /* CONFIG_CFG80211 */

/* Metadata modul: berkas ini ikut dibangun sebagai modul saat CFG80211=m
 * (lihat Kconfig VXTUX_VENDOR_ABI_CFG80211). Tanpa lisensi, memuat modul
 * menandai kernel TAINTED — tidak boleh untuk sesi bukti RE. */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Shim cfg80211_get_bss() 5.4 untuk modul blob vendor (VXTux)");
