// SPDX-License-Identifier: GPL-2.0
/*
 * berry_m_virtkrn_obs.ko -- "pengamat" BFKRT (Berry Virtual Kernel Observer).
 *
 * Modul ini menjawab dua pertanyaan yang paling sering muncul saat
 * membandingkan kernel asli dengan kernel modifikasi:
 *
 *   "kenapa probe-nya ditunda terus?"  -> event defer+ / defer- + jumlahnya
 *   "apakah mutex-nya salah urut?"     -> kedalaman kunci per task + deteksi
 *                                          pengambilan ulang kunci yang sama
 *                                          (reentran = jalan menuju deadlock)
 *
 * Ia TIDAK mengubah perilaku kernel: hanya membaca argumen lewat kprobe lalu
 * mencatatnya. Seluruh analisis (membandingkan urutan dua sesi, menentukan
 * verdict) dilakukan userspace oleh `bfkrt diff`.
 *
 * Kendali lewat /proc/bfkrt_obs:
 *     echo on > ...   mulai mengamati
 *     echo off > ...  berhenti
 *     echo clr > ...  buang catatan
 *     cat      ...    catatan + ringkasan
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/string.h>
#include "bfkrt_ko.h"

#if defined(CONFIG_ARM64)
#define BF_ARG(r, n) ((r)->regs[(n)])
#define BF_PC(r)     ((r)->pc)
#elif defined(CONFIG_X86_64)
#define BF_ARG(r, n) (((n) == 0) ? (r)->di : ((n) == 1) ? (r)->si : 0)
#define BF_PC(r)     ((r)->ip)
#else
#error "berry_m_virtkrn_obs: tambahkan makro register untuk arsitektur ini"
#endif

enum ev {
	EV_LOCK = 1,
	EV_UNLOCK = 2,
	EV_REENTRAN = 3,
	EV_DEFER_ADD = 4,
	EV_DEFER_DEL = 5,
	EV_DEPTH_PENUH = 6,
};

struct bf_ev {
	u64 ts;
	u64 arg;
	u64 pc;
	u32 pid;
	u16 type;
	u16 depth;
	char nama[40];
};

static struct bf_ev bf_ring[BFKRT_RING];
static DEFINE_SPINLOCK(bf_lock);
static u64 bf_head;
static u64 bf_snap;
static atomic_t bf_on = ATOMIC_INIT(0);
static unsigned int bf_reentran, bf_depth_penuh, bf_defer_n;

/* --- status kunci per task (tabel statis, tanpa alokasi) --- */
#define BF_MAXDEP 8
#define BF_NTASK  128

struct bf_task {
	struct task_struct *t;
	pid_t pid;
	u16 depth;
	u16 reent;
	void *stk[BF_MAXDEP];
};

static struct bf_task bf_tasks[BF_NTASK];
static DEFINE_SPINLOCK(bf_tlock);

static struct bf_task *bf_lookup(struct task_struct *t, pid_t pid)
{
	int i, kosong = -1;

	for (i = 0; i < BF_NTASK; i++) {
		if (bf_tasks[i].t == t && bf_tasks[i].pid == pid)
			return &bf_tasks[i];
		if (bf_tasks[i].t == NULL && kosong < 0)
			kosong = i;
	}
	if (kosong < 0)
		kosong = (int)(bf_head % BF_NTASK); /* tabel penuh: pakai slot acak */
	bf_tasks[kosong].t = t;
	bf_tasks[kosong].pid = pid;
	bf_tasks[kosong].depth = 0;
	bf_tasks[kosong].reent = 0;
	return &bf_tasks[kosong];
}

static void bf_catat(u16 type, u64 arg, u64 pc, u32 pid, u16 depth, const char *nama)
{
	struct bf_ev *e = &bf_ring[bf_head % BFKRT_RING];

	e->ts = ktime_get_ns();
	e->arg = arg;
	e->pc = pc;
	e->pid = pid;
	e->type = type;
	e->depth = depth;
	if (nama)
		strscpy(e->nama, nama, sizeof(e->nama));
	else
		e->nama[0] = 0;
	bf_head++;
}

