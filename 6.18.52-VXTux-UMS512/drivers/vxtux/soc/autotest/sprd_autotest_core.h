/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Layout handler autotest yang dipulihkan dari .data gpio.ko:
 *   +0x00 nama, +0x08 cmd, +0x10 list_head, +0x20 init, +0x28 test,
 *   +0x30 exit, +0x38 data.
 */
#ifndef _SPRD_AUTOTEST_CORE_H
#define _SPRD_AUTOTEST_CORE_H

#include <linux/list.h>
#include <linux/types.h>

struct sprd_autotest_handler {
	const char *name;						/* +0x00 */
	u32 cmd;							/* +0x08 */
	struct list_head list;						/* +0x10 */
	int (*init)(struct sprd_autotest_handler *h, void *data);	/* +0x20 */
	int (*test)(struct sprd_autotest_handler *h, unsigned long arg);/* +0x28 */
	void (*exit)(struct sprd_autotest_handler *h, void *data);	/* +0x30 */
	void *data;							/* +0x38 */
};

int sprd_autotest_register_handler(struct sprd_autotest_handler *h);
void sprd_autotest_unregister_handler(struct sprd_autotest_handler *h);

#endif /* _SPRD_AUTOTEST_CORE_H */
