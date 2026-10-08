// SPDX-License-Identifier: GPL-2.0
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
 * Ported from work/donors_20260930/iscle_ums512 (5.4, same SoC).
 *
 * Changes vs the donor, all in the power-domain glue:
 *   - the donor selected its sprd_jpg_pw_on() body with IS_ENABLED() on five
 *     config symbols (SPRD_JPG_CALL_VSP_PW_DOMAIN, SPRD_CAM_PW_DOMAIN_R4P0,
 *     SPRD_MM_PW_DOMAIN_R6P0, SPRD_CAM_PW_DOMAIN_R5P1/R7P0). None of those
 *     symbols exists anywhere in this tree, so IS_ENABLED() folded all of them
 *     to 0 and the donor's third branch -- the do-nothing stubs -- was what
 *     actually compiled. Kept as such, and now stated explicitly rather than
 *     left to five phantom #if branches.
 *   - the CONFIG_COMPAT ioctl translation was dropped: this board is arm64 and
 *     CONFIG_COMPAT is not set in .config, so it was dead code.
 *   - clk_prepare_enable()/clk_disable_unprepare() on a NULL clk: jpg_get_mm_clk()
 *     tolerates every devm_clk_get() failure, so the enable/disable path has to
 *     check IS_ERR_OR_NULL before touching the pointers. The donor did not.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>
#include <uapi/video/sprd_jpg.h>

#include "sprd_jpg_common.h"

#undef pr_fmt
#define pr_fmt(fmt) "sprd-jpg: " fmt

/*
 * No power domain is wired to jpg on this board. The VSP domain that the
 * 5.4 donor would have used here (CONFIG_SPRD_JPG_CALL_VSP_PW_DOMAIN ->
 * vsp_pw_on(VSP_PW_DOMAIN_VSP_JPG)) is a driver that is still NOT-PORTED, and
 * the camsys variant needs the sprd_camsys_pw_domain domain, which has its own
 * node. So these stay no-ops until a domain is actually claimed in DT; the
 * callers treat the return value as advisory.
 */
int sprd_jpg_pw_on(void)
{
	return 0;
}

int sprd_jpg_pw_off(void)
{
	return 0;
}

int sprd_jpg_domain_eb(void)
{
	return 0;
}

int sprd_jpg_domain_disable(void)
{
	return 0;
}

struct clk *jpg_get_clk_src_name(struct clock_name_map_t clock_name_map[],
				  unsigned int freq_level,
				  unsigned int max_freq_level)
{
	if (freq_level >= max_freq_level) {
		pr_info("set freq_level to 0\n");
		freq_level = 0;
	}

	pr_debug(" freq_level %d %s\n", freq_level,
		 clock_name_map[freq_level].name);
	return clock_name_map[freq_level].clk_parent;
}

int find_jpg_freq_level(struct clock_name_map_t clock_name_map[],
			unsigned long freq, unsigned int max_freq_level)
{
	int level = 0;
	int i;

	for (i = 0; i < (int)max_freq_level; i++) {
		if (clock_name_map[i].freq == freq) {
			level = i;
			break;
		}
	}
	return level;
}

