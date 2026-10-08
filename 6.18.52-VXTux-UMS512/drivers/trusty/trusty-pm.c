#include <linux/module.h>
#include <linux/init.h>
#include <linux/syscore_ops.h>
#include <linux/sched/clock.h>
#include <linux/timekeeping.h>
#include <linux/trusty/smcall.h>
#include <asm/arch_timer.h>

#include <linux/trusty/trusty.h>
/*
 * DEV-IASI 6.18 (satu-satunya di berkas ini, dan disengaja).
 *
 * Donor menyertakan <trace/hooks/psci.h> lalu mendaftarkan dua GKI vendor
 * hook:
 *     register_trace_android_vh_psci_tos_resident_on(trusty_resident_on_cpu)
 *     register_trace_android_vh_psci_cpu_suspend(trusty_check_cpu_suspend)
 * Keduanya TIDAK bisa diport: `include/trace/hooks/` tidak ada di pohon ini
 * (dan tidak ada di mainline 6.18 — sudah diverifikasi: `include/trace/`
 * hanya berisi perf/events/misc/stages), framework `DECLARE_HOOK`+
 * `trace/hooks/vendor_hooks.h` milik GKI, dan titik panggilnya ada di
 * drivers/firmware/psci/psci.c versi Android yang juga tidak ada di sini
 * (pohon ini memakai psci.c mainline: `resident_cpu` statis, diisi dari DT,
 * lihat drivers/firmware/psci/psci.c:54 `psci_tos_resident_on()').
 *
 * AKIBAT YANG JUJUR HARUS DICATAT: dua hal hilang, dan keduanya HANYA optimasi
 * daya, bukan jalur data Trusty:
 *   1. TOS tidak lagi bisa menolak CPU_OFF pada CPU tempat ia bersemayam
 *      (SMC_FC_CPU_CAN_DOWN). Yang tersisa hanyalah jawaban statis dari DT.
 *   2. Linux tidak memberi tahu TOS sebelum CPU suspend (deny flag).
 * Yang TETAP ADA dan itu bagian yang penting: sinkronisasi waktu boot Linux
 * ke TOS lewat dua fastcall, diulang setiap resume (syscore_ops) — tanpa ini
 * jam di dalam TOS melenceng setelah suspend.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "sprd-trusty-pm: " fmt

/*
 * This call synchronizes linux boot time to trusty OS by calling two SMC
 * fastcalls. One call passes arm arch timer counter to trusty OS to estimate
 * the period(T1) in which a SMC fastcall is sent from linux to trusty OS.
 * The other one passed the linux boot time(T2).
 * Assume that anytime the period of the SMC fastcall is same. The linux boot
 * time when trusty OS gets the second fastcall should be T1+T2.
 */
static int trusty_sync_boot_time(void)
{
	int ret = 0;
	u64 boot_time;
	u64 cnt;

	/*
	 * Use arm generic virtual timer counter as reference counter
	 * comparing to physical timer counter used in trusty OS.
	 * The offset between virtual timer and physical timer will be ingored.
	 */
	cnt = __arch_counter_get_cntvct();
	ret = trusty_fast_call32(NULL, SMC_FC_SYNC_TIMER_CNT,
				 (u32)cnt,  (u32)(cnt >> 32), 0);
	if (ret) {
		pr_err("Trusty fastcall SMC_FC_SYNC_TIMER_CNT failed(%d)\n",
		       ret);
		return ret;
	}

	/*
	 * Get linux boot time that includes system suspend time
	 */
	boot_time = ktime_get_boot_fast_ns();
	ret = trusty_fast_call32(NULL, SMC_FC_SYNC_BOOT_TIME,
				 (u32)boot_time, (u32)(boot_time >> 32), 0);
	if (ret) {
		pr_err("Trusty fastcall SMC_FC_SYNC_BOOT_TIME failed(%d)\n",
		       ret);
	}

	return ret;
}

static void trusty_pm_resume(void)
{
	trusty_sync_boot_time();
}

static struct syscore_ops trusty_pm_ops = {
	.resume = trusty_pm_resume,
};

static int __init trusty_pm_init(void)
{
	/* Fist time sync on boot up */
	trusty_sync_boot_time();

	register_syscore_ops(&trusty_pm_ops);

	/* Dua register_trace_android_vh_* donor dihapus di sini; alasan
	 * lengkapnya ada di blok DEV-IASI di kepala berkas. */

	return 0;
}

static void __exit trusty_pm_exit(void)
{
	unregister_syscore_ops(&trusty_pm_ops);
}

module_init(trusty_pm_init);
module_exit(trusty_pm_exit);

MODULE_DESCRIPTION("Sprd trusty pm driver");
MODULE_LICENSE("GPL v2");
