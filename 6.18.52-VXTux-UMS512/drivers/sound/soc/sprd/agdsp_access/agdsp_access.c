/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
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

#define pr_fmt(fmt) "[Audio:AGDSP_ACCESS] "fmt

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/hwspinlock.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sched.h>
/*
 * 6.18: <linux/sipc.h> & <linux/sprd_mailbox.h> adalah header vendor staging
 * yang tidak ada di tree ini. Dua shim port:
 *   - smem/vmap  : smem_alloc -> audio_mem_alloc(DDR32) [audio_mem, B4b],
 *                  shmem_ram_vmap_nocache -> audio_mem_vmap (non-cache).
 *   - smsg       : layer aud_smsg (audiosipc, B4c) — protokol kabel sama,
 *                  struktur pesan dipetakan (lihat vxtux-smsg-shim.h).
 * Cabang CONFIG_SPRD_SIPC_V2 tidak pernah aktif (simbol tidak ada di config).
 */
#include "../audiomem/audio_mem.h"
#include <linux/io.h>
#ifndef VXTUX_AGDSP_SIPC_SHIM
#define VXTUX_AGDSP_SIPC_SHIM
/* smem_alloc(size) lama mengembalikan alamat fisik DDR; padanannya di port:
 * audio_mem_alloc(DDR32, &size) (jalur gen_pool). shmem_ram_vmap_nocache
 * memetakan alamat fisik itu ke memori non-cache (memremap WC). */
static inline u32 vxtux_smem_alloc(u32 size)
{
	u32 sz = size;

	return audio_mem_alloc(DDR32, &sz);
}
#define smem_alloc(size)	vxtux_smem_alloc((size))
#define smem_free(addr, size)	audio_mem_free(DDR32, (addr), (size))
static inline void *shmem_ram_vmap_nocache(u32 addr, u32 size)
{
	return audio_mem_vmap((phys_addr_t)addr, size, 1);
}
static inline void shmem_ram_unmap(void *vaddr)
{
	audio_mem_unmap(vaddr);
}
#endif
#include "vxtux-smsg-shim.h"
#include "vxtux-syscon-shim.h"	/* RE: syscons/syscon-names DT vendor */
#include <linux/soc/sprd/sprd_mailbox.h>	/* mbox_just_sent/raw_sent */
#include <linux/slab.h>

enum  adcp_state {
	DEEP_SLEEP_STATE,
	XTL_WAIT_STATE,
	XTL_BUF_WAIT_STATE,
	DEEP_SLEEP_XTLON_STATE,
	PLL_PWR_WAIT_STATE,
	WAKEUP_STATE,
	WAKEUP_LOCK_STATE,
};

enum adsp_state {
	PD_WAKEUP_STATE = 0,
	PD_POWER_ON_SEQ_STATE = 1,
	PD_POWER_ON_M_STATE = 2,
	PD_RST_ASSERT_STATE = 3,
	PD_ISO_OFF_STATE = 6,
	PD_SHUTDOWN_STATE = 7,
	PD_ACTIVE_STATE = 8,
	PD_STANDBY_STATE = 9,
	PD_ISO_ON_STATE = 10,
	PD_RST_DEASSERT_STATE = 11,
	PD_POWER_OFF_M_STATE = 13,
	PD_BISR_RST_STATE = 14,
	PD_BISR_PROC_STATE = 15,
	PD_POWER_ON_STATE = 18,
	PD_POWER_OFF_STATE = 29,
};

/* flag for CMD/DONE msg type */
#define SMSG_CMD_AGDSP_ACCESS_INIT		0x0001
#define SMSG_DONE_AGDSP_ACCESS_INIT		0x0002
#define HWSPINLOCK_TIMEOUT		(5000)
#define AGCP_READL(addr)      readl((void __iomem *) (addr))
#define AGCP_WRITEL(b, addr)  writel(b, (void __iomem *)(addr))
#define AGDSP_ACCESS_DEBUG 0
#define TRY_CNT_MAX 1000000

#if AGDSP_ACCESS_DEBUG
#define pr_dbg(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...) pr_debug(fmt, ##__VA_ARGS__)
#endif

struct agdsp_access_state {
	int32_t ap_enable_cnt;
	int32_t cp_enable_cnt;
};

struct agdsp_access {
	struct agdsp_access_state *state;
	u32 smem_phy_addr;
	u32 smem_size;
	u32 ddr_addr_offset;
	struct task_struct *thread;
	int ready;
	uint8_t dst;
	uint8_t channel;
	uint8_t mbx_core;
	struct regmap *agcp_ahb;
	struct regmap *pmu_apb;
	u32 auto_agcp_access;
	u32 audcp_pmu_sleep_ctrl_reg;
	u32 audcp_pmu_sleep_ctrl_deepslp_mask;
	u32 audcp_pmu_slp_status_reg;
	u32 audcp_pmu_slp_status_mask;
	u32 audcp_pmu_pwr_status4_reg;
	u32 audcp_pmu_sys_slp_state_mask;
	u32 audcp_pmu_pwr_status3_reg;
	u32 audcp_pmu_slp_state_mask;
	u32 ap_access_ena_reg;
	u32 ap_access_ena_mask;
	spinlock_t spin_lock;
};

