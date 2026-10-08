#include <linux/notifier.h>

#include "wcn_glb.h"
#include <wcn-618-compat.h>	/* 5.4 -> 6.18 shim */
#include <linux/module.h>

static int wcn_reset(struct notifier_block *this, unsigned long ev, void *ptr)
{
	WCN_INFO("%s: reset callback coming\n", __func__);

	return NOTIFY_DONE;
}

static struct notifier_block wcn_reset_block = {
	.notifier_call = wcn_reset,
};

int reset_test_init(void)
{
	atomic_notifier_chain_register(&wcn_reset_notifier_list,
				       &wcn_reset_block);

	return 0;
}


/* 6.18/modpost: modul WAJIB menyatakan lisensi; build built-in (=y) tidak
 * memeriksa ini, jadi cacatnya baru muncul saat objek dibangun sebagai modul
 * (`.ko`) - ditemukan lewat build modular WCN 2026-09-18. */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Unisoc WCN port VXTux (6.18)");
