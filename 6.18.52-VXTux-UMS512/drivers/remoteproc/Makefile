# SPDX-License-Identifier: GPL-2.0
#
# Generic framework for controlling remote processors
#

obj-$(CONFIG_REMOTEPROC)		+= remoteproc.o
remoteproc-y				:= remoteproc_core.o
remoteproc-y				+= remoteproc_coredump.o
remoteproc-y				+= remoteproc_debugfs.o
remoteproc-y				+= remoteproc_sysfs.o
remoteproc-y				+= remoteproc_virtio.o
remoteproc-y				+= remoteproc_elf_loader.o
obj-$(CONFIG_REMOTEPROC_CDEV)		+= remoteproc_cdev.o
