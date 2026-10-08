# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux kernel device drivers.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-y				+= cache/
obj-y				+= irqchip/
obj-y				+= bus/

obj-$(CONFIG_GENERIC_PHY)	+= phy/

# GPIO must come after pinctrl as gpios may need to mux pins etc
obj-$(CONFIG_PINCTRL)		+= pinctrl/
obj-$(CONFIG_GPIOLIB)		+= gpio/
obj-y				+= pwm/

# LEDs must come before PCI, it is needed by NPEM driver
obj-y				+= leds/

obj-y				+= pci/

obj-$(CONFIG_RAPIDIO)		+= rapidio/
obj-y				+= video/
obj-y				+= idle/

# IPMI must come before ACPI in order to provide IPMI opregion support
obj-y				+= char/ipmi/

obj-$(CONFIG_ACPI)		+= acpi/

# PnP must come after ACPI since it will eventually need to check if acpi
# was used and do nothing if so
obj-$(CONFIG_PNP)		+= pnp/
obj-y				+= amba/

obj-y				+= clk/
# Many drivers will want to use DMA so this has to be made available
# really early.
obj-$(CONFIG_DMADEVICES)	+= dma/

# SOC specific infrastructure drivers.
obj-y				+= soc/
# soc/sprd/ di-descend HANYA oleh drivers/soc/Makefile (`obj-y += sprd/`,
# baris 41). Baris `obj-y += soc/sprd/` di sini menggandakannya: kbuild
# menghitung usbpinmux, pmic_syscon, soc_id, hwfeature, cache_efuse,
# power_manager DAN modem/{cproc,cp_dump,modem_loader} dua kali ->
# 11 duplicate symbol (sprd_mpm_*, sprd_pms_*, cp_status_info,
# cproc_dts_args, sprd_get_soc_id, ext_modem_ops). Descent yang benar
# milik direktori soc/; gate per-schild tetap di drivers/soc/sprd/Makefile.
# Restored 2026-10-01. This line and source "drivers/xen/Kconfig" were both
# missing, but block/bio.c (unmodified upstream code) still calls
# xen_domain() and xen_biovec_phys_mergeable(), which are declared by
# drivers/xen. Without them:
#   block/bio.c:935: error: call to undeclared function 'xen_domain'
# CONFIG_XEN is n on this target, so this expands to nothing; the point is that
# the declarations now exist so the core builds.
obj-$(CONFIG_XEN)			+= xen/
# Always descend. drivers/Makefile gated this on CONFIG_PM_GENERIC_DOMAINS,
# but that symbol is defined nowhere in this tree, so pmdomain/ was never
# built and drivers/pmdomain/sprd/ (the VSP power domain) was compiled but
# never linked. The per-vendor Kconfig entries under pmdomain/ gate themselves.
obj-y					+= pmdomain/

obj-y				+= virtio/
obj-$(CONFIG_VDPA)		+= vdpa/

# regulators early, since some subsystems rely on them to initialize
obj-$(CONFIG_REGULATOR)		+= regulator/

# reset controllers early, since gpu drivers might rely on them to initialize
obj-$(CONFIG_RESET_CONTROLLER)	+= reset/

# tty/ comes before char/ so that the VT console is the boot-time
# default.
obj-y				+= tty/
obj-y				+= char/

# iommu/ comes before gpu as gpu are using iommu controllers
obj-y				+= iommu/

# gpu/ comes after char for AGP vs DRM startup and after iommu
obj-y				+= gpu/

obj-$(CONFIG_CONNECTOR)		+= connector/

# i810fb depends on char/agp/
obj-$(CONFIG_FB_I810)           += video/fbdev/i810/

