// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_power_manager.c - RE reconstruction of the Unisoc modem power manager.
 *
 * Behavior model: docs/re_sprd_power_manager.md (llvm-objdump -dr of
 * dumped/re/hidden/vramdisk_ko/sprd_power_manager.ko, 51,406 B). UND imports
 * are only wakeup-source/timer/spinlock/debugfs/PM-notifier APIs: this is a
 * software wakelock + resource-arbitration layer, 23 SIPC-family stock
 * modules depend on it, no source exists in any vendor fork.
 *
 * Every function below cites the blob address it models. Unprovable details
 * are marked TBC and listed in the doc.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/errno.h>

#include "sprd_power_manager.h"

/*
 * Static registry, mirrors the blob: sprd_mpm_init_resource_ops loads
 * .bss[w0 uxtw #3] directly (max id 9), and request_resource reuses that slot
 * as the mpm pointer - so the registry stores mpm instances, and
 * init_resource_ops wires the ops triple INTO the registered mpm.
 * Consumer evidence (sipc-core.ko smsg_ipc_create @0x34ec): the mpm id comes
 * from [smsg_ipc+0x10] (checked < 10) - create writes registry[id] itself.
 */
static struct sprd_mpm *g_mpm_reg[SPRD_MPM_RESOURCE_NUM];

/* Timer callbacks defined below; mpm_create wires them at init. */
static void sprd_mpm_deactive_timer_fn(struct timer_list *t);
static void sprd_mpm_print_timer_fn(struct timer_list *t);

static struct sprd_mpm *sprd_mpm_lookup(int id)
{
	if (id < 0 || id >= SPRD_MPM_RESOURCE_NUM)
		return NULL;
	return g_mpm_reg[id];
}

/* ---- mpm lifecycle ------------------------------------------------------ */

/*
 * Blob 0x11c0 + consumer call site (sipc-core.ko smsg_ipc_create @0x34ec:
 * w0 = id from [smsg_ipc+0x10], x1 = name ptr, w2 = [smsg_ipc+0xc4] u32).
 * wakeup_source_create(name) -> [mpm+0x0]; wakeup_source_add; the u32 arg
 * feeds mpm+0xd8 (deactive delay, consumed by the down path's mod_timer);
 * name is copied to mpm+0x8c (ddebug/printk print that address).
 */
struct sprd_mpm *sprd_mpm_create(u8 id, const char *name,
				 u32 deactive_delay_ms)
{
	struct sprd_mpm *mpm;

	if (id >= SPRD_MPM_RESOURCE_NUM)
		return NULL;

	mpm = kzalloc(sizeof(*mpm), GFP_KERNEL);
	if (!mpm)
		return NULL;

	/* 5.4 blob used wakeup_source_create()+_add(); 6.18 has only the
	 * register()/unregister() pair (create/add/remove/destroy removed). */
	mpm->ws = wakeup_source_register(NULL, name);
	if (!mpm->ws) {
		kfree(mpm);
		return NULL;
	}

	strscpy(mpm->name, name, sizeof(mpm->name));
	mpm->deactive_delay_ms = deactive_delay_ms;
	spin_lock_init(&mpm->lock);
	INIT_LIST_HEAD(&mpm->clients);
	timer_setup(&mpm->deactive_timer, sprd_mpm_deactive_timer_fn, 0);
	timer_setup(&mpm->print_timer, sprd_mpm_print_timer_fn, 0);
	g_mpm_reg[id] = mpm;
	return mpm;
}

/* Blob 0x10d4 (skeleton; list teardown order TBC). */
void sprd_mpm_destroy(struct sprd_mpm *mpm)
{
	int i;

	if (!mpm)
		return;
	for (i = 0; i < SPRD_MPM_RESOURCE_NUM; i++) {
		if (g_mpm_reg[i] == mpm)
			g_mpm_reg[i] = NULL;
	}
	timer_delete_sync(&mpm->deactive_timer);
	timer_delete_sync(&mpm->print_timer);
	wakeup_source_unregister(mpm->ws);
	kfree(mpm);
}

struct sprd_mpm *sprd_mpm_get(void)
{
	/* Blob: load .bss slot 0, ret if non-NULL (TBC: single-instance read). */
	return g_mpm_reg[0];
}

