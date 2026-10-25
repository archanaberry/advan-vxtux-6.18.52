// SPDX-License-Identifier: GPL-2.0
/*
 * Spreadtrum SDHCI R11 additions: power-on write protect (POWP) user-space
 * interface, tuning read-back helper and the SWCQ request/IRQ entry points.
 *
 * Ported from the Unisoc UMS512 4.14 sources (sdhci-sprd-powp.c,
 * sdhci-sprd-tuning.c, sdhci-sprd-swcq.c) to the 6.18 MMC/SDHCI API.
 */

#include <linux/blkdev.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#include <linux/kconfig.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/sdio_func.h>

#include <linux/mmc/mmc.h>

/*
 * mmc_card_removed() and MMC_CARD_REMOVED live in drivers/mmc/core/card.h,
 * which is core-private and redefines one EXT_CSD_* constant that
 * <linux/mmc/mmc.h> already has.  Two macros are cheaper than that fight.
 */
#define MMC_CARD_REMOVED	(1 << 4)	/* card has been removed */
#define mmc_card_removed(c)	((c) && ((c)->state & MMC_CARD_REMOVED))

#include "sdhci.h"
#include "mmc_hsq.h"
#if IS_ENABLED(CONFIG_MMC_SWCQ)
#include "mmc_swcq.h"
#endif

#define SPRD_WP_MAGIC_KEY	0x53505244	/* "SPRD" */
#define SPRD_WP_MAX_DEVICES	256

#define MMC_SEND_WR_PROT_TYPE		31	/* not in MMC spec */
#define EXT_CSD_CLASS_6_CTRL		59
#define EXT_CSD_USR_WP			171
#define EXT_CSD_USR_WP_EN_PWR_WP		1

/*
 * The host only ever drives eMMC here (mmc0); the SD slot has no SWCQ/POWP.
 * Mirrors the vendor HOST_IS_EMMC_TYPE().
 */
#define SPRD_HOST_IS_EMMC(c) \
	(((c)->caps2 & (MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD)) == \
	 (MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD))

struct sprd_mmc_wp {
	bool inited;
	struct device dev;
	struct cdev chrdev;
	dev_t devt;
	struct mmc_host *mmc;
	unsigned int enabled;
	unsigned int grp_cnt;
	unsigned int grp_size;
	u64 start_addr;
	u64 prot_size;
};

static struct sprd_mmc_wp wp_dat;
static struct bus_type wp_bus_type;

int mmc_set_powp(struct mmc_card *card);
static void mmc_get_wp_current_status_and_type(struct mmc_card *card);

/*
 * sdhci_send_command() is static inside sdhci.c, and so is
 * sdhci_manual_cmd23().  Both are needed by the synchronous request paths
 * below, so sdhci.c exports one small pair of wrappers for them.  A copy of
 * the command-send path in this file would drift from upstream on every
 * merge; two forwarders cannot.
 */
bool sdhci_sprd_send_command(struct sdhci_host *host, struct mmc_command *cmd);

/*
 * mmc_cmdq_enable()/mmc_cmdq_disable() exist in the MMC core but are not
 * exported in 6.18, so the write-protect ioctl declares them itself.
 * ponytail: drop these two lines if upstream ever exports them.
 */
int mmc_cmdq_enable(struct mmc_card *card);
int mmc_cmdq_disable(struct mmc_card *card);

/* Prototypes for the exported entry points, so the build stays warning-free. */
int mmc_check_wp_fn(struct mmc_host *host);
int mmc_wp_init(struct mmc_host *mmc);
void mmc_wp_remove(struct mmc_host *mmc);
int mmc_send_tuning_read(struct mmc_host *mmc);
int sdhci_sprd_request_sync(struct mmc_host *mmc, struct mmc_request *mrq);
int mmc_hsq_swcq_init(struct sdhci_host *host, struct platform_device *pdev);
void sdhci_sprd_set_powp(struct mmc_card *card);
/* the bodies live in sdhci.c, which upstream keeps static */
int sprd_calc_hs_mode_tuning_range(u8 *value, int seq_a_off, int seq_b_off,
				   int *range, int max_range);
/*
 * The Unisoc SDHCI R11 driver ships its own renamed copy of the SDHCI core,
 * so the stock blob sdhci-sprd.ko carries these as local symbols (nm type
 * 't'): sdhci_sprd_cmd_irq, sdhci_sprd_data_irq, sdhci_sprd_prepare_data,
 * sdhci_sprd_transfer_pio and friends.  Each one is the same code, under the
 * name the blob uses.
 *
 * ponytail: the bodies live in sdhci.c, which upstream keeps static, so
 * sdhci.c exports them under '*_body' names and these wrap them.  When a
 * second vendor driver needs the same, move the bodies behind one
 * sdhci_ops entry instead.
 */
void sdhci_sprd_set_transfer_irqs(struct sdhci_host *host);
void sdhci_sprd_prepare_data(struct sdhci_host *host, struct mmc_command *cmd);
void sdhci_sprd_finish_command(struct sdhci_host *host);
void sdhci_sprd_cmd_irq(struct sdhci_host *host, u32 intmask, u32 *intmask_p);
void sdhci_sprd_data_irq(struct sdhci_host *host, u32 intmask);
void sdhci_sprd_transfer_pio(struct sdhci_host *host);
void sdhci_sprd_read_block_pio(struct sdhci_host *host);
void sdhci_sprd_write_block_pio(struct sdhci_host *host);
u8 sdhci_sprd_calc_timeout(struct sdhci_host *host, struct mmc_command *cmd,
			    bool *too_big);
