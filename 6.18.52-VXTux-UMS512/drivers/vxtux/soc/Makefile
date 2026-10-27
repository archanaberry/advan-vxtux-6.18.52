# SPDX-License-Identifier: GPL-2.0
# drivers/vxtux/soc/Makefile
obj-$(CONFIG_SPRD_USBPINMUX)	+= usbpinmux/
obj-$(CONFIG_SPRD_PMIC_SYSCON)	+= pmic_syscon/
obj-$(CONFIG_SPRD_SOCID)	+= soc_id/
obj-$(CONFIG_SPRD_HWFEATURE)	+= hwfeature/
obj-$(CONFIG_NVMEM_SPRD_CACHE_EFUSE) += cache_efuse/
obj-$(CONFIG_SPRD_POWER_MANAGER) += power_manager/

# Unisoc modem family (modem_loader/cproc/cp_dump) — 2026-10-10
obj-$(CONFIG_SPRD_MODEM) += modem/

# Unisoc time correction device (stock blob sprd_time_sync.ko) and its
# control point (sprd_time_sync_cp.ko).
obj-$(CONFIG_SPRD_TIME_SYNC) += time_sync/

# SIP secure-monitor client (stock blob sprd_sip_svc.ko).
obj-$(CONFIG_SPRD_SIP_SVC)	+= sip_svc/

# Subsystem sleep statistics (stock blob sprd_power_stat.ko).
obj-$(CONFIG_SPRD_DEBUGSTAT)	+= debugstat/

# Power debug: wakeup/sleep logging (stock blob sprd_pdbg.ko).
obj-$(CONFIG_SPRD_PDBG)	+= pdbg/

# SoC model / manufacturer strings (stock blob sprd_manufacturer_model.ko).
obj-$(CONFIG_SPRD_MANUFACTURER_MODEL)	+= manufacturer_model/

# PMIC 7s reset knobs (stock blob sprd_7sreset.ko).
obj-$(CONFIG_SPRD_7SRESET)	+= sprd_7sreset/

# Framework autotest vendor (stock blob core.ko + gpio.ko).
obj-$(CONFIG_SPRD_AUTOTEST)	+= autotest/

# IQ playback / modem data region (stock blob sprd_iq.ko).
obj-$(CONFIG_SPRD_IQ)	+= iq/
