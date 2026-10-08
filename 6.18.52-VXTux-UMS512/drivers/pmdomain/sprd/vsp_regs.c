// SPDX-License-Identifier: GPL-2.0-only
/*
 * Definition of the VSP power-domain register table.
 *
 * vsp_common.h declares `extern struct register_gpr regs[]` and both
 * sprd_vsp_pw_domain.c and sprd_vsp.c index it. In the 5.4 vendor tree the
 * definition lives in drivers/misc/sprd_vsp/vsp_common.c:32. That file is not
 * part of this port -- only vsp_common.h was recovered from the donor -- so
 * without this the link fails with "undefined symbol: regs" once
 * pmdomain/sprd actually enters vmlinux.a.
 *
 * The table is populated at probe time by the VSP video driver, which is
 * still a NOT-PORTED blob. Until that lands, every entry stays {NULL, 0, 0}.
 * That is the vendor's own defined fallback: vsp_pw_on()/vsp_pw_off() both
 * test `regs[PMU_VSP_AUTO_SHUTDOWN].gpr == NULL` first and return -1 via the
 * "skip power on/off" path, so no regmap call is ever made on a NULL handle.
 *
 * Copyright (C) 2015--2016 Spreadtrum Communications Inc.
 */

#include "vsp_common.h"

struct register_gpr regs[ARRAY_SIZE(tb_name)];