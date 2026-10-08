/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * sprd_map.h — ABI ioctl map_user Unisoc (port lokal; fork: include/uapi/linux/sprd_map.h)
 * Dipertahankan identik agar userspace vendor tetap kompatibel.
 */
#ifndef _UAPI_LINUX_SPRD_MAP_H
#define _UAPI_LINUX_SPRD_MAP_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct sprd_pmem_info {
	unsigned long	phy_addr;
	unsigned int	phys_offset;
	size_t		size;
};

#define MAP_USER_MAGIC	'm'

/* map masuk ke memori fisik reserved (mis. faceid-mem) */
#define MAP_USER_VIR	_IOWR(MAP_USER_MAGIC, 0, struct sprd_pmem_info)

#endif /* _UAPI_LINUX_SPRD_MAP_H */
