// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/vendor_abi/vendor_abi_ubsan.c
 *
 * __ubsan_handle_cfi_check_fail_abort — handler kegagalan CFI 5.4.
 *
 * MASALAH (gerbang ronde-11, kelas TOOLCHAIN-NOISE, leverage TERTINGGI)
 *   103 modul blob meng-UND simbol ini. 103 = jumlah modul terbanyak untuk
 *   satu simbol di seluruh backlog. Di 6.18 simbolnya TIDAK ADA:
 *     - lib/ubsan.c berhenti di __ubsan_handle_builtin_unreachable;
 *     - lib/ubsan.h hanya memuat nama enum `ubsan_cfi_check_fail` (ABI enum
 *       SanitizerHandler milik clang), bukan implementasi;
 *     - grep 0x di upstream/linux-6.18 untuk "cfi_check_fail" hanya menemukan
 *       baris enum itu.
 *   Jadi ini bukan noise alat: simbolnya NYATA kurang di kernel kita.
 *
 * BUKTI PENYEDIA + SIGNATURE (oracle 5.4: pohon kernel fork T618)
 *   [C1] kernel/cfi.c:17-34
 *	  #ifdef CONFIG_CFI_PERMISSIVE
 *	  #define cfi_failure_handler  __ubsan_handle_cfi_check_fail
 *	  #else
 *	  #define cfi_failure_handler  __ubsan_handle_cfi_check_fail_abort
 *	  static inline void handle_cfi_failure(void *ptr)
 *	  { pr_err("CFI failure (target: [<%px>] %pF):\n", ptr, ptr); BUG(); }
 *   [C2] kernel/cfi.c:297-301
 *	  void cfi_failure_handler(void *data, void *ptr, void *vtable)
 *	  { handle_cfi_failure(ptr); }
 *	  EXPORT_SYMBOL(cfi_failure_handler);
 *	→ signature 3 argumen (data, ptr, vtable) + BUG(); mode enforcing.
 *
 * RE SITUS PEMANGGIL DI BLOB (llvm-objdump -dr, semua modul)
 *   Fungsi __cfi_check_fail yang disisipkan clang mula-mula memeriksa data
 *   hash (ldrb/cmp #5) lalu, pada kegagalan, menimpa x0 dengan id handler:
 *	  mov  w0, #0x2          ; 2 = ubsan_cfi_check_fail (lib/ubsan.h 6.18)
 *	  bl   __ubsan_handle_cfi_check_fail_abort
 *   x1 tetap berisi target pointer dari pemanggil (__cfi_check) — itulah
 *   argumen kedua yang dicetak [C1]. Urutan argumen 3-elemen ini yang harus
 *   dipertahankan; kalau kita mendeklarasikan handler 1-argumen, cetakan
 *   pointer target akan salah.
 *
 * BATAS JUJUR
 *   Handler ini fatal (BUG) — persis perilaku vendor. Ia tidak "menutup"
 *   kegagalan CFI; ia memberi pelaporan yang benar saat kegagalan terjadi.
 */

#define pr_fmt(fmt) "vendor_abi: " fmt

#include <linux/kernel.h>
#include <linux/export.h>
#include <asm/bug.h>

#include "vendor_abi.h"

void __ubsan_handle_cfi_check_fail_abort(void *data, void *ptr, void *vtable)
{
	/* data/vtable tidak dipakai implementasi 5.4 [C1]; cetak ptr. */
	pr_err("CFI failure (target: [<%px>] %pF):\n", ptr, ptr);
	BUG();
}
EXPORT_SYMBOL(__ubsan_handle_cfi_check_fail_abort);
