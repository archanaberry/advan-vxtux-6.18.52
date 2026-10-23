// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/vendor_abi/vendor_abi_compat54.c
 *
 * Simbol 5.4 yang tidak punya padanan langsung di 6.18, tapi semantiknya
 * bisa diterjemahkan tanpa menebak.
 *
 * `init_timer_key` — 18 blob stok mengimpor simbol ini (terbanyak dari
 * kelas `hilang` di docs/UNDEF-REACHABILITY.md). 5.4:
 *
 *     void init_timer_key(struct timer_list *timer,
 *                         void (*func)(struct timer_list *),
 *                         unsigned int flags, const char *name,
 *                         struct lock_class_key *key);      [include/linux/timer.h]
 *
 * 6.18 tidak lagi punya fungsi itu; yang ada makro `timer_setup(timer, fn,
 * flags)`. Parameter `name`/`key` di 5.4 hanya untuk lockdep, jadi bisa
 * dibuang: pemanggil 6.18 memakai lockdep key miliknya sendiri.
 *
 * BATAS JUJUR
 *   Signature diambil dari header 5.4 (bukan ditebak dari blob). Wrapper ini
 *   TIDAK dipanggil kode kernel mana pun; ia hanya menyediakan simbol agar
 *   referensi blob/source port resolve. Tidak ada pengujian runtime.
 */

#include <linux/export.h>
#include <linux/module.h>
#include <linux/timer.h>

void init_timer_key(struct timer_list *timer,
		    void (*func)(struct timer_list *timer),
		    unsigned int flags, const char *name,
		    struct lock_class_key *key)
{
	/* name/key tidak dipakai: 6.18 memakai lockdep key internal timer_setup. */
	timer_setup(timer, func, flags);
}
EXPORT_SYMBOL(init_timer_key);