int jpg_get_mm_clk(struct jpg_dev_t *jpg_hw_dev)
{
	int ret = 0;
	struct clk *clk;

	clk = devm_clk_get(jpg_hw_dev->jpg_dev, "jpg_domain_eb");
	if (IS_ERR(clk)) {
		pr_err("Can't get clock [jpg_domain_eb]! %p\n", clk);
		jpg_hw_dev->jpg_domain_eb = NULL;
		ret = PTR_ERR(clk);
	} else {
		jpg_hw_dev->jpg_domain_eb = clk;
	}

	if (jpg_hw_dev->version == SHARKL3) {
		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_aon_jpg_emc_eb");
		if (IS_ERR(clk)) {
			pr_err("Can't get clock [clk_aon_jpg_emc_eb]! %p\n",
			       clk);
			jpg_hw_dev->clk_aon_jpg_emc_eb = NULL;
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->clk_aon_jpg_emc_eb = clk;
		}
	}

	clk = devm_clk_get(jpg_hw_dev->jpg_dev, "jpg_dev_eb");
	if (IS_ERR(clk)) {
		pr_err("Can't get clock [jpg_dev_eb]! %p\n", clk);
		jpg_hw_dev->jpg_dev_eb = NULL;
		ret = PTR_ERR(clk);
	} else {
		jpg_hw_dev->jpg_dev_eb = clk;
	}

	clk = devm_clk_get(jpg_hw_dev->jpg_dev, "jpg_ckg_eb");
	if (IS_ERR(clk)) {
		pr_err("Can't get clock [jpg_ckg_eb]! %p\n", clk);
		jpg_hw_dev->jpg_ckg_eb = NULL;
		ret = PTR_ERR(clk);
	} else {
		jpg_hw_dev->jpg_ckg_eb = clk;
	}

	if (jpg_hw_dev->version == PIKE2) {
		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_vsp_mq_ahb_eb");
		if (IS_ERR(clk)) {
			pr_err("Can't get clock [clk_vsp_mq_ahb_eb]! %p\n",
			       clk);
			jpg_hw_dev->clk_vsp_mq_ahb_eb = NULL;
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->clk_vsp_mq_ahb_eb = clk;
		}
	}

	if (jpg_hw_dev->version == SHARKL3) {
		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_ahb_vsp");
		if (IS_ERR(clk)) {
			pr_err("Can't get clock [clk_ahb_vsp]! %p\n", clk);
			jpg_hw_dev->clk_ahb_vsp = NULL;
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->clk_ahb_vsp = clk;
		}

		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_ahb_vsp_parent");
		if (IS_ERR(clk)) {
			pr_err("clock[clk_ahb_vsp_parent]: failed to get parent in probe!\n");
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->ahb_parent_clk = clk;
		}

		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_emc_vsp");
		if (IS_ERR(clk)) {
			pr_err("Can't get clock [clk_emc_vsp]! %p\n", clk);
			jpg_hw_dev->clk_emc_vsp = NULL;
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->clk_emc_vsp = clk;
		}

		clk = devm_clk_get(jpg_hw_dev->jpg_dev, "clk_emc_vsp_parent");
		if (IS_ERR(clk)) {
			pr_err("clock[clk_emc_vsp_parent]: failed to get parent in probe!\n");
			ret = PTR_ERR(clk);
		} else {
			jpg_hw_dev->emc_parent_clk = clk;
		}
	}

	clk = devm_clk_get(jpg_hw_dev->jpg_dev, "jpg_clk");
	if (IS_ERR(clk)) {
		pr_err("Can't get clock [jpg_clk]! %p\n", clk);
		jpg_hw_dev->jpg_clk = NULL;
		ret = PTR_ERR(clk);
	} else {
		jpg_hw_dev->jpg_clk = clk;
	}

	if (jpg_hw_dev->clk_num > 0)
		jpg_hw_dev->jpg_parent_clk_df =
			jpg_get_clk_src_name(jpg_hw_dev->clock_name_map, 0,
					     jpg_hw_dev->max_freq_level);

	return ret;
}

int jpg_get_iova(struct jpg_dev_t *jpg_hw_dev,
		 struct jpg_iommu_map_data *mapdata, void __user *arg)
{
	int ret = 0;
	struct sprd_iommu_map_data iommu_map_data;

	if (sprd_iommu_attach_device(jpg_hw_dev->jpg_dev) == 0) {
		ret = sprd_ion_get_buffer(mapdata->fd, NULL,
					  &(iommu_map_data.buf),
					  &iommu_map_data.iova_size);
		if (ret) {
			pr_err("get_sg_table failed, ret %d\n", ret);
			return ret;
		}

		iommu_map_data.ch_type = SPRD_IOMMU_FM_CH_RW;
		iommu_map_data.iova_addr = 0;
		ret = sprd_iommu_map(jpg_hw_dev->jpg_dev, &iommu_map_data);
		if (!ret) {
			mapdata->iova_addr = iommu_map_data.iova_addr;
			mapdata->size = iommu_map_data.iova_size;
			ret = copy_to_user((void __user *)arg,
					   (void *)mapdata,
					   sizeof(struct jpg_iommu_map_data));
			if (ret)
				return -EFAULT;
		} else {
			pr_err("jpg iommu map failed, ret %d\n", ret);
			pr_err("map size 0x%zx\n", iommu_map_data.iova_size);
		}
	} else {
		ret = sprd_ion_get_phys_addr(mapdata->fd, NULL,
					     &mapdata->iova_addr, &mapdata->size);
		if (ret) {
			pr_err("jpg sprd_ion_get_phys_addr failed, ret %d\n",
			       ret);
			return ret;
		}

		ret = copy_to_user((void __user *)arg, (void *)mapdata,
				   sizeof(struct jpg_iommu_map_data));
		if (ret)
			return -EFAULT;
	}
	return ret;
}

int jpg_free_iova(struct jpg_dev_t *jpg_hw_dev,
		  struct jpg_iommu_map_data *ummapdata)
{
	int ret = 0;
	struct sprd_iommu_unmap_data iommu_ummap_data;

