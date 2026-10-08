# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/audio/mcdt/Makefile — dua varian MCDT, saling meniadakan.
#
# Umbrella (descent direktori ini) = CONFIG_SPRD_AUDIO_MCDT di audio/Makefile.
# Di sini yang dipilih adalah VARIAN-nya, dan itu dijamin `choice` di Kconfig:
# tepat satu dari kedua simbol berikut bernilai y.
#
# Wiring 2026-09-19: baris descent r2p0 sebelumnya TIDAK ADA, sementara
# mcdt_r2p0/Makefile memakai CONFIG_SPRD_AUDIO_MCDT_R2P0 yang tidak ada di
# Kconfig mana pun -- cabang yang disebut dua kali tapi tak pernah bisa dicapai.
# Keduanya sekarang ada, dan ketercapaiannya dibuktikan objek yang terbentuk
# lewat tools/re/probe_wiring.sh (bukan diasumsikan dari isi Makefile).

obj-$(CONFIG_SPRD_AUDIO_MCDT_R1P0)	+= mcdt_r1p0/
obj-$(CONFIG_SPRD_AUDIO_MCDT_R2P0)	+= mcdt_r2p0/

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
