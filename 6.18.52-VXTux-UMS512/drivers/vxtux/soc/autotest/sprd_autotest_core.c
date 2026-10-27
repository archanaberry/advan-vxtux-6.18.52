// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_autotest_core - framework autotest Unisoc.
 *
 * Port of the vendor 5.4 module `core.ko` (sprd autotest core driver).
 * Bukti: docs/evidence-20261023/core.objdump.txt (objdump -dr) dan
 * ringkasan docs/RE-SPRD-AUTOTEST.md.
 *
 * Device: char device "autotest0" dalam class "autotest", minor 0..9
 * (alloc_chrdev_region(&devt, 0, 10, "autotest")).
 *
 * Handler (struct di .data gpio.ko: +0x00 nama, +0x08 cmd, +0x10 list,
 * +0x20 init, +0x28 test, +0x30 exit, +0x38 data):
 *
 *   register:  tolak h==NULL atau h->test==NULL (-EINVAL); tolak cmd
 *              duplikat (-EEXIST, "try to add dupicated autotest handler");
 *              panggil h->init(h, h->data) bila ada, lalu list_add().
 *   unregister: panggil h->exit(h, h->data) bila ada, lalu list_del().
 *   ioctl:     printk("autotest cmd = %x"), cari handler dengan cmd sama,
 *              panggil h->test(h, arg); tidak ada -> -EINVAL.
 *
 * Semua akses list dilindungi mutex di autotest->lock (+0x440 di blob);
 * list head ada di +0x430.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "sprd_autotest_core.h"

#define SPRD_AUTOTEST_NAME	"autotest"
#define SPRD_AUTOTEST_MINORS	10

struct sprd_autotest {
	struct device dev;
	struct cdev cdev;
	struct list_head handler_list;		/* +0x430 */
	struct mutex lock;			/* +0x440 */
};

static struct sprd_autotest *autotest;
static struct class *autotest_class;
static dev_t autotest_devt;

int sprd_autotest_register_handler(struct sprd_autotest_handler *h)
{
	struct sprd_autotest_handler *it;
	int ret;

	if (!h || !h->test)
		return -EINVAL;

	mutex_lock(&autotest->lock);
	list_for_each_entry(it, &autotest->handler_list, list) {
		if (it->cmd == h->cmd) {
			mutex_unlock(&autotest->lock);
			pr_err("try to add dupicated autotest handler\n");
			return -EEXIST;
		}
	}
	mutex_unlock(&autotest->lock);

	if (h->init) {
		ret = h->init(h, h->data);
		if (ret)
			return ret;
	}

	INIT_LIST_HEAD(&h->list);

	mutex_lock(&autotest->lock);
	list_add(&h->list, &autotest->handler_list);
	mutex_unlock(&autotest->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(sprd_autotest_register_handler);

void sprd_autotest_unregister_handler(struct sprd_autotest_handler *h)
{
	if (!h)
		return;

	if (h->exit)
		h->exit(h, h->data);

	mutex_lock(&autotest->lock);
	list_del(&h->list);
	mutex_unlock(&autotest->lock);
}
EXPORT_SYMBOL_GPL(sprd_autotest_unregister_handler);

static long sprd_autotest_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct sprd_autotest_handler *h, *found = NULL;

	pr_info("autotest cmd = %x\n", cmd);

	mutex_lock(&autotest->lock);
	list_for_each_entry(h, &autotest->handler_list, list) {
		if (h->cmd == cmd) {
			found = h;
			break;
		}
	}
	mutex_unlock(&autotest->lock);

	if (!found)
		return -EINVAL;

	return found->test(found, arg);
}

static int sprd_autotest_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sprd_autotest_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations sprd_autotest_fops = {
	.owner = THIS_MODULE,
	.open = sprd_autotest_open,
	.release = sprd_autotest_release,
	.unlocked_ioctl = sprd_autotest_ioctl,
	.compat_ioctl = sprd_autotest_ioctl,
};

static int __init sprd_autotest_init(void)
{
	int ret;

	autotest = kzalloc(sizeof(*autotest), GFP_KERNEL);
	if (!autotest)
		return -ENOMEM;

	autotest_class = class_create(SPRD_AUTOTEST_NAME);
	if (IS_ERR(autotest_class)) {
		ret = PTR_ERR(autotest_class);
		pr_err("failed to create autotest class.\n");
		goto err_free;
	}

	ret = alloc_chrdev_region(&autotest_devt, 0, SPRD_AUTOTEST_MINORS,
				  SPRD_AUTOTEST_NAME);
	if (ret) {
		pr_err("failed to allocate chardev region.\n");
		goto err_class;
	}

	mutex_init(&autotest->lock);
	INIT_LIST_HEAD(&autotest->handler_list);

	cdev_init(&autotest->cdev, &sprd_autotest_fops);
	autotest->cdev.owner = THIS_MODULE;

	/* blob: dev->devt = MKDEV(MAJOR(devt), 0), class + nama "autotest0" */
	autotest->dev.devt = MKDEV(MAJOR(autotest_devt), 0);
	autotest->dev.class = autotest_class;
	dev_set_name(&autotest->dev, SPRD_AUTOTEST_NAME "%d", 0);
	device_initialize(&autotest->dev);
	dev_set_drvdata(&autotest->dev, autotest);

	ret = cdev_device_add(&autotest->cdev, &autotest->dev);
	if (ret) {
		pr_err("failed to add autotest device.\n");
		goto err_chrdev;
	}

	pr_info("sprd autotest core driver registered\n");
	return 0;

err_chrdev:
	unregister_chrdev_region(autotest_devt, SPRD_AUTOTEST_MINORS);
err_class:
	class_destroy(autotest_class);
err_free:
	kfree(autotest);
	autotest = NULL;
	return ret;
}

static void __exit sprd_autotest_exit(void)
{
	if (!autotest)
		return;

	cdev_device_del(&autotest->cdev, &autotest->dev);
	unregister_chrdev_region(autotest_devt, SPRD_AUTOTEST_MINORS);
	class_destroy(autotest_class);
	kfree(autotest);
}

module_init(sprd_autotest_init);
module_exit(sprd_autotest_exit);

MODULE_DESCRIPTION("sprd autotest core driver");
MODULE_LICENSE("GPL v2");