irqreturn_t sdhci_sprd_irq(int irq, void *dev_id);
bool sdhci_sprd_send_command(struct sdhci_host *host, struct mmc_command *cmd);


int sdhci_sprd_execute_tuning(struct mmc_host *mmc, u32 opcode);
int sprd_calc_tuning_range(u8 *value, int seq_a_off, int seq_b_off,
			   int *range, int max_range);
int sprd_calc_hs_mode_tuning_range(u8 *value, int seq_a_off, int seq_b_off,
				   int *range, int max_range);
/* the bodies live in sdhci.c, which upstream keeps static */
void sdhci_sprd_set_transfer_irqs_body(struct sdhci_host *host);
void sdhci_sprd_prepare_data_body(struct sdhci_host *host,
				  struct mmc_command *cmd);
void sdhci_sprd_finish_command_body(struct sdhci_host *host);
void sdhci_sprd_cmd_irq_body(struct sdhci_host *host, u32 intmask,
			     u32 *intmask_p);
void sdhci_sprd_data_irq_body(struct sdhci_host *host, u32 intmask);
void sdhci_sprd_transfer_pio_body(struct sdhci_host *host);
void sdhci_sprd_read_block_pio_body(struct sdhci_host *host);
void sdhci_sprd_write_block_pio_body(struct sdhci_host *host);
u8 sdhci_sprd_calc_timeout_body(struct sdhci_host *host,
				 struct mmc_command *cmd, bool *too_big);
irqreturn_t sdhci_sprd_irq_body(int irq, void *dev_id);
bool sdhci_sprd_send_command_body(struct sdhci_host *host,
				   struct mmc_command *cmd);
/*
 * sdhci_sprd_irq() - the stock blob's renamed sdhci_irq().  The tree's
 * raw_sdhci_irq() above is this handler with the SW-CQ's card-edge handling;
 * the blob keeps both, the generic one and the SW-CQ one.
 */
irqreturn_t sdhci_sprd_irq(int irq, void *dev_id)
{
	return sdhci_sprd_irq_body(irq, dev_id);
}

/*
 * sdhci_sprd_send_command() - the blob's renamed sdhci_send_command().
 *
 * The body is NOT defined here: sdhci.c already provides and exports this
 * exact forwarder (see the comment there).  Keeping a second copy worked
 * while this file was a module, but breaks the moment CONFIG_MMC_SDHCI_SPRD=y
 * links it into vmlinux -- "multiple definition of sdhci_sprd_send_command".
 * One definition, in sdhci.c, works for both =m and =y.
 */
void sdhci_sprd_int_status(struct sdhci_host *host);
void sdhci_sprd_signal_voltage_on_off(struct mmc_host *mmc, bool on);
int sdhci_sprd_irq_request_swcq(struct sdhci_host *host);
/*
 * The stock blob carries its own renamed copy of the SDHCI core, so it has
 * these as local symbols.  Upstream 6.18 already exports the bodies, so
 * each one here is a name-preserving forwarder rather than a second copy
 * of sdhci.c.
 */
void sdhci_sprd_dumpregs(struct sdhci_host *host);
void _sdhci_sprd_set_clock(struct sdhci_host *host, unsigned int clock);
void __sdhci_sprd_finish_mrq(struct sdhci_host *host, struct mmc_request *mrq);
void __sdhci_sprd_finish_data(struct sdhci_host *host);
int _sdhci_request_atomic(struct mmc_host *mmc, struct mmc_request *mrq);
void sdhci_sprd_enable_sdio_irq_nolock(struct sdhci_host *host, int enable);

static inline bool sdhci_sprd_manual_cmd23(struct sdhci_host *host,
					   struct mmc_request *mrq)
{
	return mrq->sbc && !(host->flags & SDHCI_AUTO_CMD23);
}

/* ------------------------------------------------------------------ */
/* power-on write protect: status/type read-back                       */
/* ------------------------------------------------------------------ */

noinline static int mmc_get_wp_current_status(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};
	struct mmc_data data = {0};
	struct scatterlist sg;
	u64 wp_bits_sts = 0;
	unsigned long long size = 4;
	unsigned int i;
	int err = 0;
	u8 *status_buf;

	mrq.cmd = &cmd;
	mrq.data = &data;

	status_buf = kzalloc(size, GFP_KERNEL);
	if (!status_buf) {
		pr_err("%s:(WP) status_buf create false\n", mmc_hostname(host));
		return -ENOMEM;
	}

	/* CMD30 gets the write-protect bit per group */
	cmd.opcode = MMC_SEND_WRITE_PROT;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.arg = wp_dat.start_addr;

	data.blksz = size;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.blk_addr = 0;
	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, status_buf, size);

	mmc_wait_for_req(host, &mrq);

	if (cmd.error) {
		pr_err("%s:(WP) status cmd error %d\n", mmc_hostname(host), cmd.error);
		err = cmd.error;
		goto out;
	}

	if (data.error) {
		pr_err("%s:(WP) status data error %d\n", mmc_hostname(host), data.error);
		err = data.error;
		goto out;
	}

	for (i = 0; i < 4; i++)
		wp_bits_sts = (wp_bits_sts << 8) + status_buf[i];

	pr_info("%s:(WP) the status of write protection bits: 0x%llx\n",
		mmc_hostname(host), wp_bits_sts);
out:
	kfree(status_buf);
	return err;
}

