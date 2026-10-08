/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Unisoc ION <-> Trusty TEE IPC client: exported transport API.
 *
 * These four functions are what the vendor blob ion_ipc_trusty.ko exports
 * through its ksymtab (recovered with readelf: ksymtab entries for
 * ion_tipc_init, ion_tipc_read, ion_tipc_write, ion_tipc_exit), so any consumer
 * built against the vendor stack expects these exact names.
 *
 * The definitions live in drivers/dma-buf/sprd/ion_ipc_trusty.c, which also
 * documents the recovered protocol and the deliberate deviations from the blob.
 */
#ifndef _LINUX_ION_IPC_TRUSTY_H
#define _LINUX_ION_IPC_TRUSTY_H

#include <linux/errno.h>
#include <linux/types.h>

#if IS_ENABLED(CONFIG_ION_IPC_TRUSTY)

int ion_tipc_init(void);
void ion_tipc_exit(void);

/*
 * Forward one message to the TEE. Returns the number of bytes queued, or a
 * negative errno: -ENOTCONN if the channel is not connected, -ENOBUFS if the
 * message does not fit the transmit buffer, -EINVAL on bad arguments.
 */
int ion_tipc_write(const void *data, size_t len);

/*
 * Block until one message arrives from the TEE, copy up to len bytes into
 * data and return the number copied. Returns -ENOTCONN when the channel goes
 * away while waiting.
 */
int ion_tipc_read(void *data, size_t len);

#else /* !CONFIG_ION_IPC_TRUSTY */

static inline int ion_tipc_init(void)
{
	return -ENODEV;
}

static inline void ion_tipc_exit(void)
{
}

static inline int ion_tipc_write(const void *data, size_t len)
{
	return -ENODEV;
}

static inline int ion_tipc_read(void *data, size_t len)
{
	return -ENODEV;
}

#endif /* CONFIG_ION_IPC_TRUSTY */

#endif /* _LINUX_ION_IPC_TRUSTY_H */