/* ---- mpm up/down: the CFI-dispatched ops layer -------------------------- */

/*
 * Blob 0x470: lock mpm+0x88; up_cnt(+0xbc)++; busy(+0xc8) = 1; if
 * ops.power_up (+0x1e8): ret = power_up([mpm+0xb8]) (CFI typeid Fij); on
 * non-zero return printk("%s up, %d", mpm+0x8c, ret). The pms cookie passed
 * by callers is stored for stats (mpm+0xb0) but not forwarded to the op.
 */
void sprd_mpm_up(struct sprd_mpm *mpm, void *cookie)
{
	unsigned long flags;
	int ret = 0;

	if (!mpm)
		return;

	spin_lock_irqsave(&mpm->lock, flags);
	mpm->up_cnt++;
	mpm->busy = 1;
	mpm->last_up_module = (u64)(unsigned long)cookie;
	if (mpm->ops.power_up)
		ret = mpm->ops.power_up(mpm->ops_sub_id);
	spin_unlock_irqrestore(&mpm->lock, flags);

	if (ret)
		pr_warn("sprd_mpm: %s up, %d\n", mpm->name, ret);
}

/* Timer fn blob 0x12e0 (container_of mpm+0x18): busy = 0, expires = 0,
 * dispatch ops.power_down([mpm+0xb8]); printk "%s down, %d" on failure. */
static void sprd_mpm_deactive_timer_fn(struct timer_list *t)
{
	struct sprd_mpm *mpm = timer_container_of(mpm, t, deactive_timer);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&mpm->lock, flags);
	if (mpm->deactive_expires != 0) {
		mpm->busy = 0;
		mpm->deactive_expires = 0;
		if (mpm->ops.power_down)
			ret = mpm->ops.power_down(mpm->ops_sub_id);
	}
	spin_unlock_irqrestore(&mpm->lock, flags);	if (ret)
		pr_warn("sprd_mpm: %s down, %d\n", mpm->name, ret);
}

/*
 * Blob 0x5dc: lock mpm+0x88; up_cnt(+0xbc)--; if deactive_delay(+0xd8) != 0:
 * arm mod_timer(mpm+0x18, jiffies + msecs_to_jiffies(delay)) and record
 * expires in +0xd0 (busy stays 1 until the timer fires); else: busy = 0,
 * expires = 0, immediate ops.power_down dispatch. Callers' flag arg is not
 * used by the blob body.
 */
void sprd_mpm_down(struct sprd_mpm *mpm, int flag)
{
	unsigned long flags;

	if (!mpm)
		return;

	spin_lock_irqsave(&mpm->lock, flags);
	if (mpm->up_cnt > 0)
		mpm->up_cnt--;
	if (mpm->deactive_delay_ms != 0) {
		unsigned long j = msecs_to_jiffies(mpm->deactive_delay_ms);

		mod_timer(&mpm->deactive_timer, jiffies + j);
		mpm->deactive_expires = jiffies + j;
	} else {
		mpm->busy = 0;
		mpm->deactive_expires = 0;
		if (mpm->ops.power_down)
			mpm->ops.power_down(mpm->ops_sub_id);
	}
	spin_unlock_irqrestore(&mpm->lock, flags);
}

/* Blob 0xec4: id > 9 -> -EINVAL; registry slot empty -> -ENODEV. */
int sprd_mpm_init_resource_ops(int id,
			       int (*request)(int, int),
			       int (*power_up)(int), int (*power_down)(int))
{
	struct sprd_mpm *mpm;

	if (id < 0 || id > 9)
		return -EINVAL;
	mpm = g_mpm_reg[id];
	if (!mpm)
		return -ENODEV;

	mpm->ops.request = request;
	mpm->ops.power_up = power_up;
	mpm->ops.power_down = power_down;
	return 0;
}

/*
 * Blob 0xf68: returns the old [mpm+0xd8] and clears it (printk "%s set
 * later_idle = %d" with the old value); if up_cnt(+0xbc)==0 and busy(+0xc8)
 * and the deactive timer (+0x18 / expires +0xd0) is armed, cancel it,
 * clear busy, and dispatch ops.power_down([mpm+0xb8]) when present.
 */
