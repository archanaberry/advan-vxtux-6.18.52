# SPDX-License-Identifier: GPL-2.0
#
# Makefile for MMC/SD host controller drivers
#

obj-$(CONFIG_MMC_SDHCI)			+= sdhci.o
obj-$(CONFIG_MMC_SDHCI_UHS2)		+= sdhci-uhs2.o
obj-$(CONFIG_MMC_SDHCI_PCI)		+= sdhci-pci.o
sdhci-pci-y				+= sdhci-pci-core.o sdhci-pci-arasan.o
obj-$(CONFIG_MMC_SDHCI_ACPI)		+= sdhci-acpi.o
obj-$(CONFIG_MMC_SDHCI_PLTFM)		+= sdhci-pltfm.o
obj-$(CONFIG_MMC_SDHCI_OF_ARASAN)	+= sdhci-of-arasan.o
obj-$(CONFIG_MMC_SDHCI_OF_ESDHC)	+= sdhci-of-esdhc.o
obj-$(CONFIG_MMC_SDHCI_OF_DWCMSHC)	+= sdhci-of-dwcmshc.o
# The Unisoc SDHCI host driver plus its write-protect extension are one
# module, because the stock blob sdhci-sprd.ko carries both APIs.  The
# composite target is named sdhci-sprd.o while one of its objects is also
# sdhci-sprd.o, which kbuild rejects ("input file is the same as output
# file"), so the objects are gathered under the underscore spelling and the
# obj- target still produces sdhci-sprd.ko.
obj-$(CONFIG_MMC_SDHCI_SPRD)		+= sdhci-sprd.o
sdhci-sprd-y				:= sdhci_sprd_main.o sdhci-sprd-ext.o
# The Unisoc driver uses the generic MMC host software queue.  The tree
# carried a 37-byte placeholder for mmc_hsq.c, so the queue was inert;
# CONFIG_MMC_HSQ is selected by MMC_SDHCI_SPRD and this is the real
# implementation from 6.18.
obj-$(CONFIG_MMC_HSQ)			+= mmc_hsq.o

# Unisoc software command queue for eMMC (SW-CQ): same mmc_cqe_ops surface
# as cqhci, but driven through the host's request_atomic() callback.
obj-$(CONFIG_MMC_SWCQ)			+= mmc_swcq.o
obj-$(CONFIG_MMC_DW)			+= dw_mmc.o
obj-$(CONFIG_MMC_DW_PLTFM)		+= dw_mmc-pltfm.o
obj-$(CONFIG_MMC_DW_PCI)		+= dw_mmc-pci.o
obj-$(CONFIG_MMC_SPI)			+= mmc_spi.o of_mmc_spi.o
obj-$(CONFIG_MMC_CQHCI)			+= cqhci.o
cqhci-y					+= cqhci-core.o
cqhci-$(CONFIG_MMC_CRYPTO)		+= cqhci-crypto.o

