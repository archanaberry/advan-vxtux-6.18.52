/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2012--2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Ported to 6.18.52-VXTux-UMS512 from the 5.4 donor
 * work/donors_20260930/iscle_ums512 (same SoC, UMS512).
 *
 * Changes vs the 5.4 donor (see drivers/misc/sprd_jpg/PORT-NOTES.md):
 *   - struct platform_driver .remove: int -> void
 *   - syscon_regmap_lookup_by_name()/syscon_get_args_by_name() (removed
 *     upstream, and vendor-5.4 semantics differ from the syscons/
 *     syscon-names list shim) replaced by a direct
 *     of_parse_phandle_with_args() on the property names the stock UMS512
 *     DTB actually carries. Evidence: /data/local/tmp/dtsx/fdt.dts node
 *     jpg-codec@62700000 has `reset-syscon` and `aon-apb-eb-syscon`, each
 *     <phandle reg mask>, and NO syscons/syscon-names pair.
 */

#ifndef _SPRD_JPG_COMMON_H
#define _SPRD_JPG_COMMON_H

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <uapi/video/sprd_jpg.h>
#include <uapi/video/sprd_mmsys_pw_domain.h>

#define JPG_MINOR MISC_DYNAMIC_MINOR
#define JPG_TIMEOUT_MS 1000
#define DEFAULT_FREQ_DIV 0x0

#define GLB_INT_STS_OFFSET	0x20
#define GLB_INT_EN_OFFSET	0x24
#define GLB_INT_CLR_OFFSET	0x28
#define GLB_INT_RAW_OFFSET	0x2C
#define SPRD_JPG_CLK_LEVEL_NUM 4

struct jpg_fh {
	int is_jpg_acquired;
	int is_clock_enabled;
	struct jpg_dev_t *jpg_hw_dev;
};

struct sprd_jpg_cfg_data {
	unsigned int version;
	unsigned int max_freq_level;
	unsigned int qos_reg_offset;
};

struct jpg_dev_t {
	unsigned int freq_div;

	struct semaphore jpg_mutex;

	wait_queue_head_t wait_queue_work_MBIO;
	int condition_work_MBIO;
	wait_queue_head_t wait_queue_work_VLC;
	int condition_work_VLC;
	wait_queue_head_t wait_queue_work_BSM;
	int condition_work_BSM;
	int jpg_int_status;

	struct clk *jpg_clk;
	struct clk *jpg_parent_clk;
	struct clk *jpg_parent_clk_df;
	struct clk *jpg_domain_eb;
	struct clk *clk_vsp_mq_ahb_eb;
	struct clk *clk_aon_jpg_emc_eb;
	struct clk *jpg_dev_eb;
	struct clk *jpg_ckg_eb;
	struct clk *clk_ahb_vsp;
	struct clk *ahb_parent_clk;
	struct clk *clk_emc_vsp;
	struct clk *emc_parent_clk;

	unsigned int irq;
	unsigned int version;

	struct jpg_fh *jpg_fp;
	struct device_node *dev_np;
	struct device *jpg_dev;
	struct sprd_jpg_cfg_data *jpg_cfg_data;
	struct regmap *gpr_jpg_ahb;
	struct clock_name_map_t *clock_name_map;
	unsigned long sprd_jpg_virt;
	unsigned long sprd_jpg_phys;
	unsigned int jpg_softreset_reg_offset;
	unsigned int jpg_reset_mask;
	int max_freq_level;
	/*
	 * Number of entries actually filled in clock_name_map. The 5.4 driver
	 * walked ARRAY_SIZE(jpg_clk_src) slots while only some of those clocks
	 * exist in a given DT, so clock_name_map could be read past the last
	 * populated entry by jpg_get_clk_src_name().
	 */
	int clk_num;
	bool jpg_qos_exist_flag;
};

struct clock_name_map_t {
	unsigned long freq;
	char *name;
	struct clk *clk_parent;
};

enum {
	AON_APB_EB,
	RESET
};

/*
 * The stock UMS512 DTB carries one phandle property per register, named
 * after what it drives. These are the DT property names, not the 5.4
 * syscon-names identifiers -- see PORT-NOTES.md.
 */
struct jpg_syscon_prop {
	const char *prop;
};

extern const struct jpg_syscon_prop jpg_syscon_props[];

struct register_gpr {
	struct regmap *gpr;
	uint32_t reg;
	uint32_t mask;
};

struct jpg_qos_cfg {
	u8 awqos;
	u8 arqos_high;
	u8 arqos_low;
	unsigned int reg_offset;
};

struct clk *jpg_get_clk_src_name(struct clock_name_map_t clock_name_map[],
				  unsigned int freq_level,
				  unsigned int max_freq_level);
int find_jpg_freq_level(struct clock_name_map_t clock_name_map[],
			unsigned long freq, unsigned int max_freq_level);
int jpg_get_mm_clk(struct jpg_dev_t *jpg_hw_dev);
int poll_mbio_vlc_done(struct jpg_dev_t *jpg_hw_dev, int cmd0);
int jpg_get_iova(struct jpg_dev_t *jpg_hw_dev,
		 struct jpg_iommu_map_data *mapdata, void __user *arg);
int jpg_free_iova(struct jpg_dev_t *jpg_hw_dev,
		  struct jpg_iommu_map_data *ummapdata);
int sprd_jpg_pw_on(void);
int sprd_jpg_pw_off(void);
int sprd_jpg_domain_eb(void);
int sprd_jpg_domain_disable(void);
int jpg_clk_enable(struct jpg_dev_t *jpg_hw_dev);
void jpg_clk_disable(struct jpg_dev_t *jpg_hw_dev);

#endif /* _SPRD_JPG_COMMON_H */