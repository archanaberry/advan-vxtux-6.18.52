// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_pdbg - Spreadtrum power debug (wakeup / sleep logging).
 *
 * Port of the 5.4 vendor module `sprd_pdbg.ko` (40.854 B, 16 fungsi,
 * depends sprd_sip_svc, ekspor sprd_pdbg_msg_print,
 * module_param sprd_pdbg_log_force).
 *
 * Peran driver: menjelaskan kenapa SoC bangun/tidur.
 *   - menyocokkan nama irq-domain dari DT (/debuglog) dengan
 *     irq_desc->domain->name pada IRQ 0..511, sehingga tiap wakeup source
 *     dikenali firmware;
 *   - kthread "sprd-pdbg-thread" periodik mencatat sumber wakeup aktif;
 *   - pm_notifier mencatat durasi suspend (PM_SUSPEND_PREPARE/POST_SUSPEND);
 *   - cpu_pm_notifier mencatat momen cluster masuk/keluar low-power
 *     (DEEP SLEEP START / WAKEUP SCENE END) dan mengambil info tidur dari
 *     firmware lewat sprd_sip_svc (pwr_get_pdbg_info, SMC 0x82000502);
 *   - /proc/sprd-pdbg/regs_info menampilkan ringkasan info tidur.
 *
 * Semua yang direproduksi di sini dibaca dari blob dengan
 * donors/blobre.py (capstone + pyelftools, relokasi diterapkan): nama
 * prosedur, string, urutan pemanggilan, dan offset field.  Disassembly
 * lengkap: docs/evidence-20261023/pdbg_disasm.txt dan pdbg_notif.txt;
 * ringkasan: docs/RE-SPRD-PDBG.md.
 *
 * Layout struct sprd_pdbg (blob: ukuran 0x538):
 *   +0x00 task_struct *task     +0x08 struct device *dev
 *   +0x10 handle sip_svc        +0x18 pm_notifier   +0x30 cpu_pm_notifier
 *   +0x48 list_head ws_list     +0x58 delayed_work rescan
 *   +0xe0 rwlock ws_lock        +0xf8 u64 regs[7][4]
 *   +0x188 u32 poll_ms (30)     +0x1b0 bool enabled  +0x1b1 bool dual
 *   +0x148 const char *ws_names[8]  +0x1b2 char buf[0x380]
 */
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqdomain.h>
#include <linux/irqhandler.h>
#include <linux/irqnr.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/pm_wakeup.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/rwlock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sprd_sip_svc.h>
#include <linux/suspend.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#define PRFX "SPRD_PDBG"

/* 8 properti DT, urutan array blob ($d.41). */
static const char * const pdbg_prop_names[8] = {
	"sprd,pdbg-irq-domain-gic",
	"sprd,pdbg-irq-domain-gpio",
	"sprd,pdbg-irq-domain-ana",
	"sprd,pdbg-irq-domain-ana-eic",
	"sprd,pdbg-irq-domain-ap-eic-async",
	"sprd,pdbg-irq-domain-ap-eic-dbnc",
	"sprd,pdbg-irq-domain-ap-eic-latch",
	"sprd,pdbg-irq-domain-ap-eic-sync",
};

/* Jenis info tidur, argumen pertama pwr_get_pdbg_info. */
enum {
	PDBG_SLP_STATE = 0,	/* [SLP_STATE] deep, light        */
	PDBG_EB_INFO = 1,	/* [EB_INFO  ] ap1/ap2/aon1/aon2 */
	PDBG_PD_INFO = 2,	/* [PD_INFO  ]                    */
	PDBG_DEEP_CNT = 3,	/* [DEEP_CNT ] + deret u16        */
	PDBG_LIGHT_CNT = 4,	/* [LIGHT_CNT] + deret u16        */
	PDBG_LPC_INFO = 5,	/* [LPC_INFO ]                    */
	PDBG_WAKEUP_SCENE = 6,	/* dipakai cpu_pm_notifier EXIT   */
};

