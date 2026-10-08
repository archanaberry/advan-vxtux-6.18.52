obj-$(CONFIG_WCN_PCIE) += sprd_ep.o
sprd_ep-objs += pcie.o edma_engine.o pcie_dbg.o ioctl.o mchn.o edma_test.o \
		pcie_attach_bus.o pcie_pm.o

# Vendor header include paths (NOT upstream).
# The sprdwcn port kept the vendor drivers/vxtux layout: headers live in
# per-subsystem directories (include/, boot/, platform/, pcie/, ...) and nothing
# put them on the include path, so every source here failed with
#   fatal error: 'wcn_glb.h' file not found
#
# Duplicated in each sub-Makefile on purpose: kbuild does NOT inherit a parent
# directory's ccflags-y into obj-y subdirectories.
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/tty-sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/bluetooth/driver/tty-sdio/alignment
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/boot
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/fm/driver/fm_sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/include
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/pcie
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform/gnss
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform/rf
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sdio
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sipc
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sleep
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/wlan/sc2355
