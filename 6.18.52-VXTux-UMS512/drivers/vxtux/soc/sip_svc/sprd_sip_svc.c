// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_sip_svc - Spreadtrum Secure Monitor (SIP) service client.
 *
 * Port of the 5.4 vendor module `sprd_sip_svc.ko`.  The module owns no
 * device: it only wraps the SMC function IDs that the bootloader/firmware
 * implements and hands a table of wrappers to the power, cpufreq, npu,
 * storage and debug drivers through sprd_sip_svc_get_handle().
 *
 * Every ID and argument order below was recovered from the vendor blob
 * (see docs/RE-SPRD-SIP-SVC.md) - do not renumber them, firmware rejects
 * unknown IDs and the caller then sees a negative errno.
 */
#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sprd_sip_svc.h>
#include <linux/stddef.h>
#include <linux/string.h>

#define SIP_ARGS(res, id, a1, a2, a3, a4, a5, a6)			\
	__arm_smccc_smc((id), (unsigned long)(a1), (unsigned long)(a2),	\
			(unsigned long)(a3), (unsigned long)(a4),	\
			(unsigned long)(a5), (unsigned long)(a6), 0,	\
			(res), NULL)

/*
 * The firmware reports "not supported" as a0 == 0xffffffff (or any value
 * with the top bit set); everything below that is either a real return
 * value or a negative errno passed through from the monitor.
 */
static inline bool sip_failed(struct arm_smccc_res *res)
{
	return res->a0 == (unsigned long)-1 || (s32)res->a0 < 0;
}

static inline int sip_ret(struct arm_smccc_res *res)
{
	return sip_failed(res) ? -EOPNOTSUPP : (int)res->a0;
}

/*
 * Vendor error remapping, recovered from the blob's `sprd_sip_remap_err`.
 * The monitor returns a0 = -1 (or any value with the top bit set) for
 * "not supported"; everything else is passed straight through as a
 * positive-or-negative status the caller checks.  This is what the
 * cpufreq DVFS wrappers use, so their return value is the raw firmware
 * status rather than the coerced -EOPNOTSUPP that sip_ret() gives.
 */
static inline int sip_remap_err(unsigned long a0)
{
	if (sip_failed(&(struct arm_smccc_res){ .a0 = a0 }))
		return -EOPNOTSUPP;
	return (int)a0;
}

static struct sprd_sip_svc_handle sprd_handle;

/*
 * Vendor groups its function pointers into per-service sub-structs that
 * each carry a `struct sprd_sip_svc_rev_info rev`.  cpufreq-v2 walks
 * sip->dvfs_ops.* with those exact signatures, so the dvfs_ops sub-struct
 * mirrors the vendor's field-for-field (see include/linux/sprd_sip_svc.h).
 * The other services (PERF/PWR/NPU/STORAGE/DBG) stay flat because nothing
 * in-tree consumes their rev fields.
 */

static u32 sip_probe(u32 id, const char *name)
{
	struct arm_smccc_res res;
	u32 major, minor;

	SIP_ARGS(&res, id, 0, 0, 0, 0, 0, 0);
	if (sip_failed(&res)) {
		pr_info("sprd_sip_svc: %s service not present in firmware\n",
			name);
		return 0;
	}

	/* (major << 16 | minor), as printed by the vendor module. */
	major = (u32)res.a0 >> 16;
	minor = (u32)res.a0 & 0xffff;
	pr_info("SPRD SIP SVC %s:v%u.%u detected in firmware.\n",
		name, major, minor);
	return (major << 16) | (minor & 0xffff);
}

/* ------------------------------------------------------------------ PERF */

static int sip_perf_set_freq(u32 clk, u64 freq)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_SET_FREQ, clk, freq, 0, 0, 0, 0);
	return sip_ret(&res);
}

static u64 sip_perf_get_freq(u32 clk)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_GET_FREQ, clk, 0, 0, 0, 0, 0);
	return sip_failed(&res) ? 0 : res.a0;
}

static int sip_perf_set_div(u32 clk, u32 div)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_SET_DIV, clk, div, 0, 0, 0, 0);
	return sip_ret(&res);
}

static u32 sip_perf_get_div(u32 clk)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_GET_DIV, clk, 0, 0, 0, 0, 0);
	return sip_failed(&res) ? 0 : (u32)res.a0;
}

static int sip_perf_set_parent(u32 clk, u32 parent)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_SET_PARENT, clk, parent, 0, 0, 0, 0);
	return sip_ret(&res);
}

static u32 sip_perf_get_parent(u32 clk)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_PERF_GET_PARENT, clk, 0, 0, 0, 0, 0);
	return sip_failed(&res) ? 0 : (u32)res.a0;
}

/* --------------------------------------------------------------- STORAGE */

