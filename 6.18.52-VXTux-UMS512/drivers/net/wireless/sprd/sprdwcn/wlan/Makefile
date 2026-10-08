# SPDX-License-Identifier: GPL-2.0
#
# drivers/vxtux/net/wireless/sprdwcn/wlan/Makefile
# Port 6.18: sprd_wlan_combo (SC2355 WiFi) — pengganti blob sprd_wlan_combo.ko
# Sumber: gpl-source/hadenix_ums512_wcn/drivers/wcn/wlan/sc2355/

# D5(d) (2026-09-30): ketiga baris -I di bawah salah. Pohon sebenarnya
# ber-directory di drivers/net/wireless/SPRD/sprdwcn/, jadi dua baris
# terakhir kehilangan satu tingkat "sprd/". Verifikasi:
#   yang ditulis : .../wireless/sprdwcn/platform/include -> TIDAK ADA
#   yang benar   : .../wireless/sprd/sprdwcn/platform        -> ADA
# Baris pertama sebenarnya sudah benar: $(src) di sini adalah
# drivers/net/wireless/sprd/sprdwcn/wlan, jadi ../include = sprdwcn/include.
# Ditambah satu baris untuk direktori header sc2355 itu sendiri,
# karena semua .c meng#include "cfg80211.h" dll. secara lokal.
ccflags-y += -I$(srctree)/$(src)/../include
ccflags-y += -I$(srctree)/$(src)/sc2355
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/platform
ccflags-y += -I$(srctree)/drivers/net/wireless/sprd/sprdwcn/sdio
#
#
# 2026-10-04: the 13 -D flags the donor sets are now copied verbatim from
# gpl-source/hadenix_ums512_wcn/drivers/wcn/wlan/Makefile:1-15. This is not
# cosmetic. Without -DSC2355_FTR, main.c's
#     #if defined(SC2355_FTR)
#     #include "core_sc2355.h"
#     #endif
# never pulls the header in, so every type it defines is incomplete and every
# macro it defines is missing -- 20 errors in main.c alone, all of them
# "incomplete definition of type 'struct sprdwl_intf'" / "'struct
# sprdwl_peer_entry'" / "use of undeclared identifier 'SPRDWL_MAX_CMD_TXLEN'".
# The earlier note here said copying them "is an engineering decision, not a glue
# fix". That was wrong: without them the driver cannot compile at all. The
# evidence that they were missing was simply that this directory had never been
# built -- CONFIG_WCN_WLAN was "# CONFIG_WCN_WLAN is not set".
#
# WMMAC_WFA_CERTIFICATION enables the WFA certification code paths that
# cfg80211.c wraps in #ifdef; the rest select feature branches. Taken verbatim
# so the build matches the shipping configuration rather than my reading of it.
ccflags-y += -DSC2355_FTR -DSC2355_TX_FILTER -DSC2355_QOS_MAP
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DSC2355_RX_NO_LOOP
ccflags-y += -DACS_SUPPORT -DSC2355_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
ccflags-y += -DSOFTAP_HOOK
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
ccflags-y += -DSPLIT_STACK
# Port note 2026-10-04: -DDFS_MASTER is MISSING here and the driver cannot link
# without it. All of struct sprdwl_vif's DFS state (dfs_cac_work,
# dfs_cac_workqueue, dfs_chan_sw_work, dfs_chan_sw_workqueue, dfs_chandef) sits
# behind #ifdef DFS_MASTER, while 11h.c accesses those members unconditionally
# and five already-compiled files call its exports:
#   main.c       sprdwl_init_dfs_master / sprdwl_deinit_dfs_master
#   cfg80211.c   sprdwl_cfg80211_start_radar_detection / _channel_switch / abort_cac
#   cmdevt.c     sprdwl_11h_handle_radar_detected
#   work.c       sprdwl_send_dfs_cmd
# So the DFS master path is genuinely used and the flag must be on. It is
# absent from every donor Makefile I checked, which is why it was missed: the
# vendor tree reached 11h.c only through a path that defined it elsewhere.
# Same class of gap as the other 13 flags above.
ccflags-y += -DDFS_MASTER

obj-$(CONFIG_WCN_WLAN) += sprdwl.o

sprdwl-y := \
	sc2355/main.o \
	sc2355/cfg80211.o \
	sc2355/if_sc2355.o \
	sc2355/txrx.o \
	sc2355/tx_msg_sc2355.o \
	sc2355/rx_msg_sc2355.o \
	sc2355/work.o \
	sc2355/msg.o \
	sc2355/cmdevt.o \
	sc2355/ibss.o \
	sc2355/defrag.o \
	sc2355/reorder.o \
	sc2355/tcp_ack.o \
	sc2355/rtt.o \
	sc2355/qos.o \
	sc2355/npi.o \
	sc2355/nan.o \
	sc2355/softap_hook.o \
	sc2355/debug.o \
	sc2355/tracer.o \
	sc2355/core_sc2355.o \
	sc2355/rf_marlin3.o \
	sc2355/mm.o \
	sc2355/api_version.o \
	sc2355/rnd_mac_addr.o \
	sc2355/11h.o \
	sc2355/vendor.o