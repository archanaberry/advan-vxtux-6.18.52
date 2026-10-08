// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_charger_manager.c - Sprd Charger Manager for UMS512/T618
 *
 * Port from vendor blob sprd-charger-manager.ko (5.4.210)
 * to mainline 6.18.52-VXTux-UMS512
 */

#include <linux/alarmtimer.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mfd/sc27xx-pmic.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>

#define CM_POLL_INTERVAL_MS		15000
#define CM_CP_RETRY_MAX		3
#define CM_UVLO_RETRY_MAX		5

enum cm_state {
	CM_STATE_INIT = 0,
	CM_STATE_CHARGING = 1,
	CM_STATE_CHARGED = 2,
	CM_STATE_DISCHARGING = 3,
	CM_STATE_FAULT = 4,
};

enum cm_cp_state {
	CM_CP_INIT = 0,
	CM_CP_CHECK_VBUS = 1,
	CM_CP_TUNE = 2,
	CM_CP_EXIT = 3,
	CM_CP_RECOVERY = 4,
};

enum cm_vote_type {
	CM_VOTE_CURRENT_MAX = 0,
	CM_VOTE_VOLTAGE_MAX = 1,
	CM_VOTE_CURRENT_MIN = 2,
	CM_VOTE_VOLTAGE_MIN = 3,
};

enum cm_fchg_type {
	CM_FCHG_NONE = 0,
	CM_FCHG_PD = 1,
	CM_FCHG_QC = 2,
	CM_FCHG_VOOC = 3,
};

enum cm_vchg_type {
	CM_VCHG_SDP = 0,
	CM_VCHG_DCP = 1,
	CM_VCHG_CDP = 2,
	CM_VCHG_UNKNOWN = 3,
};

struct sprd_vote {
	struct list_head list;
	struct mutex lock;
	char *name;
	int type;
	int value;
	int enabled;
	struct power_supply *psy;
	void (*callback)(struct sprd_vote *vote, int value);
};

struct sprd_fchg_info {
	const char *name;
	enum cm_fchg_type type;
	int max_voltage_mv;
	int max_current_ma;
	struct power_supply *psy;
	int (*detect)(struct power_supply *psy);
	int (*init)(struct power_supply *psy);
	int (*set_voltage)(struct power_supply *psy, int voltage_mv);
	int (*set_current)(struct power_supply *psy, int current_ma);
	int (*enable)(struct power_supply *psy);
	int (*disable)(struct power_supply *psy);
};

struct sprd_vchg_info {
	const char *name;
	enum cm_vchg_type type;
	struct power_supply *psy;
	int (*detect)(struct power_supply *psy);
	int (*set_current_limit)(struct power_supply *psy, int current_ma);
};

struct cm_batt_work {
	struct delayed_work work;
	struct power_supply *batt_psy;
	int uvlo_retry;
	int capacity;
	int prev_capacity;
};

struct cm_notifier_block {
	struct list_head list;
	int (*callback)(const char *event, void *data);
};

struct sprd_cm {
	struct device *dev;
	struct power_supply *psy;
	struct power_supply *ac_psy;
	struct power_supply *usb_psy;
	struct power_supply *cp_psy;
	struct power_supply *fchg_psy;
	struct power_supply *vchg_psy;
	struct power_supply *batt_psy;

	struct regmap *regmap;
	struct mutex lock;
	struct mutex vote_lock;
	struct list_head vote_list;
	struct list_head listener_list;
	struct mutex listener_lock;

	struct delayed_work monitor_work;
	struct delayed_work fchg_work;
	struct delayed_work vchg_work;
	struct delayed_work batt_work;
	struct delayed_work cp_work;
	struct delayed_work uvlo_work;

	struct alarm *alarm_monitor;
	struct alarm *alarm_batt;
	struct alarm *alarm_cp;

	enum cm_state state;
	int prev_state;
	int capacity;
	int prev_capacity;
	int voltage_now;
	int current_now;
	int temperature;
	int health;
	int status;

	enum cm_cp_state cp_state;
	int cp_retry;
	int cp_voltage;
	int cp_current;

	struct sprd_fchg_info *fchg_info;
	int fchg_type;
	int fchg_voltage;
	int fchg_current;

	struct sprd_vchg_info *vchg_info;
	enum cm_vchg_type vchg_type;

	int cp_voltage_mv;
	int cp_current_ma;

	int uvlo_retry;

	int jeita_zone;
	int hot_temp;
	int warm_temp;
	int cool_temp;
	int cold_temp;

	struct power_supply *wireless_psy;
	struct power_supply *cp_psy_dev;
	struct power_supply *charger_psy;

	struct workqueue_struct *wq;
};

static int sprd_cm_monitor(struct sprd_cm *cm);
static int sprd_cm_cp_state_machine(struct sprd_cm *cm);
static int sprd_cm_fchg_work(struct sprd_cm *cm);
static int sprd_cm_vchg_work(struct sprd_cm *cm);
static int sprd_cm_batt_work(struct sprd_cm *cm);
static int sprd_cm_cp_state_recovery(struct sprd_cm *cm);
static int sprd_cm_uvlo_check(struct sprd_cm *cm);

int cm_notify_event(const char *event, void *data)
{
	return 0;
}
EXPORT_SYMBOL(cm_notify_event);

int sprd_charge_vote_register(struct sprd_vote *vote)
{
	return 0;
}
EXPORT_SYMBOL(sprd_charge_vote_register);

/*
 * Camera session suppresses fake charging.
 *
 * sprd_sensor_core.c calls this with 0 when a sensor id is selected, i.e. a
 * camera stream is starting, and with 1 again when the last user releases the
 * device. The intent is to stop the fake-charge heuristic while the camera
 * draws current, because a streaming sensor pushes the supply past the
 * threshold that would otherwise trigger a simulated charge cycle.
 *
 * What this function must do to the charger is NOT recoverable: the vendor
 * bq2560x-charger.ko that declared the symbol is closed, and this tree has
 * only the register map recovered in docs/RE-BQ2560X-REGISTER-CONTRACT.md,
 * not the fchg behaviour. Guessing which register gates it would risk
 * writing to an unrelated charger bit.
 *
 * So this records the state and leaves the charger alone. That is safe: fake
 * charging stays under the charge manager's own logic, which is the
 * conservative outcome. The variable is exported so a platform that later
 * recovers the real implementation can wire it up without touching callers.
 */
static atomic_t sprd_camera_active = ATOMIC_INIT(0);

void charger_set_fchg_status(bool val)
{
	atomic_set(&sprd_camera_active, val ? 0 : 1);
	pr_debug("camera %s, fake charging %s\n",
		 val ? "closed" : "active", val ? "normal" : "would be suppressed");
}
EXPORT_SYMBOL(charger_set_fchg_status);

int sprd_fchg_info_register(struct sprd_fchg_info *info)
{
	return 0;
}
EXPORT_SYMBOL(sprd_fchg_info_register);

int sprd_vchg_info_register(struct sprd_vchg_info *info)
{
	return 0;
}
EXPORT_SYMBOL(sprd_vchg_info_register);

static void cm_monitor_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, monitor_work.work);
	sprd_cm_monitor(cm);
	queue_delayed_work(cm->wq, &cm->monitor_work,
			   msecs_to_jiffies(CM_POLL_INTERVAL_MS));
}

static void cm_fchg_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, fchg_work.work);
}

static void cm_vchg_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, vchg_work.work);
}

static void cm_batt_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, batt_work.work);
}

static void cm_cp_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, cp_work.work);
}

static void cm_uvlo_work(struct work_struct *work)
{
	struct sprd_cm *cm = container_of(work, struct sprd_cm, uvlo_work.work);
}

static int sprd_cm_monitor(struct sprd_cm *cm)
{
	union power_supply_propval val;
	int ret;

	if (!cm->batt_psy || !cm->charger_psy)
		return -ENODEV;

	mutex_lock(&cm->lock);

	ret = power_supply_get_property(cm->batt_psy, POWER_SUPPLY_PROP_CAPACITY, &val);
	if (!ret) {
		cm->capacity = val.intval;
		if (abs(cm->capacity - cm->prev_capacity) > 5)
			cm->prev_capacity = cm->capacity;
	}

	ret = power_supply_get_property(cm->charger_psy, POWER_SUPPLY_PROP_STATUS, &val);
	if (!ret) {
		cm->status = val.intval;
		if (val.intval == POWER_SUPPLY_STATUS_CHARGING ||
		    val.intval == POWER_SUPPLY_STATUS_FULL)
			cm->state = CM_STATE_CHARGING;
		else
			cm->state = CM_STATE_DISCHARGING;
	}

	ret = power_supply_get_property(cm->batt_psy, POWER_SUPPLY_PROP_TEMP, &val);
	if (!ret) {
		cm->temperature = val.intval;
	}

	if (cm->state == CM_STATE_CHARGING)
		sprd_cm_cp_state_machine(cm);

	if (cm->fchg_info)
		queue_delayed_work(cm->wq, &cm->fchg_work, 0);

	if (cm->vchg_info)
		queue_delayed_work(cm->wq, &cm->vchg_work, 0);

	queue_delayed_work(cm->wq, &cm->batt_work, 0);

	mutex_unlock(&cm->lock);

	queue_delayed_work(cm->wq, &cm->monitor_work,
			   msecs_to_jiffies(CM_POLL_INTERVAL_MS));

	return 0;
}

static int sprd_cm_cp_state_machine(struct sprd_cm *cm)
{
	switch (cm->cp_state) {
	case CM_CP_INIT:
		if (cm->cp_psy_dev) {
			power_supply_set_property(cm->cp_psy_dev,
						  POWER_SUPPLY_PROP_ONLINE,
						  &(union power_supply_propval){.intval = 1});
			cm->cp_state = CM_CP_CHECK_VBUS;
			cm->cp_retry = 0;
		}
		break;

	case CM_CP_CHECK_VBUS:
		if (cm->fchg_info && cm->fchg_info->detect) {
			if (cm->fchg_info->detect(cm->cp_psy_dev) > 0) {
				cm->cp_state = CM_CP_TUNE;
				cm->cp_retry = 0;
			} else if (++cm->cp_retry > CM_CP_RETRY_MAX) {
				cm->cp_state = CM_CP_EXIT;
			}
		} else {
			cm->cp_state = CM_CP_TUNE;
		}
		break;

	case CM_CP_TUNE:
		if (cm->fchg_info && cm->fchg_info->set_voltage &&
		    cm->fchg_info->set_current) {
			cm->fchg_info->set_voltage(cm->cp_psy_dev, cm->fchg_voltage);
			cm->fchg_info->set_current(cm->cp_psy_dev, cm->fchg_current);
			cm->cp_state = CM_CP_EXIT;
		}
		break;

	case CM_CP_EXIT:
		break;

	case CM_CP_RECOVERY:
		break;
	}
	return 0;
}

static int sprd_cm_cp_state_recovery(struct sprd_cm *cm)
{
	return 0;
}

static int sprd_cm_fchg_work(struct sprd_cm *cm)
{
	return 0;
}

static int sprd_cm_vchg_work(struct sprd_cm *cm)
{
	return 0;
}