u32 sprd_mpm_set_later_idle(struct sprd_mpm *mpm)
{
	unsigned long flags;
	u32 old;
	int ret = 0;

	if (!mpm)
		return 0;

	spin_lock_irqsave(&mpm->lock, flags);
	old = mpm->deactive_delay_ms;
	mpm->deactive_delay_ms = 0;
	pr_info("sprd_mpm: %s set later_idle = %u\n", mpm->name, old);
	if (mpm->up_cnt == 0 && mpm->busy) {
		if (mpm->deactive_expires != 0) {
			timer_delete(&mpm->deactive_timer);
			mpm->deactive_expires = 0;
		}
		mpm->busy = 0;
		if (mpm->ops.power_down)
			ret = mpm->ops.power_down(mpm->ops_sub_id);
	}
	spin_unlock_irqrestore(&mpm->lock, flags);
	if (ret)
		pr_warn("sprd_mpm: %s down, %d\n", mpm->name, ret);
	return old;
}

/*
 * Blob 0x144c: walk mpm->clients under mpm->lock; collect the name copy
 * (pms+0x00, the 32-B caller-name copy) of every ACTIVE client via
 * snprintf/strlen into a stack buffer; after unlock printk the list plus
 * awake_cnt(+0xc4). Reading pms->active under the mpm lock only mirrors the
 * blob (stock is equally racy vs stay/relax's pms lock).
 */
static void sprd_mpm_print_awake(struct sprd_mpm *mpm)
{
	char buf[512];
	struct sprd_pms *pms;
	unsigned long flags;
	size_t len = 0;
	u32 awake;

	if (!mpm)
		return;

	buf[0] = '\0';
	spin_lock_irqsave(&mpm->lock, flags);
	list_for_each_entry(pms, &mpm->clients, node) {
		if (pms->active && len < sizeof(buf) - 1) {
			int n = snprintf(buf + len, sizeof(buf) - len,
					 "%s ", (char *)pms->descr);

			if (n > 0)
				len += (size_t)n;
		}
	}
	awake = mpm->awake_cnt;
	spin_unlock_irqrestore(&mpm->lock, flags);

	pr_info("sprd_mpm: %s awake list: %s, awake_cnt=%u\n",
		mpm->name, buf, awake);
}

/* Timer fn blob 0x1404 (container_of mpm+0x50): print, then re-arm +15000. */
static void sprd_mpm_print_timer_fn(struct timer_list *t)
{
	struct sprd_mpm *mpm = timer_container_of(mpm, t, print_timer);

	sprd_mpm_print_awake(mpm);
	mod_timer(&mpm->print_timer, jiffies + 15000);
}

/*
 * Blob 0xefc: id > 9 -> -EINVAL; registry slot empty -> -ENODEV; if
 * [mpm+0xdc] already holds a saved delay -> 0; else set_later_idle(mpm) and
 * store its return (the old delay) into mpm+0xdc.
 */
int sprd_mpm_disable_later_idle_for_sleep(int id)
{
	struct sprd_mpm *mpm;

	if (id < 0 || id > 9)
		return -EINVAL;
	mpm = g_mpm_reg[id];
	if (!mpm)
		return -ENODEV;
	if (mpm->later_idle_saved)
		return 0;
	mpm->later_idle_saved = sprd_mpm_set_later_idle(mpm);
	return 0;
}

/* ---- pms lifecycle ------------------------------------------------------ */

/*
 * Blob 0xd20 + consumer call sites (sipc-core.ko smsg_ch_open @0x2f28 and
 * smsg_ipc_create @0x34fc: w0 = id, x1 = name ptr, w2 = flag; second site
 * passes flag = 1). The mpm is resolved from registry[id] inside the blob
 * (no mpm pointer argument), and the flag is stored to pms+0x28
 * (strb w8,[x19,#0x28] in the create body). The 32-B caller descriptor copy
 * and mpm write to +0x20 happen as before; the name itself is not stored in
 * the 152-B pms (ddebug prints mpm+0x8c / caller-side names).
 */
struct sprd_pms *sprd_pms_create(u8 id, const char *name, u32 flag)
{
	struct sprd_mpm *mpm = sprd_mpm_lookup(id);
	struct sprd_pms *pms;
	unsigned long flags;

	if (!mpm || !name)
		return NULL;

	pms = kzalloc(sizeof(*pms), GFP_KERNEL);
	if (!pms)
		return NULL;

