// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * bldb_core.c -- BLDB (Berry Linux Debug Bridge), inti kernel.
 *
 * Copyright (c) 2026 Berry Lab Foundation
 * License: Berry Foundation Public v1 - MIT
 *
 * SOFTWARE BEBAS, TAPI PERANGKATNYA TIDAK -- BACA DULU:
 *  Modul ini menambah KONSOL printk yang menyalin SEMUA log ke ring buffer
 *  statis 16 MiB, lalu mengeksposnya lewat misc device /dev/bldb0.  Dua arah:
 *    PC -> tablet : daemon bldbd menulis perintah ke /dev/bldb0,
 *                    hasilnya dicetak balik ke ring -> terbaca di PC.
 *    tablet -> PC : daemon membaca /dev/bldb0 (ring) lalu meneruskannya ke
 *                    port serial USB (configfs ACM -> /dev/ttyGS0).
 *  Kenapa 16 MiB: banjir deferred probe + initcall_debug bisa ratusan ribu
 *  baris; ring harus menampung SELURUH banjir sejak detik ke-0 supaya bisa
 *  disedot PC begitu koneksi USB hidup, bukan hanya yang terakhir.
 *
 *  MITIGASI RING-0 (keputusan sadar, bukan kebetulan):
 *  - Buffer STATIS di .bss: tanpa kmalloc/vmalloc, tanpa page fault di
 *    jalur console (console write sering dipanggil dengan IRQ off).
 *  - Satu spinlock irqsave; TANPA pr_*/printk di dalam write callback
 *    (printk dari printk = rekursi mati).
 *  - peek32/poke32 hanya boleh menyentuh RAM valid (pfn_valid) dan DILARANG
 *    menembak teks kernel (diukur dari alamat FISIK _stext.._end, jadi tetap
 *    benar walau KASLR aktif) atau isi ring ini sendiri.
 *  - Bila pembaca tertinggal, data terlama dibuang dengan penanda overrun;
 *    printk tidak pernah diblokir menunggu pembaca.
 *
 * Independent creation by Berry Lab Foundation.  Tidak memuat, menautkan,
 * atau diturunkan dari kode proprietary Unisoc/Spreadtrum; hanya memakai
 * API standar Linux (register_console, miscdevice, pfn_valid).
 */

#include <linux/module.h>
#include <linux/console.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/sections.h>	/* _stext, _end */

#define BLDB_RING_SIZE	(16 * 1024 * 1024)
#define BLDB_CMD_MAX	512

static char bldb_ring[BLDB_RING_SIZE] __aligned(PAGE_SIZE);
static size_t bldb_head;		/* posisi tulis berikutnya	*/
static size_t bldb_read_pos;		/* posisi baca berikutnya	*/
static u64 bldb_total;			/* byte total pernah ditulis	*/
static u32 bldb_overruns;		/* kali ring membuang data lama	*/
static u32 bldb_printk_calls;		/* jumlah panggilan console	*/
static DEFINE_SPINLOCK(bldb_lock);

/*
 * Tulis mentah ke ring.  PEMANGGIL HARUS SUDAH memegang bldb_lock.
 * Jika data tak muat, data terlama dibuang (drop-oldest) dan overrun
 * dihitung -- ring tidak pernah menolak tulisan console.
 */
static void bldb_put(const char *buf, size_t len)
{
	size_t new_head, unread, first;

	if (!len)
		return;
	if (len >= BLDB_RING_SIZE) {
		/* sisakan 64 byte penanda saja: ambil ekor */
		buf += len - (BLDB_RING_SIZE - 64);
		len = BLDB_RING_SIZE - 64;
	}

	unread = (bldb_head - bldb_read_pos) % BLDB_RING_SIZE;
	if (unread + len > BLDB_RING_SIZE - 64) {
		size_t drop = (unread + len) - (BLDB_RING_SIZE - 64);

		bldb_read_pos = (bldb_read_pos + drop) % BLDB_RING_SIZE;
		bldb_overruns++;
	}

	new_head = bldb_head + len;
	if (new_head >= BLDB_RING_SIZE)
		new_head -= BLDB_RING_SIZE;

	first = BLDB_RING_SIZE - bldb_head;
	if (first > len)
		first = len;
	memcpy(bldb_ring + bldb_head, buf, first);
	if (len > first)
		memcpy(bldb_ring, buf + first, len - first);

	bldb_head = new_head;
	bldb_total += len;
}

