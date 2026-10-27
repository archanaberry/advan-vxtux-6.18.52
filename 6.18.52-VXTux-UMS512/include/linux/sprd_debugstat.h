/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Public interface of the Spreadtrum debug stat driver.
 *
 * Port of the 5.4 vendor module `sprd_power_stat.ko` (MODULE_DESCRIPTION
 * "sprd debug stat driver").  Other drivers register a subsystem sleep
 * info getter through stat_info_register() and the driver exposes the
 * collected counters under /proc/sprd-debugstat/.
 *
 * The struct size and every field offset below were read out of the vendor
 * blob with donors/blobre.py (capstone + pyelftools, relocations applied),
 * not guessed.  See docs/RE-SPRD-POWER-STAT.md.
 */
#ifndef __LINUX_SPRD_DEBUGSTAT_H
#define __LINUX_SPRD_DEBUGSTAT_H

#include <linux/list.h>
#include <linux/types.h>

struct device;

/*
 * Per subsystem counters.  The vendor blob allocates exactly 0x58 bytes
 * for it; every printer in /proc/sprd-debugstat/stat reads one of these
 * fields, so the offsets are part of the driver's own data path.
 */
struct subsys_sleep_info {
	char name[4];			/* +0x00  "AP" / "SOC"          */
	u64 total_duration;		/* +0x08  ms since boot         */
	u64 sleep_duration_total;	/* +0x10                        */
	u64 idle_duration_total;	/* +0x18                        */
	u32 state;			/* +0x20                        */
	u32 subsystem_reboot_count;	/* +0x24                        */
	u32 wakeup_reason;		/* +0x28                        */
	u32 last_wakeup_duration;	/* +0x2c                        */
	u32 last_sleep_duration;	/* +0x30                        */
	u32 active_core;		/* +0x34  jumlah CPU online      */
	u32 internal_irq_count;		/* +0x38                        */
	u32 irq_to_ap_count;		/* +0x3c                        */
	u32 soc_slp_cnt;		/* +0x40  dari regmap PMIC       */
	u8 reserved[0x14];		/* +0x44 .. +0x58               */
};

/*
 * Getter: dipanggil setiap kali stat_read() atau slp_cnt_read() membaca
 * berkas proc, dengan `dev` sebagai argumen.  Mengembalikan salinan milik
 * driver (bukan pointer statis) supaya aman dipakai lintas modul.
 */
typedef struct subsys_sleep_info *(*sprd_dbgstat_getter_t)(struct device *dev);

int stat_info_register(const char *name, sprd_dbgstat_getter_t get,
		       struct device *dev);
int stat_info_unregister(const char *name);

#endif /* __LINUX_SPRD_DEBUGSTAT_H */