noinline static int mmc_get_wp_current_type(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};
	struct mmc_data data = {0};
	struct scatterlist sg;
	unsigned long long size = 8;
	u64 actual_type = 0;
	u64 expected_type = 0;
	unsigned int i;
	int err = 0;
	u8 *type_buf;

	mrq.cmd = &cmd;
	mrq.data = &data;

	type_buf = kzalloc(size, GFP_KERNEL);
	if (!type_buf) {
		pr_err("%s:(WP) type_buf create false\n", mmc_hostname(host));
		return -ENOMEM;
	}

	/* CMD31 (Unisoc ext) gets the per-group protection type */
	cmd.opcode = MMC_SEND_WR_PROT_TYPE;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.arg = wp_dat.start_addr;

	data.blksz = size;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.blk_addr = 0;
	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, type_buf, size);

	mmc_wait_for_req(host, &mrq);

	if (cmd.error) {
		pr_err("%s:(WP) status cmd error %d\n", mmc_hostname(host), cmd.error);
		err = cmd.error;
		goto out;
	}

	if (data.error) {
		pr_err("%s:(WP) status data error %d\n", mmc_hostname(host), data.error);
		err = data.error;
		goto out;
	}

	for (i = 0; i < 8; i++)
		actual_type = (actual_type << 8) + type_buf[i];

	pr_info("%s:(WP) the really type of write protection grout: 0x%llx\n",
		mmc_hostname(host), actual_type);

	for (i = 0; i < wp_dat.grp_cnt; i++)
		expected_type = (expected_type << 2) + (wp_dat.enabled ? 0x2 : 0);

	pr_info("%s:(WP) the expected type of write protection group: 0x%llx\n",
		mmc_hostname(host), expected_type);

	if (actual_type != expected_type) {
		pr_err("%s:(WP) actual type not equal to expected type\n",
		       mmc_hostname(host));
		err = -EINVAL;
	}
out:
	kfree(type_buf);
	return err;
}

noinline void mmc_get_wp_current_status_and_type(struct mmc_card *card)
{
	mmc_get_wp_current_status(card);
	mmc_get_wp_current_type(card);
}

/* ------------------------------------------------------------------ */
/* power-on write protect: applying the protection                     */
/* ------------------------------------------------------------------ */

noinline static int mmc_enable_usr_wp(struct mmc_card *card, unsigned char wp)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};

	mrq.cmd = &cmd;

	/* clear CLASS_6 control */
	cmd.opcode = MMC_SWITCH;
	cmd.arg = (MMC_SWITCH_MODE_CLEAR_BITS << 24) |
		  (EXT_CSD_CLASS_6_CTRL << 16) | EXT_CSD_CMD_SET_NORMAL;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	mmc_wait_for_req(host, &mrq);
	if (cmd.error) {
		pr_err("%s:(WP) set class6 ctrl, cmd error %d\n",
		       mmc_hostname(host), cmd.error);
		return cmd.error;
	}

	/* reset ERASE_GROUP_DEF to 1 */
	cmd.arg = (MMC_SWITCH_MODE_CLEAR_BITS << 24) |
		  (EXT_CSD_ERASE_GROUP_DEF << 16) | (1 << 8) |
		  EXT_CSD_CMD_SET_NORMAL;
	mmc_wait_for_req(host, &mrq);
	if (cmd.error) {
		pr_err("%s:(WP) set erase group def fail, cmd error %d\n",
		       mmc_hostname(host), cmd.error);
		return cmd.error;
	}

	/* set USR_WP */
	cmd.arg = (MMC_SWITCH_MODE_SET_BITS << 24) | (EXT_CSD_USR_WP << 16) |
		  (wp << 8) | EXT_CSD_CMD_SET_NORMAL;
	mmc_wait_for_req(host, &mrq);
	if (cmd.error) {
		pr_err("%s:(WP) set usr_wp fail, cmd error %d\n",
		       mmc_hostname(host), cmd.error);
		return cmd.error;
	}

	return 0;
}

noinline static int mmc_set_usr_wp(struct mmc_card *card, unsigned char wp_type)
{
	struct mmc_host *host = card->host;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};
	unsigned int i;
	int err;

	err = mmc_enable_usr_wp(card, wp_type);
	if (err)
		return err;

	mrq.cmd = &cmd;
	cmd.opcode = MMC_SET_WRITE_PROT;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	for (i = 0; i < wp_dat.grp_cnt; i++) {
		cmd.arg = wp_dat.start_addr + (u64)i * wp_dat.grp_size;
		mmc_wait_for_req(host, &mrq);
		if (cmd.error) {
			pr_err("%s:(WP) set fail, cmd error %d\n",
			       mmc_hostname(host), cmd.error);
			return cmd.error;
		}
	}

	return 0;
}

noinline static int mmc_get_wp_params(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	struct device_node *np = dev_of_node(host->parent);
	u32 data[2];
	u64 temp64;
	int err;

	if (!np)
		return -ENODEV;

	err = of_property_read_u32_array(np, "sprd,part-address-size",
					  data, ARRAY_SIZE(data));
	if (err) {
		pr_err("%s:(WP) no sprd,part-address-size setting in node\n",
		       mmc_hostname(host));
		return err;
	}

	wp_dat.start_addr = data[0];
	wp_dat.prot_size = data[1];

	wp_dat.grp_size = card->ext_csd.hc_erase_size *
			   card->ext_csd.raw_hc_erase_gap_size;
	if (!wp_dat.grp_size)
		return -EINVAL;

	temp64 = wp_dat.prot_size;
	if (do_div(temp64, wp_dat.grp_size)) {
		pr_err("%s:(WP) partition is not aligned\n", mmc_hostname(host));
		return -EINVAL;
	}
	wp_dat.grp_cnt = (unsigned int)temp64;

	pr_info("%s:(WP) start address: 0x%llx, size: 0x%llx, group cnt: 0x%x\n",
		mmc_hostname(host), wp_dat.start_addr, wp_dat.prot_size,
		wp_dat.grp_cnt);

	return 0;
}