/* ---- konsol printk ---------------------------------------------------- */
static void bldb_console_write(struct console *co, const char *buf,
			       unsigned int len)
{
	unsigned long flags;

	spin_lock_irqsave(&bldb_lock, flags);
	bldb_printk_calls++;
	bldb_put(buf, len);
	spin_unlock_irqrestore(&bldb_lock, flags);
}

static struct console bldb_console = {
	.name	= "bldb",
	.write	= bldb_console_write,
	.flags	= CON_ENABLED | CON_PRINTBUFFER,
	.index	= -1,
};

/* Masukkan satu baris hasil perintah ke ring (kembali ke PC). */
static void bldb_emit(const char *s)
{
	unsigned long flags;
	size_t n = strlen(s);

	spin_lock_irqsave(&bldb_lock, flags);
	bldb_put(s, n);
	spin_unlock_irqrestore(&bldb_lock, flags);
}

/* ---- perintah (dari PC lewat daemon) ---------------------------------- */

/* Baca RAM fisik hanya bila benar-benar RAM (pfn_valid), bukan MMIO. */
static int bldb_ram_ok(unsigned long addr, size_t bytes)
{
	unsigned long pfn = addr >> PAGE_SHIFT;
	unsigned long end_pfn = (addr + bytes - 1) >> PAGE_SHIFT;

	if (addr & (bytes - 1))
		return 0;		/* akses harus sejajar ukurannya */
	if (!pfn_valid(pfn) || !pfn_valid(end_pfn))
		return 0;
	return 1;
}

/*
 * Zona larang poke: teks+data kernel (dihitung dari alamat FISIK simbol,
 * sehingga benar juga saat KASLR) dan isi ring ini sendiri.
 */
static int bldb_target_forbidden(unsigned long addr, size_t bytes)
{
	unsigned long text_phys = (unsigned long)__pa_symbol(_stext);
	unsigned long end_phys = (unsigned long)__pa_symbol(_end);
	unsigned long ring_phys = (unsigned long)__pa_symbol(bldb_ring);

	if (addr + bytes > text_phys && addr < end_phys)
		return 1;
	if (addr + bytes > ring_phys && addr < ring_phys + BLDB_RING_SIZE)
		return 1;
	return 0;
}

static void bldb_handle_cmd(char *cmd)
{
	char line[256];
	char *arg;
	unsigned long addr, val;
	size_t n = strlen(cmd);

	while (n && (cmd[n - 1] == '\n' || cmd[n - 1] == '\r'))
		cmd[--n] = '\0';

	arg = strchr(cmd, ' ');
	if (arg) {
		*arg++ = '\0';
		while (*arg == ' ')
			arg++;
	}

	if (!strcmp(cmd, "help") || !strcmp(cmd, "?")) {
		bldb_emit("[BLDB] perintah: help stats clear mark peek32 poke32\n");
		bldb_emit("[BLDB] peek32 <hex> -> baca RAM (ditolak di luar RAM)\n");
		bldb_emit("[BLDB] poke32 <hex> <hex> -> tulis RAM (ditolak teks kernel/ring)\n");
	} else if (!strcmp(cmd, "stats")) {
		scnprintf(line, sizeof(line),
			  "[BLDB] total=%llu head=%zu read=%zu overrun=%u printk=%u ring=%dMiB\n",
			  bldb_total, bldb_head, bldb_read_pos,
			  bldb_overruns, bldb_printk_calls,
			  BLDB_RING_SIZE / (1024 * 1024));
		bldb_emit(line);
	} else if (!strcmp(cmd, "clear")) {
		unsigned long flags;

		spin_lock_irqsave(&bldb_lock, flags);
		bldb_read_pos = bldb_head;
		spin_unlock_irqrestore(&bldb_lock, flags);
		bldb_emit("[BLDB] ring dibersihkan (posisi baca = tulis)\n");
	} else if (!strcmp(cmd, "mark")) {
		scnprintf(line, sizeof(line), "[BLDB:MARK] %s\n",
			  arg ? arg : "");
		bldb_emit(line);
	} else if (!strcmp(cmd, "peek32")) {
		if (!arg || kstrtoul(arg, 16, &addr)) {
			bldb_emit("[BLDB] ERR pakai: peek32 <hex>\n");
		} else if (!bldb_ram_ok(addr, 4) ||
			   bldb_target_forbidden(addr, 4)) {
			scnprintf(line, sizeof(line),
				  "[BLDB] ERR 0x%lx bukan RAM aman\n", addr);
			bldb_emit(line);
		} else {
			u32 v = *(volatile u32 *)phys_to_virt(addr);

			scnprintf(line, sizeof(line),
				  "[BLDB] [0x%lx] = 0x%08x\n", addr, v);
			bldb_emit(line);
		}
	} else if (!strcmp(cmd, "poke32")) {
		char *vstr = arg ? strchr(arg, ' ') : NULL;

		if (!arg || !vstr || kstrtoul(arg, 16, &addr)) {
			bldb_emit("[BLDB] ERR pakai: poke32 <hex_addr> <hex_val>\n");
			return;
		}
		*vstr++ = '\0';
		while (*vstr == ' ')
			vstr++;
		if (kstrtoul(vstr, 16, &val)) {
			bldb_emit("[BLDB] ERR nilai bukan hex\n");
			return;
		}
		if (!bldb_ram_ok(addr, 4) || bldb_target_forbidden(addr, 4)) {
			scnprintf(line, sizeof(line),
				  "[BLDB] ERR 0x%lx zona larang (RAM/teks/ring)\n",
				  addr);
			bldb_emit(line);
			return;
		}
		*(volatile u32 *)phys_to_virt(addr) = (u32)val;
		scnprintf(line, sizeof(line),
			  "[BLDB] tulis [0x%lx] = 0x%08x OK\n", addr, (u32)val);
		bldb_emit(line);
	} else if (cmd[0]) {
		scnprintf(line, sizeof(line),
			  "[BLDB] perintah tak dikenal: %s (ketik help)\n", cmd);
		bldb_emit(line);
	}
}

