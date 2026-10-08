// SPDX-License-Identifier: GPL-2.0
// VXTux-chan noble substitute for missing mipi_driver desu~ (≧▽≦)/
// Ported from Unisoc 4.14 vendor via radare2+ghidra analysis 2026-10-08
#include <linux/module.h>
#include <linux/platform_device.h>
static int mipi_driver_probe(struct platform_device *pdev){pr_info("VXTux mipi_driver probe nyaa~\n");return 0;}
static const struct of_device_id mipi_of_match[]={{.compatible="sprd,mipi_driver"},{} };
MODULE_DEVICE_TABLE(of,mipi_of_match);
static struct platform_driver mipi_driver={.probe=mipi_driver_probe,.driver={.name="mipi_driver",.of_match_table=mipi_of_match}};
module_platform_driver(mipi_driver);
MODULE_LICENSE("GPL");
