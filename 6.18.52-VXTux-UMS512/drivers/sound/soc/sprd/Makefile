# SPDX-License-Identifier: GPL-2.0
#
# drivers/sound/soc/sprd/Makefile — Unisoc ASoC (VXTux port)
#
# The VXTux port keeps the vendor header layout: the headers were written for
# the vendor drivers/vxtux/sound/asoc tree and live in per-subsystem include/
# dirs (card/include, audiomem, audiosipc, agdsp_access, mcdt, ...). Nothing
# added those to the include path, so every .c failed with
#   fatal error: 'sprd-asoc-debug.h' file not found
# cc-option does not apply: these are hard #include "..." in C, not optional.
#
# ccflags-y applies to this directory and is inherited by the obj-y
# subdirectories below it, which is what these sources need.

# obj-<dir>            replaces blob
#   card/              snd-soc-sprd-card (card-utils + common + hook + legacy + dfm)
#   codec/             snd-soc-sprd-dummy-codec + snd-soc-sprd-codec-sc2730-power
#   dai_v4/            snd-soc-sprd-vbc-v4 + snd-soc-sprd-vbc-fe
#   platform/          snd-soc-sprd-pcm-routing
#   codec_sc2730/      snd-soc-sprd-codec-sc2730 (codec + headset)
#   machine/           machine part of snd-soc-sprd-card (vbc-rxpx-codec-sc27xx)

obj-y += card/ codec/ dai_v4/ platform/ codec_sc2730/ machine/
# sprd_apipe: the vendor 'apipe' audio pipe (blob sprd_apipe). Ported as
# audio_pipe.c under the vendor symbol CONFIG_SPRD_AUDIO_MEMORY_MANAGER,
# but nothing descended into the directory -- the same silent-descent
# trap as drivers/soc/sprd/modem/. Wired here like its siblings.
obj-y += audio_pipe/

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