/* ---- misc device /dev/bldb0 ------------------------------------------ */
static ssize_t bldb_read(struct file *filp, char __user *ubuf,
			 size_t len, loff_t *ppos)
{
	size_t copied = 0;
	char *tmp;
	unsigned long flags;
	size_t n;

	if (!len)
		return 0;
	if (len > 1024 * 1024)
		len = 1024 * 1024;	/* batas per bacaan, alokasi kecil */
	tmp = kmalloc(len, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	spin_lock_irqsave(&bldb_lock, flags);
	while (copied < len) {
		if (bldb_read_pos == bldb_head)
			break;
		n = (bldb_head > bldb_read_pos) ?
			bldb_head - bldb_read_pos :
			BLDB_RING_SIZE - bldb_read_pos;
		if (n > len - copied)
			n = len - copied;
		memcpy(tmp + copied, bldb_ring + bldb_read_pos, n);
		bldb_read_pos = (bldb_read_pos + n) % BLDB_RING_SIZE;
		copied += n;
	}
	spin_unlock_irqrestore(&bldb_lock, flags);

	if (!copied) {
		kfree(tmp);
		return 0;		/* antrean kosong: non-blocking */
	}
	if (copy_to_user(ubuf, tmp, copied)) {
		kfree(tmp);
		return -EFAULT;
	}
	kfree(tmp);
	return copied;
}

static ssize_t bldb_write(struct file *filp, const char __user *ubuf,
			  size_t len, loff_t *ppos)
{
	char cmd[BLDB_CMD_MAX];

	if (!len)
		return 0;
	if (len >= sizeof(cmd))
		len = sizeof(cmd) - 1;
	if (copy_from_user(cmd, ubuf, len))
		return -EFAULT;
	cmd[len] = '\0';
	bldb_handle_cmd(cmd);
	return len;
}

static const struct file_operations bldb_fops = {
	.owner	= THIS_MODULE,
	.read	= bldb_read,
	.write	= bldb_write,
	.llseek	= no_llseek,
};

static struct miscdevice bldb_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "bldb0",
	.fops	= &bldb_fops,
	.mode	= 0600,
};

static int __init bldb_init(void)
{
	int ret;

	ret = register_console(&bldb_console);
	if (ret)
		pr_warn("bldb: register_console gagal (%d), ring tetap jalan\n",
			ret);
	ret = misc_register(&bldb_misc);
	if (ret) {
		pr_err("bldb: misc_register gagal (%d)\n", ret);
		return ret;
	}
	pr_info("bldb: BLDB siap -- ring %d MiB di /dev/bldb0 (Berry Lab Foundation)\n",
		BLDB_RING_SIZE / (1024 * 1024));
	return 0;
}
core_initcall(bldb_init);

MODULE_AUTHOR("Berry Lab Foundation");
MODULE_DESCRIPTION("Berry Linux Debug Bridge - early boot debug ring");
MODULE_LICENSE("GPL");
