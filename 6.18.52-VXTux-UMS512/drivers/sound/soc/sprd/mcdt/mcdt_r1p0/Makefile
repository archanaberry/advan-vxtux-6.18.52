# MCDT(Multi-channel Data Transfer) Support
SPRD_MCDT_TREE := drivers/vxtux/audio/mcdt/
# $(srctree)/ wajib untuk build O=<dir> (vendor memakai path relatif yang hanya
# benar pada build in-tree).
ccflags-y += -I$(srctree)/$(SPRD_MCDT_TREE)

# Wiring 2026-09-19: digerbangi simbol VARIAN (r1p0), bukan umbrella
# SPRD_AUDIO_MCDT. Dengan umbrella, kedua varian akan sama-sama terkompilasi
# begitu umbrella menyala -- dan keduanya mendefinisikan mcdt_probe/mcdt_driver
# untuk compatible yang sama (duplikasi simbol saat link).
obj-$(CONFIG_SPRD_AUDIO_MCDT_R1P0)	+= mcdt_hw.o
