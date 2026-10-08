# SPRD Audio SIPC (blob: audio_sipc)
# Port 2026-10-04 from gpl-source/fork_t618/.../drivers/misc/sprd_audio/audiosipc/
#
# The donor Makefile carried -Idrivers/staging/sprd/modem/include/, a path the
# 6.18 tree does not have (the vendor modem staging tree was pruned). The
# audiomem/ include is the one that actually resolves: audio-sipc.h uses the
# audio_mem_* API exported by the audiomem driver ported the same day.
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiomem
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/include

obj-$(CONFIG_SPRD_AUDIO_SIPC) += audio-sipc.o audio-smsg.o audio-pipe.o
