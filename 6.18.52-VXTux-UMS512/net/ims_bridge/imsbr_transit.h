/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IMSBR_TRANSIT_H
#define _IMSBR_TRANSIT_H

/* Port note 2026-10-04: this tree carries the SPRD SIPC header at the
	 * modern path; the vendor fork's <linux/sipc.h> does not exist here.
	 * Same delta as drivers/sound/soc/sprd/audiosipc. */
#include <linux/soc/sprd/sipc.h>

void imsbr_transit_process(struct imsbr_sipc *sipc, struct sblock *blk,
			   bool freeit);
#endif
