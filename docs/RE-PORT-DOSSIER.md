# Dossier RE — 12 blob stok yang sumbernya tidak ada di pohon

Dibuat oleh `donors/port_dossier.py` dari blob 5.4.210 itu sendiri
(`nm` + `strings`), tanpa decompiler. Tujuannya: tiap blob punya
spesifikasi awal yang cukup untuk menulis driver penggantinya —
ABI yang harus disediakan, API kernel yang dipakai, dan nama-nama
antarmuka (DT compatible, /proc, /sys) yang harus tetap sama.


## `bq2560x-charger.ko`

- ukuran: **60766 B**, fungsi: **32**, simbol dipakai: **65**, EXPORT: **0**
- subsystem: power supply / charger
- description: `BQ2560X Charger Driver` · author: `-` · depends: `sprd_battery_info`
- API kernel yang dipakai mengarah ke: regulator, power_supply, i2c/spi, sysfs/procfs, reset/regmap, platform/OF
- UNDEFINED (65), 12 pertama: `__cfi_slowpath`, `__dynamic_dev_dbg`, `__mutex_init`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `_dev_err`, `_dev_info`, `_dev_warn`, `alarm_cancel`, `alarm_init`, `alarm_start`
- penanda antarmuka:
  - `sprd,sc2721`
  - `sprd,ump962x-syscon`
  - `sprd,sc27xx-syscon`


## `mmc_swcq.ko`

- ukuran: **72670 B**, fungsi: **40**, simbol dipakai: **62**, EXPORT: **5**
- subsystem: MMC host queue
- description: `MMC Host Software Queue support` · author: `-` · depends: ``
- API kernel yang dipakai mengarah ke: mmc, sysfs/procfs
- EXPORT (5): `dbg_add_host_log`, `mmc_swcq_finalize_request`, `mmc_swcq_init`, `mmc_swcq_resume`, `mmc_swcq_suspend`
- UNDEFINED (62), 12 pertama: `__blk_mq_end_request`, `__cfi_slowpath`, `__dynamic_pr_debug`, `__init_waitqueue_head`, `__list_add_valid`, `__list_del_entry_valid`, `__msecs_to_jiffies`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `__wake_up`, `_raw_spin_lock_irqsave`


## `snd-soc-sprd-codec-sc2730-power.ko`

- ukuran: **57542 B**, fungsi: **21**, simbol dipakai: **23**, EXPORT: **0**
- subsystem: ASoC codec power
- description: `SPRD audio power regulator driver` · author: `Jian Chen <jian.chen@spreadtrum.com>` · depends: `snd-soc-sprd-card`
- API kernel yang dipakai mengarah ke: regulator, reset/regmap, platform/OF
- UNDEFINED (23), 12 pertama: `__cfi_slowpath`, `__const_udelay`, `__dynamic_pr_debug`, `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `__udelay`, `_dev_err`, `dev_get_regmap`, `get_sp_audio_debug_flag`, `kfree`
- penanda antarmuka:
  - `unisoc,sc2730-audio-codec`


## `snd-soc-sprd-codec-sc2730-power-dev.ko`

- ukuran: **29886 B**, fungsi: **2**, simbol dipakai: **5**, EXPORT: **0**
- subsystem: ASoC codec power (device)
- description: `SPRD audio power regulator device` · author: `Jian Chen <jian.chen@spreadtrum.com>` · depends: ``
- API kernel yang dipakai mengarah ke: platform/OF
- UNDEFINED (5), 12 pertama: `__ubsan_handle_cfi_check_fail_abort`, `of_device_is_available`, `of_find_compatible_node`, `platform_device_register`, `printk`
- penanda antarmuka:
  - `unisoc,sc2730-audio-codec`


## `sprd-cpufreq-public.ko`

- ukuran: **17334 B**, fungsi: **5**, simbol dipakai: **11**, EXPORT: **1**
- subsystem: cpufreq
- description: `-` · author: `-` · depends: ``
- API kernel yang dipakai mengarah ke: cpufreq, platform/OF
- EXPORT (1): `sprd_cpufreq_update_opp`
- UNDEFINED (11), 12 pertama: `__cfi_slowpath`, `__dynamic_pr_debug`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `cpufreq_get_policy`, `get_cpu_device`, `memset`, `of_parse_phandle`, `printk`, `strcmp`


## `sprd-cpufreq-v2.ko`

- ukuran: **56030 B**, fungsi: **23**, simbol dipakai: **68**, EXPORT: **0**
- subsystem: cpufreq
- description: `sprd cpufreq v2 driver` · author: `-` · depends: `sprd_sip_svc`
- API kernel yang dipakai mengarah ke: cpufreq, sysfs/procfs, platform/OF
- UNDEFINED (68), 12 pertama: `__cfi_slowpath`, `__cpu_possible_mask`, `__dynamic_dev_dbg`, `__list_add_valid`, `__mutex_init`, `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__sw_hweight64`, `__ubsan_handle_cfi_check_fail_abort`, `_dev_err`, `_dev_info`
- penanda antarmuka:
  - `sprd,cpufreq-boost`
  - `sprd,voltage-margin`
  - `sprd,multi-version`
  - `sprd,transition-delay`
  - `sprd,multi-supply`
  - `sprd,voltage-step`
  - `sprd,temp-threshold`
  - `sprd,cluster-cpumask`
  - `sprd,cpufreq-v2`
  - `sprd,pmic-type`


## `sprd_7sreset.ko`

- ukuran: **23350 B**, fungsi: **15**, simbol dipakai: **16**, EXPORT: **0**
- subsystem: SoC reset
- description: `Unisoc sc27xx 7SReset Driver` · author: `Erick Chen <erick.chen@unisoc.com>` · depends: ``
- API kernel yang dipakai mengarah ke: sysfs/procfs, reset/regmap, platform/OF
- UNDEFINED (16), 12 pertama: `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `_dev_err`, `dev_get_regmap`, `devm_kmalloc`, `kstrtoint`, `of_device_get_match_data`, `of_property_read_u32_index`, `platform_driver_unregister`, `regmap_read`
- penanda antarmuka:
  - `sprd,sc2720-7sreset`
  - `sprd,sc2721-7sreset`
  - `sprd,sc2730-7sreset`
  - `sprd,sc2731-7sreset`
  - `sprd,ump9620-7sreset`


