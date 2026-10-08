# SPDX-License-Identifier: GPL-2.0
#
# media/platform/sprd/sensors/Makefile — camera sensors on the Advan
# Tab VX Lite (UMS512/T618). Symbols are defined in ./Kconfig.

obj-$(CONFIG_VIDEO_GC8034)	+= gc8034.o
obj-$(CONFIG_VIDEO_GC5025)	+= gc5025.o
obj-$(CONFIG_VIDEO_S5K4H7YX)	+= s5k4h7yx.o

# Port 2026-10-04: descend into the sprd_sensor framework (Kconfig symbol
# CONFIG_SPRD_SENSOR). Without this decoration Kbuild never enters the
# directory -- the same silent-descent trap seen in drivers/soc/sprd/modem/.
obj-$(CONFIG_SPRD_SENSOR) += sprd_sensor/
