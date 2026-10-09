# SPDX-License-Identifier: GPL-2.0
#
# VSP driver: ONE composite module sprd_vsp.ko (the vendor blob it
# replaces), built from the three source objects -- same pattern as the
# cpp/ Makefile in this tree.
#
# Sebelumnya Makefile ini memakai "obj-... += sprd_vsp.o vsp_common.o
# vsp_isr_func.o" (tiga entri obj- terpisah) -- itu membuat TIGA modul
# terpisah (sprd_vsp.ko, vsp_common.ko, vsp_isr_func.ko) dan build device
# gagal: simbol internal (vsp_clk_enable, codec_instance_count, ...) jadi
# undefined lintas-modul + modpost minta MODULE_LICENSE per .o. Donor 5.4
# lolos karena semua =y (built-in); di pohon ini =m. File utama di-rename
# sprd_vsp.c -> sprd_vsp_main.c supaya nama modul (sprd_vsp) tidak bentrok
# dengan nama objek sumber (pola zram: modul zram.ko dari zram_drv.o).

sprd_vsp-objs := sprd_vsp_main.o vsp_common.o vsp_isr_func.o

obj-$(CONFIG_SPRD_VSP) += sprd_vsp.o

# ccflags-y (BUKAN subdir-ccflags-y): subdir-ccflags hanya merambat ke SUBDIR.
# sprd_vsp_main.c memakai #include "sprd_dvfs_vsp.h" yang ada di
# drivers/devfreq/apsys/ -- perlu -I eksplisit. (Pola yang sama dengan
# vendor Makefile donor: subdir-ccflags-y += -Idrivers/devfreq/apsys/.)
ccflags-y += -I$(srctree)/drivers/devfreq/apsys/ -I$(srctree)/drivers/devfreq/sprd/apsys/