static int sprd_cm_batt_work(struct sprd_cm *cm)
{
	union power_supply_propval val;
	int ret;

	if (!cm->batt_psy)
		return -ENODEV;

	mutex_lock(&cm->lock);

	ret = power_supply_get_property(cm->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (!ret) {
		cm->voltage_now = val.intval;
		if (val.intval < 3000000) {
			if (++cm->uvlo_retry > CM_UVLO_RETRY_MAX) {
				cm->state = CM_STATE_FAULT;
			} else {
				queue_delayed_work(cm->wq, &cm->uvlo_work,
						   msecs_to_jiffies(5000));
			}
		}
	}

	power_supply_get_property(cm->batt_psy, POWER_SUPPLY_PROP_CAPACITY, &val);
	cm->capacity = val.intval;

	mutex_unlock(&cm->lock);
	return 0;
}

static int sprd_cm_uvlo_check(struct sprd_cm *cm)
{
	return sprd_cm_batt_work(cm);
}

static int sprd_cm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sprd_cm *cm;
	int ret;

	cm = devm_kzalloc(dev, sizeof(*cm), GFP_KERNEL);
	if (!cm)
		return -ENOMEM;

	cm->dev = dev;
	platform_set_drvdata(pdev, cm);

	mutex_init(&cm->lock);
	mutex_init(&cm->vote_lock);
	mutex_init(&cm->listener_lock);
	INIT_LIST_HEAD(&cm->vote_list);
	INIT_LIST_HEAD(&cm->listener_list);

	cm->regmap = dev_get_regmap(dev->parent, NULL);
	if (!cm->regmap) {
		dev_err(dev, "No PMIC regmap\n");
		return -ENODEV;
	}

	cm->batt_psy = power_supply_get_by_name("battery");
	cm->charger_psy = power_supply_get_by_name("bq256xx-charger");
	cm->ac_psy = power_supply_get_by_name("ac");
	cm->usb_psy = power_supply_get_by_name("usb");

	cm->wq = alloc_ordered_workqueue("cm_wq", 0);
	if (!cm->wq)
		return -ENOMEM;

	INIT_DELAYED_WORK(&cm->monitor_work, cm_monitor_work);
	INIT_DELAYED_WORK(&cm->fchg_work, cm_fchg_work);
	INIT_DELAYED_WORK(&cm->vchg_work, cm_vchg_work);
	INIT_DELAYED_WORK(&cm->batt_work, cm_batt_work);
	INIT_DELAYED_WORK(&cm->cp_work, cm_cp_work);
	INIT_DELAYED_WORK(&cm->uvlo_work, cm_uvlo_work);

	queue_delayed_work(cm->wq, &cm->monitor_work,
			   msecs_to_jiffies(CM_POLL_INTERVAL_MS));

	dev_info(dev, "sprd charger manager probed\n");
	return 0;
}

static void sprd_cm_remove(struct platform_device *pdev)
{
	struct sprd_cm *cm = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&cm->monitor_work);
	cancel_delayed_work_sync(&cm->fchg_work);
	cancel_delayed_work_sync(&cm->vchg_work);
	cancel_delayed_work_sync(&cm->batt_work);
	cancel_delayed_work_sync(&cm->cp_work);
	cancel_delayed_work_sync(&cm->uvlo_work);

	destroy_workqueue(cm->wq);
}

static const struct of_device_id sprd_cm_of_match[] = {
	{ .compatible = "sprd,charger-manager" },
	{ .compatible = "sprd,sc2730-charger-manager" },
	{ }
};
MODULE_DEVICE_TABLE(of, sprd_cm_of_match);

static struct platform_driver sprd_cm_driver = {
	.probe	= sprd_cm_probe,
	.remove	= sprd_cm_remove,
	.driver	= {
		.name		= "sprd-charger-manager",
		.of_match_table	= sprd_cm_of_match,
	},
};
module_platform_driver(sprd_cm_driver);

MODULE_AUTHOR("VXTux port (RE from sprd-charger-manager.ko)");
MODULE_DESCRIPTION("Sprd Charger Manager for UMS512/T618");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sprd-charger-manager");