/* probe dideklarasikan dulu: handler merujuk nama-nama ini */
static struct kprobe kp_lock = { .symbol_name = "mutex_lock", .pre_handler = NULL };
static struct kprobe kp_unlock = { .symbol_name = "mutex_unlock", .pre_handler = NULL };
static struct kprobe kp_dadd = { .symbol_name = "driver_deferred_probe_add", .pre_handler = NULL };
static struct kprobe kp_ddel = { .symbol_name = "driver_deferred_probe_del", .pre_handler = NULL };

static int obs_pre(struct kprobe *p, struct pt_regs *regs)
{
	unsigned long flags;
	u64 arg = BF_ARG(regs, 0);
	u32 pid = task_pid_nr(current);
	void *m = (void *)(unsigned long)arg;
	u16 dep;
	struct bf_task *bt;

	if (!atomic_read(&bf_on))
		return 0;

	/* --- deferred probe: catat nama perangkat --- */
	if (p == &kp_dadd || p == &kp_ddel) {
		const char *nama = NULL;
		struct device *dev = (struct device *)m;

		if (dev && dev->kobj.name)
			nama = dev->kobj.name;
		spin_lock_irqsave(&bf_lock, flags);
		bf_catat(p == &kp_dadd ? EV_DEFER_ADD : EV_DEFER_DEL,
			 arg, BF_PC(regs), pid, 0, nama);
		if (p == &kp_dadd)
			bf_defer_n++;
		spin_unlock_irqrestore(&bf_lock, flags);
		return 0;
	}

	/* --- mutex: pantau kedalaman kunci milik task ini --- */
	spin_lock_irqsave(&bf_tlock, flags);
	bt = bf_lookup(current, pid);
	if (p == &kp_lock) {
		int i;

		for (i = 0; i < bt->depth; i++) {
			if (bt->stk[i] == m) {
				bt->reent++;
				spin_lock_irqsave(&bf_lock, flags);
				bf_catat(EV_REENTRAN, arg, BF_PC(regs), pid, bt->depth, "mutex");
				spin_unlock_irqrestore(&bf_lock, flags);
				bf_reentran++;
				break;
			}
		}
		if (bt->depth >= BF_MAXDEP) {
			spin_lock_irqsave(&bf_lock, flags);
			bf_catat(EV_DEPTH_PENUH, arg, BF_PC(regs), pid, bt->depth, "mutex");
			spin_unlock_irqrestore(&bf_lock, flags);
			bf_depth_penuh++;
		} else {
			bt->stk[bt->depth++] = m;
		}
	} else if (bt->depth > 0) {
		int i;

		for (i = bt->depth - 1; i >= 0; i--) {
			if (bt->stk[i] == m) {
				bt->depth = (u16)i;
				break;
			}
		}
	}
	dep = bt->depth;
	spin_unlock_irqrestore(&bf_tlock, flags);

	spin_lock_irqsave(&bf_lock, flags);
	bf_catat(p == &kp_lock ? EV_LOCK : EV_UNLOCK, arg, BF_PC(regs), pid, dep, "mutex");
	spin_unlock_irqrestore(&bf_lock, flags);
	return 0;
}

static struct kprobe *const bf_kps[] = {
	&kp_lock, &kp_unlock, &kp_dadd, &kp_ddel
};
static bool bf_ok[ARRAY_SIZE(bf_kps)];

static const char *ev_name(u16 t)
{
	switch (t) {
	case EV_LOCK: return "lock";
	case EV_UNLOCK: return "unlock";
	case EV_REENTRAN: return "REENTRAN";
	case EV_DEFER_ADD: return "defer+";
	case EV_DEFER_DEL: return "defer-";
	case EV_DEPTH_PENUH: return "DEPTH_PENUH";
	default: return "?";
	}
}

/*
 * seq_file: posisi 0..total-1 = catatan, posisi total = ringkasan (dipakai
 * sebagai penanda khusus bernilai 1). Catatan dikodekan idx+2, jadi nilai 1
 * tidak pernah menabrak catatan mana pun.
 */
static void *obs_start(struct seq_file *m, loff_t *pos)
{
	u64 total, base;

	if (*pos == 0)
		bf_snap = bf_head;
	total = bf_snap < BFKRT_RING ? bf_snap : BFKRT_RING;
	base = bf_snap - total;
	if (*pos < total)
		return (void *)((unsigned long)(base + *pos + 2));
	if (*pos == total)
		return (void *)1UL;
	return NULL;
}

