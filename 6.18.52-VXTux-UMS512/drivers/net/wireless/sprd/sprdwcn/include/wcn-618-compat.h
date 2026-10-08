/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sprdwcn/include/wcn-618-compat.h — jembatan API 5.4 -> 6.18
 *
 * Port GPL: drivers/misc/sprdwcn (fork Samsung Tab A8, T618/UMS512)
 *   -> drivers/vxtux/net/wireless/sprdwcn (kernel 6.18 VXTux)
 *
 * Prinsip port (sama dengan batch B4/B5):
 *   - HANYA shim yang benar-benar dibutuhkan oleh berkas yang DIBANGUN.
 *   - Tidak menambal subsistem lain; semua di dalam drivers/vxtux/.
 *   - Jalur PCIE dan TRUSTY tidak dibangun di papan marlin3 (SDIO),
 *     jadi API keduanya TIDAK di-shim di sini.
 *
 * Delta yang terbukti dari inspeksi sumber 5.4:
 *   1. getnstimeofday()      — dihapus 5.6  -> ktime_get_real_ts64()
 *   2. current_kernel_time() — dihapus 4.20 -> ktime_get_real_ts64()
 *   3. do_gettimeofday()     — dihapus 5.6  -> ktime_get_real_ts64()
 *   4. set_fs()/get_fs()/KERNEL_DS — dihapus 5.10 -> no-op
 *   5. access_ok(VERIFY_*, ptr, len) — 5.8+: access_ok(ptr, len)
 *   6. struct file_operations di proc_create_data() — 5.6+: struct proc_ops
 *   7. platform_driver.remove int -> void (6.11+), .remove_new dihapus (6.13)
 *   8. mm_segment_t — dihapus bersama set_fs() (5.10); gnss_dump.c memakai
 *      variabel lokal itu, sudah diganti `int` langsung di sumbernya
 *   9. class_create(THIS_MODULE, name) -> class_create(name) (6.4+)
 *      (wcn_log.c — tidak lewat makro, langsung diperbaiki di sumber)
 *
 * Pemakaian: #include <wcn-618-compat.h>  (include/ ada di -I sprdwcn)
 */
#ifndef __WCN_618_COMPAT_H__
#define __WCN_618_COMPAT_H__

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/irq.h>

/* ------------------------------------------------------------------ *
 * 1-3. timekeeping: helper 64-bit yang aman Y2038
 * ------------------------------------------------------------------ */

/* getnstimeofday(struct timespec64 *) -> isi lewat ktime_get_real_ts64 */
static inline void vxtux_wcn_getnstimeofday(struct timespec64 *ts)
{
	ktime_get_real_ts64(ts);
}

/* current_kernel_time() -> struct timespec64 */
static inline struct timespec64 vxtux_wcn_current_kernel_time(void)
{
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	return ts;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
/* Nama 5.4 dipetakan ke helper di atas; tipe argumen di call-site sudah
 * dimigrasi ke struct timespec64. */
#define getnstimeofday(ts)	vxtux_wcn_getnstimeofday(ts)
#define do_gettimeofday(tv)	vxtux_wcn_getnstimeofday(tv)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#define current_kernel_time()	vxtux_wcn_current_kernel_time()
#endif

/* ------------------------------------------------------------------ *
 * 4. set_fs()/get_fs(): dihapus 5.10 -- SHIM DIBUANG.
 * Satu-satunya pemakai (gnss_dump.c) sudah migrasi ke kernel_write(), jadi
 * no-op ini mati; dibiarkan berarti menyembunyikan API lama dari sweep.
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 * 5. access_ok(): arah VERIFY_READ/VERIFY_WRITE dibuang di 5.8
 *
 * TIDAK di-shim di sini. Keempat call-site (platform/bufring.c, semuanya
 * VERIFY_READ) sudah ditulis ulang langsung menjadi access_ok(buf, len).
 * Sengaja tidak memakai makro pembungkus: makro bernama access_ok yang di
 * dalamnya memanggil access_ok() mengandalkan "blue paint" praprosesor
 * dan sulit dibaca. Empat baris eksplisit lebih jujur.
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 * 6. struct proc_ops: pengganti struct file_operations untuk procfs
 * (5.6+). Nama field berbeda; pemetaan eksplisit supaya port terbaca.
 * ------------------------------------------------------------------ */
#define WCN_PROC_OPS_INIT(_open, _read, _write, _poll, _ioctl, _llseek, \
			  _release)					\
	{								\
		.proc_open	= (_open),				\
		.proc_read	= (_read),				\
		.proc_write	= (_write),				\
		.proc_poll	= (_poll),				\
		.proc_ioctl	= (_ioctl),				\
		.proc_lseek	= (_llseek),				\
		.proc_release	= (_release),				\
	}

/* ------------------------------------------------------------------ *
 * 7. platform_driver.remove: int -> void (6.11+)
 * ------------------------------------------------------------------ */
#define WCN_REMOVE_IS_VOID	1

/*
 * kzfree() dihapus di v6.8 (diganti kfree_sensitive()). Shim-nya DIHAPUS
 * 2026-09-23: dua-satunya pemakai (platform/wcn_pm_qos.c) sudah memanggil
 * kfree_sensitive() langsung, dan grep seluruh pohon tak menemukan pemakai lain
 * (hanya menyebut namanya di komentar). Menghapusnya membuat pemakaian kzfree()
 * baru langsung terlihat sebagai temuan BREAKS-BUILD oleh tools/re/api_sweep_618.py.
 */

/*
 * 10. helper GPIO-wakeup & chmod (dipindah dari include/vxtux/compat_618.h
 *     saat de-shim 2026-09-18: header compat global DIHAPUS karena tinggal
 *     2 dari 6 helper yang benar-benar dipakai, dan keduanya hanya dipakai
 *     keluarga WCN ini — tempat paling alami adalah header compat WCN).
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 18, 0)

/* FLAG_IS_WAKEUP (flag privat drivers/gpio/gpiolib.h di pohon vendor) dibaca
 * langsung oleh sdiohal_main.c/sdio_int.c lewat p_desc->flags. 6.18 menutup
 * struct gpio_desc, dan flag itu memang tidak pernah ada di mainline; makna
 * sebenarnya = "interrupt GPIO ini terpasang sebagai wakeup source", yang di
 * 6.18 dijawab API publik irqd_is_wakeup_set(). */
static inline bool vxtux_gpiod_irq_is_wakeup(struct gpio_desc *desc)
{
	int irq = gpiod_to_irq(desc);
	struct irq_data *d = (irq >= 0) ? irq_get_irq_data(irq) : NULL;

	return d && irqd_is_wakeup_set(d);
}

/* sys_chmod() dihapus di 6.18. Padanan kernel untuk berkas yang baru dibuat
 * filp_open(O_CREAT, 0666) = notify_change() pada dentry-nya, sebab mode O_CREAT
 * sudah terpotong current_umask(). Pemakai: gnss_dump.c (dump firmware GNSS). */
static inline int vxtux_kernel_chmod(struct file *filp, umode_t mode)
{
	struct iattr newattrs = { .ia_valid = ATTR_MODE, .ia_mode = mode };

	return notify_change(mnt_idmap(filp->f_path.mnt), filp->f_path.dentry,
			     &newattrs, NULL);
}

#endif /* >= 6.18 */

#endif /* __WCN_618_COMPAT_H__ */
