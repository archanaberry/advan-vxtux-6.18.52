# SPDX-License-Identifier: GPL-2.0

sprd-drm-y := sprd_drm.o \
	sprd_dpu.o \
	sprd_dsi.o \
	megacores_pll.o \
	disp_notify.o

obj-$(CONFIG_DRM_SPRD) += sprd-drm.o

# sprd-gsp: the kernel-internal <drm/gsp_cfg.h> (struct gsp_cfg /
# struct gsp_layer) is absent from every source tree, but its layout WAS
# recovered from the vendor blob sprd-gsp.ko by radare2 and is written up in
# drivers/gpu/drm/sprd/gsp/GSP_OFFSETS.md. That header now exists in this
# tree, so the driver is no longer blocked and enters the link again.
#
# Do not re-block this on "the struct is a hardware contract". The contract
# was recovered from the blob's own one-instruction field accessors and from
# the ioctl size bound; see include/drm/gsp_cfg.h for the per-offset
# citations.
obj-$(CONFIG_DRM_SPRD_GSP) += gsp/