/**
 * mmc_set_powp() - apply the power-on write protect state to the eMMC
 * @card: eMMC card to program
 *
 * Reads the partition geometry from the DT, then either reports the current
 * protection state or programs the erase groups for power-on write protect.
 */
int mmc_set_powp(struct mmc_card *card)
{
	struct mmc_host *mmc = card->host;
	int err;

	if (mmc->index != 0) {
		pr_err("%s:(WP) mmc index err\n", mmc_hostname(mmc));
		return -EINVAL;
	}

	err = mmc_get_wp_params(card);
	if (err)
		pr_err("%s:(WP) get params err\n", mmc_hostname(mmc));

	if (!wp_dat.enabled) {
		mmc_get_wp_current_status_and_type(card);
		pr_info("%s:(WP) status is disabled\n", mmc_hostname(mmc));
		return 0;
	}

	err = mmc_set_usr_wp(card, EXT_CSD_USR_WP_EN_PWR_WP);
	if (err)
		return err;

	mmc_get_wp_current_status_and_type(card);
	pr_info("%s:(WP) status is enabled\n", mmc_hostname(mmc));

	return 0;
}
EXPORT_SYMBOL_GPL(mmc_set_powp);

/**
 * mmc_check_wp_fn() - is the POWP user-space interface enabled in DT?
 * @host: host whose DT node is queried
 *
 * Return: 0 when "sprd,wp_fn" is "enable", <0 otherwise.
 */
int mmc_check_wp_fn(struct mmc_host *host)
{
	struct device_node *np = dev_of_node(host->parent);
	const char *name;
	int ret;

	if (!np)
		return -EINVAL;

	ret = of_property_read_string(np, "sprd,wp_fn", &name);
	if (ret) {
		pr_err("%s:(WP) can not read the property of sprd wp_fn\n",
		       mmc_hostname(host));
		return ret;
	}

	return strcmp(name, "enable") ? -EINVAL : 0;
}
EXPORT_SYMBOL_GPL(mmc_check_wp_fn);

/* ------------------------------------------------------------------ */
/* power-on write protect: char device + ioctl                          */
/* ------------------------------------------------------------------ */

struct sprd_mmc_wp_ioc_data {
	struct mmc_ioc_cmd ic;
	unsigned char *buf;
	u64 buf_bytes;
	void *reserved;
};

static struct sprd_mmc_wp_ioc_data *
mmc_wp_ioctl_copy_from_user(struct mmc_ioc_cmd __user *user)
{
	struct sprd_mmc_wp_ioc_data *idata;

	idata = kmalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return ERR_PTR(-ENOMEM);

	if (copy_from_user(&idata->ic, user, sizeof(idata->ic))) {
		kfree(idata);
		return ERR_PTR(-EFAULT);
	}

	idata->buf_bytes = (u64)idata->ic.blksz * idata->ic.blocks;
	if (!idata->buf_bytes) {
		kfree(idata);
		return ERR_PTR(-EINVAL);
	}

	idata->buf = memdup_user((void __user *)(unsigned long)
				 idata->ic.data_ptr, idata->buf_bytes);
	if (IS_ERR(idata->buf)) {
		int err = PTR_ERR(idata->buf);

		kfree(idata);
		return ERR_PTR(err);
	}

	return idata;
}

noinline static int mmc_wp_ioctl_cmd(struct mmc_host *host, struct mmc_ioc_cmd __user *ic_ptr)
{
	struct sprd_mmc_wp_ioc_data *idata;
	struct mmc_card *card = host->card;
	struct sdio_func sf = { .card = card };
	bool cmdq_dis = false;
	int err = 0;

	if (!card || mmc_card_removed(card))
		return -ENODEV;

	idata = mmc_wp_ioctl_copy_from_user(ic_ptr);
	if (IS_ERR(idata)) {
		pr_err("%s:(WP) copy from user failed\n", mmc_hostname(host));
		return PTR_ERR(idata);
	}

	if (idata->ic.opcode != SPRD_WP_MAGIC_KEY) {
		pr_err("%s:(WP) user opcode invalid\n", mmc_hostname(host));
		err = -EINVAL;
		goto out_free;
	}

	if (idata->ic.arg == wp_dat.enabled) {
		if (wp_dat.enabled)
			pr_info("%s:(WP) no need to reset, status is enabled\n",
				mmc_hostname(host));
		else
			pr_info("%s:(WP) no need to reset, status is disabled\n",
				mmc_hostname(host));
		goto out_free;
	}

	/*
	 * Serialise against the block queue, then quiesce CQE/HSQ before
	 * toggling the protection bit, as the reset below re-inits the card.
	 *
	 * ponytail: 6.18's mmc_queue has no user-visible 'busy' flag that a char
	 * device can usefully claim (it tracks internal recovery state only),
	 * so gate on the host's queue state instead. Replace with a real
	 * mq->busy claim once the SWCQ block layer is ported.
	 */
	if (host->cqe_enabled && host->cqe_ops) {
		err = host->cqe_ops->cqe_wait_for_idle(host);
		if (err) {
			pr_err("%s:(WP) wait for idle fail\n",
			       mmc_hostname(host));
			goto out_free;
		}
	}

	sdio_claim_host(&sf);
	if (card->ext_csd.cmdq_en) {
		err = mmc_cmdq_disable(card);
		if (err) {
			pr_err("%s:(WP) cmdq disable fail, err=%d\n",
			       mmc_hostname(host), err);
			goto cmd_done;
		}
		cmdq_dis = true;
	}

	/* reset eMMC so the new power-on write protect state takes effect */
	wp_dat.enabled = idata->ic.arg;
	err = mmc_hw_reset(card);

cmd_done:
	if (cmdq_dis) {
		int err2 = mmc_cmdq_enable(card);

		if (err2) {
			pr_err("%s:(WP) cmdq enable fail, err=%d\n",
			       mmc_hostname(host), err2);
			mmc_hw_reset(card);
		}
	}

	sdio_release_host(&sf);
out_free:
	kfree(idata->buf);
	kfree(idata);

	return err;
}

