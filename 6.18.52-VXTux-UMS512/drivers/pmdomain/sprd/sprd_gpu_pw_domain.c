// SPDX-License-Identifier: GPL-2.0
/*
 * Unisoc T618 / UMS512 (sharkl5pro) GPU power-domain gate.
 *
 * The GPU's power gate on this SoC is a PMU-APB "force shutdown" latch, not a
 * PMIC regulator and not a bit inside the GPU's own 0x60000000 window. The
 * stock device tree says exactly which cell it is:
 *
 *   blobs/fdt_live.dtb, /soc/mm/gpu@60000000
 *       top_force_shutdown = <&pmu_apb_regs 0x30 0x2000000>
 *
 * i.e. syscon@327e0000 (pmu_apb_regs) + 0x30, mask 0x2000000 (bit 25).
 *
 * The vendor GPL kernel source in this workspace names that same cell
 * symbolically, which is where the "power gate" reading comes from:
 *
 *   gpl-source/dts_sprd/arch/arm64/boot/dts/sprd/sharkl5Pro.dtsi:543
 *       syscons = <&pmu_apb_regs REG_PMU_APB_RF_PD_GPU_TOP_CFG0
 *                   MASK_PMU_APB_RF_PD_GPU_TOP_FORCE_SHUTDOWN>, ...
 *       syscon-names = "top_force_shutdown", ...
 *
 * How that was established and who else touches it (evidence, 2026-10-08):
 *   - The removed Mali DDK cleared these bits before enabling the GPU clocks
 *     and set them again after gating the clocks (mali_gondul.ko
 *     mali_platform_init -> mali_clock_on, and mali_platform_term; decompiled,
 *     see docs/RE-GPU-MALI-GONDUL-REMOVAL-20261008.md section 4).
 *   - No other firmware or driver in this workspace does it: the flashed
 *     vendor bootloader images (flash_prep/uboot_[ab].bin) contain "vddgpu"
 *     and "DCDC_GPU_DTM" but none of the top_force_shutdown / dvfs_index_cfg /
 *     core_indexN_map property names (0 hits), this tree's u-boot source has
 *     pmic_buck_enable()/pmic_buck_set_voltage() as empty stubs
 *     (u-boot/ums512/arch/arm/mach-sprd/ums512/spl.c:248), and no driver in
 *     the tree referenced the cell either. So the gate was simply unowned.
 *     Full census in docs/RE-GPU-MALI-GONDUL-REMOVAL-20261008.md section 5.4.
 *
 * This driver gives the gate an owner. It exposes it as a generic PM domain, so
 * the consumer is the GPU device itself and no Mali code, no vendor helper and
 * no extra consumer hook is needed:
 *
 *   drivers/base/platform.c  platform_probe()
 *       dev_pm_domain_attach(_dev, PD_FLAG_ATTACH_POWER_ON |
 *                                 PD_FLAG_DETACH_POWER_OFF)
 *
 * attaches a single "power-domains" entry before ->probe(), so the gate is
 * released before panfrost_probe() runs, and re-asserted when the GPU driver is
 * detached. genpd only calls .power_on() on an OFF->ON transition, so the
 * domain is initialised from the latch's actual state, read once at probe.
 *
 * Provenance of the register values: stock DT only. Nothing here is guessed
 * from the SoC name - the two cells in the DT are the vendor's own numbers.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/regmap.h>

#define SPRD_GPU_GATE_PROP	"top_force_shutdown"
#define SPRD_GPU_GATE_CELLS	2

/* The vendor power-up path waited one __const_udelay() tick between releasing
 * the latch and enabling the clocks. We keep a conservative settle step. */
#define SPRD_GPU_GATE_SETTLE_US	300

struct sprd_gpu_pd {
	struct device *dev;
	struct regmap *gpr;
	u32 reg;
	u32 mask;
	struct generic_pm_domain genpd;
};

static int sprd_gpu_pd_power_on(struct generic_pm_domain *genpd)
{
	struct sprd_gpu_pd *pd = container_of(genpd, struct sprd_gpu_pd, genpd);
	int ret;

	/* Clear the force-shutdown latch: 0 == the GPU may run. */
	ret = regmap_update_bits(pd->gpr, pd->reg, pd->mask, 0);
	if (ret) {
		dev_err(pd->dev, "failed to release GPU gate: %d\n", ret);
		return ret;
	}

	udelay(SPRD_GPU_GATE_SETTLE_US);

	dev_dbg(pd->dev, "GPU gate released (%#x mask %#x)\n", pd->reg, pd->mask);
	return 0;
}

