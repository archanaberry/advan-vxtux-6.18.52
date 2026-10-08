# SPDX-License-Identifier: GPL-2.0
#
# platform/Makefile — pcm-routing (pengganti blob snd-soc-sprd-pcm-routing).

VXTUX_ASOC := drivers/vxtux/sound/asoc

ccflags-y += -I$(srctree)/$(VXTUX_ASOC)/platform_include \
             -I$(srctree)/$(VXTUX_ASOC)/card/include \
             -I$(srctree)/drivers/vxtux/audio/agdsp_access \
             -I$(srctree)/drivers/vxtux/audio/audiomem \
             -I$(srctree)/drivers/vxtux/audio/audiosipc \
             -I$(srctree)/drivers/vxtux/audio/audiosipc/include \
             -I$(srctree)/drivers/vxtux/audio/mcdt

obj-$(CONFIG_VXTUX_SND_SPRD_PCM_ROUTING) += sprd-platform-pcm-routing.o

# Vendor header include paths (NOT upstream).
# The VXTux ASoC port kept the vendor drivers/vxtux/sound/asoc layout: the
# headers live in per-subsystem include dirs. Nothing added them to the include
# path, so every source here failed with
#   fatal error: 'sprd-asoc-debug.h' file not found
#
# These are duplicated in each sub-Makefile on purpose: kbuild does NOT inherit
# a parent directory's ccflags-y into obj-y subdirectories. Only ccflags-y in the
# exact directory that owns the object applies.
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/agdsp_access
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiodspdump
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiodvfs
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiomem
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiosipc
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/audiosipc/include
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/card/include
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/codec_sc2730
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/dai_v4
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/mcdt
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/mcdt/mcdt_r1p0
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/mcdt/mcdt_r2p0
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/platform_include
ccflags-y += -I$(srctree)/drivers/sound/soc/sprd/saudio
