# SPDX-License-Identifier: GPL-2.0
#
# platform/Makefile — pcm-routing (pengganti blob snd-soc-sprd-pcm-routing)
#                     + dmaengine-pcm (pengganti blob sprd-dmaengine-pcm.ko).

VXTUX_ASOC := drivers/vxtux/sound/asoc

ccflags-y += -I$(srctree)/$(VXTUX_ASOC)/platform_include \
             -I$(srctree)/$(VXTUX_ASOC)/card/include \
             -I$(srctree)/drivers/vxtux/audio/agdsp_access \
             -I$(srctree)/drivers/vxtux/audio/audiomem \
             -I$(srctree)/drivers/vxtux/audio/audiosipc \
             -I$(srctree)/drivers/vxtux/audio/audiosipc/include \
             -I$(srctree)/drivers/vxtux/audio/mcdt

obj-$(CONFIG_VXTUX_SND_SPRD_PCM_ROUTING) += sprd-platform-pcm-routing.o

# sprd-dmaengine-pcm.ko — the module MUST be named exactly like the stock blob
# so that the vermagic/alias/modprobe path stays identical.
obj-$(CONFIG_VXTUX_SND_SPRD_DMAENGINE_PCM) += sprd-dmaengine-pcm.o

# sprd-compr-2stage-dma.ko — offload MP3/AAC compress platform (Unisoc).
obj-$(CONFIG_VXTUX_SND_SPRD_COMPR_2STAGE) += sprd-compr-2stage-dma.o
