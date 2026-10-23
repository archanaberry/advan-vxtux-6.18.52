/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Spreadtrum Secure Monitor (SIP) service client interface.
 *
 * The stock 5.4 vendor module `sprd_sip_svc.ko` exposes a single entry point,
 * `sprd_sip_svc_get_handle()`, which returns a pointer to a static structure
 * holding one function pointer per SMC wrapper plus the firmware-reported
 * version of each service.  Consumers (cpufreq-v2, pdbg, power_stat, ...)
 * only ever call through that handle.
 *
 * The vendor header is not published, so the field order below is this
 * tree's own.  It is source-level API, not ABI: every consumer is built
 * from this same header, so only the SMC IDs and argument order - which are
 * the real firmware contract - have to match the stock module.
 */
#ifndef __LINUX_SPRD_SIP_SVC_H
#define __LINUX_SPRD_SIP_SVC_H

#include <linux/types.h>

/* SMC function IDs (SIP fast call, owner 0x02). */
#define SPRD_SIP_SVC_HANDLE_ID			0x82000100
#define SPRD_SIP_SVC_PERF_SET_FREQ		0x82000101
#define SPRD_SIP_SVC_PERF_GET_FREQ		0x82000102
#define SPRD_SIP_SVC_PERF_SET_DIV		0x82000103
#define SPRD_SIP_SVC_PERF_GET_DIV		0x82000104
#define SPRD_SIP_SVC_PERF_SET_PARENT		0x82000105
#define SPRD_SIP_SVC_PERF_GET_PARENT		0x82000106

#define SPRD_SIP_SVC_SVC2_ID			0x82000200

#define SPRD_SIP_SVC_STORAGE_ID			0x82000300
#define SPRD_SIP_SVC_STORAGE_UFS_CRYPTO_EN	0x82000301
#define SPRD_SIP_SVC_STORAGE_UFS_CRYPTO_DIS	0x82000302

#define SPRD_SIP_SVC_PWR_ID			0x82000500
#define SPRD_SIP_SVC_PWR_GET_WAKEUP_SOURCE	0x82000501
#define SPRD_SIP_SVC_PWR_GET_PDBG_INFO		0x82000502

#define SPRD_SIP_SVC_DVFS_ID			0x82000600
#define SPRD_SIP_SVC_DVFS_ENABLE		0x82000601
#define SPRD_SIP_SVC_DVFS_TABLE_UPDATE		0x82000602
#define SPRD_SIP_SVC_DVFS_STEP_SET		0x82000603
#define SPRD_SIP_SVC_DVFS_MARGIN_SET		0x82000604
#define SPRD_SIP_SVC_DVFS_FREQ_SET		0x82000605
#define SPRD_SIP_SVC_DVFS_FREQ_GET		0x82000606
#define SPRD_SIP_SVC_DVFS_PAIR_GET		0x82000607
#define SPRD_SIP_SVC_DVFS_PMIC_SET		0x82000608
#define SPRD_SIP_SVC_DVFS_BIN_SET		0x82000609
#define SPRD_SIP_SVC_DVFS_VERSION_SET		0x8200060a
#define SPRD_SIP_SVC_DVFS_INIT			0x8200060b
#define SPRD_SIP_SVC_DVFS_DEBUG_INIT		0x8200060c

#define SPRD_SIP_SVC_NPU_ID			0x82000700
#define SPRD_SIP_SVC_NPU_SET_FREQ		0x82000701
#define SPRD_SIP_SVC_NPU_DISABLE_IDLE		0x82000702
#define SPRD_SIP_SVC_NPU_GET_MAX_STATE		0x82000703
#define SPRD_SIP_SVC_NPU_GET_OPP			0x82000704
#define SPRD_SIP_SVC_NPU_SET_VOLTS		0x82000705

/* Debug service lives in the OEM range, not in the SIP owner range. */
#define SPRD_SIP_SVC_DBG_ID			0xc2000200
#define SPRD_SIP_SVC_DBG_SET_HANG_HDL		0xc2000201
#define SPRD_SIP_SVC_DBG_GET_HANG_CTX		0xc2000202

struct sprd_sip_svc_handle {
	/* Versions reported by the firmware, as (major << 16 | minor). */
	u32 perf_version;
	u32 storage_version;
	u32 pwr_version;
	u32 dvfs_version;
	u32 npu_version;
	u32 dbg_version;

	/* PERF */
	int (*perf_set_freq)(u32 clk, u64 freq);
	u64 (*perf_get_freq)(u32 clk);
	int (*perf_set_div)(u32 clk, u32 div);
	u32 (*perf_get_div)(u32 clk);
	int (*perf_set_parent)(u32 clk, u32 parent);
	u32 (*perf_get_parent)(u32 clk);

	/* STORAGE */
	int (*storage_ufs_crypto_enable)(u32 index);
	int (*storage_ufs_crypto_disable)(u32 index);

	/* PWR */
	int (*pwr_get_wakeup_source)(u64 *mask);
	int (*pwr_get_pdbg_info)(u32 id, u64 *lo, u64 *hi);
	/*
	 * Varian empat kata (a0..a3).  Vendor `sprd_pdbg.ko` membaca empat
	 * u64 hasil SMC 0x82000502 per id (blob: empat pointer keluar di
	 * +0xf8..+0x117, count byte di +0x116), jadi dua kata tidak cukup.
	 */
	int (*pwr_get_pdbg_info_words)(u32 id, u64 out[4]);

	/* DVFS */
	int (*dvfs_enable)(u32 enable);
	int (*dvfs_table_update)(u32 index, u64 table);
	int (*dvfs_step_set)(u32 step);
	int (*dvfs_margin_set)(u32 margin);
	int (*dvfs_freq_set)(u32 index, u64 freq);
	u64 (*dvfs_freq_get)(u32 index);
	int (*dvfs_pair_get)(u32 index, u64 *pair);
	int (*dvfs_pmic_set)(u32 type, u64 addr);
	int (*dvfs_bin_set)(u32 bin);
	int (*dvfs_version_set)(u32 version);
	int (*dvfs_init)(u64 table, u32 num);
	int (*dvfs_debug_init)(u32 enable);

	/* NPU */
	int (*npu_set_freq)(u32 freq);
	int (*npu_disable_idle)(u32 disable);
	int (*npu_get_max_state)(u32 *state);
	int (*npu_get_opp)(u32 level, u64 *opp);
	int (*npu_set_volts)(u32 level, u32 volt);

	/* DBG */
	int (*dbg_set_hang_hdl)(u64 hdl);
	int (*dbg_get_hang_ctx)(u64 *ctx);
};

struct sprd_sip_svc_handle *sprd_sip_svc_get_handle(void);

#endif /* __LINUX_SPRD_SIP_SVC_H */
