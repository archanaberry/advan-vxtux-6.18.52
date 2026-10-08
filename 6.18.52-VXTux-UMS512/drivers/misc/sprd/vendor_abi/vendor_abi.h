/* SPDX-License-Identifier: GPL-2.0 */
/*
 * drivers/vxtux/vendor_abi/vendor_abi.h — prototipe penyedia ABI blob vendor.
 *
 * Dipakai untuk menutup kelas -Wmissing-prototypes (audit_provider_proto.py);
 * implementasinya ada di vendor_abi_arm64.c / vendor_abi_ubsan.c.
 */
#ifndef _VXTUX_VENDOR_ABI_H
#define _VXTUX_VENDOR_ABI_H

/*
 * Handler kegagalan CFI/UBSAN 5.4 (kernel/cfi.c, macro cfi_failure_handler
 * saat CONFIG_CFI_PERMISSIVE mati):
 *
 *	void cfi_failure_handler(void *data, void *ptr, void *vtable)
 *	{
 *		pr_err("CFI failure (target: [<%px>] %pF):\n", ptr, ptr);
 *		BUG();
 *	}
 *
 * Signature 3-argumen ini penting: pemanggil di blob (fungsi __cfi_check_fail
 * yang dikompilasi clang) mengisi x1 dengan target pointer sementara x0
 * ditimpa dengan id handler (0x2 = ubsan_cfi_check_fail, lihat lib/ubsan.h).
 */
void __ubsan_handle_cfi_check_fail_abort(void *data, void *ptr, void *vtable);

#endif /* _VXTUX_VENDOR_ABI_H */