static struct agdsp_access  *g_agdsp_access;

/*extern int mbox_raw_sent(u8 target_id, u64 msg);*/
static int agdsp_access_init_thread(void *data)
{
	int rval = 0;
	struct smsg mcmd, mrecv;
	struct agdsp_access *dsp_ac = (struct agdsp_access *)data;
	const u32 offset = dsp_ac->ddr_addr_offset;

	pr_dbg("agdsp access thread entry.\n");

	/* since the channel open may hang, we call it in the sblock thread */
	rval = smsg_ch_open(dsp_ac->dst, dsp_ac->channel, -1);
	if (rval != 0) {
		pr_info("Failed to open channel %d,dst=%d,rval=%d\n",
			dsp_ac->channel, dsp_ac->dst, rval);
		/* assign NULL to thread poniter as failed to open channel */
		dsp_ac->thread = NULL;
		return rval;
	}

	/* handle the sblock events */
	while (!kthread_should_stop()) {
		/* monitor sblock recv smsg */
		smsg_set(&mrecv, dsp_ac->channel, 0, 0, 0);
		rval = smsg_recv(dsp_ac->dst, &mrecv, -1);
		if (rval == -EIO || rval == -ENODEV) {
			/* channel state is FREE */
			msleep(20);
			continue;
		}

		pr_info("%s: dst=%d, channel=%d, type=%d, flag=0x%04x, smem_phy_addr=0x%x, value=0x%08x\n",
			__func__, dsp_ac->dst, dsp_ac->channel, mrecv.type,
			mrecv.flag, mrecv.value, dsp_ac->smem_phy_addr);

		switch (mrecv.type) {
		case SMSG_TYPE_OPEN:
			smsg_open_ack(dsp_ac->dst, dsp_ac->channel);
			/*dsp_ac->ready = false;*/
			break;
		case SMSG_TYPE_CMD:
			/* respond cmd done for sblock init */
			WARN_ON(mrecv.flag != SMSG_CMD_AGDSP_ACCESS_INIT);
			smsg_set(&mcmd,
				dsp_ac->channel,
				SMSG_TYPE_DONE,
				SMSG_DONE_AGDSP_ACCESS_INIT,
				dsp_ac->smem_phy_addr + offset);
			smsg_send(dsp_ac->dst, &mcmd, -1);
			/*dsp_ac->ready = true;*/
			break;
		default:
			pr_info("non-handled agdsp access msg: %d-%d, %d, %d, %d\n",
				dsp_ac->dst, dsp_ac->channel,
				mrecv.type, mrecv.flag, mrecv.value);
			break;
		};
	}
	smsg_ch_close(dsp_ac->dst, dsp_ac->channel, -1);

	return rval;
}

