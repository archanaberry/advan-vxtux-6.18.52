/*
 * Copyright (C) 2019 Spreadtrum Communications Inc.
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

/*
 * PORT 6.18 (2026-09-30) - sumber: work/donors_20260930/bocchi_unisoc54/
 * drivers/soc/sprd/modem/sipc/spipe.h
 *
 * Delta dari donor:
 *  - u16 smem -> u32 smem. Alasannya KONTRAK, bukan selera: bentuk non-V2
 *    sbuf_create_ex() di include/linux/soc/sprd/sipc.h:571 yang dipakai
 *    pohon port menerima `u32 smem_idx`. Donor menyimpan smem ke u16 lalu
 *    promotion ke u32 saat panggilan (truncate 16 bit, tidak disengaja).
 *    Header T618 (gpl-source/fork_t618/.../spipe.h) memang u32.
 */
#ifndef __SPIPE_H
#define __SPIPE_H

struct spipe_init_data {
	const char *name;
	char	*sipc_name;
	u8	dst;
	u8	channel;
	u32	smem;
	u32	ringnr;
	u32	txbuf_size;
	u32	rxbuf_size;
};
#endif
