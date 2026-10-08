/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
/*
 * PROVENANCE: disalin utuh dari revisi vendor yang jalurnya tertanam di blob
 * T1030 (`drivers/unisoc_platform/sprdwcn/platform/wcn_pm_qos.h`, mirror GPL
 * kernel_ums512_5.4). Lihat platform/wcn_pm_qos.c untuk detail + sha.
 */
#ifndef __WCN_PM_QOS_H__
#define __WCN_PM_QOS_H__

#include <linux/pm_qos.h>

struct wcn_pm_qos_condition {
	bool blank_cond;
	bool cpu_pd_forbid;
	bool cpu_freq_change;
};

struct wcn_pm_qos {
	unsigned long active_modules;
	unsigned long active_modules_pending;
	struct mutex mutex;
	struct pm_qos_request pm_qos_req;
	struct freq_qos_request big_core_min_freq;
	struct cpufreq_policy *policy;
	struct device *dev;
	struct wcn_pm_qos_condition cond;
	bool cond_set;
	unsigned int min_freq, max_freq, cur_min_freq;
	bool freq_set_flag;
	bool cpu_pd_set_flag;
	bool pm_qos_disable;
	struct task_struct *test_task;
};


#define CPU_FREQ_BIG_CORE_INDEX	6
#define CPU_DMA_FORBID_POWER_DOWN_LATENCY	100

int wcn_pm_qos_config_common(unsigned int mode, bool set);
int wcn_pm_qos_init(void);
void wcn_pm_qos_exit(void);

#endif
