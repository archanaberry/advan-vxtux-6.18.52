# SPRD Audio AGDSP Access Support

# $(srctree)/ wajib: dengan build O=<dir>, `-I` relatif diresolusi terhadap CWD
# proses kbuild = direktori keluaran, bukan pohon sumber (kelas cacat yang sama
# membuat mcdt_hw.h "file not found" di audio/mcdt/mcdt_r2p0).
#
# Catatan: drivers/staging/sprd/modem/include TIDAK ADA di pohon 6.18 dan tidak
# ada berkas agdsp_access yang meng-include dari sana -- jalur itu warisan
# vendor (modem staging belum diport). Dibiarkan sebagai penanda, tapi
# di-$(srctree)-kan supaya tidak diam-diam menunjuk ke tempat lain.
ccflags-y += -I$(srctree)/drivers/staging/sprd/modem/include/ \
             -I$(srctree)/drivers/vxtux/audio/audiosipc/include
obj-$(CONFIG_SPRD_AUDIO_AGDSP_ACCESS) += agdsp_access.o

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
