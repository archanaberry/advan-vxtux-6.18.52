# SPDX-License-Identifier: GPL-2.0
# Satu modul sprd_jpg.ko dari dua objek sumber (sprd_jpg.c + sprd_jpg_common.c).
# Bentuk lama "obj-$(CONFIG_SPRD_JPG) += sprd_jpg.o sprd_jpg_common.o" membuat
# DUA .ko terpisah sebagai =m: sprd_jpg_common.c memegang semua simbol bersama
# (jpg_get_mm_clk, jpg_clk_enable, jpg_get_iova, ...) dan modpost menolaknya
# ("undefined" lintas-modul). Composite = sprd_jpg.ko tunggal.
# CATATAN NAMA (pola zram / sprd_battery_info): nama modul TIDAK BOLEH sama
# dengan nama objek sumber. "sprd_jpg-objs := sprd_jpg_common.o" saja bikin
# kbuild menganggap sprd_jpg.o nama composite, jadi sprd_jpg.c TAK PERNAH
# dikompilasi (modpost: missing MODULE_LICENSE). Karena itu file utama
# di-rename sprd_jpg.c -> sprd_jpg_main.c, lalu:
sprd_jpg-objs := sprd_jpg_main.o sprd_jpg_common.o

obj-$(CONFIG_SPRD_JPG) += sprd_jpg.o