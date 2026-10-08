# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/net/wireless/sprdwcn/Makefile
# Port GPL: drivers/misc/sprdwcn (fork Samsung Tab A8, T618/UMS512)
#   -> kernel 6.18 VXTux (batch B7). Blob yang digantikan: wcn_bsp.ko.
#
# Perubahan struktural vs Makefile vendor (ketiganya disengaja):
#
#  1. platform/ di-descend SEKALI. Vendor punya dua baris:
#       obj-$(CONFIG_WCN_BOOT)     += platform/
#       obj-$(CONFIG_WCN_PLATFORM) += platform/
#     dengan keduanya aktif, kbuild menerima "platform/" dua kali di obj-y.
#     Sekarang penjaganya satu: WCN_BOOT (WCN_PLATFORM tetap ada sebagai
#     simbol Kconfig untuk kode #ifdef, tapi tidak lagi mengendalikan descent).
#
#  2. pcie/ di-descend pada CONFIG_WCN_PCIE, tetapi simbol itu default n.
#     Papan T1030 memakai jalur SDIO (compatible "unisoc,marlin3lite_sdio").
#     Sebelum 2026-09-19 baris descent ini TIDAK ADA dan pcie/Kconfig juga
#     tidak pernah di-source, sehingga 8 berkas .c di pcie/ (4.733 baris)
#     mustahil dibangun dan `CONFIG_WCN_PCIE` tak dikenal kconfig.
#     Cabangnya sekarang benar-benar ada; tetap default n karena fungsi
#     penyedia root-complex-nya tidak punya definisi di pohon ini:
#     sprd_pcie_configure_device/unconfigure_device/register_event/
#     deregister_event/teardown_msi_irq/dump_rc_regs (dideklarasikan di
#     include/linux/pcie-rc-sprd.h, dipanggil pcie.c, tapi tak ada yang
#     mendefinisikannya -- juga tidak di gpl-source/).
#
#  3. Semua sub-simbol WCN di atas dibangun BUILT-IN (=y), mengikuti konvensi
#     proyek: simbol ported harus muncul di System.map supaya gate (C2/C3/C5)
#     bisa membuktikan link. Nama modul tidak relevan karena tidak ada .ko hasil
#     vendor yang tersisa.
#
#  4. PENGECUALIAN untuk BT/FM: keduanya `=m`. Blob aslinya memang modul
#     (sprdbt_tty.ko, sprd_fm.ko), jadi bentuk modul adalah yang benar — dan
#     itu memungkinkan bukti modinfo/vermagic atas .ko yang dihasilkan.
#     Jalur descent mengikuti pohon dalam-tree referensi:
#       drivers/wcn/{bluetooth,fm}/driver/  ->  bluetooth/, fm/

subdir-ccflags-y := \
	-I$(src)/include \
	-I$(src)/boot \
	-I$(src)/platform \
	-I$(src)/platform/rf

obj-$(CONFIG_SC23XX)	+= wcn_bus.o
obj-$(CONFIG_WCN_SIPC)	+= sipc/
# platform/ di-descend SATU kali, pemiliknya WCN_CORE (bukan lagi WCN_BOOT).
# WCN_CORE=m -> wcn_init_core() menjalankan semua init komposit (module_init
# hanya sah satu per modul; wcn_boot.c sebagai host init-nya).
obj-$(CONFIG_WCN_CORE)	+= platform/
obj-$(CONFIG_WCN_INTEG)	+= boot/
obj-$(CONFIG_SDIOHAL)	+= sdio/
obj-$(CONFIG_WCN_SLP)	+= sleep/
obj-$(CONFIG_WCN_BLUETOOTH)+= bluetooth/
obj-$(CONFIG_WCN_FM)	+= fm/
obj-$(CONFIG_WCN_PCIE)	+= pcie/

# D5(b3) (2026-09-30): wlan/ tidak pernah di-descend, sehingga driver
# WLAN SC2355 tidak pernah masuk build.
# D5f (2026-09-30): daftar objek wlan/ sudah dicocokkan 1:1 dengan
# disk (27 objek = 27 .c) dan 11h.c/11h.h sudah pulih, jadi TIDAK ada
# lagi rujukan menggantung. CONFIG_WCN_WLAN tetap TETAP =n di fragment
# sampai ada build device yang membuktikan link -- lihat
# wlan/MISSING_SOURCES.tsv untuk sisa yang sengaja tidak didaftarkan.
obj-$(CONFIG_WCN_WLAN)	+= wlan/

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
