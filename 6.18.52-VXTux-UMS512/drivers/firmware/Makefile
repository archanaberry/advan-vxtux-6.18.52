# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#
obj-$(CONFIG_ARM_SCPI_PROTOCOL)	+= arm_scpi.o
obj-$(CONFIG_ARM_SDE_INTERFACE)	+= arm_sdei.o
obj-$(CONFIG_DMI)		+= dmi_scan.o
obj-$(CONFIG_DMI_SYSFS)		+= dmi-sysfs.o
obj-$(CONFIG_EDD)		+= edd.o
obj-$(CONFIG_DMIID)		+= dmi-id.o
obj-$(CONFIG_ISCSI_IBFT_FIND)	+= iscsi_ibft_find.o
obj-$(CONFIG_ISCSI_IBFT)	+= iscsi_ibft.o
obj-$(CONFIG_FIRMWARE_MEMMAP)	+= memmap.o
obj-$(CONFIG_FW_CFG_SYSFS)	+= qemu_fw_cfg.o
obj-$(CONFIG_SYSFB)		+= sysfb.o
obj-$(CONFIG_SYSFB_SIMPLEFB)	+= sysfb_simplefb.o
obj-$(CONFIG_TRUSTED_FOUNDATIONS) += trusted_foundations.o

obj-y				+= arm_ffa/
obj-y				+= arm_scmi/
obj-y				+= efi/
obj-y				+= psci/
obj-y				+= smccc/
