# SPDX-License-Identifier: GPL-2.0
#
# CPP driver objects. subdir-ccflags brings the cpp dir itself into the
# include path so "sprd_mm.h" (copied from sensors/sprd_sensor/, same header
# the ported sensor drivers use) resolves from the driver's own dir.

cpp-objs := cpp_core.o cpp_ioctl.o scale_drv.o rot_drv.o dma_drv.o

obj-$(CONFIG_SPRD_CPP) += cpp.o

subdir-ccflags-y += -I$(srctree)/$(src)
