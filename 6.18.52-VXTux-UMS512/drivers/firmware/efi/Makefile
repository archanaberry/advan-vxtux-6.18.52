# SPDX-License-Identifier: GPL-2.0
#
# Makefile for linux kernel
#

#
# ARM64 maps efi runtime services in userspace addresses
# which don't have KASAN shadow. So dereference of these addresses
# in efi_call_virt() will cause crash if this code instrumented.
#
KASAN_SANITIZE_runtime-wrappers.o	:= n

obj-$(CONFIG_ACPI_BGRT) 		+= efi-bgrt.o
obj-$(CONFIG_EFI)			+= efi.o vars.o reboot.o memattr.o tpm.o
obj-$(CONFIG_EFI)			+= memmap.o
ifneq ($(CONFIG_EFI_CAPSULE_LOADER),)
obj-$(CONFIG_EFI)			+= capsule.o
endif
obj-$(CONFIG_EFI_PARAMS_FROM_FDT)	+= fdtparams.o
obj-$(CONFIG_EFI_ESRT)			+= esrt.o
obj-$(CONFIG_EFI_VARS_PSTORE)		+= efi-pstore.o
obj-$(CONFIG_UEFI_CPER)			+= cper.o cper_cxl.o
obj-$(CONFIG_EFI_RUNTIME_WRAPPERS)	+= runtime-wrappers.o
obj-$(CONFIG_EFI_BOOTLOADER_CONTROL)	+= efibc.o
obj-$(CONFIG_EFI_TEST)			+= test/
obj-$(CONFIG_EFI_DEV_PATH_PARSER)	+= dev-path-parser.o
obj-$(CONFIG_EFI_RCI2_TABLE)		+= rci2-table.o
obj-$(CONFIG_EFI_EMBEDDED_FIRMWARE)	+= embedded-firmware.o
obj-$(CONFIG_LOAD_UEFI_KEYS)		+= mokvar-table.o
obj-$(CONFIG_OVMF_DEBUG_LOG)		+= ovmf-debug-log.o

obj-$(CONFIG_SYSFB)			+= sysfb_efi.o

arm-obj-$(CONFIG_EFI)			:= efi-init.o arm-runtime.o
obj-$(CONFIG_ARM)			+= $(arm-obj-y)
obj-$(CONFIG_ARM64)			+= $(arm-obj-y)
obj-$(CONFIG_EFI_CAPSULE_LOADER)	+= capsule-loader.o
obj-$(CONFIG_EFI_EARLYCON)		+= earlycon.o
obj-$(CONFIG_UEFI_CPER_ARM)		+= cper-arm.o
obj-$(CONFIG_UNACCEPTED_MEMORY)		+= unaccepted_memory.o

# Missing from this tree: arch/arm64/Makefile puts
# $(objtree)/drivers/firmware/efi/libstub/lib.a in libs-y when CONFIG_EFI_STUB is
# set, but nothing descended into libstub/, so vmlinux.o failed at the final
# link with "No rule to make target 'drivers/firmware/efi/libstub/lib.a'".
# Upstream 6.18 has this line; the vendor prune dropped it.
subdir-$(CONFIG_EFI_STUB)		+= libstub
