// SPDX-License-Identifier: GPL-2.0
/*
 * berry_trig_all.ko -- perekam register BFKRT.
 *
 * Menempelkan kprobe pada fungsi-fungsi CORE kernel (bukan driver vendor),
 * sehingga modul ini bisa dipakai di perangkat apa pun: regulator, clock,
 * i2c, spi, PM runtime. Setiap kali fungsi itu dipanggil, isi register CPU
 * pada saat itu disimpan ke ring statis:
 *
 *     ts, pc, sp, lr, x0..x7, pid, cpu, simbol
 *
 * Lalu dibaca lewat /proc/bfkrt_trig:
 *     echo on   > /proc/bfkrt_trig   mulai merekam
 *     echo off  > /proc/bfkrt_trig   berhenti
 *     echo clr  > /proc/bfkrt_trig   buang isi ring
 *     cat  /proc/bfkrt_trig          dump (128 rekaman terakhir)
 *
 * Aturan ring-0: handler hanya menulis memori + spinlock. Tanpa kmalloc,
 * tanpa printk, tanpa tidur -- itulah syarat supaya aman dijalankan di
 * konteks kprobe (preemption off, bisa terjadi di interrupt context).
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include "bfkrt_ko.h"

/* --- akses register per arsitektur (satu-satunya bagian non-portable) --- */
#if defined(CONFIG_ARM64)
#define BF_ARG(r, n) ((r)->regs[(n)])
#define BF_PC(r)     ((r)->pc)
#define BF_SP(r)     ((r)->sp)
#define BF_LR(r)     ((r)->regs[30])
#elif defined(CONFIG_X86_64)
#define BF_ARG(r, n) (((n) == 0) ? (r)->di : ((n) == 1) ? (r)->si : \
		      ((n) == 2) ? (r)->dx : ((n) == 3) ? (r)->cx : 0)
#define BF_PC(r)     ((r)->ip)
#define BF_SP(r)     ((r)->sp)
#define BF_LR(r)     0
#else
#error "berry_trig_all: tambahkan makro register untuk arsitektur ini"
#endif

struct bf_rec {
	u64 ts;
	u64 pc;
	u64 sp;
	u64 lr;
	u64 x[8];
	u32 pid;
	u32 cpu;
	u16 sym;
	u16 res;
};

static struct bf_rec bf_ring[BFKRT_RING];
static DEFINE_SPINLOCK(bf_lock);
static u64 bf_head;		/* jumlah total rekaman (boleh > RING) */
static u64 bf_snap;		/* jumlah saat pembacaan dimulai (jangan kejar ring yang jalan) */
static atomic_t bf_on = ATOMIC_INIT(0);
static unsigned int bf_fail;	/* simbol yang tidak ada di kernel ini */

static const char *const bf_syms[] = {
	"regulator_enable",
	"regulator_disable",
	"clk_prepare_enable",
	"clk_disable_unprepare",
	"clk_set_rate",
	"i2c_transfer",
	"spi_sync",
	"pm_runtime_get_sync",
	"pm_runtime_put_sync",
	"driver_probe_device",
	"driver_deferred_probe_add",
};

static struct kprobe bf_kps[ARRAY_SIZE(bf_syms)];
static bool bf_ok[ARRAY_SIZE(bf_syms)];

static int bf_pre(struct kprobe *p, struct pt_regs *regs)
{
	unsigned long flags;
	struct bf_rec *r;
	u64 i;
	int n;

	if (!atomic_read(&bf_on))
		return 0;

	spin_lock_irqsave(&bf_lock, flags);
	i = bf_head;
	r = &bf_ring[i % BFKRT_RING];
	r->ts = ktime_get_ns();
	r->pc = BF_PC(regs);
	r->sp = BF_SP(regs);
	r->lr = BF_LR(regs);
	for (n = 0; n < 8; n++)
		r->x[n] = BF_ARG(regs, n);
	r->pid = task_pid_nr(current);
	r->cpu = raw_smp_processor_id();
	r->sym = (u16)(p - bf_kps);
	r->res = 0;
	bf_head = i + 1;
	spin_unlock_irqrestore(&bf_lock, flags);
	return 0;
}

