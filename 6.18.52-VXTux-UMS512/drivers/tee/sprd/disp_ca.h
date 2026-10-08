/* SPDX-License-Identifier: GPL-2.0 */
/*
 * drivers/vxtux/trusty/disp_ca.h
 * API provider display-CA Trusty (RE) — pengganti ekspor disp_trusty.c.
 *
 * Kenapa header ini ada: ketiga fungsi di bawah didefinisikan di disp_ca.c dan
 * sudah EXPORT_SYMBOL_GPL, tetapi tidak pernah punya deklarasi publik. Akibatnya
 * clang memberi -Wmissing-prototypes pada ketiganya. Warning itu baru terlihat
 * setelah wiring trusty/ diperbaiki (2026-09-18) — sebelumnya berkas ini tidak
 * pernah masuk daftar kompilasi, jadi cacatnya tersembunyi.
 *
 * Konsumen tipikal: jalur DRM/panel yang menanyakan status TEE display.
 * Tanpa TEE (kondisi 6.18 VXTux sekarang) semuanya fail-gracefully -ENODEV.
 */
#ifndef _VXTUX_TRUSTY_DISP_CA_H
#define _VXTUX_TRUSTY_DISP_CA_H

#include <linux/types.h>

ssize_t disp_ca_write(void *buf, size_t len);
int disp_ca_wait_response(void);
int disp_ca_connect(void);

#endif /* _VXTUX_TRUSTY_DISP_CA_H */
