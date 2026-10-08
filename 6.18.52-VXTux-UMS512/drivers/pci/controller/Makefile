# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_PCIE_CADENCE) += cadence/
obj-$(CONFIG_PCI_FTPCI100) += pci-ftpci100.o
obj-$(CONFIG_PCI_AARDVARK) += pci-aardvark.o
obj-$(CONFIG_PCI_HOST_COMMON) += pci-host-common.o
obj-$(CONFIG_PCI_HOST_GENERIC) += pci-host-generic.o

# pcie-hisi.o quirks are needed even without CONFIG_PCIE_DW
obj-y				+= dwc/
obj-y				+= mobiveil/
obj-y				+= plda/
