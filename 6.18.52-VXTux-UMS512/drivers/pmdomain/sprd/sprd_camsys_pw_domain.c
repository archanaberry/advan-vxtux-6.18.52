// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc camera/MMSYS power-domain driver, ported to 6.18 from the 5.4 UMS512
 * sources (fork_t618 drivers/soc/sprd/domain/camsys_pw_domain_r8p0.c, the
 * sharkl5 family variant that matches "sprd,mm-domain"). Replaces the
 * sprd_camsys_pw_domain vendor blob.
 *
 * 5.4 -> 6.18 changes actually made here (each one verified against this tree):
 *   - syscon_regmap_lookup_by_name() / syscon_get_args_by_name() do not exist in
 *     6.18 (include/linux/mfd/syscon.h). Replaced by vxtux_camsys_syscon_by_name()
 *     below, which resolves the "syscons"/"syscon-names" phandle-spec list the
 *     same way drivers/sound/soc/sprd/agdsp_access/vxtux-syscon-shim.h does.
 *   - of_clk_get_by_name(np, ...) is gone from the driver API; every clock is now
 *     devm_clk_get(&pdev->dev, id), which forwards to the same of_node lookup and
 *     additionally releases the reference on remove.
 *   - devm_kfree() was removed. devm_kzalloc() has no matching free, so the probe
 *     state is allocated with kzalloc() and released in .remove.
 *   - .remove returns void (int was deleted in 6.9).
 *   - <asm/cacheflush.h> dropped: nothing in this file touches the cache.
 *   - __builtin_return_address(0) is kept because it is what the vendor's %pS
 *     debug lines print; it is a plain builtin and needs no header.
 *
 * Exported-symbol note: the vendor .ko (blobs/re_notes/sprd_camsys_pw_domain.sym)
 * exports sprd_glb_mm_pw_{on,off}_cfg + sprd_mm_pw_notify_{register,unregister},
 * whereas the 5.4 fork source exports sprd_cam_pw_{on,off} and
 * sprd_cam_domain_{eb,disable}. Both ABIs are provided here: the fork names are
 * the real implementations, the glb_mm_* names are thin wrappers, so a consumer
 * written against either the fork or the shipped blob links.
 *
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
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

#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/string.h>
#include <linux/types.h>
#include <uapi/video/sprd_mmsys_pw_domain.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "cam_sys_pw: %d %d %s : " \
	fmt, current->pid, __LINE__, __func__

/* Indexed by the enum below; the DT "syscon-names" strings must match. */
static const char * const syscon_name[] = {
	"force_shutdown",
	"shutdown_en",	/* clear */
	"power_state",	/* on: 0; off:7 */
};

#define PD_MM_DOWN_FLAG		0x7
#define ARQOS_THRESHOLD		0x0D
#define AWQOS_THRESHOLD		0x0D
#define SHIFT_MASK(a)		(ffs(a) ? ffs(a) - 1 : 0)

enum {
	FORCE_SHUTDOWN = 0,
	SHUTDOWN_EN,	/* auto shutdown_en */
	PWR_DGB_6,	/* status dbg 6 */
};

struct register_gpr {
	struct regmap *gpr;
	u32 reg;
	u32 mask;
};

struct camsys_power_info {
	atomic_t users_pw;
	atomic_t users_clk;
	atomic_t inited;
	struct mutex mlock;

	struct clk *cam_mm_eb;
	struct clk *cam_mm_ahb_eb;

	struct clk *cam_ahb_clk;
	struct clk *cam_ahb_clk_parent;
	struct clk *cam_ahb_clk_default;

	struct clk *cam_mtx_clk;
	struct clk *cam_mtx_clk_parent;
	struct clk *cam_mtx_clk_default;

	struct clk *isppll_clk;

	struct register_gpr regs[ARRAY_SIZE(syscon_name)];
};

static struct camsys_power_info *pw_info;
static BLOCKING_NOTIFIER_HEAD(mmsys_chain);

