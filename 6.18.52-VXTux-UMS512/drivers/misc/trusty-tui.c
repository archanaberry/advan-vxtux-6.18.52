// SPDX-License-Identifier: GPL-2.0
/*
 * Spreadtrum Trusty TUI (Trusted User Interface) driver.
 *
 * Port of the vendor `trusty-tui.ko` (blobs/vendor_modules/trusty-tui.ko,
 * 26734 bytes, vermagic "5.4.210 SMP preempt mod_unload modversions aarch64").
 * No source for this driver exists anywhere in the donors or in
 * gpl-source/fork_t618 -- only RE notes. The contract below was recovered with
 * radare2 on the .ko itself, then re-implemented for 6.18.
 *
 * Recovered contract (addresses are .ko section-relative vaddrs):
 *   trusty_tui_probe  @0x08001090 : misc_register(&tui_dev); on failure
 *                                   dev_err("can't register miscdev minor=%d (%d)")
 *   trusty_tui_remove @0x08001110 : misc_deregister(&tui_dev) + s_ts teardown
 *   tui_open          @0x08001340 : kmalloc(0x28), init_waitqueue_head(&ts->tui_wq),
 *                                   s_ts = ts, filp->private_data = ts
 *   tui_read          @0x0800116c : len must be 4; copy_to_user(ubuf,&ts->state,4);
 *                                   then ts->state = -1
 *   tui_write         @0x080011ec : len must be 4; copy_from_user(&ts->in_tui,ubuf,4);
 *                                   then ts->notify_cancel = 1; wake_up(&ts->tui_wq)
 *   tui_poll          @0x0800127c : POLLIN when ts->state >= 0 || ts->notify_cancel
 *   is_in_tui         @0x0800106c : return s_ts && s_ts->in_tui > 0
 *   notify_cancel_tui @0x08001030 : s_ts->notify_cancel = 1; wake_up(&ts->tui_wq)
 *
 * Symbol table (blobs/re_notes/trusty-tui.sym) confirms the exported pair
 * is_in_tui / notify_cancel_tui and the misc device name string "tui_dev"
 * (rodata 0x080002a1). tui_dev is {int minor, fops, list, parent, this_device}:
 * minor 0xbb = 187, MISC_DYNAMIC_MINOR is 0xff so 187 is an explicit static minor.
 *
 * struct tui_ts layout recovered from field offsets used by the functions above
 * (all int, naturally aligned, then the wait queue):
 *   +0x00 int state           written by tui_read, read by tui_poll
 *   +0x04 int in_tui          written by tui_write, read by is_in_tui
 *   +0x08 int notify_cancel   set by tui_write / notify_cancel_tui, read by tui_poll
 *   +0x10 wait_queue_head_t   wait_event target (+0x10 relative to ts)
 *
 * 6.18 delta: none of the APIs used here changed. wait_event_interruptible,
 * misc_register, copy_{to,from}_user and of_match are the same in 6.18 as in
 * 5.4, which is why this driver ports without an API translation table.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/errno.h>
#include <linux/trusty/trusty_tui.h>

#define TUI_MSG_LEN		4
#define TUI_DEV_MINOR		187

struct tui_ts {
	int state;		/* +0x00: read clears to -1 */
	int in_tui;		/* +0x04: last value written by userspace */
	int notify_cancel;	/* +0x08: poll wake condition */
	wait_queue_head_t tui_wq;	/* +0x10 */
};

static struct tui_ts *s_ts;

/**
 * is_in_tui() - report whether userspace has entered the trusted UI.
 *
 * Exported on the vendor side (ksymtab entry present) so other drivers can
 * gate behaviour on a TUI session being active. Returns false when no client
 * has the device open.
 */
bool is_in_tui(void)
{
	return s_ts && s_ts->in_tui > 0;
}
EXPORT_SYMBOL_GPL(is_in_tui);

