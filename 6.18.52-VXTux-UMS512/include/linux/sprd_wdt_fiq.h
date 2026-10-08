/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Unisoc watchdog FIQ helper API. Recovered from the marohinmark_ums512 5.4 tree
 * (same UMS512 SoC); the port tree and the T618 fork did not carry it, which is
 * what blocked the sprd_wdf port. The implementation is drivers/watchdog/sprd_wdt_fiq.c.
 */
#ifndef __SPRD_WDT_FIQ_H__
#define __SPRD_WDT_FIQ_H__

#include <linux/watchdog.h>

int sprd_wdt_fiq_get_dev(struct watchdog_device **wdd);

#endif