/* provider declarations (avoid -Wmissing-prototypes) */
int sprd_mm_pw_notify_register(struct notifier_block *nb);
int sprd_mm_pw_notify_unregister(struct notifier_block *nb);
int sprd_cam_pw_on(void);
int sprd_cam_pw_off(void);
int sprd_cam_domain_eb(void);
int sprd_cam_domain_disable(void);
int sprd_glb_mm_pw_on_cfg(int client);
int sprd_glb_mm_pw_off_cfg(int client);

/*
 * 6.18 replacement for the 5.4 pair syscon_regmap_lookup_by_name() +
 * syscon_get_args_by_name(). DT layout is the vendor one:
 *   syscons      = <&phandle REG MASK>, ...
 *   syscon-names = "name0", "name1", ...
 * Returns the regmap for the entry whose name matches, or ERR_PTR().
 */
static struct regmap *vxtux_camsys_syscon_by_name(const struct device_node *np,
						  const char *name,
						  u32 *out_args, int nr_args)
{
	struct of_phandle_args a;
	int count, i, idx = -ENODEV;
	const char *s;

	if (!np || !name)
		return ERR_PTR(-EINVAL);

	count = of_property_count_strings(np, "syscon-names");
	for (i = 0; i < count; i++) {
		if (!of_property_read_string_index(np, "syscon-names", i,
						   &s) && !strcmp(s, name)) {
			idx = i;
			break;
		}
	}
	if (idx < 0)
		return ERR_PTR(idx);

	if (of_parse_phandle_with_args(np, "syscons", NULL, idx, &a))
		return ERR_PTR(-ENODEV);

	if (out_args) {
		int n = min(nr_args, (int)a.args_count);

		for (i = 0; i < n; i++)
			out_args[i] = a.args[i];
	}

	return syscon_node_to_regmap(a.np);
}

/* register */

int sprd_mm_pw_notify_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&mmsys_chain, nb);
}
EXPORT_SYMBOL_GPL(sprd_mm_pw_notify_register);

/* unregister */

int sprd_mm_pw_notify_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&mmsys_chain, nb);
}
EXPORT_SYMBOL_GPL(sprd_mm_pw_notify_unregister);

static int mmsys_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&mmsys_chain, val, v);
}

static void regmap_update_bits_mmsys(struct register_gpr *p, u32 val)
{
	if ((!p) || (!(p->gpr)))
		return;

	regmap_update_bits(p->gpr, p->reg, p->mask, val);
}

static int regmap_read_mmsys(struct register_gpr *p, u32 *val)
{
	int ret = 0;

	if ((!p) || (!(p->gpr)) || (!val))
		return -1;
	ret = regmap_read(p->gpr, p->reg, val);
	if (!ret)
		*val &= (u32)p->mask;

	return ret;
}

static int check_drv_init(void)
{
	int ret = 0;

	if (!pw_info)
		return -1;	/* need return, or pw_info->inited should error */
	if (atomic_read(&pw_info->inited) == 0)
		ret = -2;

	return ret;
}