static int agdsp_access_initialize(struct platform_device *pdev,
	struct device_node *node, struct regmap *agcp_ahb,
	struct regmap *pmu_apb, u32 auto_agcp_access,
	u32 mbx_core, u32 dst, u32 channel, const u32 offset)
{
	struct agdsp_access *dsp_ac;
	int args_count;
	unsigned int args[2];
	int ret = -EINVAL;

	g_agdsp_access = kzalloc(sizeof(struct agdsp_access), GFP_KERNEL);
	if ((g_agdsp_access == NULL) || (!agcp_ahb)) {
		pr_err("agdsp_access init:Failed to allocate\n");
		return -ENOMEM;
	}
	g_agdsp_access->agcp_ahb = agcp_ahb;
	g_agdsp_access->pmu_apb = pmu_apb;
	g_agdsp_access->mbx_core = mbx_core;
	g_agdsp_access->dst = dst;
	g_agdsp_access->channel = channel;
	g_agdsp_access->ddr_addr_offset = offset;
	g_agdsp_access->auto_agcp_access = auto_agcp_access;
	dsp_ac = g_agdsp_access;
	args_count = syscon_get_args_by_name(node, "audcp_pmu_sleep_ctrl",
		sizeof(args), args);
	if (args_count == ARRAY_SIZE(args)) {
		dsp_ac->audcp_pmu_sleep_ctrl_reg = args[0];
		dsp_ac->audcp_pmu_sleep_ctrl_deepslp_mask = args[1];
		pr_err("audcp_pmu_sleep_ctrl:reg:%x,mask:%x\n",
			args[0], args[1]);
	}
	pr_err("audcp_pmu_sleep_ctrl:args_count,:%d\n", args_count);
	args_count = syscon_get_args_by_name(node, "audcp_pmu_slp_status",
		sizeof(args), args);
	if (args_count == ARRAY_SIZE(args)) {
		dsp_ac->audcp_pmu_slp_status_reg = args[0];
		dsp_ac->audcp_pmu_slp_status_mask = args[1];
		pr_err("audcp_pmu_slp_status:reg:%x,mask:%x\n",
			args[0], args[1]);
	}

	args_count = syscon_get_args_by_name(node, "audcp_pmu_pwr_status4",
		sizeof(args), args);
	if (args_count == ARRAY_SIZE(args)) {
		dsp_ac->audcp_pmu_pwr_status4_reg = args[0];
		dsp_ac->audcp_pmu_sys_slp_state_mask = args[1];
		pr_err("audcp_pmu_sys_slp_state:reg:%x,mask:%x\n",
			args[0], args[1]);
	}

	args_count = syscon_get_args_by_name(node, "audcp_pmu_pwr_status3",
		sizeof(args), args);
	if (args_count == ARRAY_SIZE(args)) {
		dsp_ac->audcp_pmu_pwr_status3_reg = args[0];
		dsp_ac->audcp_pmu_slp_state_mask = args[1];
		pr_err("audcp_pmu_slp_state:reg:%x,mask:%x\n",
			args[0], args[1]);
	}

	args_count = syscon_get_args_by_name(node, "ap_access_ena",
		sizeof(args), args);
	if (args_count == ARRAY_SIZE(args)) {
		dsp_ac->ap_access_ena_reg = args[0];
		dsp_ac->ap_access_ena_mask = args[1];
		pr_err("ap_access_ena:reg:%x,mask:%x\n",
			args[0], args[1]);
	}
	dsp_ac->smem_size = sizeof(struct agdsp_access_state);
#ifdef CONFIG_SPRD_SIPC_V2
	dsp_ac->smem_phy_addr = smem_alloc(SIPC_ID_PSCP, dsp_ac->smem_size);
#else
	dsp_ac->smem_phy_addr = smem_alloc(dsp_ac->smem_size);
#endif
	if (dsp_ac->smem_phy_addr) {
#ifdef CONFIG_SPRD_SIPC_V2
		dsp_ac->state = shmem_ram_vmap_nocache(SIPC_ID_PSCP,
					       dsp_ac->smem_phy_addr,
					       sizeof(struct agdsp_access_state)
					       );
#else
		dsp_ac->state = shmem_ram_vmap_nocache(dsp_ac->smem_phy_addr,
					       sizeof(struct agdsp_access_state)
					       );
#endif
	} else {
		dsp_ac->state = kzalloc(sizeof(struct agdsp_access_state),
					GFP_KERNEL);
	}
	if (!dsp_ac->state) {
		pr_err("%s,shmem_ram_vmap_nocache return 0.\n", __func__);
		goto error;
	}

	dsp_ac->state->ap_enable_cnt = 0;
	dsp_ac->state->cp_enable_cnt = 0;

	spin_lock_init(&g_agdsp_access->spin_lock);

	if (g_agdsp_access->auto_agcp_access == 0) {
		pr_dbg("agdsp access init, ready.\n");
		if (!dsp_ac->auto_agcp_access) {
			regmap_update_bits(dsp_ac->agcp_ahb,
				dsp_ac->ap_access_ena_reg,
				dsp_ac->ap_access_ena_mask, 0);
		}
	}
	g_agdsp_access->thread =
		kthread_create(agdsp_access_init_thread, g_agdsp_access,
			       "agdsp_access");
	if (IS_ERR(g_agdsp_access->thread)) {
		pr_err(" g_agdsp_access->thread create failed\n");
		goto error;
	}
	wake_up_process(g_agdsp_access->thread);
	dsp_ac->ready = true;
	return 0;
error:
	if (!dsp_ac->smem_phy_addr && dsp_ac->state) {
		kfree(dsp_ac->state);
		dsp_ac->state = NULL;
	}
#ifdef CONFIG_SPRD_SIPC_V2
	if (dsp_ac->state)
		shmem_ram_unmap(SIPC_ID_PSCP, dsp_ac->state);
	if (dsp_ac->smem_phy_addr)
		smem_free(SIPC_ID_PSCP, dsp_ac->smem_phy_addr,
			  dsp_ac->smem_size);
#else
	if (dsp_ac->state)
		shmem_ram_unmap(dsp_ac->state);
	if (dsp_ac->smem_phy_addr)
		smem_free(dsp_ac->smem_phy_addr, dsp_ac->smem_size);
#endif
	kfree(g_agdsp_access);
	pr_err("agdsp_access init failed, exit.ret:%d\n", ret);

	return ret;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int agdsp_can_access(void);
int agdsp_can_access(void)
{
	int ret = 0;
	int val = 0;
	struct agdsp_access *dsp_ac = g_agdsp_access;

	if (!dsp_ac->auto_agcp_access) {
		ret = regmap_read(dsp_ac->agcp_ahb,
			dsp_ac->ap_access_ena_reg, &val);
		if (ret != 0) {
			pr_err("%s, regmap_read   AUDACCESS_APB_AGCP_CTRL error!\n",
			__func__);
			return 0;
		}
		if (!(dsp_ac->ap_access_ena_mask & val)) {
			pr_err("%s, AUDACCESS_APB_AGCP_ACCESS_EN not enable! value: %x\n",
				__func__, val);
			return 0;
		}
	}
	ret = regmap_read(dsp_ac->pmu_apb,
		dsp_ac->audcp_pmu_sleep_ctrl_reg, &val);
	if (ret != 0) {
		pr_err("%s, regmap_read   AUDACCESS_APB_AGCP_CTRL error!\n",
		__func__);
		return 0;
	}
	if (val & dsp_ac->audcp_pmu_sleep_ctrl_deepslp_mask) {
		pr_err("%s, agdsp AUDACCESS_PMU_APB_AGCP_DEEP_SLEEP !value : %x\n",
		__func__, val);
		return 0;
	}

	ret = regmap_read(dsp_ac->pmu_apb,
		dsp_ac->audcp_pmu_slp_status_reg, &val);
	if (ret != 0) {
		pr_err("%s, regmap_read AUDACCESS_REG_SYS_SLP_STATUS!\n",
		       __func__);
		return 0;
	}
	if ((val & dsp_ac->audcp_pmu_slp_status_mask) >>
		((ffs(dsp_ac->audcp_pmu_slp_status_mask)-1)) !=
		WAKEUP_LOCK_STATE) {
		pr_err("%s, BIT_PMU_APB_AGCP_SYS_SLP_STATUS not enable! value: %x\n",
		__func__, val);
		return 0;
	}

	ret = regmap_read(dsp_ac->pmu_apb,
		dsp_ac->audcp_pmu_pwr_status4_reg, &val);
	if (ret != 0) {
		pr_err("%s, regmap_read AUDACCESS_REG_AGCP_DSP_STATE!\n",
		       __func__);
		return 0;
	}
	if (((val & dsp_ac->audcp_pmu_sys_slp_state_mask)
		>> (ffs(dsp_ac->audcp_pmu_sys_slp_state_mask)-1))
			!= PD_WAKEUP_STATE) {
		pr_err("%s, AUDACCESS_BIT_AGCP_DSP_STATE not enable! value: %x\n",
		__func__, val);
		return 0;
	}

	ret = regmap_read(dsp_ac->pmu_apb,
		dsp_ac->audcp_pmu_pwr_status3_reg, &val);
	if (ret != 0) {
		pr_err("%s, regmap_read   AUDACCESS_REG_AGCP_SYS_STATE!\n",
		       __func__);
		return 0;
	}
	if (((val & dsp_ac->audcp_pmu_slp_state_mask)
		>> (ffs(dsp_ac->audcp_pmu_slp_state_mask)-1))
			!= PD_WAKEUP_STATE) {
		pr_err("%s, AUDACCESS_BIT_AGCP_SYS_STATE not enable! value: %x\n",
		__func__, val);
		return 0;
	}

	return 1;
}
/* Diekspor untuk konsumen =m (blob stok `audio-dsp-dump.ko` mengimpor
 * `agdsp_can_access`; provider ini =y jadi wajib lewat EXPORT_SYMBOL).
 */
EXPORT_SYMBOL_GPL(agdsp_can_access);

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int agdsp_access_enable(void);
int agdsp_access_enable(void)
{
	int ret = 0;
	int cnt = 0;
	int val = 0;
	struct agdsp_access *dsp_ac = g_agdsp_access;

	pr_dbg("%s entry auto agcp access=%d\n", __func__,
		dsp_ac->auto_agcp_access);
	if (!dsp_ac) {
		pr_err("agdsp access not init, exit\n");
		return -EINVAL;
	}

	if (!dsp_ac->ready || !dsp_ac->state) {
		pr_err("agdsp access not ready, dsp_ac->ready=%d, dsp_ac->state=%p\n",
			dsp_ac->ready, dsp_ac->state);
		return -EPROBE_DEFER;
	}

	spin_lock(&dsp_ac->spin_lock);

	if (!dsp_ac->auto_agcp_access) {
		ret = regmap_update_bits(dsp_ac->agcp_ahb,
			dsp_ac->ap_access_ena_reg,
			dsp_ac->ap_access_ena_mask,
			dsp_ac->ap_access_ena_mask);
		if (ret != 0) {
			pr_err("%s, regmap_update_bits AUDACCESS_APB_AGCP_CTRL error!\n",
			__func__);
			ret = -EINVAL;
			goto exit;
		}

		ret = regmap_read(dsp_ac->agcp_ahb,
			dsp_ac->ap_access_ena_reg, &val);
		if (ret != 0) {
			pr_err("%s, regmap_read AUDACCESS_APB_AGCP_CTRL error!\n",
				__func__);
			ret = -EINVAL;
			goto exit;
		}
		if (!(dsp_ac->ap_access_ena_mask & val)) {
			pr_err("%s, AUDACCESS_APB_AGCP_ACCESS_EN not enable! value: %x\n",
				__func__, val);
			ret = -EINVAL;
			goto exit;
		}
	}
	if (dsp_ac->state->ap_enable_cnt == 0) {
		/*
		 * send a mail to AGDSP to wake up it,
		 * 100 is an invalid command
		 */
		mbox_just_sent(dsp_ac->mbx_core, 100);
		udelay(20);

		do {
			cnt++;
			ret = regmap_read(dsp_ac->pmu_apb,
				dsp_ac->audcp_pmu_pwr_status4_reg, &val);
			pr_dbg("%s, regmap_read val=0x%x!,mask:%x, ffs_mask:%d\n",
				__func__, val,
				dsp_ac->audcp_pmu_sys_slp_state_mask,
				ffs(dsp_ac->audcp_pmu_sys_slp_state_mask));
			if (ret != 0) {
				pr_err("%s, regmap_read error!\n", __func__);
				break;
			}
			/* 0: power up finished;7:power off */
			if (((val & dsp_ac->audcp_pmu_sys_slp_state_mask) >>
				(ffs(dsp_ac->audcp_pmu_sys_slp_state_mask)-1))
					== PD_WAKEUP_STATE)
				break;

			/*
			 * Do not need delay,
			 * normally we need read only one time
			 */
			udelay(5);
		} while (cnt < TRY_CNT_MAX);
		if (cnt == TRY_CNT_MAX) {
			pr_err("ERR: %s wait agdsp power up timeout!\n",
				__func__);
			ret = -EBUSY;
			goto exit;
		}
	}

	AGCP_WRITEL(AGCP_READL(&dsp_ac->state->ap_enable_cnt) + 1,
		&dsp_ac->state->ap_enable_cnt);
	pr_dbg("%s out\n", __func__);

exit:
	spin_unlock(&dsp_ac->spin_lock);
	pr_dbg("%s, ap_enable_cnt=%d, cp_enable_cnt=%d.cnt=%d\n",
		__func__, dsp_ac->state->ap_enable_cnt,
		dsp_ac->state->cp_enable_cnt, cnt);

	return ret;
}
EXPORT_SYMBOL(agdsp_access_enable);


/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int agdsp_access_disable(void);
int agdsp_access_disable(void)
{
	int ret = 0;
	struct agdsp_access *dsp_ac = g_agdsp_access;

	pr_dbg("%s entry\n", __func__);
	if (!dsp_ac)
		return -EINVAL;

	if (!dsp_ac->ready || !dsp_ac->state)
		return -EINVAL;

	spin_lock(&dsp_ac->spin_lock);

	if (AGCP_READL(&dsp_ac->state->ap_enable_cnt) > 0) {
		AGCP_WRITEL(AGCP_READL(&dsp_ac->state->ap_enable_cnt) - 1,
			&dsp_ac->state->ap_enable_cnt);
	}
	if (!dsp_ac->auto_agcp_access) {
		if ((AGCP_READL(&dsp_ac->state->ap_enable_cnt) == 0)) {
			ret = regmap_update_bits(dsp_ac->agcp_ahb,
				dsp_ac->ap_access_ena_reg,
				dsp_ac->ap_access_ena_mask, 0);
			pr_dbg("%s,update register AUDACCESS_APB_AGCP_CTRL, ret=%d\n",
				__func__, ret);
		}
	}

	spin_unlock(&dsp_ac->spin_lock);

	pr_dbg("%s,dsp_ac->state->ap_enable_cnt=%d,dsp_ac->state->cp_enable_cnt=%d.\n",
		__func__, dsp_ac->state->ap_enable_cnt,
		dsp_ac->state->cp_enable_cnt);

	return 0;
}
EXPORT_SYMBOL(agdsp_access_disable);

static int restore_auto_access(void)
{
	return 0;
}

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int restore_access(void);
int restore_access(void)
{
	int ret = -EINVAL;
	int cnt = 0;
	int val;
	struct agdsp_access *dsp_ac = g_agdsp_access;

	pr_dbg("%s entry\n", __func__);
	if (!dsp_ac) {
		pr_err("agdsp access not init, exit\n");
		return -EINVAL;
	}

	if (dsp_ac->auto_agcp_access == 1) {
		pr_dbg("%s restore use auto access do nothing\n", __func__);
		return restore_auto_access();
	}

	if (!dsp_ac->ready || !dsp_ac->state) {
		pr_err("agdsp access not ready, dsp_ac->ready=%d, dsp_ac->state=%p\n",
			   dsp_ac->ready, dsp_ac->state);
		return -EINVAL;
	}

	spin_lock(&dsp_ac->spin_lock);

	if (!dsp_ac->auto_agcp_access) {
		ret = regmap_update_bits(dsp_ac->agcp_ahb,
			dsp_ac->ap_access_ena_reg,
			dsp_ac->ap_access_ena_mask,
			dsp_ac->ap_access_ena_mask);
		if (ret != 0) {
			pr_err("%s, regmap_update_bits AUDACCESS_APB_AGCP_CTRL error!\n",
			__func__);
			goto exit;
		}

		ret = regmap_read(dsp_ac->agcp_ahb,
			dsp_ac->ap_access_ena_reg, &val);
		if (ret != 0) {
			pr_err("%s, regmap_read AUDACCESS_APB_AGCP_CTRL error!\n",
				__func__);
			goto exit;
		}
		if (!(dsp_ac->ap_access_ena_mask & val)) {
			pr_err("%s, AUDACCESS_APB_AGCP_ACCESS_EN not enable! value: %x\n",
				__func__, val);
			goto exit;
		}
	}
	if (dsp_ac->state->ap_enable_cnt != 0) {
		/*
		 * send a mail to AGDSP to wake up it,
		 * 100 is an invalid command
		 */
		mbox_raw_sent(dsp_ac->mbx_core, 100);
		udelay(20);
		do {
			ret = regmap_read(dsp_ac->pmu_apb,
				dsp_ac->audcp_pmu_pwr_status4_reg, &val);
			pr_dbg("%s, regmap_read val=0x%x!\n", __func__, val);
			if (ret != 0) {
				pr_err("%s, regmap_read error!\n", __func__);
				break;
			}
			/* 0: power up finished;7:power off */
			if (((val & dsp_ac->audcp_pmu_sys_slp_state_mask) >>
				(ffs(dsp_ac->audcp_pmu_sys_slp_state_mask)-1))
					== PD_WAKEUP_STATE)
				break;
			udelay(5);
		} while (++cnt < TRY_CNT_MAX);
		if (cnt == TRY_CNT_MAX) {
			pr_err("ERR: %s wait agdsp power up timeout!\n",
				   __func__);
			ret = -EBUSY;
		}
	}
exit:
	regmap_read(dsp_ac->agcp_ahb, dsp_ac->ap_access_ena_reg, &val);
	pr_info("%s, ap_enable_cnt=%d, cp_enable_cnt=%d.cnt=%d, access value=%#x\n",
			__func__, dsp_ac->state->ap_enable_cnt,
			dsp_ac->state->cp_enable_cnt, cnt, val);
	spin_unlock(&dsp_ac->spin_lock);

	return ret;
}
EXPORT_SYMBOL(restore_access);


/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int disable_access_force(void);
int disable_access_force(void)
{
	int ret;
	int retval;
	struct agdsp_access *dsp_ac = g_agdsp_access;

	if (!dsp_ac) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (dsp_ac->auto_agcp_access == 1) {
		pr_dbg("%s disable_auto_access_force do nothing\n", __func__);
		return 0;
	}
	if (!dsp_ac->ready || !dsp_ac->state) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	spin_lock(&dsp_ac->spin_lock);

	ret = regmap_update_bits(dsp_ac->agcp_ahb,
		dsp_ac->ap_access_ena_reg,
		dsp_ac->ap_access_ena_mask, 0);
	regmap_read(dsp_ac->agcp_ahb, dsp_ac->ap_access_ena_reg, &retval);
	pr_info("%s,update register AUDACCESS_APB_AGCP_CTRL, ret=%d, access value =%#x\n",
		__func__, ret, retval);

	spin_unlock(&dsp_ac->spin_lock);

	return 0;
}
EXPORT_SYMBOL(disable_access_force);

/*
 * ==== RE B8/R1: dua ekspor blob agdsp_access.ko yang tidak ada di sumber GPL ====
 *
 * Blob vendor agdsp_access.ko (5.4.210, build 2023-11-07) mengekspor:
 *   agdsp_set_mboxchan     0x04a0..0x04ac  (12 B, 5 instruksi)
 *   agdsp_access_dumpreg   0x0a98..0x0c70  (472 B, 185 instruksi)
 * Bukti mentah + tabel offset struct: dumped/re/agdsp_access.ko.sym.txt,
 * dumped/re/agdsp_access.ko.agdsp_set_mboxchan.dis.txt,
 * dumped/re/agdsp_access.ko.agdsp_access_dumpreg.dis.txt; uraian:
 * dumped/re/R1_audio_4sym.md.
 *
 * Pemanggilnya blob audio_sipc.ko (audio_sipc_probe untuk set_mboxchan;
 * aud_smsg_irq_handler/aud_smsg_dump_func untuk dumpreg) — dua-duanya kini ada
 * di pohon port, jadi permukaan ekspor kedua modul kembali lengkap.
 */
struct mbox_chan;   /* cukup forward: blob hanya menyimpan pointer (8 B) */

/* .bss+0x8 di blob (nm: g_agdsp_mboxchan, static, 8 B). Diserahkan probe
 * audio_sipc (nilai yang sama dengan aud_smsg_set_mboxchan()), bukan hasil
 * request sendiri. */
static struct mbox_chan *g_agdsp_mboxchan;

/* Blob 0x4a0: adrp/str/ret — simpan argumen ke g_agdsp_mboxchan. */
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void agdsp_set_mboxchan(struct mbox_chan *chan);
void agdsp_set_mboxchan(struct mbox_chan *chan)
{
	g_agdsp_mboxchan = chan;
}
EXPORT_SYMBOL(agdsp_set_mboxchan);

/*
 * Blob 0xa98..0xc70: cetak state AP lalu 5 register AGDSP lewat regmap_read.
 * Urutan, field, dan format diambil persis dari blob (format .rodata blob
 * pada +0x10c3, +0x397, +0x528, +0xf15, +0xa91, +0x71d dan pasangan
 * error-nya), dan field struct-nya cocok satu-satu dengan struct agdsp_access
 * di berkas ini:
 *   agcp_ahb (+0x28) -> ap_access_ena_reg         (+0x5c)
 *   pmu_apb  (+0x30) -> audcp_pmu_sleep_ctrl_reg  (+0x3c)
 *                       audcp_pmu_slp_status_reg  (+0x44)
 *                       audcp_pmu_pwr_status3_reg (+0x54)
 *                       audcp_pmu_pwr_status4_reg (+0x4c)
 * (inspeksi 0x0a98 blok demi blok; tiap regmap_read diikuti cabang w0 != 0 ke
 * pr_err "... error!", jadi maknanya: berhasil -> pr_info nilai, gagal ->
 * pr_err nama register.)
 *
 * Catatan kesetiaan, sengaja TIDAK diubah: blob tidak memeriksa
 * g_agdsp_access == NULL, dan fungsi ini juga tidak. Pemanggil yang aman hanya
 * boleh memanggilnya setelah agdsp_access_initialize() (yang mengisi global
 * itu) — sama seperti di blob.
 */
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
void agdsp_access_dumpreg(void);
void agdsp_access_dumpreg(void)
{
	struct agdsp_access *dsp_ac = g_agdsp_access;
	u32 val;

	pr_info("[sprd-aud-agdsp] %s, ap_enable_cnt = %d\n",
		__func__, dsp_ac->state->ap_enable_cnt);

	if (!regmap_read(dsp_ac->agcp_ahb, dsp_ac->ap_access_ena_reg, &val))
		pr_info("[sprd-aud-agdsp] %s, ap_access_ena_reg (dsp assert) val = 0x%x\n",
			__func__, val);
	else
		pr_err("[sprd-aud-agdsp] %s, regmap_read ap_access_ena_reg error!\n",
		       __func__);

	if (!regmap_read(dsp_ac->pmu_apb, dsp_ac->audcp_pmu_sleep_ctrl_reg, &val))
		pr_info("[sprd-aud-agdsp] %s, audcp_pmu_sleep_ctrl_reg (dsp assert) val = 0x%x\n",
			__func__, val);
	else
		pr_err("[sprd-aud-agdsp] %s, regmap_read audcp_pmu_sleep_ctrl_reg error!\n",
		       __func__);

	if (!regmap_read(dsp_ac->pmu_apb, dsp_ac->audcp_pmu_slp_status_reg, &val))
		pr_info("[sprd-aud-agdsp] %s, audcp_pmu_slp_status_reg (dsp assert) val = 0x%x\n",
			__func__, val);
	else
		pr_err("[sprd-aud-agdsp] %s, regmap_read audcp_pmu_slp_status_reg error!\n",
		       __func__);

	if (!regmap_read(dsp_ac->pmu_apb, dsp_ac->audcp_pmu_pwr_status3_reg, &val))
		pr_info("[sprd-aud-agdsp] %s, audcp_pmu_pwr_status3_reg (dsp assert) val = 0x%x\n",
			__func__, val);
	else
		pr_err("[sprd-aud-agdsp] %s, regmap_read audcp_pmu_pwr_status3_reg error!\n",
		       __func__);

	if (!regmap_read(dsp_ac->pmu_apb, dsp_ac->audcp_pmu_pwr_status4_reg, &val))
		pr_info("[sprd-aud-agdsp] %s, audcp_pmu_pwr_status4_reg (dsp assert) val = 0x%x\n",
			__func__, val);
	else
		pr_err("[sprd-aud-agdsp] %s, regmap_read audcp_pmu_pwr_status4_reg error!\n",
		       __func__);
}
EXPORT_SYMBOL(agdsp_access_dumpreg);

#if defined(BIT_PMU_APB_XTL0_FRC_ON)
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int force_on_xtl(bool on_off);
int force_on_xtl(bool on_off)
{
	int ret;
	struct agdsp_access *dsp_ac = g_agdsp_access;
	unsigned int mask;

	pr_dbg("%s entry\n", __func__);
	if (!dsp_ac)
		return -EINVAL;

	mask = BIT_PMU_APB_XTL0_FRC_ON;
	ret = regmap_update_bits(dsp_ac->pmu_apb, REG_PMU_APB_XTL0_REL_CFG,
				 mask, on_off ? mask : 0);
	if (ret) {
		pr_err("%s, regmap_update_bits REG_PMU_APB_XTL0_REL_CFG error!\n",
			__func__);
		return ret;
	}

	mask = BIT_PMU_APB_XTLBUF0_FRC_ON;
	ret = regmap_update_bits(dsp_ac->pmu_apb, REG_PMU_APB_XTLBUF0_REL_CFG,
				 mask, on_off ? mask : 0);
	if (ret)
		pr_err("%s, regmap_update_bits REG_PMU_APB_XTLBUF0_REL_CFG error!\n",
			__func__);

	return ret;
}
#else
/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int force_on_xtl(bool on_off);
int force_on_xtl(bool on_off) { return 0; }
#endif

#if defined(CONFIG_DEBUG_FS)
static int agdsp_access_debug_info_show(struct seq_file *m, void *private)
{
	u32 val = 0;

	seq_printf(m, "ap_enable_cnt:%d  cp_enable_cnt=%d\n",
		g_agdsp_access->state->ap_enable_cnt,
		g_agdsp_access->state->cp_enable_cnt);
	seq_printf(m, "smem_phy_addr:0x%x  size=%d\n",
		g_agdsp_access->smem_phy_addr, g_agdsp_access->smem_size);
	seq_printf(m, "thread:0x%p\n", g_agdsp_access->thread);
	seq_printf(m, "ready:%d\n", g_agdsp_access->ready);
	seq_printf(m, "dst:%d  channel=%d\n",
		g_agdsp_access->dst, g_agdsp_access->channel);
	seq_printf(m, "agcp_ahb:0x%p  pmu_apb=0x%p\n",
		g_agdsp_access->agcp_ahb, g_agdsp_access->pmu_apb);

	regmap_read(g_agdsp_access->agcp_ahb,
		g_agdsp_access->ap_access_ena_reg, &val);
	seq_printf(m, "AUDACCESS_APB_AGCP_CTRL:0x%x (Bit0 0:sleep en)\n", val);

	regmap_read(g_agdsp_access->pmu_apb,
		g_agdsp_access->audcp_pmu_pwr_status4_reg, &val);
	seq_printf(m,
		"AUDACCESS_REG_AGCP_SYS_STATE:0x%x\n",
		val);

	regmap_read(g_agdsp_access->pmu_apb,
		g_agdsp_access->audcp_pmu_sleep_ctrl_reg, &val);
	seq_printf(m, "AUDACCESS_PMU_APB_SLEEP_CTRL:0x%x\n", val);

	return 0;
}

static int agdsp_access_debug_info_open(struct inode *inode, struct file *file)
{
	return single_open(file,
		agdsp_access_debug_info_show, inode->i_private);
}

static const struct file_operations agdsp_access_debug_info_fops = {
	.open = agdsp_access_debug_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init agdsp_access_init_debugfs_info(void)
{
	debugfs_create_file("agdsp_access",
		0444, NULL, NULL, &agdsp_access_debug_info_fops);

	return 0;
}

device_initcall(agdsp_access_init_debugfs_info);
#endif /* CONFIG_DEBUG_FS */

static int agdsp_access_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct regmap *agcp_ahb;
	struct regmap *pmu_apb;
	u32 mbx_core = 0;
	u32 dst = 0, channel = 0;
	int ret = 0;
	u32 offset;
	u32 auto_agcp_access;

	ret = of_property_read_u32(node, "sprd,mailbox-core", &mbx_core);
	if (ret) {
		pr_err("%s get mbx core err\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "sprd,dst", &dst);
	if (ret) {
		pr_err("%s get dst err\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "sprd,channel", &channel);
	if (ret) {
		pr_err("%s get channel err\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_bool(node, "sprd,ddr-addr-offset")) {
		if (of_property_read_u32
		    (node, "sprd,ddr-addr-offset", &offset)) {
			pr_err("%s, parse 'sprd,ddr-addr-offset' failed!\n",
			       __func__);
			return -EINVAL;
		}
	} else
		offset = 0;
	pr_dbg("%s ddr addr offset: %#x\n", __func__, offset);

	ret = of_property_read_u32(node, "sprd,auto_agcp_access",
		&auto_agcp_access);
	if (ret)
		auto_agcp_access = 0;
	pr_info("%s auto agcp access = %d\n", __func__, auto_agcp_access);
	agcp_ahb = syscon_regmap_lookup_by_phandle(node,
		"sprd,syscon-agcp-ahb");
	pmu_apb = syscon_regmap_lookup_by_phandle(node,
		"sprd,syscon-pmu-apb");
	if (IS_ERR(agcp_ahb) || IS_ERR(pmu_apb)) {
		pr_err("%s, agcp-ahb or pmu_apb not exist!\n", __func__);
	} else {
		pr_dbg("%s, agcp_ahb=0x%p, pmu_apb=0x%p\n",
			__func__, agcp_ahb, pmu_apb);
		return agdsp_access_initialize(pdev, node, agcp_ahb, pmu_apb,
			auto_agcp_access, mbx_core, dst, channel, offset);
	}

	return -EINVAL;
}

static const struct of_device_id agdsp_access_of_match[] = {
	{.compatible = "sprd,agdsp-access", },
	{ }
};

static struct platform_driver agdsp_access_driver = {
	.driver = {
		.name = "agdsp_access",
		.owner = THIS_MODULE,
		.of_match_table = agdsp_access_of_match,
	},
	.probe = agdsp_access_probe,
};

static int __init agdsp_access_init(void)
{
	int ret;

	ret = platform_driver_register(&agdsp_access_driver);

	return ret;
}

static void __exit agdsp_access_exit(void)
{
	platform_driver_unregister(&agdsp_access_driver);
}

module_init(agdsp_access_init);
module_exit(agdsp_access_exit);

MODULE_DESCRIPTION("agdsp access driver");
MODULE_AUTHOR("yintang.ren <yintang.ren@spreadtrum.com>");
MODULE_LICENSE("GPL");