	if (sprd_iommu_attach_device(jpg_hw_dev->jpg_dev) == 0) {
		iommu_ummap_data.iova_addr = ummapdata->iova_addr;
		iommu_ummap_data.iova_size = ummapdata->size;
		iommu_ummap_data.ch_type = SPRD_IOMMU_FM_CH_RW;
		iommu_ummap_data.buf = NULL;
		ret = sprd_iommu_unmap(jpg_hw_dev->jpg_dev,
				       &iommu_ummap_data);

		if (ret) {
			pr_err("jpg iommu unmap failed ret %d\n", ret);
			pr_err("unmap addr&size 0x%lx 0x%zx\n",
			       ummapdata->iova_addr, ummapdata->size);
		}
	}

	return ret;
}

int poll_mbio_vlc_done(struct jpg_dev_t *jpg_hw_dev, int cmd0)
{
	int ret = 0;

	pr_debug("jpg_poll_begin\n");
	if (cmd0 == INTS_MBIO) {
		ret = wait_event_interruptible_timeout(
			jpg_hw_dev->wait_queue_work_MBIO,
			jpg_hw_dev->condition_work_MBIO,
			msecs_to_jiffies(JPG_TIMEOUT_MS));

		if (ret == -ERESTARTSYS) {
			pr_err("jpg error start -ERESTARTSYS\n");
			ret = -EINVAL;
		} else if (ret == 0) {
			pr_err("jpg error start  timeout\n");
			ret = -ETIMEDOUT;
		} else {
			ret = 0;
		}

		if (ret) {
			/* timeout, clear jpg int */
			writel_relaxed((1 << 3) | (1 << 2) | (1 << 1) |
				       (1 << 0),
				(void __iomem *)(jpg_hw_dev->sprd_jpg_virt +
					GLB_INT_CLR_OFFSET));
			ret = 1;
		}

		jpg_hw_dev->jpg_int_status &= ~0x8;
		jpg_hw_dev->condition_work_MBIO = 0;
	} else if (cmd0 == INTS_VLC) {
		ret = wait_event_interruptible_timeout(
			jpg_hw_dev->wait_queue_work_VLC,
			jpg_hw_dev->condition_work_VLC,
			msecs_to_jiffies(JPG_TIMEOUT_MS));

		if (ret == -ERESTARTSYS) {
			pr_err("jpg error start -ERESTARTSYS\n");
			ret = -EINVAL;
		} else if (ret == 0) {
			pr_err("jpg error start  timeout\n");
			ret = -ETIMEDOUT;
		} else {
			ret = 0;
		}

		if (ret) {
			/* timeout, clear jpg int */
			writel_relaxed((1 << 3) | (1 << 2) | (1 << 1) |
				       (1 << 0),
				(void __iomem *)(jpg_hw_dev->sprd_jpg_virt +
					GLB_INT_CLR_OFFSET));
			ret = 1;
		} else {
			ret = 4;
		}

		jpg_hw_dev->jpg_int_status &= ~0x2;
		jpg_hw_dev->condition_work_VLC = 0;
	} else {
		pr_err("JPG_ACQUAIRE_MBIO_DONE error arg\n");
		ret = -1;
	}
	pr_debug("jpg_poll_end\n");
	return ret;
}

