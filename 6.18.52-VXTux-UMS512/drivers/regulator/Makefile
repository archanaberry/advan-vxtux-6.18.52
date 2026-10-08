# SPDX-License-Identifier: GPL-2.0
#
# Makefile for regulator drivers.
#


obj-$(CONFIG_REGULATOR) += core.o dummy.o fixed-helper.o helpers.o devres.o irq_helpers.o
obj-$(CONFIG_REGULATOR_NETLINK_EVENTS) += event.o
obj-$(CONFIG_OF) += of_regulator.o
obj-$(CONFIG_REGULATOR_FIXED_VOLTAGE) += fixed.o
obj-$(CONFIG_REGULATOR_VIRTUAL_CONSUMER) += virtual.o
obj-$(CONFIG_REGULATOR_USERSPACE_CONSUMER) += userspace-consumer.o
obj-$(CONFIG_REGULATOR_GPIO) += gpio-regulator.o
obj-$(CONFIG_REGULATOR_PWM) += pwm-regulator.o
obj-$(CONFIG_REGULATOR_ARM_SCMI) += scmi-regulator.o
obj-$(CONFIG_REGULATOR_SC2731) += sc2731-regulator.o
obj-$(CONFIG_REGULATOR_SC2730) += sc2730-regulator.o
obj-$(CONFIG_REGULATOR_VCTRL) += vctrl-regulator.o

ccflags-$(CONFIG_REGULATOR_DEBUG) += -DDEBUG
