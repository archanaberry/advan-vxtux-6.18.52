# SPRD audcp devfreq (blob: audiodvfs)
# Port 2026-10-04 from gpl-source/fork_t618/.../drivers/misc/sprd_audio/audiodvfs/
#
# The donor added -I for audiomem/, audiosipc/ and agdsp_access/ under
# drivers/misc/sprd_audio/. In this tree those ports live at
# drivers/sound/soc/sprd/<name>/, so the include paths point there instead.
obj-$(CONFIG_SPRD_HW_DEVICE_DVFS_AGCP) += sprd_audcp_dvfs.o
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/include
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiomem
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiosipc
