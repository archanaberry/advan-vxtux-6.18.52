/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_WAKELOCK_H
#define _LINUX_WAKELOCK_H

#include <linux/device.h>
#include <linux/pm.h>
#include <linux/pm_wakeup.h>
#include <linux/jiffies.h>

/*
 * Port note 2026-10-04 (FM driver):
 *
 * This vendor header is a thin wrapper over the wakeup source API. Only the
 * lifecycle pair moved in 6.18 -- verified against include/linux/pm_wakeup.h:
 *
 *   wakeup_source_init(&ws, name)   -> wakeup_source_register(dev, name)
 *   wakeup_source_trash(&ws)        -> wakeup_source_unregister(ws)
 *
 * Everything else is unchanged and still takes a `struct wakeup_source *`:
 * __pm_stay_awake(ws), __pm_relax(ws), __pm_wakeup_event(ws, msec), and the
 * bool ->active:1 member. So do NOT migrate this to the device-wakeup
 * framework -- dev_pm_device_wakeup_enable(), device_wakeup_wake() and
 * device_is_wakeup_enabled() do not exist in this tree at all.
 *
 * The one real consequence is that registration is now tied to a struct
 * device and returns the source, so wake_lock_init takes the device too.
 * The `type` parameter is unused: 6.18 has one wakeup source per device
 * rather than one per suspend class, and this driver only ever used
 * WAKE_LOCK_SUSPEND.
 */

enum {
	WAKE_LOCK_SUSPEND, /* Prevent suspend */
	WAKE_LOCK_TYPE_COUNT
};

struct wake_lock {
	struct wakeup_source ws;
};

static inline void wake_lock_init(struct wake_lock *lock, struct device *dev,
				  int type, const char *name)
{
	/* type is unused in 6.18; see the port note above. */
	(void)type;
	wakeup_source_register(dev, name);
}

static inline void wake_lock_destroy(struct wake_lock *lock)
{
	wakeup_source_unregister(&lock->ws);
}

static inline void wake_lock(struct wake_lock *lock)
{
	__pm_stay_awake(&lock->ws);
}

static inline void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
	__pm_wakeup_event(&lock->ws, jiffies_to_msecs(timeout));
}

static inline void wake_unlock(struct wake_lock *lock)
{
	__pm_relax(&lock->ws);
}

static inline int wake_lock_active(struct wake_lock *lock)
{
	return lock->ws.active;
}

#endif
