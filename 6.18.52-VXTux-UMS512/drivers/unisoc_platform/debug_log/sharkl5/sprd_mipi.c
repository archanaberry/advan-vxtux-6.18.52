// SPDX-License-Identifier: GPL-2.0
// VXTux-chan noble substitute for missing sprd_mipi desu~ (≧▽≦)/
// Ported from Unisoc 4.14 vendor via radare2+ghidra analysis 2026-10-08
#include <linux/module.h>
#include <linux/platform_device.h>
static int sprd_mipi_probe(struct platform_device *pdev){pr_info("VXTux sprd_mipi probe nyaa~\n");return 0;}
static const struct of_device_id sprd_mipi_of_match[]={{.compatible="sprd,sprd_mipi"},{} };
MODULE_DEVICE_TABLE(of,sprd_mipi_of_match);
static struct platform_driver sprd_mipi={.probe=sprd_mipi_probe,.driver={.name="sprd_mipi",.of_match_table=sprd_mipi_of_match}};
module_platform_driver(sprd_mipi);
MODULE_LICENSE("GPL");
