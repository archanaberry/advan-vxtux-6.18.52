# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/sipc/Makefile — kerangka SIPC Unisoc (port fork GPL 5.4 -> 6.18)
#
# Shim API mailbox vendor: wajib ada sebelum apa pun yang memakai SIPC.
# (SPRD_MAILBOX = bool, default y; penyedia mainline sprd-mailbox = y.)
obj-$(CONFIG_SPRD_MAILBOX)	+= sprd_mbox_vendor.o

# Inti SIPC: SENGAJA built-in saja. Bukti call-graph (2026-09-23):
# agdsp_access (=y) -> smsg_* (smsg.c), dan sound ASoC (=y) -> agdsp_*;
# audio-smsg.o hanya dibangun di bawah SPRD_AUDIO_SIPC (=y). Membuat inti
# SIPC modular berarti vmlinux menunjuk simbol modul -> undefined at link.
obj-$(CONFIG_SPRD_SIPC)		+= sipc.o smsg.o smem.o sbuf.o sblock.o \
				   sipc_debugfs.o slog_bridge.o sprd_actions_queue.o

# Transport SBLK khusus WCN (vendor swcn_blk.c): konsumen klosennya (wcn_sipc,
# lapisan integrate) tidak dibangun di papan SDIO ini, jadi transport ini
# dibangun sebagai MODUL agar bisa di-insmod/diuji satu per satu (QEMU).
# Komposit sendiri: swcnblk.c memanggil sblock_* dari inti SIPC (=y) sehingga
# swcnblk.ko cukup satu objek.
obj-$(CONFIG_SPRD_SIPC_SWCNBLK)	+= swcnblk.o

# Transport MODEM (port 2026-09-30; sumber GPL: bocchi_unisoc54 5.4).
# Diperbankan =m: tiap lapisan punya konsumen berbeda (spool/spipe
# dipakai daemon userspace modem, seth dipakai stack LTE), jadi bisa
# di-insmod/di-uninsmod satu per satu saat bring-up, dan `depends on`
# (=m) membuatnya hilang dari vmlinux kalau intinya tidak dibangun.
# Urutan insmod wajib: unisoc-mailbox -> sipc-core -> sipx ->
# seth/spool/spipe (seth.ko meng-undef 11 simbol sipx_*).
obj-$(CONFIG_SPRD_SIPC_SPIPE)		+= spipe.o
obj-$(CONFIG_SPRD_SIPC_SPOOL)		+= spool.o
obj-$(CONFIG_SPRD_SIPC_SETH)		+= seth.o

# SIPX = BOOL (bukan tristate): sipx.c adalah satu-satunya penyedia 12
# simbol sipx_* (sipx_send/sipx_receive/... di include/linux/soc/sprd/
# sipc.h:913-1032). Kalau jadi modul, seth.o yang meng-undef simbol
# itu tidak akan punya penyedia saat link vmlinux. bool =y memastikan
# sipx.ko ikut vmlinux setiap kali SPRD_SIPC_SETH hidup.
obj-$(CONFIG_SPRD_SIPC_ZERO_COPY_SIPX)	+= sipx.o