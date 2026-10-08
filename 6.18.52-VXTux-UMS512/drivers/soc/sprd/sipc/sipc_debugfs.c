/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
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

#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/soc/sprd/sipc.h>

#if defined(CONFIG_DEBUG_FS)
#include "sipc_debugfs.h"

static int __init sipc_init_debugfs(void)
{
	struct dentry *root = debugfs_create_dir("sipc", NULL);

	if (!root)
		return -ENXIO;

	smsg_init_debugfs(root);
#if defined(CONFIG_SPRD_SIPC_SMSGC)
	smsgc_init_debugfs(root);
#endif
	sbuf_init_debugfs(root);
	sblock_init_debugfs(root);
	/* Diaktifkan lagi 2026-09-30: sipx.c sudah ada di subtree ini
	 * (port dari work/donors_20260930/bocchi_unisoc54/.../sipx.c) dan
	 *.symbol sipx_init_debugfs ada di drivers/soc/sprd/sipc/sipc_debugfs.h:20
	 * di bawah #ifdef yang sama, jadi tidak ada lagi risksymbol hilang.
	 */
#ifdef CONFIG_SPRD_SIPC_ZERO_COPY_SIPX
	sipx_init_debugfs(root);
#endif
#ifdef CONFIG_SPRD_SIPC_SWCNBLK
	swcnblk_init_debugfs(root);
#endif
	smem_init_debugfs(root);
#ifdef CONFIG_SPRD_MAILBOX
	mbox_init_debugfs(root);
#endif
	return 0;
}

device_initcall(sipc_init_debugfs);

#endif /* CONFIG_DEBUG_FS */
