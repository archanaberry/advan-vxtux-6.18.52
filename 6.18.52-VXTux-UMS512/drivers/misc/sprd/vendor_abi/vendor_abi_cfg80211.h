/* SPDX-License-Identifier: GPL-2.0 */
/*
 * drivers/vxtux/vendor_abi/vendor_abi_cfg80211.h — prototipe shim
 * cfg80211_get_bss (simbol 5.4 untuk modul blob sprd_wlan_combo).
 *
 * Header ini memuat deklarasi bertipe lengkap (butuh <net/cfg80211.h>);
 * implementasinya ada di vendor_abi_cfg80211.c, yang meng-rename static
 * inline 6.18 lebih dulu agar bisa mendefinisikan fungsi bernama sama.
 */
#ifndef _VXTUX_VENDOR_ABI_CFG80211_H
#define _VXTUX_VENDOR_ABI_CFG80211_H

#include <linux/types.h>
#include <net/cfg80211.h>

struct cfg80211_bss *
cfg80211_get_bss(struct wiphy *wiphy, struct ieee80211_channel *channel,
		 const u8 *bssid, const u8 *ssid, size_t ssid_len,
		 enum ieee80211_bss_type bss_type,
		 enum ieee80211_privacy privacy);

#endif /* _VXTUX_VENDOR_ABI_CFG80211_H */