/**
 * notify_cancel_tui() - cancel an active TUI session from kernel context.
 *
 * Sets the cancel flag and wakes the poll waiter, which is how a TUI session
 * is torn down when, for example, the display pipeline is reconfiguring.
 */
void notify_cancel_tui(void)
{
	if (!s_ts)
		return;

	s_ts->notify_cancel = 1;
	wake_up_interruptible(&s_ts->tui_wq);
}
EXPORT_SYMBOL_GPL(notify_cancel_tui);

static int tui_open(struct inode *inode, struct file *filp)
{
	struct tui_ts *ts;

	ts = kmalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	init_waitqueue_head(&ts->tui_wq);
	s_ts = ts;
	filp->private_data = ts;

	return 0;
}

static int tui_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	if (filp->private_data == s_ts)
		s_ts = NULL;
	filp->private_data = NULL;

	return 0;
}

static ssize_t tui_read(struct file *filp, char __user *ubuf,
			size_t len, loff_t *ppos)
{
	struct tui_ts *ts = filp->private_data;

	if (len != TUI_MSG_LEN) {
		printk("tui_read: input buf err, expect(%d) actual(%zu)\n",
		       TUI_MSG_LEN, len);
		return -EINVAL;
	}

	if (copy_to_user(ubuf, &ts->state, TUI_MSG_LEN))
		return -EFAULT;

	/* one-shot: the state word is consumed by the read */
	ts->state = -1;
	printk("tui_read: read notify done.\n");

	return TUI_MSG_LEN;
}

static ssize_t tui_write(struct file *filp, const char __user *ubuf,
			 size_t len, loff_t *ppos)
{
	struct tui_ts *ts = filp->private_data;

	if (len != TUI_MSG_LEN) {
		printk("tui_write: output buf err, expect(%d) actual(%zu)\n",
		       TUI_MSG_LEN, len);
		return -EINVAL;
	}

	if (copy_from_user(&ts->in_tui, ubuf, TUI_MSG_LEN))
		return -EFAULT;

	ts->notify_cancel = 1;
	wake_up_interruptible(&ts->tui_wq);

	return TUI_MSG_LEN;
}

static __poll_t tui_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct tui_ts *ts = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &ts->tui_wq, wait);

	if (ts->notify_cancel > 0) {
		ts->notify_cancel = 0;
		printk("tui wake up pollerr\n");
		mask |= EPOLLERR;
	}
	if (ts->state >= 0) {
		printk("tui wake up pollin\n");
		mask |= EPOLLIN;
	}

	return mask;
}

static const struct file_operations tui_fops = {
	.owner		= THIS_MODULE,
	.open		= tui_open,
	.release	= tui_release,
	.read		= tui_read,
	.write		= tui_write,
	.poll		= tui_poll,
};

static struct miscdevice tui_dev = {
	.minor		= TUI_DEV_MINOR,
	.name		= "tui_dev",
	.fops		= &tui_fops,
};

static int trusty_tui_probe(struct platform_device *pdev)
{
	int ret;

	ret = misc_register(&tui_dev);
	if (ret) {
		dev_err(&pdev->dev,
			"can't register miscdev minor=%d (%d)\n", tui_dev.minor, ret);
		return ret;
	}

	dev_info(&pdev->dev, "trusty tui probe success\n");

	return 0;
}

static void trusty_tui_remove(struct platform_device *pdev)
{
	misc_deregister(&tui_dev);
}

static const struct of_device_id trusty_tui_of_match[] = {
	{ .compatible = "sprd,trusty-tui-v1" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, trusty_tui_of_match);

static struct platform_driver trusty_tui_driver = {
	.probe		= trusty_tui_probe,
	.remove		= trusty_tui_remove,
	.driver		= {
		.name	= "trusty_tui",
		.of_match_table = trusty_tui_of_match,
	},
};

module_platform_driver(trusty_tui_driver);

MODULE_DESCRIPTION("Sprd trusty tui driver");
MODULE_LICENSE("GPL v2");