static int sprd_campw_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	int i, ret = 0;
	const char *pname;
	struct regmap *tregmap;
	u32 args[2];

	pr_info("cam power init begin\n");

	pw_info = kzalloc(sizeof(*pw_info), GFP_KERNEL);
	if (!pw_info)
		return -ENOMEM;

	pw_info->cam_mm_eb = devm_clk_get(dev, "clk_mm_eb");
	if (IS_ERR_OR_NULL(pw_info->cam_mm_eb)) {
		ret = PTR_ERR(pw_info->cam_mm_eb);
		goto err_free;
	}

	pw_info->cam_mm_ahb_eb = devm_clk_get(dev, "clk_mm_ahb_eb");
	if (IS_ERR_OR_NULL(pw_info->cam_mm_ahb_eb)) {
		ret = PTR_ERR(pw_info->cam_mm_ahb_eb);
		goto err_free;
	}

	pw_info->cam_ahb_clk = devm_clk_get(dev, "clk_mm_ahb");
	if (IS_ERR_OR_NULL(pw_info->cam_ahb_clk)) {
		ret = PTR_ERR(pw_info->cam_ahb_clk);
		goto err_free;
	}

	pw_info->cam_ahb_clk_parent = devm_clk_get(dev, "clk_mm_ahb_parent");
	if (IS_ERR_OR_NULL(pw_info->cam_ahb_clk_parent)) {
		ret = PTR_ERR(pw_info->cam_ahb_clk_parent);
		goto err_free;
	}

	/* clk_get_parent() still exists in 6.18 (include/linux/clk.h:917); the
	 * "default" parent is the clock's own current parent, not a named DT
	 * clock, so it must not be fetched by name. */
	pw_info->cam_ahb_clk_default = clk_get_parent(pw_info->cam_ahb_clk);
	if (IS_ERR_OR_NULL(pw_info->cam_ahb_clk_default)) {
		ret = PTR_ERR(pw_info->cam_ahb_clk_default);
		goto err_free;
	}

	/* need set cgm_mm_emc_sel :512m , DDR matrix clk */
	pw_info->cam_mtx_clk = devm_clk_get(dev, "clk_mm_mtx");
	if (IS_ERR_OR_NULL(pw_info->cam_mtx_clk)) {
		ret = PTR_ERR(pw_info->cam_mtx_clk);
		goto err_free;
	}

	pw_info->cam_mtx_clk_parent = devm_clk_get(dev, "clk_mm_mtx_parent");
	if (IS_ERR_OR_NULL(pw_info->cam_mtx_clk_parent)) {
		ret = PTR_ERR(pw_info->cam_mtx_clk_parent);
		goto err_free;
	}

	pw_info->cam_mtx_clk_default = clk_get_parent(pw_info->cam_mtx_clk);
	if (IS_ERR_OR_NULL(pw_info->cam_mtx_clk_default)) {
		ret = PTR_ERR(pw_info->cam_mtx_clk_default);
		goto err_free;
	}

	pw_info->isppll_clk = devm_clk_get(dev, "clk_isppll");
	if (IS_ERR_OR_NULL(pw_info->isppll_clk)) {
		ret = PTR_ERR(pw_info->isppll_clk);
		goto err_free;
	}

	/* read global register */
	for (i = 0; i < ARRAY_SIZE(syscon_name); i++) {
		pname = syscon_name[i];
		args[0] = 0;
		args[1] = 0;
		tregmap = vxtux_camsys_syscon_by_name(np, pname, args, 2);
		if (IS_ERR_OR_NULL(tregmap)) {
			pr_err("fail to read %s regmap\n", pname);
			continue;
		}
		pw_info->regs[i].gpr = tregmap;
		pw_info->regs[i].reg = args[0];
		pw_info->regs[i].mask = args[1];
		pr_info("dts[%s] 0x%x 0x%x\n", pname,
			pw_info->regs[i].reg,
			pw_info->regs[i].mask);
	}

	mutex_init(&pw_info->mlock);
	atomic_set(&pw_info->inited, 1);

	pr_info("cam power init end\n");

	return 0;

err_free:
	kfree(pw_info);
	pw_info = NULL;
	return ret;
}

int sprd_cam_pw_off(void)
{
	int ret = 0;
	unsigned int power_state1 = 0;
	unsigned int power_state2 = 0;
	unsigned int power_state3 = 0;
	unsigned int read_count = 0;
	int shift = 0;

	ret = check_drv_init();
	if (ret) {
		pr_err("uses: %d, cb: %p, ret %d\n",
			atomic_read(&pw_info->users_pw),
			__builtin_return_address(0), ret);
		return -ENODEV;
	}

	mutex_lock(&pw_info->mlock);
	if (atomic_dec_return(&pw_info->users_pw) == 0) {
		/* 1:auto shutdown en, shutdown with ap; 0: control by b25 */
		regmap_update_bits_mmsys(&pw_info->regs[SHUTDOWN_EN], 0);
		/* set 1 to shutdown */
		regmap_update_bits_mmsys(&pw_info->regs[FORCE_SHUTDOWN],
					~((u32)0));
		/* shift for power off status bits */
		if (pw_info->regs[PWR_DGB_6].gpr != NULL)
			shift = SHIFT_MASK(pw_info->regs[PWR_DGB_6].mask);
		do {
			cpu_relax();
			usleep_range(300, 350);
			read_count++;

			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state1);
			if (ret)
				goto err_pw_off;
			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state2);
			if (ret)
				goto err_pw_off;
			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state3);
			if (ret)
				goto err_pw_off;
		} while (((power_state1 != (PD_MM_DOWN_FLAG << shift)) &&
				(read_count < 30)) ||
				(power_state1 != power_state2) ||
				(power_state2 != power_state3));
		if (power_state1 != (PD_MM_DOWN_FLAG << shift)) {
			pr_err("failed, power_state1=0x%x\n", power_state1);
			ret = -1;
			goto err_pw_off;
		}
	}
	mutex_unlock(&pw_info->mlock);
	/* if count != 0, other using */
	pr_info("Done, read count %d, cb: %p\n",
		read_count, __builtin_return_address(0));

	return 0;

