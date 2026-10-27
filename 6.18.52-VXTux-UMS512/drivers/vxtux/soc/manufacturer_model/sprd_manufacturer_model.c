// SPDX-License-Identifier: GPL-2.0
/*
 * sprd_manufacturer_model - expose the SoC model / manufacturer strings.
 *
 * Port of the vendor 5.4 module `sprd_manufacturer_model.ko` (5 fungsi,
 * 7 simbol).  Bukti: docs/evidence-20261023/sprd_manufacturer_model.objdump.txt
 * (objdump -dr, relokasi diterapkan) dan ringkasan docs/RE-SPRD-MANUFACTURER.md.
 *
 * Alur blob (init_module, .init.text 0x0..0xbc):
 *   np = of_find_node_opts_by_path("/", NULL);
 *   of_property_read_string(np, "soc-module",       &sprd_soc_model);
 *   of_property_read_string(np, "soc-manufacturer", &sprd_soc_manufacturer);
 *   device_initialize(&sprd_manufacturer_model_dev);
 *   sysfs_create_group(&...->kobj, &sprd_manufacturer_model_group);
 *   printk(sukses ? "Init manufacturer_model mudule successfully"
 *                 : "Init manufacturer_model mudule failed");
 *
 * Di stok DTB properti itu ada di root: soc-module = "UMS512",
 * soc-manufacturer = "Spreadtrum".  Kedua atribut memakai format string
 * yang sama di blob ("soc_model: %s \n" di .rodata+0x36) — dipertahankan
 * apa adanya supaya keluaran sysfs identik dengan firmware stok.
 *
 * Device-nya hanya `device_initialize()` (tidak pernah `device_add()`),
 * jadi grup sysfs gagal dipasang dan modul mencetak "failed" — persis
 * perilaku blob; string tetap terbaca lewat variabel global.
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/sysfs.h>

static const char *sprd_soc_model;		/* .bss+0x00 */
static const char *sprd_soc_manufacturer;	/* .bss+0x08 */
static struct device sprd_manufacturer_model_dev;	/* .bss+0x10 */

static ssize_t soc_model_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "soc_model: %s \n", sprd_soc_model);
}

static ssize_t soc_manufacturer_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "soc_model: %s \n", sprd_soc_manufacturer);
}

static DEVICE_ATTR_RO(soc_model);
static DEVICE_ATTR_RO(soc_manufacturer);

static struct attribute *default_attrs[] = {
	&dev_attr_soc_model.attr,
	&dev_attr_soc_manufacturer.attr,
	NULL,
};

static const struct attribute_group sprd_manufacturer_model_group = {
	.attrs = default_attrs,
};

static int __init sprd_manufacturer_model_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_node_opts_by_path("/", NULL);
	if (np) {
		if (of_property_read_string(np, "soc-module",
					    &sprd_soc_model))
			sprd_soc_model = NULL;
		if (of_property_read_string(np, "soc-manufacturer",
					    &sprd_soc_manufacturer))
			sprd_soc_manufacturer = NULL;
	}

	device_initialize(&sprd_manufacturer_model_dev);
	ret = sysfs_create_group(&sprd_manufacturer_model_dev.kobj,
				 &sprd_manufacturer_model_group);
	printk("%s", ret ? "Init manufacturer_model mudule failed"
			 : "Init manufacturer_model mudule successfully");
	return 0;
}

static void __exit sprd_manufacturer_model_exit(void)
{
	printk("Exit manufacturer_model mudule...");
}

module_init(sprd_manufacturer_model_init);
module_exit(sprd_manufacturer_model_exit);

MODULE_DESCRIPTION("sprd manufacturer model");
MODULE_LICENSE("GPL v2");