static int sprd_gpu_pd_power_off(struct generic_pm_domain *genpd)
{
	struct sprd_gpu_pd *pd = container_of(genpd, struct sprd_gpu_pd, genpd);
	int ret;

	/* Set the latch: 1 == force the GPU domain into shutdown. */
	ret = regmap_update_bits(pd->gpr, pd->reg, pd->mask, pd->mask);
	if (ret) {
		dev_err(pd->dev, "failed to assert GPU gate: %d\n", ret);
		return ret;
	}

	dev_dbg(pd->dev, "GPU gate asserted (%#x mask %#x)\n", pd->reg, pd->mask);
	return 0;
}

static int sprd_gpu_pd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	unsigned int cells[SPRD_GPU_GATE_CELLS];
	struct sprd_gpu_pd *pd;
	unsigned int cur = 0;
	bool is_off;
	int ret;

	pd = devm_kzalloc(dev, sizeof(*pd), GFP_KERNEL);
	if (!pd)
		return -ENOMEM;
	pd->dev = dev;

	pd->gpr = syscon_regmap_lookup_by_phandle_args(np, SPRD_GPU_GATE_PROP,
						      SPRD_GPU_GATE_CELLS, cells);
	if (IS_ERR(pd->gpr)) {
		dev_err(dev, "no usable %s syscon spec: %pe\n",
			SPRD_GPU_GATE_PROP, pd->gpr);
		return PTR_ERR(pd->gpr);
	}

	pd->reg = cells[0];
	pd->mask = cells[1];
	if (!pd->mask) {
		dev_err(dev, "%s has an empty mask\n", SPRD_GPU_GATE_PROP);
		return -EINVAL;
	}

	/*
	 * Model the domain from the latch's real state instead of assuming one.
	 * On a boot where something else already released the gate, this line is
	 * the measurement of that; on a boot where nothing did, genpd will call
	 * .power_on() when the GPU attaches and the gate gets released here.
	 */
	ret = regmap_read(pd->gpr, pd->reg, &cur);
	if (ret) {
		dev_err(dev, "cannot read GPU gate: %d\n", ret);
		return ret;
	}
	is_off = !!(cur & pd->mask);

	dev_info(dev, "GPU gate %#x mask %#x is %s at probe (boot chain left it %s)\n",
		 pd->reg, pd->mask, is_off ? "ASSERTED" : "released",
		 is_off ? "asserted" : "released");

	pd->genpd.name = "gpu_pd";
	pd->genpd.power_on = sprd_gpu_pd_power_on;
	pd->genpd.power_off = sprd_gpu_pd_power_off;

	ret = pm_genpd_init(&pd->genpd, NULL, is_off);
	if (ret) {
		dev_err(dev, "pm_genpd_init failed: %d\n", ret);
		return ret;
	}

	ret = of_genpd_add_provider_simple(np, &pd->genpd);
	if (ret) {
		dev_err(dev, "of_genpd_add_provider_simple failed: %d\n", ret);
		pm_genpd_remove(&pd->genpd);
		return ret;
	}

	platform_set_drvdata(pdev, pd);
	return 0;
}

static void sprd_gpu_pd_remove(struct platform_device *pdev)
{
	struct sprd_gpu_pd *pd = platform_get_drvdata(pdev);

	of_genpd_del_provider(pdev->dev.of_node);
	pm_genpd_remove(&pd->genpd);
}

static const struct of_device_id sprd_gpu_pd_of_match[] = {
	{ .compatible = "sprd,sharkl5pro-gpu-domain" },
	{ }
};
MODULE_DEVICE_TABLE(of, sprd_gpu_pd_of_match);

static struct platform_driver sprd_gpu_pd_driver = {
	.probe = sprd_gpu_pd_probe,
	.remove = sprd_gpu_pd_remove,
	.driver = {
		.name = "sprd-gpu-pw-domain",
		.of_match_table = sprd_gpu_pd_of_match,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(sprd_gpu_pd_driver);

MODULE_DESCRIPTION("Unisoc T618 (sharkl5pro) GPU power-domain gate");
MODULE_LICENSE("GPL");
