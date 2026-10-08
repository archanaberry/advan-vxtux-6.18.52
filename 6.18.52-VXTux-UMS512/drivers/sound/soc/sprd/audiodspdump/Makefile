# SPRD Audio Dsp Log Support

SPRD_SND_TREE	:= sound/soc/sprd/
SPRD_AUDIO_ASSISTANT_TREE := drivers/vxtux/audio/
# $(srctree)/ WAJIB: dengan O=<dir> kbuild menjalankan kompilasi dari buildd/, jadi
# path relatif seperti drivers/vxtux/audio/audiomem/ tidak pernah ketemu dan
# header port (audio_mem.h, agdsp_access.h) dilaporkan "file not found" meski
# berkasnya ada di pohon sumber.
ccflags-y += -I$(srctree)/$(SPRD_AUDIO_ASSISTANT_TREE)/audiomem/ \
	     -I$(srctree)/$(SPRD_AUDIO_ASSISTANT_TREE)/audiosipc/ \
	     -I$(srctree)/$(SPRD_AUDIO_ASSISTANT_TREE)/audiosipc/include/ \
	     -I$(srctree)/$(SPRD_AUDIO_ASSISTANT_TREE)/agdsp_access/ \
	     -I$(srctree)/$(SPRD_SND_TREE)/include

# Komposit SAH: SPRD_AUDIO_DSP_DUMP kini tristate (=m untuk uji insmod).
# anchor + -y = SATU modul berisi kedua objek, PERSIS seperti artefak stok:
# `dumped/re/hidden/vramdisk_ko/audio-dsp-dump.ko` memuat simbol audio_dsp_*
# DAN audio_sblock_* dalam satu .ko (lld -r), dan tak satu pun blob stok
# mengimpor audio_sblock_* -> tak ada modul terpisah di device.
# Anchor WAJIB berbeda dari nama anggota: `audio_dsp_dump.o` sebagai anchor dan
# sebagai anggota sekaligus membuat kbuild memotong dependensi sirkular dan
# `ld` melink objek ke dirinya sendiri (build #13: duplicate symbol di .text).
obj-$(CONFIG_SPRD_AUDIO_DSP_DUMP) += audio-dsp-dump.o
audio-dsp-dump-y := audio_sblock.o audio_dsp_dump.o

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
