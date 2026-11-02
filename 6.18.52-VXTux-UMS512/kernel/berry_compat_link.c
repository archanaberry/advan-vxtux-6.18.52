// SPDX-License-Identifier: GPL-2.0
/* V18: Berry compat - real (non-inline) stubs for symbols that the
 * VXTux 6.18 tree references but does not provide.
 *
 * These are link-time symbols, so they cannot live in the header-only
 * include/linux/berry_compat.h shim.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/ip_tunnels.h>
#include <net/secure_seq.h>
#include <net/sock.h>
#include <net/tcp.h>

/* ====== ip_tunnel_init: VXTux 6.18 renamed it to __ip_tunnel_init ====== */
int ip_tunnel_init(struct net_device *dev)
{
	return __ip_tunnel_init(dev);
}
EXPORT_SYMBOL_GPL(ip_tunnel_init);

/* ====== ip_tunnel_limit_headroom: dropped upstream, still referenced ====== */
int ip_tunnel_limit_headroom(struct net_device *dev, unsigned int headroom)
{
	(void)dev;
	(void)headroom;
	return 0;
}
EXPORT_SYMBOL_GPL(ip_tunnel_limit_headroom);


/* ====== kzalloc_flex: upstream 6.18 helper, missing here ====== */
void *kzalloc_flex(size_t size, gfp_t flags)
{
	return kzalloc(size, flags);
}
EXPORT_SYMBOL_GPL(kzalloc_flex);

/* ====== amba_bustype: drivers/amba/ is an empty stub in VXTux 6.18 ====== */
const struct bus_type amba_bustype = {
	.name		= "amba",
};
EXPORT_SYMBOL_GPL(amba_bustype);