static void *trig_start(struct seq_file *m, loff_t *pos)
{
	u64 total;

	if (*pos == 0)
		bf_snap = bf_head;	/* kunci jumlah pada awal pembacaan */
	total = bf_snap < BFKRT_RING ? bf_snap : BFKRT_RING;
	if (*pos >= total)
		return NULL;
	/* indeks global = (bf_snap - total) + pos, disimpan sebagai ptr+1 */
	return (void *)((unsigned long)((bf_snap - total) + *pos + 1));
}

static void *trig_next(struct seq_file *m, void *v, loff_t *pos)
{
	u64 idx = (u64)(unsigned long)v - 1;
	u64 stop = bf_snap;

	(*pos)++;
	if (idx + 1 >= stop)
		return NULL;
	return (void *)((unsigned long)(idx + 2));
}

static void trig_stop(struct seq_file *m, void *v)
{
}

static int trig_show(struct seq_file *m, void *v)
{
	u64 idx = (u64)(unsigned long)v - 1;
	const struct bf_rec *r = &bf_ring[idx % BFKRT_RING];
	u16 s = r->sym < ARRAY_SIZE(bf_syms) ? r->sym : 0;

	seq_printf(m, "%llu %s pid=%u cpu=%u pc=0x%llx sp=0x%llx lr=0x%llx",
		   r->ts / 1000, bf_syms[s], r->pid, r->cpu,
		   (unsigned long long)r->pc,
		   (unsigned long long)r->sp,
		   (unsigned long long)r->lr);
	seq_printf(m, " x0=0x%llx x1=0x%llx x2=0x%llx x3=0x%llx\n",
		   (unsigned long long)r->x[0], (unsigned long long)r->x[1],
		   (unsigned long long)r->x[2], (unsigned long long)r->x[3]);
	return 0;
}

static const struct seq_operations trig_sops = {
	.start = trig_start,
	.next  = trig_next,
	.stop  = trig_stop,
	.show  = trig_show,
};

static ssize_t trig_write(struct file *f, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	char cmd[8] = {0};

	if (count == 0)
		return 0;
	if (count >= sizeof(cmd))
		count = sizeof(cmd) - 1;
	if (copy_from_user(cmd, buf, count))
		return -EFAULT;

	if (sysfs_streq(cmd, "on")) {
		bf_head = 0;
		atomic_set(&bf_on, 1);
	} else if (sysfs_streq(cmd, "off")) {
		atomic_set(&bf_on, 0);
	} else if (sysfs_streq(cmd, "clr")) {
		bf_head = 0;
	} else {
		return -EINVAL;
	}
	return count;
}

static int trig_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &trig_sops);
}

static const struct proc_ops trig_ops = {
	.proc_open    = trig_open,
	.proc_read    = seq_read,
	.proc_write   = trig_write,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release,
};

static int __init bfkrt_init(void)
{
	int i, good = 0;

	for (i = 0; i < ARRAY_SIZE(bf_kps); i++) {
		bf_kps[i].symbol_name = bf_syms[i];
		bf_kps[i].pre_handler = bf_pre;
		if (register_kprobe(&bf_kps[i]) == 0) {
			bf_ok[i] = true;
			good++;
		} else {
			bf_fail++;
		}
	}

	if (!good) {
		pr_err("berry_trig: tidak ada satu pun simbol yang bisa diprobes\n");
		for (i = 0; i < ARRAY_SIZE(bf_kps); i++)
			if (bf_ok[i])
				unregister_kprobe(&bf_kps[i]);
		return -ENOENT;
	}
	if (!proc_create(BFKRT_PROC_TRIG, 0644, NULL, &trig_ops)) {
		for (i = 0; i < ARRAY_SIZE(bf_kps); i++)
			if (bf_ok[i])
				unregister_kprobe(&bf_kps[i]);
		return -ENOMEM;
	}
	pr_info("berry_trig: %d simbol terpasang, %d tidak ada\n", good, bf_fail);
	return 0;
}

static void __exit bfkrt_exit(void)
{
	int i;

	atomic_set(&bf_on, 0);
	for (i = 0; i < ARRAY_SIZE(bf_kps); i++)
		if (bf_ok[i])
			unregister_kprobe(&bf_kps[i]);
	remove_proc_entry(BFKRT_PROC_TRIG, NULL);
	pr_info("berry_trig: dilepas\n");
}

module_init(bfkrt_init);
module_exit(bfkrt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BFKRT");
MODULE_DESCRIPTION("Berry Fetching Kernel Runtime -- perekam register fungsi core");