err_pw_off:
	mutex_unlock(&pw_info->mlock);
	pr_err("failed, ret: %d, count: %d, cb: %p\n", ret, read_count,
		__builtin_return_address(0));

	return ret;
}
EXPORT_SYMBOL_GPL(sprd_cam_pw_off);

int sprd_cam_pw_on(void)
{
	int ret = 0;
	unsigned int power_state1 = 0;
	unsigned int power_state2 = 0;
	unsigned int power_state3 = 0;
	unsigned int read_count = 0;

	pr_info("sprd cam pw on\n");

	ret = check_drv_init();
	if (ret) {
		pr_info("uses: %d, cb: %p, ret %d\n",
			atomic_read(&pw_info->users_pw),
			__builtin_return_address(0), ret);
		return -ENODEV;
	}

	mutex_lock(&pw_info->mlock);
	if (atomic_inc_return(&pw_info->users_pw) == 1) {
		/* clear force shutdown */
		regmap_update_bits_mmsys(&pw_info->regs[FORCE_SHUTDOWN], 0);
		/* power on */
		regmap_update_bits_mmsys(&pw_info->regs[SHUTDOWN_EN], 0);

		do {
			cpu_relax();
			usleep_range(300, 350);
			read_count++;

			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state1);
			if (ret)
				goto err_pw_on;
			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state2);
			if (ret)
				goto err_pw_on;
			ret = regmap_read_mmsys(&pw_info->regs[PWR_DGB_6],
						&power_state3);
			if (ret)
				goto err_pw_on;
		} while ((power_state1 && read_count < 30) ||
				(power_state1 != power_state2) ||
				(power_state2 != power_state3));

		if (power_state1) {
			pr_err("cam domain pw on failed 0x%x\n", power_state1);
			ret = -1;
			goto err_pw_on;
		}
	}
	mutex_unlock(&pw_info->mlock);
	/* if count != 0, other using */
	pr_info("Done, uses: %d, read count %d, cb: %p\n",
		atomic_read(&pw_info->users_pw), read_count,
		__builtin_return_address(0));
	pr_info("sprd cam pw on end\n");
	return 0;
err_pw_on:
	atomic_dec_return(&pw_info->users_pw);
	mutex_unlock(&pw_info->mlock);
	pr_info("cam domain, failed to power on, ret = %d\n", ret);
	return ret;
}
EXPORT_SYMBOL_GPL(sprd_cam_pw_on);

int sprd_cam_domain_eb(void)
{
	int ret = 0;

	ret = check_drv_init();
	if (ret) {
		pr_err("fail to get init state %d, cb %p, ret %d\n",
			atomic_read(&pw_info->users_pw),
			__builtin_return_address(0), ret);
		return -ENODEV;
	}

	pr_info("users count %d, cb %p\n",
		atomic_read(&pw_info->users_clk),
		__builtin_return_address(0));

	mutex_lock(&pw_info->mlock);
	if (atomic_inc_return(&pw_info->users_clk) == 1) {
		/* mm bus enable */
		clk_prepare_enable(pw_info->cam_mm_eb);
		clk_prepare_enable(pw_info->cam_mm_ahb_eb);
		/* config cam ahb clk */
		clk_set_parent(pw_info->cam_ahb_clk,
			       pw_info->cam_ahb_clk_parent);
		clk_prepare_enable(pw_info->cam_ahb_clk);

		/* config cam mtx clk */
		clk_set_parent(pw_info->cam_mtx_clk,
			       pw_info->cam_mtx_clk_parent);
		clk_prepare_enable(pw_info->cam_mtx_clk);

		clk_prepare_enable(pw_info->isppll_clk);

		mmsys_notifier_call_chain(_E_PW_ON, NULL);
	}
	mutex_unlock(&pw_info->mlock);
	return 0;
}
EXPORT_SYMBOL_GPL(sprd_cam_domain_eb);