static long mmc_wp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mmc_host *host = file->private_data;

	if (cmd != MMC_IOC_CMD)
		return -EINVAL;

	return mmc_wp_ioctl_cmd(host, (struct mmc_ioc_cmd __user *)arg);
}

#ifdef CONFIG_COMPAT
static long mmc_wp_ioctl_compat(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	return mmc_wp_ioctl(file, cmd, arg);
}
#endif

static int mmc_wp_chrdev_open(struct inode *inode, struct file *filp)
{
	struct sprd_mmc_wp *wp = container_of(inode->i_cdev,
					     struct sprd_mmc_wp, chrdev);

	get_device(&wp->dev);
	filp->private_data = wp->mmc;

	return nonseekable_open(inode, filp);
}

static int mmc_wp_chrdev_release(struct inode *inode, struct file *filp)
{
	struct sprd_mmc_wp *wp = container_of(inode->i_cdev,
					     struct sprd_mmc_wp, chrdev);

	put_device(&wp->dev);
	return 0;
}

static const struct file_operations wp_fileops = {
	.owner = THIS_MODULE,
	.open = mmc_wp_chrdev_open,
	.release = mmc_wp_chrdev_release,
	.unlocked_ioctl = mmc_wp_ioctl,
	.llseek = noop_llseek,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mmc_wp_ioctl_compat,
#endif
};

/**
 * mmc_wp_init() - register the power-on write protect char device
 * @mmc: host that owns the char device
 *
 * Return: 0 on success, negative errno on failure.
 */
int mmc_wp_init(struct mmc_host *mmc)
{
	char name[64];
	int res;

	if (mmc->index != 0 || wp_dat.inited)
		return 0;

	if (!wp_bus_type.name) {
		wp_bus_type.name = "mmc_wp";
		res = bus_register(&wp_bus_type);
		if (res < 0) {
			pr_err("%s:(WP) could not register WP bus type\n",
			       mmc_hostname(mmc));
			wp_bus_type.name = NULL;
			return res;
		}
	}

	snprintf(name, sizeof(name), "%s_wp", mmc_hostname(mmc));
	res = alloc_chrdev_region(&wp_dat.devt, 0, SPRD_WP_MAX_DEVICES, name);
	if (res < 0) {
		pr_err("%s:(WP) failed to allocate chrdev region\n",
		       mmc_hostname(mmc));
		goto out_bus_unreg;
	}

	wp_dat.dev.init_name = name;
	wp_dat.dev.bus = &wp_bus_type;
	wp_dat.dev.devt = MKDEV(MAJOR(wp_dat.devt), 1);
	wp_dat.dev.parent = &mmc->class_dev;
	device_initialize(&wp_dat.dev);

	dev_set_drvdata(&wp_dat.dev, &wp_dat);

	cdev_init(&wp_dat.chrdev, &wp_fileops);
	wp_dat.chrdev.owner = THIS_MODULE;

	res = cdev_device_add(&wp_dat.chrdev, &wp_dat.dev);
	if (res) {
		pr_err("%s:(WP) could not add character device\n",
		       mmc_hostname(mmc));
		goto out_put_device;
	}

	wp_dat.mmc = mmc;
	pr_info("%s:(WP) create chardev(%d:1)\n", mmc_hostname(mmc),
		MAJOR(wp_dat.devt));
	wp_dat.inited = true;
	wp_dat.enabled = 1;

	return 0;

out_put_device:
	put_device(&wp_dat.dev);
	unregister_chrdev_region(wp_dat.devt, SPRD_WP_MAX_DEVICES);
out_bus_unreg:
	if (!wp_dat.inited) {
		bus_unregister(&wp_bus_type);
		wp_bus_type.name = NULL;
	}
	return res;
}
EXPORT_SYMBOL_GPL(mmc_wp_init);

/**
 * mmc_wp_remove() - tear down the power-on write protect char device
 * @mmc: host that owns the char device
 */
void mmc_wp_remove(struct mmc_host *mmc)
{
	if (mmc->index != 0 || !wp_dat.inited)
		return;

	put_device(&wp_dat.dev);
	unregister_chrdev_region(wp_dat.devt, SPRD_WP_MAX_DEVICES);

	bus_unregister(&wp_bus_type);
	wp_bus_type.name = NULL;

	wp_dat.inited = false;
	wp_dat.enabled = 0;
}
EXPORT_SYMBOL_GPL(mmc_wp_remove);

/* ------------------------------------------------------------------ */
/* tuning read-back                                                     */
/* ------------------------------------------------------------------ */

/**
 * mmc_send_tuning_read() - probe bus integrity with a single 512B read
 * @host: host to read from
 *
 * Used by the DLL delay search to verify the sampled delay actually moves
 * data. Reads block 0 of the card.
 *
 * Return: 0 on success, negative errno on failure.
 */
