# SPDX-License-Identifier: GPL-2.0
#
# VSP driver objects. subdir-ccflags brings in drivers/devfreq/apsys/ so that
# "sprd_dvfs_vsp.h" resolves (the driver includes it unconditionally; with
# CONFIG_SPRD_APSYS_DVFS_DEVFREQ unset its inline stub is used, but the header
# still has to parse -- same flag as the donor Makefile).

obj-$(CONFIG_SPRD_VSP) += sprd_vsp.o vsp_common.o vsp_isr_func.o

subdir-ccflags-y += -Idrivers/devfreq/apsys/