int jpg_clk_enable(struct jpg_dev_t *jpg_hw_dev)
{
	int ret = 0;

	pr_info("jpg JPG_ENABLE\n");

	/*
	 * Every one of these clk pointers is NULL when devm_clk_get() failed in
	 * jpg_get_mm_clk(); clk_prepare_enable(NULL) is a NULL dereference, so
	 * the donor's unconditional calls are gated here.
	 */
	if (IS_ERR_OR_NULL(jpg_hw_dev->jpg_domain_eb)) {
		pr_err("jpg_domain_eb unavailable\n");
		return -ENODEV;
	}

	ret = clk_prepare_enable(jpg_hw_dev->jpg_domain_eb);
	if (ret) {
		pr_err("jpg jpg_domain_eb clk_prepare_enable failed!\n");
		return ret;
	}

	if (jpg_hw_dev->version == SHARKL3 &&
	    !IS_ERR_OR_NULL(jpg_hw_dev->clk_aon_jpg_emc_eb)) {
		ret = clk_prepare_enable(jpg_hw_dev->clk_aon_jpg_emc_eb);
		if (ret)
			goto clk_disable_0;
	}

	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_dev_eb)) {
		ret = clk_prepare_enable(jpg_hw_dev->jpg_dev_eb);
		if (ret)
			goto clk_disable_1;
	}

	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_ckg_eb)) {
		ret = clk_prepare_enable(jpg_hw_dev->jpg_ckg_eb);
		if (ret)
			goto clk_disable_2;
	}

	if (jpg_hw_dev->version == SHARKL3) {
		if (IS_ERR_OR_NULL(jpg_hw_dev->clk_ahb_vsp) ||
		    IS_ERR_OR_NULL(jpg_hw_dev->ahb_parent_clk))
			goto clk_disable_3;

		ret = clk_set_parent(jpg_hw_dev->clk_ahb_vsp,
				     jpg_hw_dev->ahb_parent_clk);
		if (ret)
			goto clk_disable_3;

		ret = clk_prepare_enable(jpg_hw_dev->clk_ahb_vsp);
		if (ret)
			goto clk_disable_3;

		if (IS_ERR_OR_NULL(jpg_hw_dev->clk_emc_vsp) ||
		    IS_ERR_OR_NULL(jpg_hw_dev->emc_parent_clk))
			goto clk_disable_4;

		ret = clk_set_parent(jpg_hw_dev->clk_emc_vsp,
				     jpg_hw_dev->emc_parent_clk);
		if (ret)
			goto clk_disable_4;

		ret = clk_prepare_enable(jpg_hw_dev->clk_emc_vsp);
		if (ret)
			goto clk_disable_4;
	}

	if (!IS_ERR_OR_NULL(jpg_hw_dev->clk_vsp_mq_ahb_eb)) {
		ret = clk_prepare_enable(jpg_hw_dev->clk_vsp_mq_ahb_eb);
		if (ret)
			goto clk_disable_5;
	}

	if (IS_ERR_OR_NULL(jpg_hw_dev->jpg_clk) ||
	    IS_ERR_OR_NULL(jpg_hw_dev->jpg_parent_clk_df)) {
		pr_err("jpg_clk unavailable\n");
		ret = -ENODEV;
		goto clk_disable_6;
	}

	ret = clk_set_parent(jpg_hw_dev->jpg_clk,
			     jpg_hw_dev->jpg_parent_clk_df);
	if (ret)
		goto clk_disable_6;

	ret = clk_set_parent(jpg_hw_dev->jpg_clk,
			     jpg_hw_dev->jpg_parent_clk);
	if (ret)
		goto clk_disable_6;

	ret = clk_prepare_enable(jpg_hw_dev->jpg_clk);
	if (ret)
		goto clk_disable_6;

	pr_info("jpg_clk clk_prepare_enable ok.\n");
	return ret;

clk_disable_6:
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_clk))
		clk_disable_unprepare(jpg_hw_dev->jpg_clk);
clk_disable_5:
	if (!IS_ERR_OR_NULL(jpg_hw_dev->clk_vsp_mq_ahb_eb))
		clk_disable_unprepare(jpg_hw_dev->clk_vsp_mq_ahb_eb);
clk_disable_4:
	if (jpg_hw_dev->version == SHARKL3 &&
	    !IS_ERR_OR_NULL(jpg_hw_dev->clk_emc_vsp))
		clk_disable_unprepare(jpg_hw_dev->clk_emc_vsp);
clk_disable_3:
	if (jpg_hw_dev->version == SHARKL3 &&
	    !IS_ERR_OR_NULL(jpg_hw_dev->clk_ahb_vsp))
		clk_disable_unprepare(jpg_hw_dev->clk_ahb_vsp);
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_ckg_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_ckg_eb);
clk_disable_2:
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_dev_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_dev_eb);
clk_disable_1:
	if (jpg_hw_dev->version == SHARKL3 &&
	    !IS_ERR_OR_NULL(jpg_hw_dev->clk_aon_jpg_emc_eb))
		clk_disable_unprepare(jpg_hw_dev->clk_aon_jpg_emc_eb);
clk_disable_0:
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_domain_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_domain_eb);

	return ret;
}

void jpg_clk_disable(struct jpg_dev_t *jpg_hw_dev)
{
	if (jpg_hw_dev->version == SHARKL3) {
		if (!IS_ERR_OR_NULL(jpg_hw_dev->clk_ahb_vsp))
			clk_disable_unprepare(jpg_hw_dev->clk_ahb_vsp);
		if (!IS_ERR_OR_NULL(jpg_hw_dev->clk_emc_vsp))
			clk_disable_unprepare(jpg_hw_dev->clk_emc_vsp);
	}
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_clk))
		clk_disable_unprepare(jpg_hw_dev->jpg_clk);
	if (jpg_hw_dev->version == SHARKL3 &&
	    !IS_ERR_OR_NULL(jpg_hw_dev->clk_aon_jpg_emc_eb))
		clk_disable_unprepare(jpg_hw_dev->clk_aon_jpg_emc_eb);
	if (!IS_ERR_OR_NULL(jpg_hw_dev->clk_vsp_mq_ahb_eb))
		clk_disable_unprepare(jpg_hw_dev->clk_vsp_mq_ahb_eb);
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_ckg_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_ckg_eb);
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_dev_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_dev_eb);
	if (!IS_ERR_OR_NULL(jpg_hw_dev->jpg_domain_eb))
		clk_disable_unprepare(jpg_hw_dev->jpg_domain_eb);
}