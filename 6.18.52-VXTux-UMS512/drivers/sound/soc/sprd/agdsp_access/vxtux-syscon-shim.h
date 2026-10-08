/* SPDX-License-Identifier: GPL-2.0
 *
 * vxtux-syscon-shim.h — shim syscon vendor (5.4 Unisoc) -> 6.18 mainline.
 * Hanya untuk berkas port drivers/vxtux/ yang memakai DT property vendor.
 *
 * RE (dari perilaku 5 pemakai fork + format DT fork, sharkl5Pro-fork.dtsi):
 *   DT:
 *     syscons      = <&phandle REG MASK>, <&phandle REG MASK>, ...  (args per
 *                    entri selalu genap: reg lalu mask)
 *     syscon-names = "nama0", "nama1", ...
 *   API:
 *     struct regmap *syscon_regmap_lookup_by_name(np, name)
 *       -> regmap dari phandle entri yang namanya cocok.
 *     int syscon_get_args_by_name(np, name, int nr_bytes, u32 *out)
 *       -> menyalin pasangan {reg, mask} entri itu (nr_args = nr_bytes /
 *          sizeof(u32)), mengembalikan jumlah argumen tersalin.
 *
 * Implementasi 6.18: syscon_regmap_lookup_by_phandle_args() mainline bisa
 * mengambil regmap + args sekaligus; kita hanya perlu mencocokkan indeks
 * nama. Tidak ada perubahan DT — properti vendor tetap dibaca apa adanya.
 */
#ifndef _VXTUX_SYSCON_SHIM_H
#define _VXTUX_SYSCON_SHIM_H

#include <linux/mfd/syscon.h>
#include <linux/of.h>

static inline int vxtux_syscon_name_index(const struct device_node *np,
					  const char *name)
{
	int count, i;
	const char *s;

	if (!np || !name)
		return -EINVAL;

	count = of_property_count_strings(np, "syscon-names");
	if (count < 0)
		return count;

	for (i = 0; i < count; i++) {
		if (!of_property_read_string_index(np, "syscon-names", i, &s) &&
		    !strcmp(s, name))
			return i;
	}

	return -ENODEV;
}

static inline struct regmap *
syscon_regmap_lookup_by_name(const struct device_node *np, const char *name)
{
	struct of_phandle_args a;
	int idx = vxtux_syscon_name_index(np, name);
	int ret;

	if (idx < 0)
		return ERR_PTR(idx);

	/* syscons terdaftar sebagai daftar spesifikasi phandle; #args cell
	 * mengikuti definisi node syscon (biasanya 2: reg, mask). */
	ret = of_parse_phandle_with_args(np, "syscons", NULL, idx, &a);
	if (ret)
		return ERR_PTR(ret);

	return syscon_node_to_regmap(a.np);
}

static inline int syscon_get_args_by_name(const struct device_node *np,
					  const char *name, int nr_bytes,
					  u32 *out)
{
	struct of_phandle_args a;
	int idx = vxtux_syscon_name_index(np, name);
	int nr_args, i, ret;

	if (idx < 0)
		return idx;

	ret = of_parse_phandle_with_args(np, "syscons", NULL, idx, &a);
	if (ret)
		return ret;

	nr_args = nr_bytes / sizeof(u32);
	if (nr_args > a.args_count)
		nr_args = a.args_count;

	for (i = 0; i < nr_args; i++)
		out[i] = a.args[i];

	return nr_args;
}

#endif /* _VXTUX_SYSCON_SHIM_H */
