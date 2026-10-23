// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_debugstat - Spreadtrum subsystem sleep statistics.
 *
 * Port of the 5.4 vendor module `sprd_power_stat.ko`
 * (MODULE_DESCRIPTION "sprd debug stat driver", depends sprd_sip_svc).
 *
 * One struct subsys_sleep_info is kept per registered subsystem; every read
 * of /proc/sprd-debugstat/stat or slp_cnt refreshes them from the SIP
 * firmware (wakeup reason, PMIC sleep counter) before printing.
 *
 * Everything reproduced here was read out of the vendor blob with
 * donors/blobre.py (capstone + pyelftools, relocations applied): proc names
 * and modes, every column name, format string, field offset, error message,
 * and the stat_info_register() ABI.  See docs/RE-SPRD-POWER-STAT.md.
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regmap.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sprd_sip_svc.h>
#include <linux/string.h>
#include <linux/cpumask.h>
#include <linux/mfd/syscon.h>

#include <linux/sprd_debugstat.h>

/* Boot time in detik, persis seperti pembagi reciprocal 2^84/1e9 di blob. */
static inline u64 sprd_boot_sec(void)
{
	return div_u64(ktime_get_boot_fast_ns(), 1000000000ULL);
}

struct stat_info {
	const char *name;			/* +0x00 */
	struct subsys_sleep_info *data;		/* +0x08  salinan cache  */
	sprd_dbgstat_getter_t get;		/* +0x10 */
	struct device *dev;			/* +0x18 */
	struct list_head list;			/* +0x20 */
};

struct sprd_dbgstat_data {
	struct device *dev;			/* +0x00 */
	struct subsys_sleep_info *ap;		/* +0x08 */
	struct subsys_sleep_info *soc;		/* +0x10 */
	struct regmap *regmap;			/* +0x18 */
	u32 slp_reg_offset;			/* +0x20 */
	u32 slp_reg_mask;			/* +0x24 */
};

static LIST_HEAD(info_node_list);
static DEFINE_MUTEX(info_mutex);

/* ------------------------------------------------------------------ API */

int stat_info_register(const char *name, sprd_dbgstat_getter_t get,
		       struct device *dev)
{
	struct stat_info *info, *new;
	int ret = 0;

