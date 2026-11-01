// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_pmic_wdt.c - SC27xx PMIC watchdog control via SIPC sbuf
 *
 * Deblob reconstruction of the vendor-only binary
 * vendor_modules/sprd_pmic_wdt.ko (vermagic 5.4.210, aarch64).
 * Reverse-engineered with radare2 (MCP) on 2026-11-01; no vendor C source
 * exists anywhere on this host (checked: linux-5.4.210, vendor kernel trees,
 * full E: grep for 'sprd_pmic_wdt'/'sc2730-wdt' -> only the DTB).
 *
 * Evidence from the binary:
 *   - of_match: sprd,sc27{20,21,23t,30,31}-wdt + sprd,ump9620-wdt
 *   - probe(): devm_kmalloc(0x108); dev_get_regmap(pdev->dev.parent, NULL);
 *     of_property_read_variable_u32_array(np, "reg"); device_init_wakeup();
 *     kthread_create_on_node(kthread_worker_fn, ..., "pmic_wdt_worker");
 *     sched_setscheduler(SCHED_FIFO, prio 1); reads /chosen "bootargs",
 *     strstr("androidboot.wdten=") + sscanf("androidboot.wdten=%8s"),
 *     enabled = (value == "e551"); sbuf_register_notifier(6, 6, 0, cb, wdt).
 *   - notifier cb (event, data): event 0 -> "sbuf ready for pmic wdt init!",
 *     event 1 -> "sbuf read for pmic wdt init!" (guarded by ->inited);
 *     pm_wakeup_dev_event(dev, 2000, false) + kthread_queue_work().
 *   - work(): sbuf_write(6, 6, 0, cmd, len, 25);
 *     enabled -> "watchdog on"   (len 12 = 11 + NUL)
 *     else    -> "watchdog rstoff" (len 16 = 15 + NUL)
 *     printk("cm4 watchdog on/off: len = %d, nwrite = %d\n", ...)
 *   - remove(): sbuf_register_notifier(6, 6, 0, NULL, NULL) is the vendor's
 *     unregister idiom (port sbuf.c:1001 sets ring->handler = handler, so
 *     NULL handler unregisters); then kthread_flush_worker + kthread_stop.
 *
 * So this is NOT an AP-side rebooting watchdog: it tells the PM subsystem
 * (SIPC dst 6 == SIPC_ID_PM_SYS, the "cm4") to switch its watchdog on or
 * off, using androidboot.wdten from the bootloader command line.  Without
 * the bootarg it sends "watchdog rstoff" - i.e. the safe default for a
 * pure-Linux boot, which is exactly what stock does when LK does not set
 * androidboot.wdten=e551.
 *
 * Deviations from the blob (deliberate, both safer on 6.18):
 *   - sched_set_fifo() replaces sched_setscheduler(SCHED_FIFO, 1); same
 *     policy/priority, and the latter is being removed for external callers.
 *   - regmap/reg are validated at probe like the blob does, but the driver
 *     never touches them afterwards (the blob does not either).
 */

#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/regmap.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/soc/sprd/sipc.h>

#define PMIC_WDT_SIPC_DST	SIPC_ID_PM_SYS	/* blob used dst 6 == PM_SYS (cm4) */
#define PMIC_WDT_SIPC_CH	6
#define PMIC_WDT_CMD_ON		"watchdog on"
#define PMIC_WDT_CMD_OFF	"watchdog rstoff"
#define PMIC_WDT_CMD_TIMEOUT	25

struct sprd_pmic_wdt {
	struct regmap		*regmap;
	struct device		*dev;
	u32			reg;
	bool			enabled;
	struct kthread_worker	wdt_kworker;
	struct kthread_work	wdt_work;
	struct task_struct	*task;
	u32			inited;
};

static void sprd_pmic_wdt_work(struct kthread_work *work)
{
	struct sprd_pmic_wdt *pmic_wdt = container_of(work, struct sprd_pmic_wdt,
						      wdt_work);
	const char *cmd;
	u32 len;
	int nwrite;

	dev_info(pmic_wdt->dev, "sprd pmic wdt work enter!\n");

	if (pmic_wdt->enabled) {
		cmd = PMIC_WDT_CMD_ON;
		len = sizeof(PMIC_WDT_CMD_ON);
	} else {
		cmd = PMIC_WDT_CMD_OFF;
		len = sizeof(PMIC_WDT_CMD_OFF);
	}

	nwrite = sbuf_write(PMIC_WDT_SIPC_DST, PMIC_WDT_SIPC_CH, 0,
			    (void *)cmd, len, PMIC_WDT_CMD_TIMEOUT);
	printk(KERN_INFO "cm4 watchdog on/off: len = %d, nwrite = %d\n",
	       len, nwrite);
	if (nwrite != len)
		dev_err(pmic_wdt->dev, "failed to set pmic wdt %d!\n",
			pmic_wdt->enabled);
}

static void sprd_pmic_wdt_notify(int event, void *data)
{
	struct sprd_pmic_wdt *pmic_wdt = data;

	switch (event) {
	case SBUF_NOTIFY_READY:
		dev_info(pmic_wdt->dev, "sbuf ready for pmic wdt init!\n");
		break;
	case SBUF_NOTIFY_READ:
		if (pmic_wdt->inited)
			return;
		dev_info(pmic_wdt->dev, "sbuf read for pmic wdt init!\n");
		break;
	default:
		return;
	}

	pm_wakeup_dev_event(pmic_wdt->dev, 2000, false);
	kthread_queue_work(&pmic_wdt->wdt_kworker, &pmic_wdt->wdt_work);
	pmic_wdt->inited = 1;
}