	/* Blob: 32-B copy FROM the name buffer (ldp x8,x9,[x21]; ldp x10,x11,
	 * [x21,#0x10] with x21 = arg x1) to pms+0x00..0x1f. */
	memcpy(pms->descr, name, sizeof(pms->descr));
	pms->mpm = mpm;
	pms->hw_flag = (u8)flag;
	spin_lock_init(&pms->lock_res);
	spin_lock_init(&pms->lock_wl);
	spin_lock_init(&pms->lock_awake);
	timer_setup(&pms->wakelock_timer, sprd_pms_relax_wakelock_timer, 0);

	spin_lock_irqsave(&mpm->lock, flags);
	list_add_tail(&pms->node, &mpm->clients);
	spin_unlock_irqrestore(&mpm->lock, flags);

	return pms;
}

/* Blob 0xc34: del_timer, list_del(+0x50), kfree. */
void sprd_pms_destroy(struct sprd_pms *pms)
{
	unsigned long flags;

	if (!pms)
		return;

	timer_delete_sync(&pms->wakelock_timer);
	spin_lock_irqsave(&pms->mpm->lock, flags);
	list_del(&pms->node);
	spin_unlock_irqrestore(&pms->mpm->lock, flags);
	kfree(pms);
}

/* ---- wakelock (pms 0x29 flag / 0x2c counter / mpm 0xc0,0xc4) ------------ */

/*
 * Blob 0xcc: lock pms+0x48 (lockmap); stay_cnt(+0x2c)++; if flag(+0x29) was 0
 * -> set 1; unlock; then mpm block: lock mpm+0x88, awake_cnt(+0xc4)++, and on
 * its own 0->1: wake_events (+0xc0)++ and __pm_stay_awake(ws).
 */
void sprd_pms_stay_awake(struct sprd_pms *pms)
{
	struct sprd_mpm *mpm;
	unsigned long flags;
	bool became_active;

	if (!pms)
		return;
	mpm = pms->mpm;
	if (!mpm)
		return;

	spin_lock_irqsave(&pms->lock_awake, flags);
	pms->stay_cnt++;
	became_active = (pms->active == 0);
	if (became_active)
		pms->active = 1;
	spin_unlock_irqrestore(&pms->lock_awake, flags);

	if (!became_active)
		return;

	spin_lock_irqsave(&mpm->lock, flags);
	if (mpm->awake_cnt++ == 0) {
		mpm->wake_events++;
		__pm_stay_awake(mpm->ws);
	}
	spin_unlock_irqrestore(&mpm->lock, flags);
}

/*
 * Blob 0x894: if flag(+0x29) == 0 -> nothing. Else flag = 0; lock mpm:
 * awake_cnt(--), __pm_relax(ws) only when the post-decrement hits 0.
 */
void sprd_pms_relax(struct sprd_pms *pms)
{
	struct sprd_mpm *mpm;
	unsigned long flags;

	if (!pms)
		return;
	mpm = pms->mpm;
	if (!mpm)
		return;

	spin_lock_irqsave(&pms->lock_awake, flags);
	if (!pms->active) {
		spin_unlock_irqrestore(&pms->lock_awake, flags);
		return;
	}
	pms->active = 0;
	spin_unlock_irqrestore(&pms->lock_awake, flags);

	spin_lock_irqsave(&mpm->lock, flags);
	if (mpm->awake_cnt > 0 && --mpm->awake_cnt == 0)
		__pm_relax(mpm->ws);
	spin_unlock_irqrestore(&mpm->lock, flags);
}

/*
 * Timer callback (blob 0xe38, CFI thunk .text+0x2368 -> 0xe38 confirms it is
 * the timer fn): pms = container_of(timer, +0x60); lock pms+0x44; if
 * expires(+0x38) set and !time_before(jiffies, expires) -> expires = 0,
 * unlock, sprd_pms_relax(pms).
 */
void sprd_pms_relax_wakelock_timer(struct timer_list *t)
{
	struct sprd_pms *pms = timer_container_of(pms, t, wakelock_timer);
	unsigned long flags;
	bool do_relax = false;

	spin_lock_irqsave(&pms->lock_wl, flags);
	if (pms->timer_expires != 0 &&
	    !time_before(jiffies, pms->timer_expires)) {
		pms->timer_expires = 0;
		do_relax = true;
	}
	spin_unlock_irqrestore(&pms->lock_wl, flags);

	if (do_relax)
		sprd_pms_relax(pms);
}

