# SPDX-License-Identifier: GPL-2.0

spi-nor-objs			:= core.o sfdp.o otp.o sysfs.o
spi-nor-$(CONFIG_DEBUG_FS)	+= debugfs.o
obj-$(CONFIG_MTD_SPI_NOR)	+= spi-nor.o

obj-$(CONFIG_MTD_SPI_NOR)	+= controllers/
