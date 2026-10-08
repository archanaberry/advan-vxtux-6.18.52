// SPDX-License-Identifier: GPL-2.0
/*
 * STUB -- sprd_ion_is_reserved() shim for the sprdwcn GSP driver.
 *
 * gsp_layer_get_dmabuf() calls sprd_ion_is_reserved() and
 * sprd_ion_get_phys_addr(). On the stock Unisoc 5.4 tree these live in
 * drivers/staging/android/ion plus a Unisoc out-of-tree ion carrier
 * (sprd_ion_*.c). ION was removed from mainline, and this tree carries
 * only the *headers* (include/linux/ion.h, include/linux/sprd_ion.h)
 * copied from the marohinmark UMS512 donor, which is the same SoC.
 * No donor (fork_t618, UMS9230, UWE5621DS) contains a .c that defines
 * sprd_ion_is_reserved.
 *
 * The reservation judgement is an ION-carrier property: it needs the ION
 * buffer metadata only a real ION heap driver exposes. This is therefore a
 * documented shim, not a reconstruction. No ION semantics are invented.
 *
 * sprd_ion_get_phys_addr() is supplied by the ported driver at
 * drivers/dma-buf/sprd/sprd_ion.c and is deliberately not duplicated here.
 */

#include <linux/errno.h>
#include <linux/sprd_ion.h>

/*
 * STUB: cannot judge reservation without an ION carrier. Report
 * "not reserved" rather than erroring, so the caller takes the IOVA
 * branch, which is the branch a dma-heap buffer would legitimately use.
 * Still -EOPNOTSUPP would abort the layer setup, so this one returns 0
 * with *reserved = false and is the only stub that reports success.
 */
int sprd_ion_is_reserved(int fd, struct dma_buf *dmabuf, bool *reserved)
{
	if (reserved)
		*reserved = false;
	return 0;
}

/*
 * sprd_ion_get_phys_addr() is intentionally NOT defined here.
 *
 * drivers/dma-buf/sprd/sprd_ion.c is the ported Unisoc ION driver and already
 * provides it. Defining it in both places is a hard duplicate-symbol link
 * error once CONFIG_DRM_SPRD_GSP and CONFIG_SPRD_ION are both enabled, so
 * this shim supplies only sprd_ion_is_reserved(), the one symbol the real
 * driver lacks.
 */
