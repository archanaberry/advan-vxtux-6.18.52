# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/Makefile — LinDroid/VXTux v1.0 (design gen 3.0)
# Dibangun external: M=drivers/vxtux

obj-y += disp_notify.o

# Instrumen diagnostik boot (2026-10-26): perekam dmesg ke partisi cache.
# debug/ menampung FLIGHTREC + BEACON + LKFB; turunkan direktorinya bila
# SALAH SATU saja aktif, supaya mematikan perekam tidak ikut mematikan
# kanal layar (kelas DIR-TAKDIRUJUK yang sama seperti temuan audit 2026-09-18).
ifneq ($(filter y,$(CONFIG_VXTUX_FLIGHTREC) $(CONFIG_VXTUX_BEACON) $(CONFIG_VXTUX_LKFB) $(CONFIG_VXTUX_BLDB)),)
obj-y += debug/
endif

obj-$(CONFIG_TOUCHSCREEN_HIMAX_CHIPSET)		+= touch/hx83102e/
obj-$(CONFIG_TOUCHSCREEN_CHIPONE)		+= touch/chipone_tddi/
obj-$(CONFIG_VXTUX_HYBRIDSWAP_REF)		+= mm/hybridswap/

# --- sensor (perbaikan wiring 2026-09-18) ---
# Sebelum ini `sensor/` tidak punya baris descent SAMA SEKALI, padahal
# Kconfig-nya sudah di-source dan fragmen menyalakan CONFIG_VXTUX_SENSORHUB=y.
# Akibatnya nol objek dibangun dan kesalahan di dalam berkasnya tersembunyi
# dari build. Ditemukan oleh tools/re/audit_wiring.py (kelas DIR-TAKDIRUJUK).
#
# `power/` (CONFIG_VXTUX_SC27XX_FUELGAUGE) dihapus 2026-09-18 beserta
# stub-nya: probe-nya hanya mencetak log tanpa mendaftarkan power_supply dan
# sprd_battery_get_info() selalu mengembalikan nol, sementara tidak ada sumber
# vendor fuel gauge di repo ini. Batere papan ini dilayani driver mainline
# sc27xx_fuel_gauge.c (CONFIG_FUEL_GAUGE_SC27XX=y, "sprd,sc2731-fgu").
obj-$(CONFIG_VXTUX_SENSORHUB)			+= sensor/

# Port hasil fork GPL (2026-09-16): USB pinmux + map_user
obj-$(CONFIG_SPRD_USBPINMUX)			+= soc/
obj-$(CONFIG_SPRD_MAP)				+= misc/

# Kerangka SIPC Unisoc + shim API mailbox vendor (fondasi audio AGDSP & WCN).
# Port 2026-09-17: drivers/soc/sprd/modem/sipc (fork GPL) -> drivers/vxtux/sipc.
obj-$(CONFIG_SPRD_SIPC)				+= sipc/

# ASoC Unisoc (fork GPL sound/soc/sprd -> drivers/vxtux/sound/asoc): card-utils,
# dummy codec, power SC27xx. Mengganti blob snd-soc-sprd-card / dummy-codec /
# codec-sc2730-power.
obj-$(CONFIG_VXTUX_SND_SPRD_ASOC)		+= sound/

# Infrastruktur audio AGDSP/AGCP (bertahap; semua sub-simbol default n).
obj-$(CONFIG_SPRD_AUDIO_INFRA)			+= audio/

# WCN Unisoc (batch B7): sprdwcn + wcn_bus = pengganti blob wcn_bsp.ko
# (WiFi/BT/GNSS integrated, marlin3lite_sdio). Penjaga: CONFIG_SC23XX.
# Thermal (RE 2026-09-19): model daya GPU -- provider ABI
# create_gpu_cooling_device/destroy_gpu_cooling_device (blob sprd_gpu_cooling.ko).
obj-$(CONFIG_VXTUX_SPRD_GPU_COOLING)		+= thermal/

obj-$(CONFIG_SC23XX)				+= net/

# NPU/VDSP (2026-10-14): sprd_vdsp.ko — donor catalog mods-sprd kernel4.14
# (Cadence/XRP). Entry ke-103 inventory; tanpa baris ini tally berhenti 102/103.
obj-$(CONFIG_SPRD_VDSP)			+= npu/vdsp/

# DRM/KMS murni (tanpa fb): panel DSI generik UMS512
obj-$(CONFIG_DRM_PANEL_VXTUX_UMS512)		+= disp/sprd_panel_vxtux/
# DPU/DRM consolidated (RE). Sebelumnya disp/sprd_dpu/ punya Kconfig (di-source
# Kconfig:27) + Makefile + sumber, dan CONFIG_VXTUX_SPRD_DPU=y, tetapi tidak ada
# baris descent -> nol objek dibangun (perbaikan wiring 2026-09-18).
obj-$(CONFIG_VXTUX_SPRD_DPU)			+= disp/sprd_dpu/

# Helper display-CA Trusty (RE). Sebelumnya trusty/ punya Kconfig + Makefile +
# disp_ca.c dan CONFIG_VXTUX_TRUSTY_DISP=y, tetapi tidak pernah di-descend
# (perbaikan wiring 2026-09-18, tools/re/audit_wiring.py).
obj-$(CONFIG_VXTUX_TRUSTY_DISP)			+= trusty/

# gpu/mali_kbase (DDK) — dibangun in-tree bila MALI_MIDGARD aktif (jalur kbase);
# jalur GPU utama 6.18 = DRM_PANFROST (mainline).
obj-$(CONFIG_MALI_MIDGARD)			+= gpu/mali_kbase/

# B40 (2026-09-26): USB pendukung hasil RE (sc27xx_typec).
# B42 (2026-09-26): tiga simbol kini hidup di dalam usb/ (typec + dua penyedia
# ABI rantai RNDIS). Descent dikunci PER-SIMBOL, bukan lewat satu gerbang:
# mengaktifkan VXTUX_SPRD_* tanpa VXTUX_SC27XX_TYPEC tetap harus membangun
# direktori itu (kelas DIR-TAKDIRUJUK, tools/re/audit_wiring.py).
obj-$(CONFIG_VXTUX_SC27XX_TYPEC)		+= usb/
obj-$(CONFIG_VXTUX_SPRD_U_ETHER)		+= usb/
obj-$(CONFIG_VXTUX_SPRD_F_RNDIS)		+= usb/
# B43 (2026-09-26): penyedia ABI blob vendor 5.4 (global arm64 + handler CFI
# + shim cfg80211). BUILT-IN, bukan modul — kimage_vaddr butuh &_text dan
# cpu_number harus hidup di area per-CPU kernel (lihat vendor_abi/Kconfig).
obj-$(CONFIG_VXTUX_VENDOR_ABI)		+= vendor_abi/

# catatan: glue MUSB Unisoc (RE) pindah ke kernel/6.18-vxtux-ums512/musb_sprd_vxtux.c
# — dipasang patch langsung di drivers/usb/musb/ (butuh akses internal musb core).