/* Nilai sentinel firmware (blob: cmp x3, 0xeeddccbbaa998877). */
#define PDBG_LPC_INVALID	0xeeccbbaa99887766ULL

#define PDBG_BUF_SZ		0x380
#define PDBG_POLL_MS		30	/* blob: str w30, [x19, #0x188]   */
#define PDBG_RESCAN_MS		500	/* blob: queue_delayed_work 0x1f4 */
#define PDBG_RESCAN_RETRY	10
#define PDBG_MAX_IRQ		512	/* blob: cmp w20, #0x200         */

struct sprd_pdbg_ws {
	u32 id;			/* index properti DT yang cocok  */
	struct irq_domain *domain; /* blob: ldr x24,[desc,#0x50] (domain),
				     str x24,[node,#8] — 5.4: desc->domain */
	struct list_head list;
};

struct sprd_pdbg {
	struct task_struct *task;	/* +0x00 */
	struct device *dev;		/* +0x08 */
	const struct sprd_sip_svc_handle *svc;	/* +0x10 */
	struct notifier_block pm_notifier;	/* +0x18 */
	struct notifier_block cpu_pm_notifier;	/* +0x30 */
	struct list_head ws_list;	/* +0x48 */
	struct delayed_work rescan_work;	/* +0x58 */
	rwlock_t ws_lock;		/* +0xe0 */
	u64 regs[7][4];			/* +0xf8  a0..a3 per id          */
	const char *ws_names[8];	/* +0x148 */
	u32 poll_ms;			/* +0x188 */
	ktime_t suspend_start;		/* +0x190 */
	bool enabled;			/* +0x1b0  gate sprd_pdbg_msg_print */
	unsigned int rescan_retry;	/* .bss sym#86+0x10 */
	struct proc_dir_entry *dir;
	char buf[PDBG_BUF_SZ];		/* +0x1b2 */
};

/*
 * module_param blob (simbol __param_sprd_pdbg_log_force): bila aktif, pesan
 * sleep/wakeup dicetak dengan KERN_ERR (vendor: "3[...]") alih-alih
 * KERN_INFO ("6[...]").
 */
static bool sprd_pdbg_log_force;
module_param_named(sprd_pdbg_log_force, sprd_pdbg_log_force, bool, 0644);
MODULE_PARM_DESC(sprd_pdbg_log_force, "log sleep/wakeup dengan level err");

static struct sprd_pdbg *pdbg_data;

