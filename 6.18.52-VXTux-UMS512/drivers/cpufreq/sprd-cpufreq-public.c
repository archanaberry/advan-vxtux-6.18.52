// SPDX-License-Identifier: GPL-2.0
//
// Unisoc cpufreq driver public data
//
// Copyright (C) 2021 Unisoc, Inc.
//
// Ported to 6.18 for the VXTux tree.  The exported symbol
// sprd_cpufreq_update_opp() lets thermal/cooling consumers reach into the
// cpufreq-v2 driver's per-cluster update_opp hook.  Behavior matches the
// vendor blob (see docs/evidence-20261023/cpufreq_public.resolved.txt): get
// cpu0's policy, resolve its cpufreq-data-v1 phandle, compare the node name
// to pick the "normal" (cluster_info) or "common" (driver_data) layout, then
// call through the matching function pointer with (cpu, temp).

#include "sprd-cpufreq-common.h"
#include "sprd-cpufreq-v2.h"

unsigned int sprd_cpufreq_update_normal(struct cpufreq_policy *policy,
					int cpu, int temp)
{
	struct cluster_info *info = NULL;

	info = policy->driver_data;
	if (IS_ERR_OR_NULL(info) || !info->update_opp)
		return 0;

	return info->update_opp(cpu, temp);
}

unsigned int sprd_cpufreq_update_common(struct cpufreq_policy *policy,
					int cpu, int temp)
{
	struct sprd_cpufreq_driver_data *data = NULL;

	data = policy->driver_data;
	if (IS_ERR_OR_NULL(data) || !data->update_opp)
		return 0;

	return data->update_opp(cpu, temp);
}

unsigned int sprd_cpufreq_update_opp(int cpu, int temp_now)
{
	struct device_node *cpu_np, *cpufreq_np;
	struct device *cpu_dev;
	struct cpufreq_policy *policy;
	unsigned int max_freq = 0;

	/*
	 * The vendor blob called the 4.14 cpufreq_get_policy(&policy, 0),
	 * which filled a stack copy.  6.18 dropped that entry point in favour
	 * of the refcounted cpufreq_cpu_get(), which returns the live policy
	 * and needs a matching cpufreq_cpu_put() on every exit path.
	 */
	policy = cpufreq_cpu_get(0);
	if (!policy) {
		pr_debug("%s: No cpu data found\n", __func__);
		return 0;
	}

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		pr_err("%s: Failed to get cpu0 device\n", __func__);
		goto out_put;
	}

	cpu_np = of_node_get(cpu_dev->of_node);
	if (!cpu_np) {
		pr_err("%s: Failed to find cpu node\n", __func__);
		goto out_put;
	}

	cpufreq_np = of_parse_phandle(cpu_np, "cpufreq-data-v1", 0);
	if (!cpufreq_np) {
		pr_err("%s: No cpufreq data found for cpu0\n", __func__);
		of_node_put(cpu_np);
		goto out_put;
	}

	pr_debug("%s: cluster name is: %s\n", __func__, cpufreq_np->full_name);

	if (!strcmp(cpufreq_np->full_name, "cpufreq-cluster0"))
		max_freq = sprd_cpufreq_update_normal(policy, cpu, temp_now);
	else if (!strcmp(cpufreq_np->full_name, "cpufreq-clus0"))
		max_freq = sprd_cpufreq_update_common(policy, cpu, temp_now);
	else
		pr_err("%s: Error name of cpufreq data v1!\n", __func__);

	of_node_put(cpufreq_np);
	of_node_put(cpu_np);

out_put:
	cpufreq_cpu_put(policy);

	return max_freq;
}
EXPORT_SYMBOL_GPL(sprd_cpufreq_update_opp);

MODULE_LICENSE("GPL v2");
