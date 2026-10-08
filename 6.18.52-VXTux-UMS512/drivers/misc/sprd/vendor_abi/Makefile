# SPDX-License-Identifier: GPL-2.0
#
# Provider ABI blob vendor 5.4 — BUILT-IN (lihat Kconfig untuk alasannya).
# Shim cfg80211_get_bss PUNYA SIMBOL KONFIGURASI SENDIRI dan mengikuti nilai
# CFG80211: bila CFG80211=m ia dibangun sebagai modul vendor_abi_cfg80211.ko.
# Sebabnya terukur (defect B45, 2026-09-27) — objek built-in tidak bisa
# mereferensikan __cfg80211_get_bss yang hidup di cfg80211.ko:
#   ld.lld: error: undefined symbol: __cfg80211_get_bss
#   >>> referenced by vendor_abi_cfg80211.c:53
# (relink2 device 00:04; CFG80211=m di r9 maupun r10-qemu.)
obj-$(CONFIG_VXTUX_VENDOR_ABI)		+= vendor_abi_arm64.o
obj-$(CONFIG_VXTUX_VENDOR_ABI)		+= vendor_abi_ubsan.o
obj-$(CONFIG_VXTUX_VENDOR_ABI_CFG80211)	+= vendor_abi_cfg80211.o
