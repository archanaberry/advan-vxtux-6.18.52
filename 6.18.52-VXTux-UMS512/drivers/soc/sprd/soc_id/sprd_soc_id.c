// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_soc_id.c — Unisoc SoC identity reader (chip/plat/impl/mft/ver),
 * port 5.4 -> 6.18.
 *
 * Sumber GPL: fork Samsung Tab A8 (T618/UMS512)
 *   drivers/misc/sprd_soc_id.c (C) 2021 Spreadtrum, Luting Guo.
 *
 * Adaptasi 6.18 (terverifikasi):
 *   - syscon_regmap_lookup_by_name() / syscon_get_args_by_name() TIDAK ADA
 *     di mainline -> of_parse_phandle_with_fixed_args() + syscon_node_to_regmap()
 *     (properti vendor "<name> = <&phandle reg mask>" tetap dipertahankan).
 *   - of_device.h -> of.h (of_device.h kini internal).
 *
 * DT live stok (acuan): node soc/aon/socid@402e00e0 compatible "sprd,soc-id",
 *   chip_id         = <0x0c 0xe0 0xffffffff>   (2 reg: +0x0, +0x4)
 *   plat_id         = <0x0c 0xe8 0xffffffff>
 *   implement_id    = <0x0c 0xf0 0xffffffff>
 *   manufacture_id  = <0x0c 0xf4 0xffffffff>
 *   version_id      = <0x0c 0xf8 0xffffffff>
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/soc/sprd/sprd_soc_id.h>

static const char * const syscon_name[] = {
	"chip_id",
	"plat_id",
	"implement_id",
	"manufacture_id",
	"version_id"
};

struct register_gpr {
	struct regmap *gpr;
	u32 reg;
	u32 mask;
};
static struct register_gpr syscon_regs[ARRAY_SIZE(syscon_name)];

/* deklarasi provider (dibuat tools/re/fix_provider_proto.py; menutup -Wmissing-prototypes) */
int sprd_get_soc_id(sprd_soc_id_type_t soc_id_type, u32 *id, int id_len);
int sprd_get_soc_id(sprd_soc_id_type_t soc_id_type, u32 *id, int id_len)
{
	int ret;
	u32 chip_id[2];

	switch (soc_id_type) {
	case AON_CHIP_ID:
	case AON_PLAT_ID:
		if (id_len < 2) {
			pr_err("id_len < 2\n");
			return -EINVAL;
		}
		if (!syscon_regs[soc_id_type].gpr)
			return -ENODEV;

		ret = regmap_read(syscon_regs[soc_id_type].gpr,
				  syscon_regs[soc_id_type].reg, &chip_id[0]);
		if (ret) {
			pr_err("Failed to read chip_id[0]\n");
			return -EINVAL;
		}
		ret = regmap_read(syscon_regs[soc_id_type].gpr,
				  syscon_regs[soc_id_type].reg + 0x4,
				  &chip_id[1]);
		if (ret) {
			pr_err("Failed to read chip_id[1]\n");
			return -EINVAL;
		}
		id[0] = chip_id[0];
		id[1] = chip_id[1];
		break;
	case AON_IMPL_ID:
	case AON_MFT_ID:
	case AON_VER_ID:
		if (!syscon_regs[soc_id_type].gpr)
			return -ENODEV;

		ret = regmap_read(syscon_regs[soc_id_type].gpr,
				  syscon_regs[soc_id_type].reg, id);
		if (ret) {
			pr_err("Failed to read soc id\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sprd_get_soc_id);

static int sprd_soc_id_probe(struct platform_device *pdev)
{
	int i, ret;
	struct device_node *np = pdev->dev.of_node;
	struct of_phandle_args args;
	struct regmap *tregmap;

	for (i = 0; i < ARRAY_SIZE(syscon_name); i++) {
		ret = of_parse_phandle_with_fixed_args(np, syscon_name[i],
						       2, 0, &args);
		if (ret) {
			pr_err("fail to parse %s args (%d)\n", syscon_name[i],
			       ret);
			continue;
		}

		tregmap = syscon_node_to_regmap(args.np);
		of_node_put(args.np);
		if (IS_ERR_OR_NULL(tregmap)) {
			pr_err("fail to map %s regmap\n", syscon_name[i]);
			continue;
		}

		syscon_regs[i].gpr = tregmap;
		syscon_regs[i].reg = args.args[0];
		syscon_regs[i].mask = args.args[1];
		pr_debug("dts[%s] 0x%x 0x%x\n", syscon_name[i],
			 syscon_regs[i].reg, syscon_regs[i].mask);
	}

	return 0;
}

static const struct of_device_id sprd_soc_id_of_match[] = {
	{ .compatible = "sprd,soc-id" },
	{ },
};
MODULE_DEVICE_TABLE(of, sprd_soc_id_of_match);

static struct platform_driver sprd_soc_id_driver = {
	.probe = sprd_soc_id_probe,
	.driver = {
		.name = "sprd-soc-id",
		.of_match_table = sprd_soc_id_of_match,
	},
};
module_platform_driver(sprd_soc_id_driver);

MODULE_AUTHOR("Luting Guo <luting.guo@spreadtrum.com> (fork GPL)");
MODULE_AUTHOR("LinDroid/VXTux — port 6.18");
MODULE_DESCRIPTION("Spreadtrum soc id driver (VXTux port)");
MODULE_LICENSE("GPL v2");