#define pdbg_log(fmt, ...) \
	do { \
		if (sprd_pdbg_log_force) \
			pr_err(fmt, ##__VA_ARGS__); \
		else \
			pr_info(fmt, ##__VA_ARGS__); \
	} while (0)

/* Pengganti pm_get_active_wakeup_sources() milik 5.4 (tidak ada di 6.18). */
static void pdbg_active_wakeup_sources(char *buf, size_t size)
{
	struct wakeup_source *ws;
	size_t len = 0;

	buf[0] = '\0';
	for_each_wakeup_source(ws) {
		if (!ws->active)
			continue;
		len += scnprintf(buf + len, size - len, "%s ", ws->name);
		if (len >= size - 1)
			break;
	}
	if (!buf[0])
		strscpy(buf, "none ", size);
}

/* --- akses firmware ---------------------------------------------------- */

/*
 * blob: sprd_pdbg_regs_get_once @ 0x14e0 — SMC pwr_get_pdbg_info(id)
 * mengisi empat u64 di +0xf8..+0x117 (blob memanggil dengan empat pointer
 * keluar).  Wrapper 6.18 kita membawa a0..a3 lewat
 * pwr_get_pdbg_info_words().
 */
static int pdbg_regs_get_once(struct sprd_pdbg *pd, u32 id)
{
	if (!pd || !pd->svc || !pd->svc->pwr_get_pdbg_info_words)
		return -EINVAL;
	return pd->svc->pwr_get_pdbg_info_words(id, pd->regs[id]);
}

/* jumlah elemen deret u16 DEEP/LIGHT_CNT: byte ke-6 u64[3] (blob +0x116) */
static u32 pdbg_cnt_count(const u64 r[4])
{
	return (u32)((r[3] >> 48) & 0xff);
}

static int pdbg_cnt_print(char *buf, int used, const u64 r[4])
{
	u32 i, n = pdbg_cnt_count(r);

	for (i = 0; i < n && used < PDBG_BUF_SZ; i++)
		used += scnprintf(buf + used, PDBG_BUF_SZ - used, "%5d, ",
				  (u16)(r[i / 4] >> ((i % 4) * 16)));
	return used;
}

/*
 * blob: sprd_pdbg_regs_get @ 0x112c — susun blok teks berurutan ke buf.
 * Argumen ketiga blob (w2) bukan "refresh" melainkan "juga cetak tiap blok
 * ke kernel log" (tbnz w20 di belakang tiap scnprintf, memilih "6[%s] %s"
 * atau "3[%s] %s" sesuai sprd_pdbg_log_force).
 */
static int pdbg_regs_get(struct sprd_pdbg *pd, char *buf, bool to_log)
{
	int used = 0, start;

#define PDBG_BLOCK(fmt, ...) \
	do { \
		start = used; \
		used += scnprintf(buf + used, PDBG_BUF_SZ - used, fmt, \
				  ##__VA_ARGS__); \
		if (to_log) \
			pdbg_log("[%s] %s", PRFX, buf + start); \
	} while (0)

	if (!pd) {
		pr_err("3[%s] %s: Parameter is error\n", PRFX, __func__);
		return -EINVAL;
	}

	if (!pdbg_regs_get_once(pd, PDBG_SLP_STATE))
		PDBG_BLOCK("[SLP_STATE] deep: 0x%llx, light: 0x%llx\n",
			   pd->regs[PDBG_SLP_STATE][0],
			   pd->regs[PDBG_SLP_STATE][1]);

	if (!pdbg_regs_get_once(pd, PDBG_EB_INFO))
		PDBG_BLOCK("[EB_INFO  ] ap1: 0x%llx, ap2: 0x%llx, "
			   "aon1: 0x%llx, aon2: 0x%llx\n",
			   pd->regs[PDBG_EB_INFO][0],
			   pd->regs[PDBG_EB_INFO][1],
			   pd->regs[PDBG_EB_INFO][2],
			   pd->regs[PDBG_EB_INFO][3]);

	if (!pdbg_regs_get_once(pd, PDBG_PD_INFO) &&
	    pd->regs[PDBG_PD_INFO][0] != PDBG_LPC_INVALID)
		PDBG_BLOCK("[PD_INFO  ] 0x%llx\n", pd->regs[PDBG_PD_INFO][0]);

	if (!pdbg_regs_get_once(pd, PDBG_DEEP_CNT)) {
		start = used;
		used += scnprintf(buf + used, PDBG_BUF_SZ - used,
				  "[DEEP_CNT ] ");
		used = pdbg_cnt_print(buf, used, pd->regs[PDBG_DEEP_CNT]);
		used += scnprintf(buf + used, PDBG_BUF_SZ - used, "\n");
		if (to_log)
			pdbg_log("[%s] %s", PRFX, buf + start);
	}

	if (!pdbg_regs_get_once(pd, PDBG_LIGHT_CNT)) {
		start = used;
		used += scnprintf(buf + used, PDBG_BUF_SZ - used,
				  "[LIGHT_CNT] ");
		used = pdbg_cnt_print(buf, used, pd->regs[PDBG_LIGHT_CNT]);
		used += scnprintf(buf + used, PDBG_BUF_SZ - used, "\n");
		if (to_log)
			pdbg_log("[%s] %s", PRFX, buf + start);
	}

	if (!pdbg_regs_get_once(pd, PDBG_LPC_INFO) &&
	    pd->regs[PDBG_LPC_INFO][0] != PDBG_LPC_INVALID)
		PDBG_BLOCK("[LPC_INFO ] 0x%llx\n", pd->regs[PDBG_LPC_INFO][0]);

#undef PDBG_BLOCK
	return used;
}

/* --- pemetaan irq-domain ------------------------------------------------ */

static void pdbg_rescan_worker(struct work_struct *work);

/*
 * blob: sprd_pdbg_parse_irq_domain @ 0xaa4 — scan IRQ 0..511, cocokkan
 * desc->domain->name dengan nama dari DT, catat {id, irq_data} sekali lalu
 * buang namanya.  Bila tidak ada yang cocok sama sekali, ulangi maksimal
 * 10 kali dengan jeda 500 ms (domain mungkin belum terdaftar saat probe).
 */
static void pdbg_parse_irq_domain(struct sprd_pdbg *pd)
{
	bool matched_any = false;
	unsigned int irq;

	for (irq = 0; irq < PDBG_MAX_IRQ; irq++) {
		struct irq_desc *desc = irq_to_desc(irq);
		const char *name;
		int i;

		/* 6.18: domain pindah ke dalam irq_data (5.4: desc->domain). */
		if (!desc || !desc->irq_data.domain)
			continue;
		name = desc->irq_data.domain->name;
		if (!name)
			continue;

		for (i = 0; i < ARRAY_SIZE(pd->ws_names); i++) {
			struct sprd_pdbg_ws *it, *node;
			bool exists = false;

			if (!pd->ws_names[i] || strcmp(name, pd->ws_names[i]))
				continue;

			read_lock(&pd->ws_lock);
			list_for_each_entry(it, &pd->ws_list, list) {
				if (it->id == (u32)i) {
					exists = true;
					break;
				}
			}
			read_unlock(&pd->ws_lock);
			if (exists)
				continue;

			node = kzalloc(sizeof(*node), GFP_KERNEL);
			if (!node) {
				pr_err("3[%s] %s: ws irq domain alloc error\n",
				       PRFX, __func__);
				continue;
			}
			node->id = i;
			node->domain = desc->irq_data.domain;
			write_lock(&pd->ws_lock);
			list_add_tail(&node->list, &pd->ws_list);
			write_unlock(&pd->ws_lock);

			pdbg_log("[%s] match irq domain[%s]\n", PRFX, name);
			matched_any = true;
			pd->ws_names[i] = NULL;	/* blob: str xzr, [x22] */
			break;
		}
	}

	if (!matched_any) {
		if (pd->rescan_retry++ < PDBG_RESCAN_RETRY)
			queue_delayed_work(system_wq, &pd->rescan_work,
					   msecs_to_jiffies(PDBG_RESCAN_MS));
		else
			pr_err("3[%s] ws irq_domain[%u] match error\n", PRFX,
			       pd->rescan_retry);
	}
}

static void pdbg_rescan_worker(struct work_struct *work)
{
	struct sprd_pdbg *pd = container_of(to_delayed_work(work),
					    struct sprd_pdbg, rescan_work);

	pdbg_parse_irq_domain(pd);
}

/* --- kthread ------------------------------------------------------------ */

/* blob: sprd_pdbg_thread @ 0xfd0 */
static int pdbg_thread(void *arg)
{
	struct sprd_pdbg *pd = arg;
	char sources[PDBG_BUF_SZ];

	while (!kthread_should_stop() && READ_ONCE(pd->task)) {
		pdbg_active_wakeup_sources(sources, sizeof(sources));
		pdbg_log("[%s] %s\n", PRFX, sources);

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(pd->poll_ms * 250));
	}
	return 0;
}

/* --- API ekspor --------------------------------------------------------- */

/* blob: sprd_pdbg_msg_print @ 0x30 — va_format, cetak dengan prefiks. */
void sprd_pdbg_msg_print(const char *fmt, ...); /* -Wmissing-prototypes */
void sprd_pdbg_msg_print(const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (!pdbg_data || !pdbg_data->enabled)
		return;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	pdbg_log("[%s] %pV\n", PRFX, &vaf);
	va_end(args);
}
EXPORT_SYMBOL_GPL(sprd_pdbg_msg_print);

/* --- notifier ----------------------------------------------------------- */

/* blob: sprd_pdbg_pm_notifier @ 0x930 */
static int pdbg_pm_notifier_call(struct notifier_block *nb,
				 unsigned long event, void *unused)
{
	struct sprd_pdbg *pd = container_of(nb, struct sprd_pdbg, pm_notifier);

	switch (event) {
	case PM_SUSPEND_PREPARE:
		pd->suspend_start = ktime_get();
		break;
	case PM_POST_SUSPEND: {
		u64 ms;

		if (!pd->suspend_start) {
			pdbg_log("[%s] The system has not yet entered sleep mode\n",
				 PRFX);
			break;
		}
		ms = ktime_to_ms(ktime_sub(ktime_get(), pd->suspend_start));
		pd->suspend_start = 0;
		pdbg_log("[%s] kernel suspend %llums\n", PRFX, ms);
		break;
	}
	default:
		break;
	}
	return NOTIFY_DONE;
}

/* blob: sprd_pdbg_cpu_pm_notifier @ 0x568 */
static int pdbg_cpu_pm_notifier_call(struct notifier_block *nb,
				     unsigned long event, void *unused)
{
	struct sprd_pdbg *pd = container_of(nb, struct sprd_pdbg,
					    cpu_pm_notifier);

	switch (event) {
	case CPU_CLUSTER_PM_ENTER:
		pdbg_log("[%s] #---------PDBG DEEP SLEEP START---------#\n",
			 PRFX);
		pdbg_regs_get(pd, pd->buf, true);
		break;
	case CPU_CLUSTER_PM_EXIT: {
		int ret;

		pdbg_log("[%s] #---------PDBG WAKEUP SCENE END-----------#\n",
			 PRFX);
		ret = pdbg_regs_get_once(pd, PDBG_WAKEUP_SCENE);
		if (ret)
			pdbg_log("[%s] Get pdbg info: %d error\n", PRFX, ret);
		break;
	}
	default:
		break;
	}
	return NOTIFY_DONE;
}

/* --- proc --------------------------------------------------------------- */

static int pdbg_regs_info_show(struct seq_file *m, void *unused)
{
	struct sprd_pdbg *pd = m->private;

	pdbg_regs_get(pd, pd->buf, false);
	seq_printf(m, "%s", pd->buf);
	return 0;
}

static int pdbg_regs_info_open(struct inode *inode, struct file *file)
{
	/* 6.18: PDE_DATA() -> pde_data() */
	return single_open(file, pdbg_regs_info_show, pde_data(inode));
}

static const struct proc_ops pdbg_regs_info_ops = {
	.proc_open = pdbg_regs_info_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* --- lifecycle ---------------------------------------------------------- */

/* blob: sprd_pdbg_release @ 0x458 — lewat devm action saat device dilepas. */
static void pdbg_cleanup(void *res)
{
	struct sprd_pdbg *pd = res;
	struct sprd_pdbg_ws *it, *tmp;

	cancel_delayed_work_sync(&pd->rescan_work);

	write_lock(&pd->ws_lock);
	list_for_each_entry_safe(it, tmp, &pd->ws_list, list) {
		list_del(&it->list);
		kfree(it);
	}
	write_unlock(&pd->ws_lock);

	remove_proc_entry("sprd-pdbg", NULL);
	if (pd->task) {
		kthread_stop(pd->task);
		pd->task = NULL;
	}
	cpu_pm_unregister_notifier(&pd->cpu_pm_notifier);
	unregister_pm_notifier(&pd->pm_notifier);
	pdbg_data = NULL;
	kfree(pd);
}

/* blob: sprd_pdbg_probe @ 0x120 */
static int pdbg_probe(struct platform_device *pdev)
{
	struct sprd_pdbg *pd;
	int i, ret;

	pd = pdbg_data;
	if (!pd) {
		pd = kzalloc(sizeof(*pd), GFP_KERNEL);
		if (!pd)
			return -ENOMEM;
		pdbg_data = pd;
	}

	ret = devm_add_action_or_reset(&pdev->dev, pdbg_cleanup, pd);
	if (ret) {
		pr_err("3[%s] failed to add pdbg devm action\n", PRFX);
		return ret;
	}

	INIT_LIST_HEAD(&pd->ws_list);
	rwlock_init(&pd->ws_lock);
	INIT_DELAYED_WORK(&pd->rescan_work, pdbg_rescan_worker);
	pd->poll_ms = PDBG_POLL_MS;
	pd->dev = &pdev->dev;
	platform_set_drvdata(pdev, pd);

	/* blob: kedua notifier di-set .notifier_call saat inisialisasi. */
	pd->pm_notifier.notifier_call = pdbg_pm_notifier_call;
	pd->cpu_pm_notifier.notifier_call = pdbg_cpu_pm_notifier_call;

	pd->svc = sprd_sip_svc_get_handle();
	if (!pd->svc)
		pr_err("3[%s] fail to get pdbg instance.\n", PRFX);

	for (i = 0; i < ARRAY_SIZE(pd->ws_names); i++)
		of_property_read_string(pdev->dev.of_node, pdbg_prop_names[i],
					&pd->ws_names[i]);

	queue_delayed_work(system_wq, &pd->rescan_work,
			   msecs_to_jiffies(PDBG_RESCAN_MS));

	pd->dir = proc_mkdir("sprd-pdbg", NULL);
	if (!pd->dir) {
		pr_err("3[%s] Proc dir create failed\n", PRFX);
		return -EINVAL;
	}
	if (!proc_create("regs_info", 0444, pd->dir, &pdbg_regs_info_ops)) {
		pr_err("3[%s] Proc file create failed\n", PRFX);
		return -EINVAL;
	}

	pd->task = kthread_create_on_node(pdbg_thread, pd, NUMA_NO_NODE,
					  "sprd-pdbg-thread");
	if (IS_ERR(pd->task)) {
		pr_err("3[%s] Unable to start kernel thread.\n", PRFX);
		pd->task = NULL;
		return -ENOMEM;
	}
	wake_up_process(pd->task);

	ret = cpu_pm_register_notifier(&pd->cpu_pm_notifier);
	if (ret) {
		pr_err("3[%s] cpu_pm_notifier_block register failed!!!\n",
		       PRFX);
		return ret;
	}
	ret = register_pm_notifier(&pd->pm_notifier);
	if (ret) {
		pr_err("3[%s] pm_notifier_block register failed!!!\n", PRFX);
		return ret;
	}

	pd->enabled = true;
	return 0;
}

static const struct of_device_id pdbg_of_match[] = {
	{ .compatible = "sprd,debuglog" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pdbg_of_match);

static struct platform_driver pdbg_driver = {
	.probe = pdbg_probe,
	.driver = {
		.name = "sprd-pdbg",
		.of_match_table = pdbg_of_match,
	},
};

static int __init sprd_pdbg_init(void)
{
	return platform_driver_register(&pdbg_driver);
}
module_init(sprd_pdbg_init);

static void __exit sprd_pdbg_exit(void)
{
	platform_driver_unregister(&pdbg_driver);
}
module_exit(sprd_pdbg_exit);

MODULE_DESCRIPTION("SPRD_PDBG");
MODULE_LICENSE("GPL v2");