static int sip_ufs_crypto(u32 index, u32 id)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, id, index, 0, 0, 0, 0, 0);
	return sip_ret(&res);
}

static int sip_storage_ufs_crypto_enable(u32 index)
{
	return sip_ufs_crypto(index, SPRD_SIP_SVC_STORAGE_UFS_CRYPTO_EN);
}

static int sip_storage_ufs_crypto_disable(u32 index)
{
	return sip_ufs_crypto(index, SPRD_SIP_SVC_STORAGE_UFS_CRYPTO_DIS);
}

/* ------------------------------------------------------------------- PWR */

static int sip_pwr_get_wakeup_source(u64 *mask)
{
	struct arm_smccc_res res;

	if (!mask)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_PWR_GET_WAKEUP_SOURCE, 0, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	*mask = res.a0;
	return 0;
}

static int sip_pwr_get_pdbg_info(u32 id, u64 *lo, u64 *hi)
{
	struct arm_smccc_res res;

	if (!lo || !hi)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_PWR_GET_PDBG_INFO, id, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	*lo = res.a0;
	*hi = res.a1;
	return 0;
}

static int sip_pwr_get_pdbg_info_words(u32 id, u64 out[4])
{
	struct arm_smccc_res res;

	if (!out)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_PWR_GET_PDBG_INFO, id, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	out[0] = res.a0;
	out[1] = res.a1;
	out[2] = res.a2;
	out[3] = res.a3;
	return 0;
}

/* ------------------------------------------------------------------ DVFS */

static int sip_dvfs_enable(u32 cluster)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_ENABLE, cluster, 0, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_table_update(u32 cluster, u32 temp, u32 *num)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_TABLE_UPDATE, cluster, temp, 0, 0, 0, 0);
	if (num)
		*num = (u32)res.a1;
	return sip_remap_err(res.a0);
}

static int sip_dvfs_step_set(u32 cluster, u32 step)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_STEP_SET, cluster, step, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_margin_set(u32 cluster, u32 margin)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_MARGIN_SET, cluster, margin, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_freq_set(u32 cluster, u32 index)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_FREQ_SET, cluster, index, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_freq_get(u32 cluster, u64 *freq)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_FREQ_GET, cluster, 0, 0, 0, 0, 0);
	if (freq)
		*freq = res.a1;
	return sip_remap_err(res.a0);
}

static int sip_dvfs_pair_get(u32 cluster, u32 index, u64 *freq, u64 *vol)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_PAIR_GET, cluster, index, 0, 0, 0, 0);
	if (freq)
		*freq = res.a1;
	if (vol)
		*vol = res.a2;
	return sip_remap_err(res.a0);
}

static int sip_dvfs_pmic_set(u32 cluster, u32 num)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_PMIC_SET, cluster, num, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_bin_set(u32 cluster, u32 bin)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_BIN_SET, cluster, bin, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_version_set(u32 cluster, u64 *ver)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_VERSION_SET, cluster, *ver, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_init(u32 flag)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_INIT, flag, 0, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

static int sip_dvfs_debug_init(void)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DVFS_DEBUG_INIT, 0, 0, 0, 0, 0, 0);
	return sip_remap_err(res.a0);
}

/* ------------------------------------------------------------------- NPU */

static int sip_npu_set_freq(u32 freq)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_NPU_SET_FREQ, freq, 0, 0, 0, 0, 0);
	return sip_ret(&res);
}

static int sip_npu_disable_idle(u32 disable)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_NPU_DISABLE_IDLE, disable, 0, 0, 0, 0, 0);
	return sip_ret(&res);
}

static int sip_npu_get_max_state(u32 *state)
{
	struct arm_smccc_res res;

	if (!state)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_NPU_GET_MAX_STATE, 0, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	*state = (u32)res.a0;
	return 0;
}

static int sip_npu_get_opp(u32 level, u64 *opp)
{
	struct arm_smccc_res res;

	if (!opp)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_NPU_GET_OPP, level, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	*opp = res.a0;
	return 0;
}

static int sip_npu_set_volts(u32 level, u32 volt)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_NPU_SET_VOLTS, level, volt, 0, 0, 0, 0);
	return sip_ret(&res);
}

/* ------------------------------------------------------------------- DBG */

static int sip_dbg_set_hang_hdl(u64 hdl)
{
	struct arm_smccc_res res;

	SIP_ARGS(&res, SPRD_SIP_SVC_DBG_SET_HANG_HDL, hdl, 0, 0, 0, 0, 0);
	return sip_ret(&res);
}

static int sip_dbg_get_hang_ctx(u64 *ctx)
{
	struct arm_smccc_res res;

	if (!ctx)
		return -EINVAL;

	SIP_ARGS(&res, SPRD_SIP_SVC_DBG_GET_HANG_CTX, 0, 0, 0, 0, 0, 0);
	if (sip_failed(&res))
		return -EOPNOTSUPP;

	*ctx = res.a0;
	return 0;
}

