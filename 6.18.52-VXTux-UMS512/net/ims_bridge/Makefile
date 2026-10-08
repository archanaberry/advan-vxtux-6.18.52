#
# SPRD IMS bridge -- replaces blob ims_bridge.ko
# Port 2026-10-04 from work/donors_20260930/bocchi_unisoc54/net/ims_bridge/
#
# Verified before porting: every non-local dependency resolves in-tree, notably
# <net/esp.h> (include/net/esp.h exists in this 6.18 tree, it is NOT a Unisoc
# addition). The uapi header was missing and came from the donor.
#
obj-$(CONFIG_SPRD_IMS_BRIDGE) += ims_bridge.o

ims_bridge-objs := imsbr_main.o imsbr_hooks.o imsbr_sipc.o imsbr_packet.o \
		imsbr_core.o imsbr_netlink.o imsbr_test.o

obj-$(CONFIG_SPRD_IMS_BRIDGE_TRANSIT) += ims_bridge_t.o

ims_bridge_t-objs := imsbr_transit.o
