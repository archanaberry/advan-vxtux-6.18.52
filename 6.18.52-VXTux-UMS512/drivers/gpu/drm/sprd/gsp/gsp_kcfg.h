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
#ifndef _GSP_KCFG_H
#define _GSP_KCFG_H


#include <linux/completion.h>
/* In 6.18 <linux/time.h> only forward-declares "struct timespec" and does
 * not define it for kernel code. The start_time member below uses
 * struct timespec64, whose definition comes from <linux/time64.h>. */
/* struct timespec is only forward-declared by <linux/time.h> in 6.18; the
 * definition lives in <uapi/linux/time.h> (include/uapi/linux/time.h:11).
 * time64.h only forward-declares it, so the member below stayed incomplete. */
#include <uapi/linux/time.h>
#include <drm/gsp_cfg.h>
#include "gsp_sync.h"

struct gsp_core;
struct gsp_workqueue;

#define GSP_WAIT_COMPLETION_TIMEOUT msecs_to_jiffies(3000)



struct gsp_kcfg {
	struct gsp_cfg *cfg;
	struct gsp_cfg __user *user_cfg_addr;

	size_t cfg_size;

	/* member 'list' is only for workqueue */
	struct list_head list;
	/* member 'sibling' is for gsp core to debug */
	struct list_head sibling;
	/* member 'link' is for kcfg list */
	struct list_head link;

	struct gsp_fence_data data;

	bool async;
	bool pulled;
	/* indicate this kcfg is the last one at kcfg list */
	bool last;
	bool need_iommu;
	int tag;

	struct gsp_core *bind_core;
	struct gsp_workqueue *wq;

	/* start from trigger, used to timeout judgment.
	 * Never read or written anywhere in the driver (dead field kept for
	 * layout parity with the donor). In 6.18 "struct timespec" is not
	 * available to kernel code; <linux/time64.h> provides timespec64. */
	struct timespec64 start_time;
	struct completion complete;
};

struct gsp_kcfg_list {
	int num;
	bool async;
	bool split;
	/* indicate the cfg size from user process */
	size_t size;
	unsigned int cost;
	struct list_head head;
};

int gsp_kcfg_to_tag(struct gsp_kcfg *kcfg);
int gsp_kcfg_verify(struct gsp_kcfg *kcfg);

/* rerutn value: 0->async 1->not async */
int gsp_kcfg_is_async(struct gsp_kcfg *kcfg);
int gsp_kcfg_is_pulled(struct gsp_kcfg *kcfg);

void gsp_kcfg_set_pulled(struct gsp_kcfg *kcfg);

void gsp_kcfg_init(struct gsp_kcfg *kcfg, struct gsp_core *core,
		   struct gsp_workqueue *wq);

void gsp_kcfg_list_init(struct gsp_kcfg_list *kl, bool aync,
			bool split, size_t sizei, int num);

int gsp_kcfg_list_acquire(struct gsp_dev *gsp,
			  struct gsp_kcfg_list *kl, int num);

int gsp_kcfg_list_fill(struct gsp_kcfg_list *kl, void __user *arg);
int gsp_kcfg_list_push(struct gsp_kcfg_list *kl);
void gsp_kcfg_list_release(struct gsp_kcfg_list *kl);
void gsp_kcfg_list_put(struct gsp_kcfg_list *kl);
int gsp_kcfg_list_wait(struct gsp_kcfg_list *kl);
int gsp_kcfg_list_is_empty(struct gsp_kcfg_list *kl);

int gsp_kcfg_fence_wait(struct gsp_kcfg *kcfg);
void gsp_kcfg_fence_signal(struct gsp_kcfg *kcfg);
void gsp_kcfg_fence_free(struct gsp_kcfg *kcfg);

void gsp_kcfg_iommu_unmap(struct gsp_kcfg *kcfg);
int gsp_kcfg_iommu_map(struct gsp_kcfg *kcfg);

void gsp_kcfg_release(struct gsp_kcfg *kcfg);
void gsp_kcfg_put(struct gsp_kcfg *kcfg);
void gsp_kcfg_put_dmabuf(struct gsp_kcfg *kcfg);

void gsp_kcfg_complete(struct gsp_kcfg *kcfg);
#endif