obj-$(CONFIG_PARPORT)		+= parport/
# Always descend: CONFIG_MISC_SPRD is not a Kconfig symbol, so sprd_map.o
# (CONFIG_SPRD_MAP=y) was never compiled.  misc/sprd/Makefile gates itself.
obj-y				+= misc/sprd/
obj-y				+= base/ block/ misc/ mfd/ nfc/
obj-$(CONFIG_LIBNVDIMM)		+= nvdimm/
obj-y				+= dax/
obj-$(CONFIG_DMA_SHARED_BUFFER) += dma-buf/
obj-$(CONFIG_NUBUS)		+= nubus/
obj-y				+= cxl/
obj-y				+= scsi/
obj-y				+= nvme/
obj-$(CONFIG_ATA)		+= ata/
obj-$(CONFIG_TARGET_CORE)	+= target/
obj-$(CONFIG_MTD)		+= mtd/
obj-$(CONFIG_SPI)		+= spi/
obj-$(CONFIG_SPMI)		+= spmi/
obj-$(CONFIG_HSI)		+= hsi/
obj-$(CONFIG_SLIMBUS)		+= slimbus/
# JANGAN descend ke net/wireless/sprd/ dari sini. Baris ini pernah ada
# ("Always descend, gate never fired") DAN drivers/net/wireless/Makefile:32
# juga punya `obj-$(CONFIG_SC23XX) += sprd/`. CONFIG_SC23IXX=y, jadi kbuild
# menghitung drivers/net/wireless/sprd/ dua kali: seluruh subtree sprdwcn
# (wcn_core, wcn_integrate, bufring, gnss/*, fm_sdio/*, tty-sdio/*) masuk
# built-in.a dua kali dan ld.lld melaporkan 21 "duplicate symbol" yang sama
# sekali tidak terkait (cp_status_info, sprd_mpm_*, sprd_get_soc_id).
# Satu-satunya jalur descent yang benar adalah milik wireless/Makefile.
obj-y				+= net/
obj-$(CONFIG_ATM)		+= atm/
obj-$(CONFIG_FUSION)		+= message/
obj-y				+= firewire/
obj-$(CONFIG_UIO)		+= uio/
obj-$(CONFIG_VFIO)		+= vfio/
obj-y				+= cdrom/
obj-y				+= auxdisplay/
obj-$(CONFIG_PCCARD)		+= pcmcia/
obj-$(CONFIG_DIO)		+= dio/
obj-$(CONFIG_ATA_OVER_ETH)	+= block/aoe/
obj-$(CONFIG_TC)		+= tc/
obj-$(CONFIG_USB_PHY)		+= usb/
obj-$(CONFIG_USB)		+= usb/
obj-$(CONFIG_USB_SUPPORT)	+= usb/
obj-$(CONFIG_PCI)		+= usb/
obj-$(CONFIG_USB_GADGET)	+= usb/
obj-$(CONFIG_TYPEC_SPRD)		+= usb/typec/sprd/
obj-$(CONFIG_USB_GADGET_SPRD)	+= usb/gadget/function/sprd/
obj-$(CONFIG_OF)		+= usb/
obj-$(CONFIG_TOUCHSCREEN_SPRD)	+= input/touchscreen/sprd/
obj-$(CONFIG_SERIO)		+= input/serio/
obj-$(CONFIG_SND_SOC_SPRD)		+= sound/soc/sprd/
obj-$(CONFIG_MEDIA_SPRD)		+= media/platform/sprd/
obj-$(CONFIG_GAMEPORT)		+= input/gameport/
obj-$(CONFIG_INPUT)		+= input/
obj-$(CONFIG_RTC_LIB)		+= rtc/
obj-y				+= i2c/ i3c/ media/
# Always descend: CONFIG_THERMAL_SPRD is not a Kconfig symbol in this tree
# (the children below use SPRD_SOC_THM / SPRD_CPU_COOLING / SPRD_TSENSOR_THERMAL
# / VXTUX_SPRD_GPU_COOLING, all defined).  thermal/Makefile:43 already had a
# thermal/sprd/ di-descend HANYA oleh drivers/thermal/Makefile (baris 43,
# `obj-y += sprd/`). Baris `obj-y += thermal/sprd/` di sini menggandakannya:
# kbuild menghitung sprd_cpu_cooling.o, sprd_soc_thm.o dan
# sc27xx_tsensor_thermal.o dua kali -> 3 duplicate symbol yang tidak ada
# hubungannya dengan thermal. Tidak ada baris pengganti di sini: file
# sprd_thermal.o berada di drivers/thermal/, bukan drivers/.
obj-$(CONFIG_PPS)		+= pps/
obj-y				+= ptp/
obj-$(CONFIG_W1)		+= w1/
obj-y				+= power/
obj-$(CONFIG_HWMON)		+= hwmon/
obj-$(CONFIG_THERMAL)		+= thermal/
obj-$(CONFIG_WATCHDOG)		+= watchdog/
obj-$(CONFIG_MD)		+= md/
obj-$(CONFIG_BT)		+= bluetooth/
obj-$(CONFIG_ACCESSIBILITY)	+= accessibility/
obj-$(CONFIG_ISDN)		+= isdn/
obj-$(CONFIG_EDAC)		+= edac/
obj-$(CONFIG_EISA)		+= eisa/
obj-$(CONFIG_PM_OPP)		+= opp/
obj-$(CONFIG_CPU_FREQ)		+= cpufreq/
obj-$(CONFIG_CPU_IDLE)		+= cpuidle/
obj-y				+= mmc/
obj-y				+= ufs/
obj-$(CONFIG_MEMSTICK)		+= memstick/
obj-$(CONFIG_INFINIBAND)	+= infiniband/
obj-y				+= firmware/
obj-$(CONFIG_FWCTL)		+= fwctl/
obj-$(CONFIG_CRYPTO)		+= crypto/
obj-y				+= clocksource/
obj-$(CONFIG_DCA)		+= dca/
obj-$(CONFIG_HID_SUPPORT)	+= hid/
obj-$(CONFIG_OF)		+= of/
obj-$(CONFIG_SSB)		+= ssb/
obj-$(CONFIG_BCMA)		+= bcma/
obj-$(CONFIG_COMEDI)		+= comedi/
obj-$(CONFIG_SPRD_SIPC)		+= soc/sprd/sipc/
obj-$(CONFIG_STAGING)		+= staging/
obj-y				+= platform/

