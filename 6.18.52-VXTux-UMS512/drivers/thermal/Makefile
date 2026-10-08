# SPDX-License-Identifier: GPL-2.0
#
# Makefile for sensor chip drivers.
#
CFLAGS_thermal_core.o		:= -I$(src)
obj-$(CONFIG_THERMAL)		+= thermal_sys.o
thermal_sys-y			+= thermal_core.o thermal_sysfs.o
thermal_sys-y			+= thermal_trip.o thermal_helpers.o
thermal_sys-y			+= thermal_thresholds.o

# netlink interface to manage the thermal framework
thermal_sys-$(CONFIG_THERMAL_NETLINK)		+= thermal_netlink.o

thermal_sys-$(CONFIG_THERMAL_DEBUGFS)	+= thermal_debugfs.o

# interface to/from other layers providing sensors
thermal_sys-$(CONFIG_THERMAL_HWMON)		+= thermal_hwmon.o
thermal_sys-$(CONFIG_THERMAL_OF)		+= thermal_of.o

# governors
CFLAGS_gov_power_allocator.o			:= -I$(src)
thermal_sys-$(CONFIG_THERMAL_GOV_FAIR_SHARE)	+= gov_fair_share.o
thermal_sys-$(CONFIG_THERMAL_GOV_BANG_BANG)	+= gov_bang_bang.o
thermal_sys-$(CONFIG_THERMAL_GOV_STEP_WISE)	+= gov_step_wise.o
thermal_sys-$(CONFIG_THERMAL_GOV_USER_SPACE)	+= gov_user_space.o
thermal_sys-$(CONFIG_THERMAL_GOV_POWER_ALLOCATOR)	+= gov_power_allocator.o

# cpufreq cooling
thermal_sys-$(CONFIG_CPU_FREQ_THERMAL)	+= cpufreq_cooling.o
thermal_sys-$(CONFIG_CPU_IDLE_THERMAL)	+= cpuidle_cooling.o

# devfreq cooling
thermal_sys-$(CONFIG_DEVFREQ_THERMAL) += devfreq_cooling.o

thermal_sys-$(CONFIG_PCIE_THERMAL) += pcie_cooling.o

# platform thermal drivers
obj-$(CONFIG_THERMAL_MMIO)		+= thermal_mmio.o
obj-$(CONFIG_GENERIC_ADC_THERMAL)	+= thermal-generic-adc.o
obj-$(CONFIG_SPRD_THERMAL)	+= sprd_thermal.o
obj-$(CONFIG_THERMAL_CORE_TESTING)	+= testing/

obj-y				+= sprd/