/*
 * Blob 0x1c0: NULL -> -EINVAL; lock pms+0x44; if expires set -> del_timer +
 * expires = 0; unlock; tail-call stay_awake. (del_timer under spinlock is
 * IRQ-safe; order mirrored from the blob.)
 */
int sprd_pms_request_wakelock(struct sprd_pms *pms)
{
	unsigned long flags;

	if (!pms)
		return -EINVAL;

	spin_lock_irqsave(&pms->lock_wl, flags);
	if (pms->timer_expires != 0) {
		timer_delete(&pms->wakelock_timer);
		pms->timer_expires = 0;
	}
	spin_unlock_irqrestore(&pms->lock_wl, flags);

	sprd_pms_stay_awake(pms);
	return 0;
}

/* Blob 0x7fc: same guard, tail-call relax. */
int sprd_pms_release_wakelock(struct sprd_pms *pms)
{
	unsigned long flags;

	if (!pms)
		return -EINVAL;

	spin_lock_irqsave(&pms->lock_wl, flags);
	if (pms->timer_expires != 0) {
		timer_delete(&pms->wakelock_timer);
		pms->timer_expires = 0;
	}
	spin_unlock_irqrestore(&pms->lock_wl, flags);

	sprd_pms_relax(pms);
	return 0;
}

/*
 * Blob 0x633 (release_wakelock_later): if flag active ->
 * mod_timer(pms+0x60, jiffies + delta). Inactive path return value TBC.
 */
int sprd_pms_release_wakelock_later(struct sprd_pms *pms, u64 delta_jiffies)
{
	if (!pms)
		return -EINVAL;

	if (pms->active)
		mod_timer(&pms->wakelock_timer,
			  jiffies + (unsigned long)delta_jiffies);
	return 0;
}

/*
 * Blob 0x30: flag != 0 -> store flag to +0x29 (byte), tail-call stay_awake;
 * flag == 0 -> tail-call release_wakelock (+0x7fc) without touching +0x29.
 */
int sprd_pms_request_wakelock_period(struct sprd_pms *pms, u8 flag)
{
	if (!pms)
		return -EINVAL;

	if (flag != 0) {
		pms->active = flag;
		sprd_pms_stay_awake(pms);
	} else {
		sprd_pms_release_wakelock(pms);
	}
	return 0;
}

/* ---- power gate (hw_flag 0x28; shared refcnt 0x34 under lock_res 0x40) -- */

/* Blob 0xa5c: single path (flag 0) -> mpm_up(mpm, pms); multi -> refcnt
 * (+0x34)++ under lock +0x40, mpm_up on 0->1. */
int sprd_pms_power_up(struct sprd_pms *pms)
{
	struct sprd_mpm *mpm;
	unsigned long flags;
	u32 prev;

	if (!pms)
		return -EINVAL;
	mpm = pms->mpm;
	if (!mpm)
		return -ENODEV;

	if (pms->hw_flag == 0) {
		sprd_mpm_up(mpm, pms);
		return 0;
	}

	spin_lock_irqsave(&pms->lock_res, flags);
	prev = pms->power_refcnt++;
	spin_unlock_irqrestore(&pms->lock_res, flags);
	if (prev == 0)
		sprd_mpm_up(mpm, pms);
	return 0;
}

/* Blob 0xb38: mirror; mpm_down on 1->0. */
int sprd_pms_power_down(struct sprd_pms *pms)
{
	struct sprd_mpm *mpm;
	unsigned long flags;
	u32 now;

	if (!pms)
		return -EINVAL;
	mpm = pms->mpm;
	if (!mpm)
		return -ENODEV;

	if (pms->hw_flag == 0) {
		sprd_mpm_down(mpm, 0);
		return 0;
	}

	spin_lock_irqsave(&pms->lock_res, flags);
	now = --pms->power_refcnt;
	spin_unlock_irqrestore(&pms->lock_res, flags);
	if (now == 0)
		sprd_mpm_down(mpm, 0);
	return 0;
}

/* ---- resource requests (ops->request dispatch) -------------------------- */

