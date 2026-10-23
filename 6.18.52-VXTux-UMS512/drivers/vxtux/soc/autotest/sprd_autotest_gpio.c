// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_autotest_gpio - handler autotest untuk GPIO.
 *
 * Port of the vendor 5.4 module `gpio.ko` ("sprd autotest gpio driver",
 * depends=core).  Bukti: docs/evidence-20261023/gpio.objdump.txt.
 *
 * Handler di .data blob: +0x00 "gpio", +0x08 cmd = 0xc0044101
 * (_IOWR('A', 1, 4)), +0x28 gpio_test.  init_module hanya memanggil
 * sprd_autotest_register_handler(&handler).
 *
 * Argumen ioctl (u32 di user): bit 0..15 offset, bit 16 dir, bit 17 value.
 *   - chip dicari lewat gpiochip_find(NULL, match) dengan match
 *     `chip->ngpio == 256` (ldrh [chip,#140] == 0x100);
 *   - gpio = chip->base + offset;
 *   - dir  : bila line itu IRQ -> chip->set(chip, offset, value),
 *            selain itu gpiod_direction_output(desc, value);
 *   - input: gpiod_direction_input(desc) lalu gpiod_get_value(desc),
 *            hasilnya ditulis balik ke bit 17 argumen user;
 *   - gpio_request() yang gagal dengan -EBUSY dianggap sukses (line sudah
 *     dipegang pihak lain).
 */
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "sprd_autotest_core.h"

#define SPRD_AUTOTEST_GPIO_CMD	0xc0044101

#define SPRD_GPIO_OFFSET_MASK	GENMASK(15, 0)
#define SPRD_GPIO_DIR		BIT(16)
#define SPRD_GPIO_VALUE		BIT(17)

static int gpio_match_ngpio(struct gpio_chip *chip, const void *data)
{
	return chip->ngpio == 256;
}

static int gpio_test(struct sprd_autotest_handler *h, unsigned long arg)
{
	struct gpio_device *gdev;
	struct gpio_chip *chip;
	struct gpio_desc *desc;
	u32 req;
	int offset, dir, value, ret;
	bool own = true;

	if (get_user(req, (u32 __user *)arg))
		return -EFAULT;

	offset = req & SPRD_GPIO_OFFSET_MASK;
	dir = !!(req & SPRD_GPIO_DIR);
	value = !!(req & SPRD_GPIO_VALUE);

	pr_info("offset=%d, dir=%d, value=%d\n", offset, dir, value);

	/* 6.18: gpiochip_find()/gpio_request() legacy sudah dihapus. */
	gdev = gpio_device_find(NULL, gpio_match_ngpio);
	if (!gdev) {
		pr_err("get gpio chip failed.\n");
		return -ENODEV;
	}

	chip = gpio_device_get_chip(gdev);

	/* 6.18 tidak lagi punya gpio_request()/gpiod_request(); yang setara
	 * adalah kepemilikan desc lewat gpiochip_request_own_desc().
	 */
	desc = gpiochip_request_own_desc(chip, offset, "gpio",
					 GPIO_LOOKUP_FLAGS_DEFAULT, GPIOD_ASIS);
	if (IS_ERR(desc)) {
		ret = PTR_ERR(desc);
		if (ret != -EBUSY) {
			pr_err("gpio request failed.\n");
			gpio_device_put(gdev);
			return ret;
		}
		/* -EBUSY: blob tetap lanjut memakai line yang sudah dipegang
		 * pihak lain; di sini cukup ambil referensi desc-nya.
		 */
		own = false;
		desc = gpio_device_get_desc(gdev, offset);
		if (!desc) {
			gpio_device_put(gdev);
			return -EINVAL;
		}
	} else {
		own = true;
	}

	if (dir) {
		if (gpiochip_line_is_irq(chip, offset))
			chip->set(chip, offset, value);
		else
			ret = gpiod_direction_output(desc, value);
	} else {
		ret = gpiod_direction_input(desc);
		if (!ret) {
			value = gpiod_get_value(desc);
			req = (req & ~SPRD_GPIO_VALUE) |
			      ((value != 0) ? SPRD_GPIO_VALUE : 0);
			if (put_user(req, (u32 __user *)arg))
				ret = -EFAULT;
		}
	}

	if (ret)
		pr_err("write to user failed.\n");

	if (own)
		gpiochip_free_own_desc(desc);
	else
		gpiod_put(desc);
	gpio_device_put(gdev);
	return ret;
}

static struct sprd_autotest_handler gpio_handler = {
	.name = "gpio",
	.cmd = SPRD_AUTOTEST_GPIO_CMD,
	.test = gpio_test,
};

static int __init sprd_autotest_gpio_init(void)
{
	return sprd_autotest_register_handler(&gpio_handler);
}

static void __exit sprd_autotest_gpio_exit(void)
{
	sprd_autotest_unregister_handler(&gpio_handler);
}

module_init(sprd_autotest_gpio_init);
module_exit(sprd_autotest_gpio_exit);

MODULE_DESCRIPTION("sprd autotest gpio driver");
MODULE_LICENSE("GPL v2");
