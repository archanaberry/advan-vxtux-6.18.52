# SPDX-License-Identifier: GPL-2.0
#
# Scoped to the gsp_lite_r2p0 core plus the gsp_interface_pike2 interface,
# because the T618 is a "pike2" part (DT compatible "sprd,gsp-lite_r2p0-pike2").
# The r6p0 / r7p0 / r8p0 / r9p0 / lite_r3p0 cores and the sharkl3 / sharkl5 /
# sharkl5pro / sharkle / roc1 / qogirl6 / qogirn6pro interfaces are still in the
# tree but are not built: they belong to other SoCs, and r8p0/r9p0 additionally
# need the Unisoc Trusty SMC headers, which do not exist in this tree.
# gsp_ipc_trusty.c is likewise left unbuilt for the same Trusty reason.

subdir-ccflags-y += -I$(src)
subdir-ccflags-y += -I$(src)/gsp_lite_r2p0
subdir-ccflags-y += -I$(src)/gsp_interface
# gsp_dev.c includes sprd_drm.h and sprd_drm_gsp.h, which live one level up
# in the parent sprd/ directory, alongside the other Unisoc DRM headers.
subdir-ccflags-y += -I$(src)/..

# gsp_ion_stub.c supplies the two Unisoc ION helpers gsp_layer.c calls
# (sprd_ion_is_reserved, sprd_ion_get_phys_addr). No real ION provider
# exists in this tree, so they are documented stubs that fail at runtime;
# without them modpost hard-errors on the undefined symbols.
obj-$(CONFIG_DRM_SPRD_GSP) += gsp_core.o gsp_dev.o gsp_interface.o \
	gsp_kcfg.o gsp_layer.o gsp_ion_stub.o gsp_sync.o gsp_sysfs.o \
	gsp_workqueue.o \
	gsp_lite_r2p0/ gsp_interface/
