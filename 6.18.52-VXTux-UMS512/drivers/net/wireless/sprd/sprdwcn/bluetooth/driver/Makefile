# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/net/wireless/sprdwcn/bluetooth/driver/Makefile
#
# sprdbt_tty.ko — BT HCI di atas tty-sdio (blob: sprdbt_tty.ko, 5.4.210).
#
# Daftar objek ini PERSIS dari Makefile dalam-tree referensi
# (gpl-source/hadenix_ums512_wcn/drivers/wcn/bluetooth/driver/Makefile), yang
# memakai bentuk kbuild dalam-pohon:
#
#     obj-$(CONFIG_WCN_BLUETOOTH) += sprdbt_tty.o
#     sprdbt_tty-objs := tty-sdio/dump.o tty-sdio/lpm.o tty-sdio/rfkill.o tty-sdio/tty.o
#     sprdbt_tty-objs += tty-sdio/alignment/sitm.o
#
# Yang TIDAK diambil dari Kbuild vendor generasi-external:
#   * $(error no BSP_BOARD_UNISOC_WCN_SOCKET ...) — variabel lingkungan build
#     eksternal; di sini transport yang benar dipilih langsung oleh ccflags.
#   * KO_MODULE_* / obj-m : bentuk itu untuk `make M=` di luar pohon. Di dalam
#     pohon, `obj-$(CONFIG_WCN_BLUETOOTH)` dengan nilai `m` sudah menghasilkan
#     obj-m dan .ko.
#
# `-DDEBUG` dipertahankan seperti vendor. Add-on: -DBSP_BOARD_UNISOC_WCN_SOCKET_SDIO
# meneruskan pilihan transport SDIO (papan T1030 = SC2355 via SDIO; blob memuat
# string sdiohal_*), tanpa mengarang makro baru.

ccflags-y += -DDEBUG
ccflags-y += -DBSP_BOARD_UNISOC_WCN_SOCKET_SDIO

obj-$(CONFIG_WCN_BLUETOOTH) += sprdbt_tty.o
sprdbt_tty-objs := tty-sdio/dump.o tty-sdio/lpm.o tty-sdio/rfkill.o tty-sdio/tty.o
sprdbt_tty-objs += tty-sdio/alignment/sitm.o

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