## `sprd_iq.ko`

- ukuran: **32566 B**, fungsi: **15**, simbol dipakai: **41**, EXPORT: **0**
- subsystem: camera ISP tuning
- description: `SPRD IQ driver` · author: `zhongping tan` · depends: `usb_f_vser,sipc-core`
- API kernel yang dipakai mengarah ke: platform/OF, misc/char
- UNDEFINED (41), 12 pertama: `__arch_copy_from_user`, `__arch_copy_to_user`, `__cfi_slowpath`, `__init_waitqueue_head`, `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `__wake_up`, `_dev_err`, `_dev_info`, `arm64_const_caps_ready`
- penanda antarmuka:
  - `sprd,region`
  - `sprd,mapping-offs`
  - `sprd,iq`


## `sprd_manufacturer_model.ko`

- ukuran: **16902 B**, fungsi: **5**, simbol dipakai: **7**, EXPORT: **0**
- subsystem: SoC id
- description: `-` · author: `Shian.Wang` · depends: ``
- API kernel yang dipakai mengarah ke: sysfs/procfs, platform/OF
- UNDEFINED (7), 12 pertama: `__ubsan_handle_cfi_check_fail_abort`, `device_initialize`, `of_find_node_opts_by_path`, `of_property_read_string`, `printk`, `sprintf`, `sysfs_create_group`


## `sprd_pdbg.ko`

- ukuran: **40854 B**, fungsi: **16**, simbol dipakai: **51**, EXPORT: **1**
- subsystem: power debug
- description: `sprd power debug driver` · author: `-` · depends: `sprd_sip_svc`
- API kernel yang dipakai mengarah ke: sysfs/procfs, platform/OF
- EXPORT (1): `sprd_pdbg_msg_print`
- UNDEFINED (51), 12 pertama: `__cfi_slowpath`, `__list_add_valid`, `__list_del_entry_valid`, `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `_raw_read_lock`, `_raw_read_unlock`, `_raw_write_lock`, `_raw_write_unlock`, `cancel_delayed_work_sync`
- penanda antarmuka:
  - `sprd,pdbg-irq-domain-gpio`
  - `sprd,pdbg-irq-domain-ap-eic-async`
  - `sprd,pdbg-irq-domain-ana`
  - `sprd,pdbg-irq-domain-ap-eic-latch`
  - `sprd,pdbg-irq-domain-gic`
  - `sprd,pdbg-irq-domain-ap-eic-dbnc`
  - `sprd,pdbg-irq-domain-ap-eic-sync`
  - `sprd,pdbg-irq-domain-ana-eic`
  - `sprd,debuglog`


## `sprd_power_stat.ko`

- ukuran: **42670 B**, fungsi: **30**, simbol dipakai: **30**, EXPORT: **2**
- subsystem: power debug
- description: `sprd debug stat driver` · author: `-` · depends: `sprd_sip_svc`
- API kernel yang dipakai mengarah ke: sysfs/procfs, reset/regmap, platform/OF
- EXPORT (2): `stat_info_register`, `stat_info_unregister`
- UNDEFINED (30), 12 pertama: `__cfi_slowpath`, `__cpu_online_mask`, `__list_add_valid`, `__list_del_entry_valid`, `__platform_driver_register`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `devm_kmalloc`, `kfree`, `kmalloc_caches`, `kmem_cache_alloc_trace`
- penanda antarmuka:
  - `sprd,debugstat`


