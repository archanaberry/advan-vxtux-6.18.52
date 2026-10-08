/* SPDX-License-Identifier: GPL-2.0-only */
//
// Spreadtrum reset clock driver
//
// Copyright (C) 2022 Spreadtrum, Inc.
// Author: Zhifeng Tang <zhifeng.tang@unisoc.com>
//
// Provenance: imported from codeberg.org/ums9230-mainline/linux, branch
// "ums9230", commit 4a5b97b821b846f9a25a99a4fdcb3040fd910965
// (drivers/clk/sprd/reset.h). No source change was needed for the 7.1 -> 6.18
// move: struct reset_control_ops and struct reset_controller_dev keep the same
// type names, and every field reset.c assigns (rcdev.of_node / rcdev.ops /
// rcdev.nr_resets) is unchanged. The 7.1 header only ADDS fwnode_* fields and
// a mutex, and DROPS reset_control_lookup / RESET_LOOKUP /
// reset_controller_add_lookup(); none of those are referenced here.

#ifndef _SPRD_RESET_H_
#define _SPRD_RESET_H_

#include <linux/reset-controller.h>
#include <linux/spinlock.h>

struct sprd_reset_map {
	u32	reg;
	u32	mask;
	u32	sc_offset;
};

struct sprd_reset {
	struct reset_controller_dev	rcdev;
	const struct sprd_reset_map	*reset_map;
	struct regmap			*regmap;
	spinlock_t			lock;
};

extern const struct reset_control_ops sprd_reset_ops;
extern const struct reset_control_ops sprd_sc_reset_ops;

#endif /* _SPRD_RESET_H_ */