	if (!name || !get) {
		pr_err("3%s: Parameter is error\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&info_mutex);
	list_for_each_entry(new, &info_node_list, list) {
		if (!strcmp(new->name, name)) {
			pr_err("4%s: The node(%s) is exsit\n", __func__, name);
			ret = -EEXIST;
			goto out;
		}
	}

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("3%s: Node allock error\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	info->data = kzalloc(sizeof(*info->data), GFP_KERNEL);
	if (!info->data) {
		pr_err("3%s: dbgstat_data alloc error\n", __func__);
		kfree(info);
		ret = -ENOMEM;
		goto out;
	}

	info->name = name;
	info->get = get;
	info->dev = dev;
	list_add_tail(&info->list, &info_node_list);
out:
	mutex_unlock(&info_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(stat_info_register);

int stat_info_unregister(const char *name)
{
	struct stat_info *info, *tmp;
	int ret = 0;

	if (!name) {
		pr_err("3%s: Parameter is error\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&info_mutex);
	list_for_each_entry_safe(info, tmp, &info_node_list, list) {
		if (!strcmp(info->name, name)) {
			kfree(info->data);
			list_del(&info->list);
			kfree(info);
			goto out;
		}
	}
	pr_err("3%s: The node(%s) is not found\n", __func__, name);
out:
	mutex_unlock(&info_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(stat_info_unregister);

/* --------------------------------------------------------------- kolom */

/*
 * Tiap baris dicetak sebagai satu buffer 128 byte: label "%22s:" lalu nilai
 * tiap node dengan format "%10s"/"%10llu"/"%10u"/"%#10x", diakhiri "\n".
 * Urutan dan nama kolom di bawah persis urutan stat_print_* di blob.
 */
struct stat_column {
	const char *name;
	const char *fmt;
	unsigned int off;
	bool is_str;
	bool is64;
};

static const struct stat_column stat_columns[] = {
	{ "subsystem_name",		"%10s",	0x00, true,  false },
	{ "total_duration",		"%10llu", 0x08, false, true },
	{ "sleep_duration_total",	"%10llu", 0x10, false, true },
	{ "idle_duration_total",	"%10llu", 0x18, false, true },
	{ "state",			"%10u", 0x20, false, false },
	{ "subsystem_reboot_count",	"%10u", 0x24, false, false },
	{ "wakeup_reason",		"%10u", 0x28, false, false },
	{ "last_wakeup_duration",	"%10u", 0x2c, false, false },
	{ "last_sleep_duration",	"%10u", 0x30, false, false },
	{ "active_core",		"%#10x", 0x34, false, false },
	{ "internal_irq_count",		"%10u", 0x38, false, false },
	{ "irq_to_ap_count",		"%10u", 0x3c, false, false },
};

static void dbgstat_refresh(void)
{
	struct stat_info *info;

	list_for_each_entry(info, &info_node_list, list) {
		struct subsys_sleep_info *fresh;

		fresh = info->get(info->dev);
		if (fresh)
			memcpy(info->data, fresh, sizeof(*info->data));
	}
}

static int stat_read(struct seq_file *m, void *unused)
{
	struct stat_info *info;
	unsigned int col;
	char buf[128];

	mutex_lock(&info_mutex);
	dbgstat_refresh();

	seq_printf(m, "Subsys information:\n");
	for (col = 0; col < ARRAY_SIZE(stat_columns); col++) {
		const struct stat_column *c = &stat_columns[col];
		unsigned int len, n;

		len = scnprintf(buf, sizeof(buf), "%22s:", c->name);
		list_for_each_entry(info, &info_node_list, list) {
			const char *base = (const char *)info->data;

			if (len >= sizeof(buf) - 16)
				break;
			if (c->is_str) {
				n = scnprintf(buf + len, sizeof(buf) - len,
					      c->fmt, base);
			} else if (c->is64) {
				u64 v;

				memcpy(&v, base + c->off, sizeof(v));
				n = scnprintf(buf + len, sizeof(buf) - len,
					      c->fmt, v);
			} else {
				u32 v;

				memcpy(&v, base + c->off, sizeof(v));
				n = scnprintf(buf + len, sizeof(buf) - len,
					      c->fmt, v);
			}
			len += n;
		}
		seq_printf(m, "%s\n", buf);
	}
	mutex_unlock(&info_mutex);
	return 0;
}

static int stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, stat_read, NULL);
}

/* ------------------------------------------------------------- slp_cnt */

#define SOC_SYS_NAME	"soc_sys"

static int slp_cnt_read(struct seq_file *m, void *unused)
{
	struct stat_info *info;
	u32 val = 0;
	bool found = false;

	mutex_lock(&info_mutex);
	list_for_each_entry(info, &info_node_list, list) {
		struct subsys_sleep_info *fresh;

		if (strcmp(info->name, SOC_SYS_NAME))
			continue;

		fresh = info->get(info->dev);
		if (fresh)
			memcpy(info->data, fresh, sizeof(*info->data));

		val = info->data->soc_slp_cnt;
		found = true;
		break;
	}
	mutex_unlock(&info_mutex);

	if (!found || val == (u32)-1) {
		pr_err("3%s: Get %s error\n", __func__, SOC_SYS_NAME);
		return 0;
	}

	seq_printf(m, "%d\n", val);
	return 0;
}

static int slp_cnt_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, slp_cnt_read, NULL);
}

static const struct proc_ops stat_fops = {
	.proc_open = stat_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static const struct proc_ops slp_cnt_fops = {
	.proc_open = slp_cnt_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static struct proc_dir_entry *sprd_debugstat_dir;

static int sprd_debugstat_core_init(void)
{
	sprd_debugstat_dir = proc_mkdir("sprd-debugstat", NULL);
	if (!sprd_debugstat_dir) {
		pr_err("3%s: Proc dir create failed\n", __func__);
		return -EINVAL;
	}

	if (!proc_create("stat", 0444, sprd_debugstat_dir, &stat_fops)) {
		pr_err("3%s: Proc file create failed\n", __func__);
		goto err_dir;
	}

	if (!proc_create("slp_cnt", 0644, sprd_debugstat_dir, &slp_cnt_fops)) {
		pr_err("3%s: Proc file slp_cnt create failed\n", __func__);
		goto err_stat;
	}

	return 0;

err_stat:
	remove_proc_entry("stat", sprd_debugstat_dir);
err_dir:
	remove_proc_entry("sprd-debugstat", NULL);
	sprd_debugstat_dir = NULL;
	return -EINVAL;
}

static void sprd_debugstat_core_exit(void)
{
	if (!sprd_debugstat_dir)
		return;
	remove_proc_entry("slp_cnt", sprd_debugstat_dir);
	remove_proc_entry("stat", sprd_debugstat_dir);
	remove_proc_entry("sprd-debugstat", NULL);
	sprd_debugstat_dir = NULL;
}

/* ---------------------------------------------------------- sumber data */

static struct subsys_sleep_info *ap_sleep_info_get(struct device *dev)
{
	struct sprd_dbgstat_data *priv = dev_get_drvdata(dev);
	struct sprd_sip_svc_handle *handle;
	u64 wakeup = 0;

	if (!priv || !priv->ap)
		return NULL;

	priv->ap->total_duration = sprd_boot_sec();
	priv->ap->active_core = cpumask_weight(cpu_online_mask);

	handle = sprd_sip_svc_get_handle();
	if (handle && handle->pwr_get_wakeup_source &&
	    !handle->pwr_get_wakeup_source(&wakeup)) {
		/*
		 * Firmware mengembalikan bitfield; blob memakai
		 * ((v >> 11) & 0x1fffe0) + (v & 0xffff).
		 */
		priv->ap->wakeup_reason =
			((wakeup >> 11) & 0x1fffe0) + (wakeup & 0xffff);
	}

	return priv->ap;
}

static struct subsys_sleep_info *soc_sleep_info_get(struct device *dev)
{
	struct sprd_dbgstat_data *priv = dev_get_drvdata(dev);
	u32 val = 0;

	if (!priv || !priv->regmap || !priv->soc)
		return NULL;

	if (regmap_read(priv->regmap, priv->slp_reg_offset, &val))
		priv->soc->soc_slp_cnt = (u32)-1;
	else
		priv->soc->soc_slp_cnt = val & priv->slp_reg_mask;

	return priv->soc;
}

/* ------------------------------------------------------------- platform */

static int sprd_debugstat_driver_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct subsys_sleep_info *info;
	struct sprd_dbgstat_data *priv;
	unsigned int spec[2] = { };
	struct regmap *regmap;
	int ret;

	if (!dev) {
		pr_err("3%s: Get debug device error\n", __func__);
		return -EINVAL;
	}

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("3%s: Sleep info alloc error\n", __func__);
		return -ENOMEM;
	}
	memcpy(info->name, "AP", 3);
	info->state = 1;
	priv->ap = info;

	ret = stat_info_register("ap_sys", ap_sleep_info_get, dev);
	if (ret) {
		pr_err("3%s: Register ap sleep info get error\n", __func__);
		return ret;
	}

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("3%s: Sleep info alloc error\n", __func__);
		return -ENOMEM;
	}
	memcpy(info->name, "SOC", 4);
	priv->soc = info;

	regmap = syscon_regmap_lookup_by_phandle_args(dev->of_node,
						     "soc_slp_cnt_reg", 2, spec);
	if (IS_ERR_OR_NULL(regmap)) {
		pr_err("3%s: slp_cnt_regmap error, check dts.\n", __func__);
		ret = PTR_ERR_OR_ZERO(regmap);
		if (!ret)
			ret = -ENOMEM;
		goto err_ap;
	}

	priv->regmap = regmap;
	priv->slp_reg_offset = spec[0];
	priv->slp_reg_mask = spec[1];

	ret = stat_info_register("soc_sys", soc_sleep_info_get, dev);
	if (ret) {
		pr_err("3%s: Register soc sleep info get error\n", __func__);
		goto err_ap;
	}

	dev_set_drvdata(dev, priv);

	ret = sprd_debugstat_core_init();
	if (ret) {
		pr_err("3%s: sprd_debugstat_socsys_init error\n", __func__);
		stat_info_unregister(SOC_SYS_NAME);
		goto err_ap;
	}

	return 0;

err_ap:
	stat_info_unregister("ap_sys");
	return ret;
}

static void sprd_debugstat_driver_remove(struct platform_device *pdev)
{
	sprd_debugstat_core_exit();
	stat_info_unregister(SOC_SYS_NAME);
	stat_info_unregister("ap_sys");
}

static int sprd_debugstat_suspend(struct device *dev)
{
	struct sprd_dbgstat_data *priv = dev_get_drvdata(dev);

	if (priv && priv->ap)
		priv->ap->last_sleep_duration = sprd_boot_sec();

	return 0;
}

static int sprd_debugstat_resume(struct device *dev)
{
	struct sprd_dbgstat_data *priv = dev_get_drvdata(dev);

	if (priv && priv->ap) {
		priv->ap->last_wakeup_duration = sprd_boot_sec();
		/* tidur = waktu terakhir terbangun - waktu terakhir tidur */
		priv->ap->sleep_duration_total +=
			priv->ap->last_wakeup_duration -
			priv->ap->last_sleep_duration;
	}

	return 0;
}

static const struct dev_pm_ops sprd_debugstat_pm = {
	.suspend = sprd_debugstat_suspend,
	.resume = sprd_debugstat_resume,
};

static const struct of_device_id sprd_debugstat_match[] = {
	{ .compatible = "sprd,debugstat" },
	{ },
};

static struct platform_driver sprd_debugstat_driver = {
	.probe = sprd_debugstat_driver_probe,
	.remove = sprd_debugstat_driver_remove,
	.driver = {
		.name = "sprd_debugstat",
		.pm = &sprd_debugstat_pm,
		.of_match_table = sprd_debugstat_match,
	},
};

static int __init sprd_debugstat_init(void)
{
	return platform_driver_register(&sprd_debugstat_driver);
}

static void sprd_debugstat_exit(void)
{
	platform_driver_unregister(&sprd_debugstat_driver);
}

module_init(sprd_debugstat_init);
module_exit(sprd_debugstat_exit);

MODULE_DESCRIPTION("sprd debug stat driver");
MODULE_LICENSE("GPL v2");
