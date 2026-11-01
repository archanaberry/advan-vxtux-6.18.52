// SPDX-License-Identifier: GPL-2.0
/*
 * berryfetcher_kernel.ko -- "pintu masuk ruang kernel" milik BFKRT.
 *
 * Tugasnya kecil dan sengaja tidak melakukan apa pun yang berbahaya:
 *   /proc/bfkrt_info   -- release, banner, umur sistem, status resolver simbol
 *   /proc/bfkrt_sym    -- tulis nama simbol, baca alamatnya (kallsyms)
 *
 * Pemecahan masalah klasik Android 5.7+: kallsyms_lookup_name tidak lagi
 * di-export ke modul. Caranya kita pasang kprobe sesaat pada simbol itu,
 * membaca alamatnya dari struct kprobe (p->addr), lalu melepas kprobe lagi.
 * Tidak ada patching permanen, tidak ada yang berubah setelah init selesai.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/jiffies.h>
#include "bfkrt_ko.h"

static unsigned long (*bf_kln)(const char *name);
static char bf_sym[64];

static int kln_pre(struct kprobe *p, struct pt_regs *regs)
{
	/* p->addr = alamat kallsyms_lookup_name pada kernel yang sedang jalan */
	bf_kln = (unsigned long (*)(const char *))p->addr;
	return 0;
}

static struct kprobe kln_kp = {
	.symbol_name = "kallsyms_lookup_name",
	.pre_handler = kln_pre,
};

static int info_show(struct seq_file *m, void *v)
{
	seq_printf(m, "modul   : berryfetcher_kernel\n");
	seq_printf(m, "release : %s\n", init_utsname()->release);
	seq_printf(m, "banner  : %s\n", init_utsname()->version);
	seq_printf(m, "uptime  : %lu detik\n", jiffies / HZ);
	seq_printf(m, "resolver: %s\n", bf_kln ? "siap" : "tidak tersedia");
	seq_printf(m, "cara    : tulis nama simbol ke /proc/bfkrt_sym, lalu baca lagi\n");
	return 0;
}

static ssize_t sym_read(struct file *f, char __user *buf, size_t count, loff_t *ppos)
{
	char kbuf[128];
	int len;

	if (!bf_sym[0]) {
		len = scnprintf(kbuf, sizeof(kbuf),
				"(kosong) tulis nama simbol ke /proc/bfkrt_sym\n");
	} else if (!bf_kln) {
		len = scnprintf(kbuf, sizeof(kbuf),
				"%s = ? (kallsyms_lookup_name tidak terjangkau)\n", bf_sym);
	} else {
		unsigned long a = bf_kln(bf_sym);
		len = scnprintf(kbuf, sizeof(kbuf), "%s = 0x%lx%s\n",
				bf_sym, a, a ? "" : " (tidak ketemu)");
	}
	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static ssize_t sym_write(struct file *f, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	char tmp[64];
	size_t n;
	ssize_t r;

	memset(tmp, 0, sizeof(tmp));
	n = count < sizeof(tmp) - 1 ? count : sizeof(tmp) - 1;
	r = simple_write_to_buffer(tmp, sizeof(tmp), ppos, buf, n);
	if (r < 0)
		return r;
	tmp[sizeof(tmp) - 1] = 0;
	strscpy(bf_sym, tmp, sizeof(bf_sym));
	strim(bf_sym);
	return r;
}

static const struct proc_ops sym_ops = {
	.proc_read  = sym_read,
	.proc_write = sym_write,
};

static int __init bfkrt_init(void)
{
	int ret;

	ret = register_kprobe(&kln_kp);
	if (ret == 0)
		unregister_kprobe(&kln_kp);
	else
		pr_info("berryfetcher: kallsyms_lookup_name tidak terjangkau (%d)\n", ret);

	if (!proc_create_single(BFKRT_PROC_INFO, 0444, NULL, info_show)) {
		pr_err("berryfetcher: gagal buat /proc/%s\n", BFKRT_PROC_INFO);
		return -ENOMEM;
	}
	if (!proc_create(BFKRT_PROC_SYM, 0644, NULL, &sym_ops)) {
		remove_proc_entry(BFKRT_PROC_INFO, NULL);
		pr_err("berryfetcher: gagal buat /proc/%s\n", BFKRT_PROC_SYM);
		return -ENOMEM;
	}
	pr_info("berryfetcher: siap (resolver %s)\n", bf_kln ? "aktif" : "pasif");
	return 0;
}

static void __exit bfkrt_exit(void)
{
	remove_proc_entry(BFKRT_PROC_SYM, NULL);
	remove_proc_entry(BFKRT_PROC_INFO, NULL);
	pr_info("berryfetcher: dilepas\n");
}

module_init(bfkrt_init);
module_exit(bfkrt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BFKRT");
MODULE_DESCRIPTION("Berry Fetching Kernel Runtime -- fetcher ruang kernel");
