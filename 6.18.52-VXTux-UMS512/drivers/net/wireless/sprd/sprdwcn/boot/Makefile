# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/net/wireless/sprdwcn/boot/Makefile
#
# Vendor menulis -Idrivers/misc/sprdwcn/... (lokasi asli fork). Setelah
# port, pohonnya ada di drivers/vxtux/net/wireless/sprdwcn, jadi keempat
# -I itu menunjuk direktori yang tidak ada. Induk (../Makefile) sudah
# menambahkan subdir-ccflags-y dengan -I yang benar, tapi ccflags-y di
# sini tetap dipertahankan (eksplisit) supaya berkas ini bisa dibaca
# mandiri tanpa melihat ke atas.
ccflags-y += -I$(srctree)/$(src)/../include
ccflags-y += -I$(srctree)/$(src)/../platform
ccflags-y += -I$(srctree)/$(src)/../platform/rf

# Penjaga tetap nama vendor: WCN_INTEG (chip integrated). Simbol ini
# WAJIB =y di fragmen karena WCN_SIPC/WCN_PLATFORM bergantung padanya.
obj-$(CONFIG_WCN_INTEG) += wcn_integrate_dev.o
obj-$(CONFIG_WCN_INTEG) += wcn_integrate_boot.o
obj-$(CONFIG_WCN_INTEG) += wcn_integrate.o

# Vendor header include paths (NOT upstream).
# The sprdwcn port kept the vendor drivers/vxtux layout: headers live in
# per-subsystem directories (include/, boot/, platform/, pcie/, ...) and nothing
# put them on the include path, so every source here failed with
#   fatal error: 'wcn_glb.h' file not found
#
# Duplicated in each sub-Makefile on purpose: kbuild does NOT inherit a parent
# directory's ccflags-y into obj-y subdirectories.
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/tty-sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/tty-sdio/alignment
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/boot
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/include
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/pcie
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform/gnss
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform/rf
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sipc
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sleep
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/wlan/sc2355