/* ---------------------------------------------------------------- handle */

struct sprd_sip_svc_handle *sprd_sip_svc_get_handle(void)
{
	return &sprd_handle;
}
EXPORT_SYMBOL_GPL(sprd_sip_svc_get_handle);

static int __init sprd_sip_svc_init(void)
{
	struct arm_smccc_res res;

	/*
	 * 0x82000100 also carries the monitor's own descriptors; the vendor
	 * module logs them before touching any service.
	 */
	SIP_ARGS(&res, SPRD_SIP_SVC_HANDLE_ID, 0, 0, 0, 0, 0, 0);
	if (!sip_failed(&res))
		pr_info("sprd_sip_svc: func_addr = 0x%lx, pdg = 0x%lx, level = 0x%lx\n",
			(unsigned long)res.a0, (unsigned long)res.a1,
			(unsigned long)res.a2);
	else
		pr_info("sprd_sip_svc: monitor handle not reported\n");

	sprd_handle.perf_version = sip_probe(SPRD_SIP_SVC_HANDLE_ID, "PERF");
	sprd_handle.storage_version = sip_probe(SPRD_SIP_SVC_STORAGE_ID, "STORAGE");
	sprd_handle.pwr_version = sip_probe(SPRD_SIP_SVC_PWR_ID, "PWR");
	/* The DVFS version lives inside the ops block (see sprd_sip_svc.h). */
	sprd_handle.dvfs_ops.rev = sip_probe(SPRD_SIP_SVC_DVFS_ID, "DVFS");
	sprd_handle.npu_version = sip_probe(SPRD_SIP_SVC_NPU_ID, "NPU");
	sprd_handle.dbg_version = sip_probe(SPRD_SIP_SVC_DBG_ID, "DBG");

	sprd_handle.perf_set_freq = sip_perf_set_freq;
	sprd_handle.perf_get_freq = sip_perf_get_freq;
	sprd_handle.perf_set_div = sip_perf_set_div;
	sprd_handle.perf_get_div = sip_perf_get_div;
	sprd_handle.perf_set_parent = sip_perf_set_parent;
	sprd_handle.perf_get_parent = sip_perf_get_parent;

	sprd_handle.storage_ufs_crypto_enable = sip_storage_ufs_crypto_enable;
	sprd_handle.storage_ufs_crypto_disable = sip_storage_ufs_crypto_disable;

	sprd_handle.pwr_get_wakeup_source = sip_pwr_get_wakeup_source;
	sprd_handle.pwr_get_pdbg_info = sip_pwr_get_pdbg_info;
	sprd_handle.pwr_get_pdbg_info_words = sip_pwr_get_pdbg_info_words;

	sprd_handle.dvfs_ops.dvfs_enable = sip_dvfs_enable;
	sprd_handle.dvfs_ops.table_update = sip_dvfs_table_update;
	sprd_handle.dvfs_ops.step_set = sip_dvfs_step_set;
	sprd_handle.dvfs_ops.margin_set = sip_dvfs_margin_set;
	sprd_handle.dvfs_ops.freq_set = sip_dvfs_freq_set;
	sprd_handle.dvfs_ops.freq_get = sip_dvfs_freq_get;
	sprd_handle.dvfs_ops.pair_get = sip_dvfs_pair_get;
	sprd_handle.dvfs_ops.pmic_set = sip_dvfs_pmic_set;
	sprd_handle.dvfs_ops.bin_set = sip_dvfs_bin_set;
	sprd_handle.dvfs_ops.version_set = sip_dvfs_version_set;
	sprd_handle.dvfs_ops.dvfs_init = sip_dvfs_init;
	sprd_handle.dvfs_ops.dvfs_debug_init = sip_dvfs_debug_init;

	sprd_handle.npu_set_freq = sip_npu_set_freq;
	sprd_handle.npu_disable_idle = sip_npu_disable_idle;
	sprd_handle.npu_get_max_state = sip_npu_get_max_state;
	sprd_handle.npu_get_opp = sip_npu_get_opp;
	sprd_handle.npu_set_volts = sip_npu_set_volts;

	sprd_handle.dbg_set_hang_hdl = sip_dbg_set_hang_hdl;
	sprd_handle.dbg_get_hang_ctx = sip_dbg_get_hang_ctx;

	return 0;
}

static void __exit sprd_sip_svc_exit(void)
{
	memset(&sprd_handle, 0, sizeof(sprd_handle));
}

module_init(sprd_sip_svc_init);
module_exit(sprd_sip_svc_exit);

MODULE_DESCRIPTION("Spreadtrum SIP secure monitor service client");
MODULE_LICENSE("GPL v2");
