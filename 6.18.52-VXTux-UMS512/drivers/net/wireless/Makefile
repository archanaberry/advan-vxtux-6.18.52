# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Wireless network device drivers.
#

# D5(b1) (2026-09-30): sprd/ tidak pernah di-descend, sehingga
# sprdwcn/ (dan seluruh driver WCN di dalamnya) tidak pernah dibangun.
# Penjaga memakai CONFIG_SC23XX -- simbol kompatibilitas chip yang
# SUDAH ada di sprd/Makefile:4 (obj-$(CONFIG_SC23XX) += sprdwcn/) dan
# di fragmen (CONFIG_SC23XX=y). Tidak ada simbol Kconfig baru yang
# perlu direview terpisah, jadi gate_fragment tidak akan menandai
# ORPHAN.
obj-$(CONFIG_SC23XX) += sprd/

obj-$(CONFIG_WLAN) += virtual/