int sprd_cam_domain_disable(void)
{
	int ret = 0;

	ret = check_drv_init();
	if (ret) {
		pr_err("fail to get init state %d, cb %p, ret %d\n",
			atomic_read(&pw_info->users_pw),
			__builtin_return_address(0), ret);
	}

	pr_info("users count %d, cb %p\n",
		atomic_read(&pw_info->users_clk),
		__builtin_return_address(0));

	mutex_lock(&pw_info->mlock);
	if (atomic_dec_return(&pw_info->users_clk) == 0) {
		mmsys_notifier_call_chain(_E_PW_OFF, NULL);

		clk_disable_unprepare(pw_info->isppll_clk);

		clk_set_parent(pw_info->cam_ahb_clk,
			       pw_info->cam_ahb_clk_default);
		clk_disable_unprepare(pw_info->cam_ahb_clk);

		clk_set_parent(pw_info->cam_mtx_clk,
			       pw_info->cam_mtx_clk_default);
		clk_disable_unprepare(pw_info->cam_mtx_clk);

		clk_disable_unprepare(pw_info->cam_mm_ahb_eb);
		clk_disable_unprepare(pw_info->cam_mm_eb);
	}
	mutex_unlock(&pw_info->mlock);
	return 0;
}
EXPORT_SYMBOL_GPL(sprd_cam_domain_disable);

/*
 * Blob ABI wrappers. The shipped sprd_camsys_pw_domain.ko exports only these
 * four names; consumers (sprd_camera, mmdvfs) were compiled against them.
 */
int sprd_glb_mm_pw_on_cfg(int client)
{
	return sprd_cam_pw_on();
}
EXPORT_SYMBOL_GPL(sprd_glb_mm_pw_on_cfg);

int sprd_glb_mm_pw_off_cfg(int client)
{
	return sprd_cam_pw_off();
}
EXPORT_SYMBOL_GPL(sprd_glb_mm_pw_off_cfg);

static int sprd_campw_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("cam power probe begin\n");
	ret = sprd_campw_init(pdev);
	if (ret) {
		pr_err("fail to init cam power domain\n");
		return -ENODEV;
	}

	return ret;
}

static void sprd_campw_remove(struct platform_device *pdev)
{
	atomic_set(&pw_info->inited, 0);
	kfree(pw_info);
	pw_info = NULL;
}

static const struct of_device_id sprd_campw_match_table[] = {
	{ .compatible = "sprd,mm-domain", },
	{ .compatible = "sprd,sharkl3-camsys-domain", },
	{ .compatible = "sprd,sharkl5-camsys-domain", },
	{ .compatible = "sprd,sharkl5pro-camsys-domain", },
	{ .compatible = "sprd,sharkle-camsys-domain", },
	{ .compatible = "sprd,qogirl6-camsys-domain", },
	{ .compatible = "sprd,qogirl6l-camsys-domain", },
	{ .compatible = "sprd,qogirn6l-camsys-domain", },
	{ .compatible = "sprd,qogirn6pro-camsys-domain", },
	{ .compatible = "sprd,pike2-camsys-domain", },
	{}
};

static struct platform_driver sprd_campw_driver = {
	.probe = sprd_campw_probe,
	.remove = sprd_campw_remove,
	.driver = {
		.name = "camsys-power",
		.of_match_table = of_match_ptr(sprd_campw_match_table),
	},
};

module_platform_driver(sprd_campw_driver);

MODULE_DESCRIPTION("Camsys Power Driver");
MODULE_AUTHOR("Multimedia_Camera@unisoc.com");
MODULE_LICENSE("GPL");