/*
 * The blob parses /chosen/bootargs for "androidboot.wdten=" and enables the
 * watchdog only when the value is exactly "e551" (a 4-byte compare against
 * 0x31353565 == "e551" little-endian).  Keep that magic - it is the contract
 * with LK.
 */
static bool sprd_pmic_wdt_bootargs_enabled(struct device *dev)
{
	struct device_node *np;
	const char *bootargs;
	char val[9] = { };
	bool enabled = false;
	int ret;

	np = of_find_node_by_path("/chosen");
	if (!np)
		np = of_find_node_by_path("/chosen@0");
	if (!np) {
		dev_warn(dev, "sprd_pmic_wdt can't find /chosen\n");
		return false;
	}

	ret = of_property_read_string(np, "bootargs", &bootargs);
	of_node_put(np);
	if (ret) {
		dev_warn(dev, "sprd_pmic_wdt can't not parse bootargs property\n");
		return false;
	}

	if (!strstr(bootargs, "androidboot.wdten="))
		return false;

	sscanf(bootargs, "androidboot.wdten=%8s", val);
	enabled = !strcmp(val, "e551");

	return enabled;
}

static int sprd_pmic_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sprd_pmic_wdt *pmic_wdt;
	u32 reg;
	int ret;

	pmic_wdt = devm_kzalloc(dev, sizeof(*pmic_wdt), GFP_KERNEL);
	if (!pmic_wdt)
		return -ENOMEM;

	platform_set_drvdata(pdev, pmic_wdt);
	pmic_wdt->dev = dev;

	/*
	 * The blob demands a PMIC regmap on the parent (SC27xx MFD) and a
	 * "reg" property on the node; probe fails without either.  Mirror
	 * that so a missing/misparented node is loud instead of silent.
	 */
	pmic_wdt->regmap = dev_get_regmap(dev->parent, NULL);
	if (!pmic_wdt->regmap) {
		dev_err(dev, "sprd pmic wdt probe failed!\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(dev->of_node, "reg", &reg);
	if (ret) {
		dev_err(dev, "failed to get pmic wdt base address\n");
		return ret;
	}
	pmic_wdt->reg = reg;

	device_init_wakeup(dev, true);
	pmic_wdt->enabled = sprd_pmic_wdt_bootargs_enabled(dev);

	kthread_init_worker(&pmic_wdt->wdt_kworker);
	pmic_wdt->task = kthread_run(kthread_worker_fn, &pmic_wdt->wdt_kworker,
				     "pmic_wdt_worker");
	if (IS_ERR(pmic_wdt->task)) {
		dev_err(dev, "failed to run pmic_wdt_thread:\n");
		device_init_wakeup(dev, false);
		return PTR_ERR(pmic_wdt->task);
	}
	sched_set_fifo(pmic_wdt->task);

	kthread_init_work(&pmic_wdt->wdt_work, sprd_pmic_wdt_work);

	ret = sbuf_register_notifier(PMIC_WDT_SIPC_DST, PMIC_WDT_SIPC_CH, 0,
				     sprd_pmic_wdt_notify, pmic_wdt);
	if (ret) {
		dev_err(dev, "sbuf notifier failed rval = %d\n", ret);
		kthread_flush_worker(&pmic_wdt->wdt_kworker);
		kthread_stop(pmic_wdt->task);
		pmic_wdt->task = NULL;
		device_init_wakeup(dev, false);
		return ret;
	}

	return 0;
}

static void sprd_pmic_wdt_remove(struct platform_device *pdev)
{
	struct sprd_pmic_wdt *pmic_wdt = platform_get_drvdata(pdev);
	int ret;

	/* NULL handler == unregister (see header comment). */
	ret = sbuf_register_notifier(PMIC_WDT_SIPC_DST, PMIC_WDT_SIPC_CH, 0,
				     NULL, NULL);
	if (ret)
		dev_err(&pdev->dev, "sbuf unregitster notifier failed rval = %d\n",
			ret);

	kthread_flush_worker(&pmic_wdt->wdt_kworker);
	if (pmic_wdt->task)
		kthread_stop(pmic_wdt->task);
	device_init_wakeup(&pdev->dev, false);
}

static const struct of_device_id sprd_pmic_wdt_of_match[] = {
	{ .compatible = "sprd,sc2720-wdt" },
	{ .compatible = "sprd,sc2721-wdt" },
	{ .compatible = "sprd,sc2723t-wdt" },
	{ .compatible = "sprd,sc2730-wdt" },
	{ .compatible = "sprd,sc2731-wdt" },
	{ .compatible = "sprd,ump9620-wdt" },
	{ },
};
MODULE_DEVICE_TABLE(of, sprd_pmic_wdt_of_match);

static struct platform_driver sprd_pmic_wdt_driver = {
	.probe = sprd_pmic_wdt_probe,
	.remove = sprd_pmic_wdt_remove,
	.driver = {
		.name = "sprd-pmic-wdt",
		.of_match_table = sprd_pmic_wdt_of_match,
	},
};
module_platform_driver(sprd_pmic_wdt_driver);

MODULE_DESCRIPTION("Spreadtrum PMIC watchdog control via SIPC");
MODULE_LICENSE("GPL");
