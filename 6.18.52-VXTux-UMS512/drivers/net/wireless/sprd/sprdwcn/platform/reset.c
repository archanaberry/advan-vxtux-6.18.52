/*
 * This function include:
 * 1. register reset callback
 * 2. notify BT FM WIFI GNSS CP2 Assert
 */

#include <linux/debug_locks.h>
#include <linux/sched/debug.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/vt_kern.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/ratelimit.h>

#include "wcn_glb.h"
#include <wcn-618-compat.h>	/* 5.4 -> 6.18 shim */

ATOMIC_NOTIFIER_HEAD(wcn_reset_notifier_list);
EXPORT_SYMBOL_GPL(wcn_reset_notifier_list);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void wcn_reset_cp2(void);
void wcn_reset_cp2(void)
{
	wcn_chip_power_off();
	atomic_notifier_call_chain(&wcn_reset_notifier_list, 0, NULL);
}
EXPORT_SYMBOL_GPL(wcn_reset_cp2);

/* 6.18/modpost: modul WAJIB menyatakan lisensi; build built-in (=y) tidak
 * memeriksa ini, jadi cacatnya baru muncul saat objek dibangun sebagai modul
 * (`.ko`) - ditemukan lewat build modular WCN 2026-09-18. */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Unisoc WCN port VXTux (6.18)");
