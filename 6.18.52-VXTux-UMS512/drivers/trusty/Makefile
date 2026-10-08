# SPDX-License-Identifier: GPL-2.0
#
# Port Unisoc/UMS512 dari donor GPL work/donors_20260930/bocchi_unisoc54
# (kernel_unisoc-5.4, HEAD 4d2f575) drivers/trusty/.
#
# `trusty-tui.o` TIDAK ada di sini: pohon ini sudah punya drivers/misc/trusty-tui.c
# dengan simbol CONFIG_TRUSTY_TUI yang sama (lihat catatan di Kconfig).

obj-$(CONFIG_TRUSTY)		+= trusty.o
trusty-y			:= trusty-core.o trusty-mem.o

obj-$(CONFIG_TRUSTY)		+= trusty-irq.o
obj-$(CONFIG_TRUSTY)		+= trusty-pm.o
obj-$(CONFIG_TRUSTY_LOG)	+= trusty-log.o
obj-$(CONFIG_TRUSTY_VIRTIO)	+= trusty-virtio.o
obj-$(CONFIG_TRUSTY_VIRTIO_IPC)	+= trusty-ipc.o
obj-$(CONFIG_SPRD_TSHM)		+= sprd_shm.o
