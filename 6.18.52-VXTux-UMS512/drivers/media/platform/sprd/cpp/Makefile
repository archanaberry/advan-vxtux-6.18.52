# SPDX-License-Identifier: GPL-2.0
#
# CPP driver objects. subdir-ccflags brings the cpp dir itself into the
# include path so "sprd_mm.h" (copied from sensors/sprd_sensor/, same header
# the ported sensor drivers use) resolves from the driver's own dir.

cpp-objs := cpp_core.o cpp_ioctl.o scale_drv.o rot_drv.o dma_drv.o

obj-$(CONFIG_SPRD_CPP) += cpp.o

# ccflags-y (BUKAN subdir-ccflags-y): subdir-ccflags hanya merambat ke
# SUBDIR, tidak berlaku untuk objek milik direktori ini. cpp_core.c memakai
# #include <sprd_mm.h> (angle-bracket) -- perlu -I eksplisit ke dir sendiri
# supaya resolve. (Bug ini ketemu saat build device 2026-10-09: compile
# gagal "sprd_mm.h: No such file" padahal file ada di direktori yang sama.)
ccflags-y += -I$(src)