int mmc_send_tuning_read(struct mmc_host *host)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	int size = 512;
	int err = 0;
	u8 *data_buf;

	data_buf = kzalloc(size, GFP_KERNEL);
	if (!data_buf) {
		pr_err("%s: data_buf create false\n", mmc_hostname(host));
		return -ENOMEM;
	}

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_ADTC;

	data.blksz = size;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.blk_addr = 0;
	data.timeout_ns = 100000000;
	data.timeout_clks = 0;

	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, data_buf, size);

	mmc_wait_for_req(host, &mrq);

	if (cmd.error)
		err = cmd.error;
	else if (data.error)
		err = data.error;

	kfree(data_buf);

	return err;
}
EXPORT_SYMBOL_GPL(mmc_send_tuning_read);

/* ------------------------------------------------------------------ */
/* SWCQ request paths                                                   */
/* ------------------------------------------------------------------ */

/*
 * The software queue calls into these instead of sdhci_request(), because that
 * path takes host->lock itself and completes requests via complete_work. These
 * take the lock and issue the command inline, which is what the queue's
 * polling loop needs in order to observe the inhibit bits itself.
 *
 * sdhci_send_command() is static in sdhci.c, so presence is checked with the
 * exported sdhci_present_error() and the command is handed to the generic
 * sdhci_request_atomic() once the host is known alive.
 */

/**
 * sdhci_sprd_request_sync() - send one request with host->lock already held
 * @mmc: host
 * @mrq: request to send
 *
 * Return: 0 on success, -EBUSY if the controller has not released its
 * inhibit bits, in which case the caller retries in non-atomic context.
 */
int sdhci_sprd_request_sync(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	int ret = 0;

	if (sdhci_present_error(host, mrq->cmd, true)) {
		sdhci_finish_mrq(host, mrq);
		goto out;
	}

	cmd = sdhci_sprd_manual_cmd23(host, mrq) ? mrq->sbc : mrq->cmd;

	if (sdhci_sprd_send_command(host, cmd))
		ret = -EBUSY;

out:
	return ret;
}
EXPORT_SYMBOL_GPL(sdhci_sprd_request_sync);

/**
 * _sdhci_request_atomic() - issue one request from atomic (hard IRQ) context
 * @mmc: host
 * @mrq: request to send
 *
 * Return: 0 on success, -EBUSY if the controller is still busy.
 */
int _sdhci_request_atomic(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sdhci_host *host = mmc_priv(mmc);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&host->lock, flags);
	ret = sdhci_sprd_request_sync(mmc, mrq);
	spin_unlock_irqrestore(&host->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(_sdhci_request_atomic);

/**
 * mmc_hsq_swcq_init() - attach the right software queue for this host
 * @host: SDHCI host
 * @pdev: platform device backing the host
 *
 * eMMC hosts get the Unisoc software command queue (mmc_swcq_init), SD hosts
 * the upstream HSQ (mmc_hsq_init).
 *
 * Return: 0 on success, negative errno on failure.
 */
int mmc_hsq_swcq_init(struct sdhci_host *host, struct platform_device *pdev)
{
	struct mmc_hsq *hsq;

	/*
	 * Decoded from the stock sdhci-sprd.ko (mmc_hsq_swcq_init @ 0x4af0):
	 * eMMC hosts are the ones with
	 *	(caps2 & (MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD)) == both,
	 * which sdhci_sprd_probe() sets for non-removable controllers.  They
	 * get the Unisoc software command queue; SD/SDIO hosts keep HSQ.
	 */
#if IS_ENABLED(CONFIG_MMC_SWCQ)
	if ((host->mmc->caps2 & (MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD)) ==
	    (MMC_CAP2_NO_SDIO | MMC_CAP2_NO_SD)) {
		struct mmc_swcq *swcq;

		swcq = devm_kzalloc(&pdev->dev, sizeof(*swcq), GFP_KERNEL);
		if (!swcq)
			return -ENOMEM;

		return mmc_swcq_init(swcq, host->mmc);
	}
#endif

	hsq = devm_kzalloc(&pdev->dev, sizeof(*hsq), GFP_KERNEL);
	if (!hsq)
		return -ENOMEM;

	return mmc_hsq_init(hsq, host->mmc);
}
EXPORT_SYMBOL_GPL(mmc_hsq_swcq_init);

/*
 * SWCQ's threaded half of the IRQ.  sdhci_thread_irq() is the upstream
 * handler and the stock blob keeps its own copy here, because the SW-CQ
 * completion path must reach the host's request-done hook without going
 * through complete_work.
 *
 * ponytail: this forwards to the upstream threaded handler, so the SW-CQ
 * still completes requests through complete_work.  Replace with the
 * SW-CQ's own completion drain once mmc_swcq.c is in the tree.
 */
static irqreturn_t raw_sdhci_thread_irq(int irq, void *dev_id)
{
	return sdhci_thread_irq(irq, dev_id);
}

/*
 * SWCQ's raw hard-IRQ handler. sdhci_irq() is static in sdhci.c and hands
 * command/data completion to complete_work, which the SWCQ completion path
 * must not race with, so the SWCQ carries its own copy of the top-level
 * handler and re-requests it after every runtime resume.
 *
 * ponytail: this is the upstream handler with host->ops->irq kept, because
 * that hook is where the SWCQ injects its own command/data handling. Replace
 * with a single shared sdhci_irq() + a queue pointer in sdhci_host once
 * sdhci.c grows an sdhci_ops entry for the hard-IRQ half.
 */
static irqreturn_t raw_sdhci_irq(int irq, void *dev_id)
{
	struct sdhci_host *host = dev_id;
	u32 intmask, mask, unexpected = 0;
	int max_loops = 16;

	spin_lock(&host->lock);

	if (host->runtime_suspended) {
		spin_unlock(&host->lock);
		return IRQ_NONE;
	}

	intmask = sdhci_readl(host, SDHCI_INT_STATUS);
	if (!intmask || intmask == 0xffffffff) {
		spin_unlock(&host->lock);
		return IRQ_NONE;
	}

	do {
		if (host->ops->irq) {
			intmask = host->ops->irq(host, intmask);
			if (!intmask)
				goto cont;
		}

		/* Clear selected interrupts. */
		mask = intmask & (SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK |
				  SDHCI_INT_BUS_POWER);
		sdhci_writel(host, mask, SDHCI_INT_STATUS);

		if (intmask & (SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE)) {
			u32 present = sdhci_readl(host, SDHCI_PRESENT_STATE) &
				      SDHCI_CARD_PRESENT;

			/*
			 * Only watch the edge that does not immediately
			 * re-fire, otherwise the interrupt storms.
			 */
			host->ier &= ~(SDHCI_INT_CARD_INSERT |
				       SDHCI_INT_CARD_REMOVE);
			host->ier |= present ? SDHCI_INT_CARD_REMOVE :
					       SDHCI_INT_CARD_INSERT;
			sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
			sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);

			sdhci_writel(host, intmask & (SDHCI_INT_CARD_INSERT |
						     SDHCI_INT_CARD_REMOVE),
				     SDHCI_INT_STATUS);

			host->thread_isr |= intmask &
					   (SDHCI_INT_CARD_INSERT |
					    SDHCI_INT_CARD_REMOVE);
		}

		if (intmask & (SDHCI_INT_RETUNE))
			mmc_retune_needed(host->mmc);

		if ((intmask & SDHCI_INT_CARD_INT) &&
		    (host->ier & SDHCI_INT_CARD_INT)) {
			sdhci_sprd_enable_sdio_irq_nolock(host, false);
			sdio_signal_irq(host->mmc);
		}

		intmask &= ~(SDHCI_INT_CARD_INSERT | SDHCI_INT_CARD_REMOVE |
			     SDHCI_INT_CMD_MASK | SDHCI_INT_DATA_MASK |
			     SDHCI_INT_ERROR | SDHCI_INT_BUS_POWER |
			     SDHCI_INT_RETUNE | SDHCI_INT_CARD_INT);

		if (intmask) {
			unexpected |= intmask;
			sdhci_writel(host, intmask, SDHCI_INT_STATUS);
		}
cont:
		intmask = sdhci_readl(host, SDHCI_INT_STATUS);
	} while (intmask && --max_loops);

	spin_unlock(&host->lock);

	if (unexpected) {
		pr_err("%s: Unexpected interrupt 0x%08x.\n",
		       mmc_hostname(host->mmc), unexpected);
		sdhci_dumpregs(host);
	}

	return IRQ_HANDLED;
}

