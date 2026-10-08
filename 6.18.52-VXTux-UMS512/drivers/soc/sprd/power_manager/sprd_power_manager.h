/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sprd_power_manager.h - API of the RE-reconstructed Unisoc modem power manager.
 *
 * Behavior model: docs/re_sprd_power_manager.md, built from llvm-objdump -dr of
 * dumped/re/hidden/vramdisk_ko/sprd_power_manager.ko (51,406 B). Consumers per
 * stock modules.dep: 23 SIPC-family modules (sipc-core, spool, spipe, sbuf,
 * sblock/slog_bridge, sipx, sensorhub, seth, ims_bridge, wlan, dvfs, ...).
 *
 * Two layers (offsets evidenced from disassembly):
 *   struct sprd_mpm - aggregate: one global wakeup source, client list,
 *                     PM notifier + debugfs stats.
 *   struct sprd_pms - per-consumer handle (0x98 = 152 B): descriptor words,
 *                     wakelock refcount + timer, membership in mpm list.
 */
#ifndef __SPRD_POWER_MANAGER_H
#define __SPRD_POWER_MANAGER_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/pm_wakeup.h>

/* Resource ids accepted by sprd_mpm_init_resource_ops(): 0..9 (cmp w0, #0x9). */
#define SPRD_MPM_RESOURCE_NUM		10

/*
 * Offsets from disassembly; lock map extracted mechanically by
 * tools/re/pm_lock_map.py (docs/re_sprd_power_manager.lockmap.txt):
 * power/resource paths lock +0x40, wakelock+timer +0x44, stay/relax +0x48.
 */
struct sprd_pms {
	u64 descr[4];			/* 0x00-0x1f: 32-B copy of the caller's name buffer */
	struct sprd_mpm *mpm;		/* 0x20: 5th copy word = mpm pointer  */
	u8 hw_flag;			/* 0x28: 0 = single path (power_*)    */
	u8 active;			/* 0x29: wakelock currently held      */
	u32 stay_cnt;			/* 0x2c: stay_awake call counter      */
	u32 power_refcnt;		/* 0x34: shared power/resource refcnt */
	u64 timer_expires;		/* 0x38: wakelock timer expiry        */
	spinlock_t lock_res;		/* 0x40: guards power/resource path   */
	spinlock_t lock_wl;		/* 0x44: guards wakelock + timer      */
	spinlock_t lock_awake;		/* 0x48: guards active flag (stay/relax) */
	struct list_head node;		/* 0x50: on sprd_mpm.clients          */
	struct timer_list wakelock_timer; /* 0x60: fn = relax_wakelock_timer */
	/* 0x88..0x97: padding to 152 B total */
};

/*
 * Ops triple stored INSIDE each mpm (evidence: request_resource reads
 * [mpm+0x1e0] after x22 = [pms+0x20] = mpm; power paths use +0x1e8/+0x1f0).
 * Registered via sprd_mpm_init_resource_ops(id, ...) onto registry[id].
 * CFI typeid for the dispatch is Fij (TBC: exact arity - the blob calls the
 * request fn with w0 = mpm sub-id and w1 = resource id).
 */
struct sprd_mpm_resource_ops {
	int (*request)(int sub_id, int res_id);	/* +0x1e0 */
	int (*power_up)(int sub_id);		/* +0x1e8 */
	int (*power_down)(int sub_id);		/* +0x1f0 */
};

/* Aggregate layer: created once by sprd_mpm_create(name). */
struct sprd_mpm {
	struct timer_list deactive_timer;	/* 0x18: armed by the down path  */
	struct timer_list print_timer;	/* 0x50: awake-stats printer        */
	struct wakeup_source *ws;	/* 0x00: wakeup_source_create(name)  */
	int ops_sub_id;			/* 0xb8: int fed as ops arg-1        */
	u64 last_up_module;		/* 0xb0: stats (TBC source register) */
	u32 up_cnt;			/* 0xbc: live power_up users         */
	u32 busy;			/* 0xc8: 1 = busy                    */
	u64 deactive_expires;		/* 0xd0: timer expiry when armed     */
	u32 deactive_delay_ms;		/* 0xd8: arg-3 of mpm_create         */
	u32 later_idle_saved;		/* 0xdc: delay saved by disable_*    */
	char name[20];			/* 0x8c: ddebug prints mpm+0x8c      */
	spinlock_t lock;		/* 0x88: guards clients + counters   */
	struct list_head clients;	/* 0x90: list of sprd_pms            */
	u32 wake_events;		/* 0xc0: ++ on each awake 0->1       */
	u32 awake_cnt;			/* 0xc4: live awake clients          */
	struct sprd_mpm_resource_ops ops;	/* 0x1e0/0x1e8/0x1f0     */
};

/* ---- pms API (per consumer; all NULL-ptr checked, -EINVAL on bad arg) ---- */
struct sprd_pms *sprd_pms_create(u8 id, const char *name, u32 flag);
void sprd_pms_destroy(struct sprd_pms *pms);

int sprd_pms_request_wakelock(struct sprd_pms *pms);
int sprd_pms_release_wakelock(struct sprd_pms *pms);
int sprd_pms_request_wakelock_period(struct sprd_pms *pms, u8 flag);
int sprd_pms_release_wakelock_later(struct sprd_pms *pms, u64 delta_jiffies);

void sprd_pms_stay_awake(struct sprd_pms *pms);
void sprd_pms_relax(struct sprd_pms *pms);
void sprd_pms_relax_wakelock_timer(struct timer_list *t);

int sprd_pms_power_up(struct sprd_pms *pms);
int sprd_pms_power_down(struct sprd_pms *pms);

int sprd_pms_request_resource(struct sprd_pms *pms, int res_id);
int sprd_pms_release_resource(struct sprd_pms *pms, int res_id);

/* ---- mpm API (aggregate; id > 9 -> -EINVAL, empty slot -> -ENODEV) ----
 *
 * Signatures LOCKED from consumer call sites (sipc-core.ko smsg_ipc_create
 * @0x34ec, smsg_ch_open @0x2f28/0x2f6c: w0 = u8 id, x1 = name, w2 = u32).
 * mpm_create registers at registry[id] (its third arg = [smsg_ipc+0xc4],
 * the deactive delay feeding mpm+0xd8); pms_create resolves its mpm via
 * registry[id] and stores flag to pms+0x28 (strb w8,[x19,#0x28]).
 */
struct sprd_mpm *sprd_mpm_create(u8 id, const char *name,
				 u32 deactive_delay_ms);
void sprd_mpm_destroy(struct sprd_mpm *mpm);
struct sprd_mpm *sprd_mpm_get(void);

void sprd_mpm_up(struct sprd_mpm *mpm, void *cookie);
void sprd_mpm_down(struct sprd_mpm *mpm, int flag);
/*
 * Registry: static .bss array of mpm pointers indexed by id (init_resource_ops
 * loads .bss[w0 uxtw #3] directly; up to 10 mpm instances supported).
 */
int sprd_mpm_init_resource_ops(int id,
			       int (*request)(int, int),
			       int (*power_up)(int), int (*power_down)(int));

/* PM-notifier helper. LOCKED from blob 0xefc: int(id) - id > 9 -> -EINVAL,
 * empty registry slot -> -ENODEV; saves the old later_idle delay into
 * mpm+0xdc and clears mpm+0xd8 via sprd_mpm_set_later_idle(). */
int sprd_mpm_disable_later_idle_for_sleep(int id);
u32 sprd_mpm_set_later_idle(struct sprd_mpm *mpm);

#endif /* __SPRD_POWER_MANAGER_H */
