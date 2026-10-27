# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_TYPEC)		+= typec.o
typec-y				:= class.o mux.o bus.o pd.o retimer.o
typec-$(CONFIG_ACPI)		+= port-mapper.o
obj-$(CONFIG_TYPEC)		+= altmodes/
obj-$(CONFIG_TYPEC_TCPM)	+= tcpm/
obj-$(CONFIG_TYPEC_UCSI)	+= ucsi/
obj-$(CONFIG_TYPEC_TPS6598X)	+= tipd/
obj-$(CONFIG_TYPEC_ANX7411)	+= anx7411.o
obj-$(CONFIG_TYPEC_HD3SS3220)	+= hd3ss3220.o
obj-$(CONFIG_TYPEC_STUSB160X) 	+= stusb160x.o
obj-$(CONFIG_TYPEC_RT1719)	+= rt1719.o
obj-$(CONFIG_TYPEC_WUSB3801)	+= wusb3801.o
obj-$(CONFIG_TYPEC)		+= mux/
# Unisoc SC27xx Type-C (vendor). Sebelum ini direktori sprd/ TIDAK pernah
# di-descend dari sini, jadi sc27xx_typec.ko mustahil ter-build walau
# CONFIG_TYPEC_SPRD=m dan sumbernya ada (temuan docs/PORT-AUDIT.md).
obj-$(CONFIG_TYPEC_SPRD)		+= sprd/
