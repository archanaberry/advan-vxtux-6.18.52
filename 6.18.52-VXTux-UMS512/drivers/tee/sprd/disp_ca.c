// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/vxtux/trusty/disp_ca.c
 * Trusty display-CA helpers — RE rewrite dari stub proven (build v4).
 * Signature proven Fase 10:
 *   ssize_t disp_ca_write(void *, size_t)
 *   int     disp_ca_wait_response(void)
 *   int     disp_ca_connect(void)          (dari dump provider asli)
 * Provider asli: disp_trusty.c (masuk built-in saat TRUSTY_VIRTIO_IPC=y).
 */
#include <linux/module.h>
#include <linux/types.h>

#include <vxtux/compat.h>
#include "disp_ca.h"

/* Fail-gracefully tanpa TEE — pola proven stub v4 (boot tetap hijau) */
ssize_t disp_ca_write(void *buf, size_t len)
{
	return -ENODEV;	/* TODO(RE): kirim SMC call dari decompile disp_trusty */
}
EXPORT_SYMBOL_GPL(disp_ca_write);

int disp_ca_wait_response(void)
{
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(disp_ca_wait_response);

int disp_ca_connect(void)
{
	return -ENODEV;
}
EXPORT_SYMBOL_GPL(disp_ca_connect);

MODULE_AUTHOR("VXTux Project");
MODULE_DESCRIPTION("Trusty display-CA helpers (RE) for UMS512");
MODULE_LICENSE("GPL");
