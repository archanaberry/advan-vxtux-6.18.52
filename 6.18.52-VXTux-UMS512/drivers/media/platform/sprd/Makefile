# SPDX-License-Identifier: GPL-2.0
#
# media/platform/sprd/Makefile — Unisoc/VXTux camera pipeline + sensors.
# Without this file kbuild cannot descend into this directory.

obj-$(CONFIG_MEDIA_SPRD_PIPELINE)	+= pipeline/

# sensors/ di-descend SATU kali. Sebelumnya ada tiga baris (GC8034, GC5025,
# S5K4H7YX) yang semuanya += sensors/. Dengan lebih dari satu simbol=y pada
# bersamaan, kbuild menerima "sensors/" lebih dari sekali di obj-y, objeknya
# masuk built-in.a dua kali, dan ld.lld melaporkan 21 "duplicate symbol"
# untuk hal yang sama sekali tidak terkait (gc5025.o, gc8034.o, wcn_*).
# Setiap sensor tetap punya syaratnya sendiri DI DALAM sensors/Makefile,
# jadi tidak ada gating yang hilang.
obj-$(CONFIG_VIDEO_SPRD_SENSORS)	+= sensors/
