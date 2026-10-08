# SPDX-License-Identifier: GPL-2.0
#
# linux/drivers/maps/Makefile
#

ifeq ($(CONFIG_MTD_COMPLEX_MAPPINGS),y)
obj-$(CONFIG_MTD)		+= map_funcs.o
endif

# Chip mappings
obj-$(CONFIG_MTD_CFI_FLAGADM)	+= cfi_flagadm.o
obj-$(CONFIG_MTD_PHYSMAP)	+= physmap.o
physmap-y			:= physmap-core.o
physmap-$(CONFIG_MTD_PHYSMAP_VERSATILE) += physmap-versatile.o
obj-$(CONFIG_MTD_PLATRAM)	+= plat-ram.o