/**
 * sdhci_sprd_irq_request_swcq() - re-hook the host IRQ for SWCQ eMMC hosts
 * @host: SDHCI host
 *
 * sdhci_runtime_resume_host() re-arms the shared threaded IRQ with the
 * generic handlers, which would drop the SWCQ handlers on every resume, so
 * re-request it here.
 *
 * Return: 0 on success, negative errno on failure.
 */
int sdhci_sprd_irq_request_swcq(struct sdhci_host *host)
{
	if (!SPRD_HOST_IS_EMMC(host->mmc))
		return 0;

	free_irq(host->irq, host);

	return request_threaded_irq(host->irq, raw_sdhci_irq,
				    raw_sdhci_thread_irq, IRQF_SHARED,
				    mmc_hostname(host->mmc), host);
}
EXPORT_SYMBOL_GPL(sdhci_sprd_irq_request_swcq);

/* ---- SDHCI-core name forwarders (stock blob's local symbols) ---- */

void sdhci_sprd_dumpregs(struct sdhci_host *host)
{
	sdhci_dumpregs(host);
}

/*
 * sdhci_sprd_int_status() - the stock blob's interrupt-status dump.  It
 * reads the same register set as upstream's sdhci_dumpregs() and prints it
 * with print_hex_dump(), so it forwards there.
 */
void sdhci_sprd_int_status(struct sdhci_host *host)
{
	sdhci_dumpregs(host);
}

void _sdhci_sprd_set_clock(struct sdhci_host *host, unsigned int clock)
{
	sdhci_set_clock(host, clock);
}

void __sdhci_sprd_finish_mrq(struct sdhci_host *host, struct mmc_request *mrq)
{
	__sdhci_finish_mrq(host, mrq);
}

void __sdhci_sprd_finish_data(struct sdhci_host *host)
{
	__sdhci_finish_data_common(host, false);
}

/*
 * sdhci_sprd_set_powp() - the stock blob's sdhci_ops->set_power hook for the
 * power-on write protect state.  The work itself is mmc_set_powp() above;
 * this is the symbol the blob carries.
 */
void sdhci_sprd_set_powp(struct mmc_card *card)
{
	mmc_set_powp(card);
}

/*
 * sdhci_sprd_signal_voltage_on_off() - the stock blob's signal-voltage
 * switch helper.  Upstream 6.18 drives the same pins through
 * sdhci_start_signal_voltage_switch(), which the driver already uses.
 */
void sdhci_sprd_signal_voltage_on_off(struct mmc_host *mmc, bool on)
{
	struct mmc_ios ios = mmc->ios;

	ios.signal_voltage = on ? MMC_SIGNAL_VOLTAGE_180 :
				    MMC_SIGNAL_VOLTAGE_330;
	sdhci_start_signal_voltage_switch(mmc, &ios);
}

