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

#ifndef __SPRD_AGDSP_ACCESS_H__
#define __SPRD_AGDSP_ACCESS_H__
#include <linux/types.h>

int agdsp_access_enable(void);
int agdsp_access_disable(void);
int agdsp_can_access(void);
int force_on_xtl(bool on_off);
int disable_access_force(void);
int restore_access(void);

/*
 * Dua ekspor blob agdsp_access.ko yang dipulihkan lewat RE (B8/R1) — lihat
 * bagian penjelas di agdsp_access.c dan dumped/re/R1_audio_4sym.md:
 *   agdsp_set_mboxchan   blob 0x04a0 (terima chan audio dari probe audio_sipc)
 *   agdsp_access_dumpreg blob 0x0a98 (cetak state AP + 5 register audcp)
 * Pemanggil di blob: audio_sipc.ko (probe / irq handler / dump func).
 */
struct mbox_chan;
void agdsp_set_mboxchan(struct mbox_chan *chan);
void agdsp_access_dumpreg(void);

#endif