/*
 * Blob 0x258: two-arg (pms, res_id). Refcount +0x34 under lock +0x40 (multi
 * path) with mpm_up at 0->1; then if ops.request (+0x1e0) exists: call with
 * w0 = [mpm+0xb8], w1 = res_id. Error unwind: on ret < 0 && ret !=
 * -ERESTARTSYS, decrement +0x34 and mpm_down at 1->0 (blob 0x340-0x384).
 * Blob tail 0x3ec-0x3d0 not fully modeled (TBC).
 */
int sprd_pms_request_resource(struct sprd_pms *pms, int res_id)
{
	struct sprd_mpm *mpm;
	unsigned long flags;
	u32 prev;
	int ret;

	if (!pms)
		return -EINVAL;
	mpm = pms->mpm;
	if (!mpm)
		return -ENODEV;

	if (pms->hw_flag != 0) {
		spin_lock_irqsave(&pms->lock_res, flags);
		prev = pms->power_refcnt++;
		spin_unlock_irqrestore(&pms->lock_res, flags);
		if (prev == 0)
			sprd_mpm_up(mpm, pms);
	} else {
		pms->power_refcnt++;
	}

	if (!mpm->ops.request)
		return 0;

	ret = mpm->ops.request(mpm->ops_sub_id, res_id);
	if (ret < 0 && ret != -ERESTARTSYS) {
		/* Blob 0x340-0x384: unwind the refcnt taken above. */
		spin_lock_irqsave(&pms->lock_res, flags);
		if (pms->hw_flag != 0) {
			if (--pms->power_refcnt == 0)
				sprd_mpm_down(mpm, 0);
		} else {
			pms->power_refcnt--;
		}
		spin_unlock_irqrestore(&pms->lock_res, flags);
	}
	return ret;
}

/*
 * Blob 0x96c: res_id is NOT used (no ops dispatch in the observed body);
 * decrement +0x34, mpm_down(mpm, 0) at 1->0.
 */
int sprd_pms_release_resource(struct sprd_pms *pms, int res_id)
{
	struct sprd_mpm *mpm;
	unsigned long flags;
	u32 now;

	if (!pms)
		return -EINVAL;
	mpm = pms->mpm;
	if (!mpm)
		return -ENODEV;

	spin_lock_irqsave(&pms->lock_res, flags);
	now = --pms->power_refcnt;
	spin_unlock_irqrestore(&pms->lock_res, flags);
	if (now == 0)
		sprd_mpm_down(mpm, 0);
	return 0;
}

/* ---- PM notifier + debugfs ---------------------------------------------- */

/*
 * Blob 0x15ac - exact format strings extracted from .rodata
 * (dumped/re/pm_rodata.hex, TBC-5 resolved):
 * sep 0xa6, "All mpm list:" 0x6c, "mpm = %s info:" 0x227,
 * "  %s: active_cnt=%d, awake=%d" 0x208, "later = %d, old_later =%d" 0x67a,
 * "last up module = %s info:" 0x2e6, "left %dms to idle" 0x279,
 * "active pms list:" 0x5a9.
 */