static void *obs_next(struct seq_file *m, void *v, loff_t *pos)
{
	u64 total, base;

	total = bf_snap < BFKRT_RING ? bf_snap : BFKRT_RING;
	base = bf_snap - total;
	(*pos)++;
	if (*pos < total)
		return (void *)((unsigned long)(base + *pos + 2));
	if (*pos == total)
		return (void *)1UL;
	return NULL;
}

static void obs_stop(struct seq_file *m, void *v)
{
}

static int obs_show(struct seq_file *m, void *v)
{
	unsigned long raw = (unsigned long)v;

	if (raw == 1) {
		seq_printf(m, "--- ringkasan ---\n");
		seq_printf(m, "reentran  : %u kali (indikasi deadlock)\n", bf_reentran);
		seq_printf(m, "kedalaman : %u kali melebihi %d\n", bf_depth_penuh, BF_MAXDEP);
		seq_printf(m, "deferred  : %u kali ditambahkan\n", bf_defer_n);
		seq_printf(m, "catatan   : %llu total\n", bf_head);
		return 0;
	} else {
		u64 idx = raw - 2;
		const struct bf_ev *e = &bf_ring[idx % BFKRT_RING];

		seq_printf(m, "%llu %s pid=%u dep=%u arg=0x%llx pc=0x%llx",
			   e->ts / 1000, ev_name(e->type), e->pid, e->depth,
			   (unsigned long long)e->arg, (unsigned long long)e->pc);
		if (e->nama[0])
			seq_printf(m, " %s", e->nama);
		seq_putc(m, '\n');
		return 0;
	}
}

static const struct seq_operations obs_sops = {
	.start = obs_start,
	.next  = obs_next,
	.stop  = obs_stop,
	.show  = obs_show,
};

static ssize_t obs_write(struct file *f, const char __user *buf,
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
		atomic_set(&bf_on, 1);
	} else if (sysfs_streq(cmd, "off")) {
		atomic_set(&bf_on, 0);
	} else if (sysfs_streq(cmd, "clr")) {
		unsigned long flags;

		spin_lock_irqsave(&bf_lock, flags);
		bf_head = 0;
		bf_reentran = bf_depth_penuh = bf_defer_n = 0;
		memset(bf_tasks, 0, sizeof(bf_tasks));
		spin_unlock_irqrestore(&bf_lock, flags);
	} else {
		return -EINVAL;
	}
	return count;
}

static int obs_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &obs_sops);
}

static const struct proc_ops obs_ops = {
	.proc_open    = obs_open,
	.proc_read    = seq_read,
	.proc_write   = obs_write,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release,
};

static int __init bfkrt_init(void)
{
	int i, good = 0;

	kp_lock.pre_handler = obs_pre;
	kp_unlock.pre_handler = obs_pre;
	kp_dadd.pre_handler = obs_pre;
	kp_ddel.pre_handler = obs_pre;

	for (i = 0; i < ARRAY_SIZE(bf_kps); i++) {
		if (register_kprobe(bf_kps[i]) == 0) {
			bf_ok[i] = true;
			good++;
		}
	}
	if (!good) {
		pr_err("virtkrn_obs: tidak ada simbol yang bisa diprobes\n");
		return -ENOENT;
	}
	if (!proc_create(BFKRT_PROC_OBS, 0644, NULL, &obs_ops)) {
		for (i = 0; i < ARRAY_SIZE(bf_kps); i++)
			if (bf_ok[i])
				unregister_kprobe(bf_kps[i]);
		return -ENOMEM;
	}
	pr_info("virtkrn_obs: %d probe terpasang (mutex + deferred probe)\n", good);
	return 0;
}

static void __exit bfkrt_exit(void)
{
	int i;

	atomic_set(&bf_on, 0);
	for (i = 0; i < ARRAY_SIZE(bf_kps); i++)
		if (bf_ok[i])
			unregister_kprobe(bf_kps[i]);
	remove_proc_entry(BFKRT_PROC_OBS, NULL);
	pr_info("virtkrn_obs: dilepas\n");
}

module_init(bfkrt_init);
module_exit(bfkrt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BFKRT");
MODULE_DESCRIPTION("Berry Fetching Kernel Runtime -- pengamat urutan & status kunci");
