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
obj-$(CONFIG_MMC_SDHCI_SPRD)		+= sdhci-sprd.o
obj-$(CONFIG_MMC_DW)			+= dw_mmc.o
obj-$(CONFIG_MMC_DW_PLTFM)		+= dw_mmc-pltfm.o
obj-$(CONFIG_MMC_DW_PCI)		+= dw_mmc-pci.o
obj-$(CONFIG_MMC_SPI)			+= mmc_spi.o of_mmc_spi.o
obj-$(CONFIG_MMC_CQHCI)			+= cqhci.o
cqhci-y					+= cqhci-core.o
cqhci-$(CONFIG_MMC_CRYPTO)		+= cqhci-crypto.o
obj-$(CONFIG_MMC_HSQ)			+= mmc_hsq.o