static int sprd_mpm_stats_show(struct seq_file *m, void *v)
{
	struct sprd_mpm *mpm;
	int i;

	seq_puts(m, "------------------------------------\n");
	seq_puts(m, "All mpm list:\n");
	for (i = 0; i < SPRD_MPM_RESOURCE_NUM; i++) {
		struct sprd_pms *pms;
		unsigned long flags;
		unsigned long left;

		mpm = g_mpm_reg[i];
		if (!mpm)
			continue;
		seq_printf(m, "mpm = %s info:\n", mpm->name);
		spin_lock_irqsave(&mpm->lock, flags);
		seq_printf(m, "  %s: active_cnt=%d, awake=%d\n",
			   mpm->name, mpm->up_cnt, mpm->awake_cnt);
		seq_printf(m, "later = %d, old_later =%d\n",
			   mpm->deactive_delay_ms, mpm->later_idle_saved);
		seq_printf(m, "last up module = %s info:\n",
			   mpm->last_up_module ?
			   (char *)(unsigned long)mpm->last_up_module : "null");
		left = mpm->deactive_expires ?
		       mpm->deactive_expires - jiffies : 0;
		spin_unlock_irqrestore(&mpm->lock, flags);
		if (left)
			seq_printf(m, "left %dms to idle\n",
				   (int)jiffies_to_msecs(left));
		seq_puts(m, "active pms list:\n");
		spin_lock_irqsave(&mpm->lock, flags);
		list_for_each_entry(pms, &mpm->clients, node)
			if (pms->active)
				seq_printf(m, "  %s: active_cnt=%d, awake=%d\n",
					   (char *)pms->descr, pms->stay_cnt,
					   pms->active);
		spin_unlock_irqrestore(&mpm->lock, flags);
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(sprd_mpm_stats);

/*
 * Blob 0x1784, LOCKED: event == 3 (PM_SUSPEND_PREPARE) -> del_timer(mpm+
 * 0x50) for every registered mpm (registry .bss[0..9]); event == 4
 * (PM_POST_SUSPEND) -> print_awake(mpm) + mod_timer(mpm+0x50, jiffies +
 * 15000) per mpm. Other events ignored. The blob unrolls the first four
 * slots for event 3; the loop form is equivalent for all 10.
 */
static int sprd_mpm_pm_event(struct notifier_block *nb, unsigned long event,
			     void *ptr)
{
	int i;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		for (i = 0; i < SPRD_MPM_RESOURCE_NUM; i++) {
			struct sprd_mpm *mpm = g_mpm_reg[i];				if (mpm)
					timer_delete(&mpm->print_timer);
		}
		break;
	case PM_POST_SUSPEND:
		for (i = 0; i < SPRD_MPM_RESOURCE_NUM; i++) {
			struct sprd_mpm *mpm = g_mpm_reg[i];

			if (mpm) {
				sprd_mpm_print_awake(mpm);
				mod_timer(&mpm->print_timer, jiffies + 15000);
			}
		}
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block sprd_mpm_pm_nb = {
	.notifier_call = sprd_mpm_pm_event,
};

/* ---- module (blob init_module: pm notifier + debugfs dir + 0444 file) --- */

static int __init sprd_power_manager_init(void)
{
	struct dentry *dir;

	register_pm_notifier(&sprd_mpm_pm_nb);

	dir = debugfs_create_dir("mpm", NULL);
	if (!IS_ERR(dir))
		debugfs_create_file("power_manage", 0444, dir, NULL,
				    &sprd_mpm_stats_fops);

	return 0;
}

static void __exit sprd_power_manager_exit(void)
{
	unregister_pm_notifier(&sprd_mpm_pm_nb);
}

module_init(sprd_power_manager_init);
module_exit(sprd_power_manager_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Unisoc modem power manager (RE reconstruction)");

/* Exported ABI: the 23 stock SIPC-family consumers call these. */
EXPORT_SYMBOL_GPL(sprd_mpm_create);
EXPORT_SYMBOL_GPL(sprd_mpm_destroy);
EXPORT_SYMBOL_GPL(sprd_mpm_get);
EXPORT_SYMBOL_GPL(sprd_mpm_up);
EXPORT_SYMBOL_GPL(sprd_mpm_down);
EXPORT_SYMBOL_GPL(sprd_mpm_init_resource_ops);
EXPORT_SYMBOL_GPL(sprd_mpm_disable_later_idle_for_sleep);
EXPORT_SYMBOL_GPL(sprd_pms_create);
EXPORT_SYMBOL_GPL(sprd_pms_destroy);
EXPORT_SYMBOL_GPL(sprd_pms_request_wakelock);
EXPORT_SYMBOL_GPL(sprd_pms_release_wakelock);
EXPORT_SYMBOL_GPL(sprd_pms_request_wakelock_period);
EXPORT_SYMBOL_GPL(sprd_pms_release_wakelock_later);
EXPORT_SYMBOL_GPL(sprd_pms_stay_awake);
EXPORT_SYMBOL_GPL(sprd_pms_relax);
EXPORT_SYMBOL_GPL(sprd_pms_relax_wakelock_timer);
EXPORT_SYMBOL_GPL(sprd_pms_power_up);
EXPORT_SYMBOL_GPL(sprd_pms_power_down);
EXPORT_SYMBOL_GPL(sprd_pms_request_resource);
EXPORT_SYMBOL_GPL(sprd_pms_release_resource);
