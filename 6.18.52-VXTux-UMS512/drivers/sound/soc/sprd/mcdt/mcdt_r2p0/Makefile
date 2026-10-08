# MCDT(Multi-channel Data Transfer) Support
SPRD_MCDT_TREE := drivers/vxtux/audio/mcdt/
# $(srctree)/ WAJIB untuk build O=<dir>.
# Bukti cacat (probe wiring 2026-09-19): tanpa $(srctree)/, `-I` relatif
# diresolusi terhadap CWD proses kbuild = DIREKTORI KELUARAN (buildd/), bukan
# pohon sumber. mcdt_hw.h ada di mcdt/ (induk), jadi `-I drivers/.../mcdt/`
# menunjuk ke buildd/drivers/.../mcdt/ yang tidak punya header itu, dan build
# berhenti `fatal error: 'mcdt_hw.h' file not found`. Cacat ini tidak pernah
# terlihat sebelumnya karena cabang r2p0 sendiri belum pernah tercapai.
# mcdt_r1p0/Makefile sudah memakai bentuk $(srctree)/ yang benar.
ccflags-y += -I$(srctree)/$(SPRD_MCDT_TREE)

obj-$(CONFIG_SPRD_AUDIO_MCDT_R2P0)	+= mcdt_hw_r2p0.o
