# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the i2c bus drivers.
#

# ACPI drivers
obj-$(CONFIG_I2C_SCMI)		+= i2c-scmi.o

# Embedded system I2C/SMBus host controller drivers
obj-$(CONFIG_I2C_DESIGNWARE_CORE)			+= i2c-designware-core.o
i2c-designware-core-y					:= i2c-designware-common.o
i2c-designware-core-y					+= i2c-designware-master.o
i2c-designware-core-$(CONFIG_I2C_DESIGNWARE_SLAVE) 	+= i2c-designware-slave.o
obj-$(CONFIG_I2C_DESIGNWARE_PLATFORM)			+= i2c-designware-platform.o
i2c-designware-platform-y 				:= i2c-designware-platdrv.o
i2c-designware-platform-$(CONFIG_I2C_DESIGNWARE_AMDPSP)	+= i2c-designware-amdpsp.o
i2c-designware-platform-$(CONFIG_I2C_DESIGNWARE_BAYTRAIL) += i2c-designware-baytrail.o
obj-$(CONFIG_I2C_DESIGNWARE_AMDISP) += i2c-designware-amdisp.o
obj-$(CONFIG_I2C_DESIGNWARE_PCI)			+= i2c-designware-pci.o
i2c-designware-pci-y					:= i2c-designware-pcidrv.o

obj-$(CONFIG_I2C_GPIO)		+= i2c-gpio.o
obj-$(CONFIG_I2C_CBUS_GPIO)	+= i2c-cbus-gpio.o
obj-$(CONFIG_I2C_SPRD)		+= i2c-sprd.o
obj-$(CONFIG_I2C_VERSATILE)	+= i2c-versatile.o
obj-$(CONFIG_I2C_MPC)		+= i2c-mpc.o
obj-$(CONFIG_I2C_VIRTIO)	+= i2c-virtio.o

ccflags-$(CONFIG_I2C_DEBUG_BUS) := -DDEBUG