## `sprd_sip_svc.ko`

- ukuran: **30326 B**, fungsi: **32**, simbol dipakai: **5**, EXPORT: **1**
- subsystem: firmware SIP service
- description: `sprd implementation-specific services` · author: `-` · depends: ``
- API kernel yang dipakai mengarah ke: firmware/SIP
- EXPORT (1): `sprd_sip_svc_get_handle`
- UNDEFINED (5), 12 pertama: `__arm_smccc_smc`, `__stack_chk_fail`, `__stack_chk_guard`, `__ubsan_handle_cfi_check_fail_abort`, `printk`


## Urutan port yang disarankan

Dihitung dari field `depends` di modinfo blob: modul yang tidak
bergantung pada blob lain harus di-port lebih dulu.

**Tidak bergantung blob lain (bisa mulai sekarang):**

- `bq2560x-charger.ko`
- `mmc_swcq.ko`
- `snd-soc-sprd-codec-sc2730-power-dev.ko`
- `snd-soc-sprd-codec-sc2730-power.ko`
- `sprd-cpufreq-public.ko`
- `sprd_7sreset.ko`
- `sprd_iq.ko`
- `sprd_manufacturer_model.ko`
- `sprd_sip_svc.ko`

**Menunggu blob lain:**

- `sprd-cpufreq-v2.ko` ← `sprd_sip_svc`
- `sprd_pdbg.ko` ← `sprd_sip_svc`
- `sprd_power_stat.ko` ← `sprd_sip_svc`

## Compatible DT: ada node-nya?

Blob hanya akan probe kalau `compatible`-nya ada di DTS. Kolom
`ada di DTB stok` dibaca dari `donors/stock_initramfs/stock.dtb`:
kalau **ya** tapi kolom DTS vxtux **tidak ada**, berarti node DT-nya
ikut hilang dan harus di-port juga — bukan cuma kodenya.

| blob | compatible | ada di DTB stok | ada di DTS vxtux |
|---|---|---|---|
| `bq2560x-charger.ko` | `sprd,sc2721` | tidak | **tidak ada** |
| `bq2560x-charger.ko` | `sprd,ump962x-syscon` | tidak | **tidak ada** |
| `bq2560x-charger.ko` | `sprd,sc27xx-syscon` | ya | `sprd\sc2730.dtsi` |
| `snd-soc-sprd-codec-sc2730-power.ko` | `unisoc,sc2730-audio-codec` | ya | **tidak ada** |
| `snd-soc-sprd-codec-sc2730-power-dev.ko` | `unisoc,sc2730-audio-codec` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,cpufreq-boost` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,voltage-margin` | tidak | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,multi-version` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,transition-delay` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,multi-supply` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,voltage-step` | tidak | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,temp-threshold` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,cluster-cpumask` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,cpufreq-v2` | ya | **tidak ada** |
| `sprd-cpufreq-v2.ko` | `sprd,pmic-type` | ya | **tidak ada** |
| `sprd_7sreset.ko` | `sprd,sc2720-7sreset` | tidak | **tidak ada** |
| `sprd_7sreset.ko` | `sprd,sc2721-7sreset` | tidak | **tidak ada** |
| `sprd_7sreset.ko` | `sprd,sc2730-7sreset` | ya | **tidak ada** |
| `sprd_7sreset.ko` | `sprd,sc2731-7sreset` | tidak | **tidak ada** |
| `sprd_7sreset.ko` | `sprd,ump9620-7sreset` | tidak | **tidak ada** |
| `sprd_iq.ko` | `sprd,region` | ya | **tidak ada** |
| `sprd_iq.ko` | `sprd,mapping-offs` | ya | **tidak ada** |
| `sprd_iq.ko` | `sprd,iq` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-gpio` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ap-eic-async` | tidak | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ana` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ap-eic-latch` | tidak | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-gic` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ap-eic-dbnc` | tidak | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ap-eic-sync` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,pdbg-irq-domain-ana-eic` | ya | **tidak ada** |
| `sprd_pdbg.ko` | `sprd,debuglog` | ya | **tidak ada** |
| `sprd_power_stat.ko` | `sprd,debugstat` | ya | **tidak ada** |

## Cara memakai dossier ini

1. `UNDEFINED` menentukan subsistem: kalau isinya `regulator_*` dan
   `power_supply_*`, driver baru harus mendaftar sebagai power supply,
   bukan sebagai misc device.
2. `EXPORT` adalah kontrak ke modul lain — kalau blob lain memanggil
   simbol itu, penggantinya wajib mengekspor nama yang sama, kalau
   tidak `modprobe` modul pemakainya akan gagal.
3. Penanda `sprd,...` adalah `compatible` DT. Node DT di
   `ums512-1h10-vxtux.dtb` harus cocok, kalau tidak driver tidak
   pernah probe walau kodenya benar.
