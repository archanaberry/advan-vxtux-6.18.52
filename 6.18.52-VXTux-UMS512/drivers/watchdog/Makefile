# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the WatchDog device drivers.
#

# The WatchDog Timer Driver Core.
obj-$(CONFIG_WATCHDOG_CORE)	+= watchdog.o

watchdog-objs	+= watchdog_core.o watchdog_dev.o

watchdog-$(CONFIG_WATCHDOG_PRETIMEOUT_GOV)	+= watchdog_pretimeout.o
watchdog-$(CONFIG_WATCHDOG_HRTIMER_PRETIMEOUT)	+= watchdog_hrtimer_pretimeout.o

obj-$(CONFIG_WATCHDOG_PRETIMEOUT_GOV_NOOP)	+= pretimeout_noop.o
obj-$(CONFIG_WATCHDOG_PRETIMEOUT_GOV_PANIC)	+= pretimeout_panic.o

# Only one watchdog can succeed. We probe the ISA/PCI/USB based
# watchdog-cards first, then the architecture specific watchdog
# drivers and then the architecture independent "softdog" driver.
# This means that if your ISA/PCI/USB card isn't detected that
# you can fall back to an architecture specific driver and if
# that also fails then you can fall back to the software watchdog
# to give you some cover.

# Generic ISA/PCI Watchdog Cards
obj-$(CONFIG_WDT) += wdt.o
obj-$(CONFIG_WDTPCI) += wdt_pci.o

# ARM Architecture
obj-$(CONFIG_ARM_SP805_WATCHDOG) += sp805_wdt.o
obj-$(CONFIG_ARM_SBSA_WATCHDOG) += sbsa_gwdt.o
obj-$(CONFIG_DW_WATCHDOG) += dw_wdt.o
obj-$(CONFIG_SPRD_WATCHDOG) += sprd_wdt.o
# Unisoc watchdog FIQ helper: supplies sprd_wdt_fiq_get_dev() to sprd_wdf.
obj-$(CONFIG_SPRD_WATCHDOG_FIQ) += sprd_wdt_fiq.o
# Unisoc watchdog feed detector. Ported from marohinmark_ums512; it lived under
# drivers/soc/sprd/debug/hang_debug in 5.4, but that whole subdirectory chain has
# no Makefile in this tree, so it would never have been built there.
obj-$(CONFIG_SPRD_WDF) += sprd_wdf.o

# X86 Architecture
obj-$(CONFIG_ITCO_WDT) += iTCO_wdt.o
ifeq ($(CONFIG_ITCO_VENDOR_SUPPORT),y)
obj-$(CONFIG_ITCO_WDT) += iTCO_vendor_support.o
endif
obj-$(CONFIG_NV_TCO) += nv_tco.o

# Architecture Independent
obj-$(CONFIG_GPIO_WATCHDOG)	+= gpio_wdt.o
obj-$(CONFIG_WDAT_WDT) += wdat_wdt.o
obj-$(CONFIG_SOFT_WATCHDOG) += softdog.o