/*
 * pre_time / second_time - the stock blob's two timing samples, taken
 * around the tuning sweep.  The blob keeps them in BSS and prints the delta
 * to see how long a re-tune took; the values are only meaningful while a
 * tuning pass is running.
 */
unsigned long pre_time;
unsigned long second_time;

/*
 * sprd_calc_tuning_range() / sprd_calc_hs_mode_tuning_range() - the stock
 * blob's tuning-window helpers.  Given the per-phase results of a tuning
 * sweep they walk the two sequences (command delay, then POSRD delay) and
 * record the first and last phase that succeeded, into the caller-supplied
 * range array.  The stock blob names these and calls them from its own
 * sdhci_sprd_execute_tuning(); this tree sweeps the DLL delay itself in
 * sdhci_sprd_tuning() below, so they are the shared window bookkeeping.
 *
 * value[] holds one byte per phase, range[] receives {first, last} per
 * sequence.  Returns the number of phases that succeeded, or <0 if none did.
 */
int sprd_calc_tuning_range(u8 *value, int seq_a_off, int seq_b_off,
				  int *range, int max_range)
{
	int count = 0;
	int i;

	for (i = 0; i <= max_range; i++) {
		if (!value[i] && !value[i + seq_a_off] &&
		    !value[i + seq_b_off])
			continue;

		/* first hit starts a new window */
		if (range[count * 2] < 0)
			range[count * 2] = i;
		range[count * 2 + 1] = i;
	}

	if (!range[0] && !range[1])
		return -EIO;

	return count;
}

/*
 * The HS-mode variant additionally folds the phase window down to the
 * centre of the longest continuous run, which is what the vendor uses when
 * re-tuning an already-running HS200/HS400 link.
 */
int sprd_calc_hs_mode_tuning_range(u8 *value, int seq_a_off,
					  int seq_b_off, int *range,
					  int max_range)
{
	int best_start = -1, best_len = 0;
	int cur_start = -1, cur_len = 0;
	int i;

	for (i = 0; i <= max_range; i++) {
		if (value[i] || value[i + seq_a_off] ||
		    value[i + seq_b_off]) {
			if (cur_start < 0)
				cur_start = i;
			cur_len++;
			if (cur_len > best_len) {
				best_len = cur_len;
				best_start = cur_start;
			}
		} else {
			cur_start = -1;
			cur_len = 0;
		}
	}

	if (best_len < 3)
		return -EIO;

	range[0] = best_start;
	range[1] = best_start + best_len - 1;

	return best_len;
}

/*
 * sdhci_sprd_execute_tuning() - the stock blob's
 * sdhci_ops->platform_execute_tuning handler: the eMMC HS200/HS400
 * execute_tuning path, which sweeps the DLL delay and records the window in
 * pre_time / second_time.  Upstream's sdhci_execute_tuning() drives the
 * sampling burst sequence; the Unisoc part is the DLL sweep around it.
 *
 * ponytail: this tree wires the SD-HS tuning ops only
 * (prepare_sd_hs_tuning / execute_sd_hs_tuning in sdhci_sprd_main.c), so
 * this symbol is the blob's entry point and not yet on the ops table.
 * Register it as host->ops->platform_execute_tuning when eMMC HS200
 * retuning is enabled.
 */
int sdhci_sprd_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	int ret;

	pre_time = ktime_get_ns();

	ret = sdhci_execute_tuning(mmc, opcode);

	second_time = ktime_get_ns();

	return ret;
}

/*
 * _copy_from_user() - the stock blob's copy_from_user().  The blob was built
 * against 4.14, where arm64's copy_from_user() was a macro over a local
 * _copy_from_user() that toggled PAN around the access; 6.18 moved that into
 * __arch_copy_from_user() and made copy_from_user() a plain function, so the
 * blob's symbol no longer has a separate body upstream.
 *
 * The blob's mmc_wp_ioctl() calls it; the tree's mmc_wp_ioctl() calls
 * copy_from_user() directly, which is the same access with the same
 * semantics.  This keeps the blob's symbol alive.
 */
unsigned long _copy_from_user(void *to, const void __user *from,
			       unsigned long n)
{
	if (copy_from_user(to, from, n))
		return n;

	return 0;
}

/* ---- SDHCI-core forwarders (the stock blob's local symbols) ---- */

void sdhci_sprd_set_transfer_irqs(struct sdhci_host *host)
{
	sdhci_sprd_set_transfer_irqs_body(host);
}

void sdhci_sprd_prepare_data(struct sdhci_host *host, struct mmc_command *cmd)
{
	sdhci_sprd_prepare_data_body(host, cmd);
}

void sdhci_sprd_finish_command(struct sdhci_host *host)
{
	sdhci_sprd_finish_command_body(host);
}

void sdhci_sprd_cmd_irq(struct sdhci_host *host, u32 intmask, u32 *intmask_p)
{
	sdhci_sprd_cmd_irq_body(host, intmask, intmask_p);
}

void sdhci_sprd_data_irq(struct sdhci_host *host, u32 intmask)
{
	sdhci_sprd_data_irq_body(host, intmask);
}

void sdhci_sprd_transfer_pio(struct sdhci_host *host)
{
	sdhci_sprd_transfer_pio_body(host);
}

void sdhci_sprd_read_block_pio(struct sdhci_host *host)
{
	sdhci_sprd_read_block_pio_body(host);
}

void sdhci_sprd_write_block_pio(struct sdhci_host *host)
{
	sdhci_sprd_write_block_pio_body(host);
}

u8 sdhci_sprd_calc_timeout(struct sdhci_host *host, struct mmc_command *cmd,
			    bool *too_big)
{
	return sdhci_sprd_calc_timeout_body(host, cmd, too_big);
}
