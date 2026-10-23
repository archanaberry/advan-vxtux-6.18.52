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