obj-$(CONFIG_MAILBOX)		+= mailbox/
obj-$(CONFIG_HWSPINLOCK)	+= hwspinlock/
obj-$(CONFIG_REMOTEPROC)	+= remoteproc/
obj-$(CONFIG_RPMSG)		+= rpmsg/
obj-$(CONFIG_SOUNDWIRE)		+= soundwire/
# Always descend: CONFIG_MEMORY_SPRD is not a Kconfig symbol in this tree, so
# hybridswap_core.c was never built.  memory/sprd/Makefile gates itself.
obj-y				+= memory/sprd/

# Always descend: CONFIG_IIO_SPRD is not a Kconfig symbol in this tree, so
# sensorhub_core.c was never built even though CONFIG_VXTUX_SENSORHUB=y.
obj-y				+= iio/sprd/

obj-$(CONFIG_PM_DEVFREQ)	+= devfreq/
obj-$(CONFIG_EXTCON)		+= extcon/
obj-$(CONFIG_MEMORY)		+= memory/
obj-$(CONFIG_IIO)		+= iio/
obj-$(CONFIG_IPACK_BUS)		+= ipack/
obj-$(CONFIG_NTB)		+= ntb/
obj-$(CONFIG_POWERCAP)		+= powercap/
obj-$(CONFIG_MCB)		+= mcb/
obj-$(CONFIG_PERF_EVENTS)	+= perf/
obj-$(CONFIG_RAS)		+= ras/
# Always descend: CONFIG_TEE_SPRD is not a Kconfig symbol in this tree.  The
# child gates itself on CONFIG_VXTUX_TRUSTY_DISP.  NOTE: CONFIG_VXTUX_TRUSTY_DISP
# is currently ABSENT from .config and was NOT in the fragment, so tee/sprd/ will
# still build nothing until that symbol is set -- this line only removes the dead
# outer gate, it does not turn the driver on.
obj-y				+= tee/sprd/
obj-$(CONFIG_CORESIGHT)		+= hwtracing/coresight/
obj-y				+= hwtracing/intel_th/
obj-$(CONFIG_STM)		+= hwtracing/stm/
obj-$(CONFIG_HISI_PTT)		+= hwtracing/ptt/
obj-y				+= android/
obj-$(CONFIG_NVMEM)		+= nvmem/
obj-$(CONFIG_FPGA)		+= fpga/
obj-$(CONFIG_FSI)		+= fsi/
obj-$(CONFIG_TEE)		+= tee/
obj-$(CONFIG_MULTIPLEXER)	+= mux/
obj-$(CONFIG_SIOX)		+= siox/
obj-$(CONFIG_GNSS)		+= gnss/
obj-$(CONFIG_INTERCONNECT)	+= interconnect/
obj-$(CONFIG_COUNTER)		+= counter/
obj-$(CONFIG_MOST)		+= most/
obj-$(CONFIG_DRM_ACCEL)		+= accel/
obj-$(CONFIG_CDX_BUS)		+= cdx/
obj-$(CONFIG_DPLL)		+= dpll/

obj-$(CONFIG_DIBS)		+= dibs/

# TRUSTY (2026-10-07). Direktori drivers/trusty/ sudah diport dari donor GPL
# bocchi_unisoc54 SEBELUM hari ini, dan drivers/Kconfig sudah men-`source`
# Kconfig-nya — tetapi TIDAK ADA baris descent di sini. Akibatnya kbuild tidak
# pernah menghitung satu pun objeknya: `CONFIG_TRUSTY` diset pun, .o-nya tidak
# lahir, dan simbol `tipc_chan_*` tidak pernah ada di System.map.
#
# Yang bergantung pada baris ini (dibuktikan dari Kconfig, bukan dugaan):
#   drivers/gpu/drm/sprd/gsp/Kconfig  `depends on TRUSTY_VIRTIO_IPC`
#   blobs/vendor_modules/ion_ipc_trusty.ko  (baris ledger BLOCKED; mengimpor
#      9 simbol tipc_*, disediakan trusty-ipc.c di direktori ini)
#   drivers/misc/trusty-tui.c (CONFIG_TRUSTY_TUI=y sudah lama di fragmen)
#
# Gaya `obj-$(CONFIG_TRUSTY)` (bukan `obj-y`) dipakai karena direktori ini
# punya gerbang Kconfig nyata; sub-objek di dalamnya bergerbang sendiri di
# drivers/trusty/Makefile.
obj-$(CONFIG_TRUSTY)		+= trusty/


# SPRD camera flash HAL (blob: sprd_flash_drv). Ported 2026-10-04 from the
# vendor sources in work/donorcatalog/mods-sprd/.../flash/flash_drv/.
obj-y += sprd/
