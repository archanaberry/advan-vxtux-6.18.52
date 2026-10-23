# 10 blob yang belum tertangani — bukti dan langkah berikutnya

Status per build CI `39946023755` (commit `a0f2e70ce`, 229 `.ko`, rc=0):

```
119/129   KO=58  Y=47  BUILT-IN=12  SUPERSEDED=2  BELUM=10
```

Angka diambil [coverage_129.py](../donors/coverage_129.py) terhadap daftar
`.ko` hasil build ([ko_list_tick16.txt](evidence-20261023/ko_list_tick16.txt))
dan [PORT-AUDIT.md](PORT-AUDIT.md) yang baru diregenerasi. Dua blob
(`core.ko`, `gpio.ko`) sebelumnya terhitung selesai karena dipasangkan ke
Kconfig yang salah; lihat catatan di bawah.

Setiap entri di bawah menyebut **bukti** yang sudah ada dan **apa yang masih
kurang**. Tidak ada driver yang ditulis dari tebakan: yang belum punya bukti
cukup tetap berstatus belum.

---

## 1. `core.ko` + 2. `gpio.ko` — framework SPRD autotest

Bukti dari blob:

| Item | Nilai |
|---|---|
| `core.ko` MODULE_DESCRIPTION | `sprd autotest core driver` |
| `gpio.ko` MODULE_DESCRIPTION | `sprd autotest gpio driver`, `depends=core` |
| Ekspor `core.ko` | `sprd_autotest_register_handler`, `sprd_autotest_unregister_handler` |
| String `core.ko` | `autotest`, `autotest%d`, `failed to create autotest class`, `autotest cmd = %x`, `try to add dupicated autotest handler` |
| API `core.ko` | `cdev_init`, `cdev_device_add`, `alloc_chrdev_region`, `class_create`, `mutex_*`, `kmem_cache_alloc_trace` |

`init_module` `gpio.ko` cuma memanggil `sprd_autotest_register_handler(&h)`.

Hasil baca `sprd_autotest_ioctl` (objdump, 196 B) — ini yang sudah pasti:

- argumen: `(file, unsigned int cmd, unsigned long arg)`; cmd dicetak lewat
  `printk("autotest cmd = %x")`;
- `mutex_lock(&autotest->lock)`, lalu menelusuri `list_head` di `+0x440`;
- pencocokan `ldur w10, [node, #-8]` ⇒ `cmd` ada di `handler + 0x08`;
- node = `&handler->list`, `container_of` = `node - 0x10` ⇒ `list` di `+0x10`;
- handler dipanggil `blr` dengan `x0 = handler`, `x1 = arg`, hasil `sxtw`;
- `handler + 0x28` = pointer fungsi (relokasi `.data+0x28` → `.text+0x10c8`);
- `handler + 0x00` = pointer nama (relokasi ke `.rodata`);
- tidak ada handler cocok ⇒ `-EINVAL`.

Struct `gpio.ko` di `.data` cocok dengan pola itu: `+0x00` nama, `+0x08`
`cmd = 0xc0044101` (arah `_IOWR`, size 4, type `'A'`, nr 1).

**Yang masih kurang:** isi `gpio_test` (`0x30`..`0x10c8`) memakai GPIO
descriptor API dan pencocokan nama chip (`gpio_match_name_gpio`), tapi
urutan tes (pin mana, pola apa) baru terbaca sebagian. Butuh pembacaan
penuh fungsi itu sebelum bisa ditulis 1:1.

---

## 3. `mmc_swcq.ko` — MMC host software queue

| Item | Nilai |
|---|---|
| ukuran / fungsi | 72.670 B / 40 |
| EXPORT | `dbg_add_host_log`, `mmc_swcq_finalize_request`, `mmc_swcq_init`, `mmc_swcq_resume`, `mmc_swcq_suspend` |
| simbol | 62, didominasi `blk_mq_*` (`__blk_mq_end_request`, `blk_mq_*`) + `mmc_*` |

Blob ini menambah lapisan antrean di atas `mmc_host` dan blk-mq. 40 fungsi
tanpa satu pun header/ABI publik yang bisa dipakai sebagai acuan; ini
kandidat paling mahal dari sepuluh sisanya. **Butuh sumber vendor** (atau
dekompilasi penuh) sebelum bisa ditulis.

## 4. `sprd-cpufreq-v2.ko` — cpufreq DVFS

| Item | Nilai |
|---|---|
| ukuran / fungsi | 56.030 B / 23 |
| depends | `sprd_sip_svc` (kini tersedia) |
| penanda DT | `sprd,cpufreq-v2`, `sprd,cpufreq-boost`, `sprd,voltage-margin`, `sprd,multi-version`, `sprd,transition-delay`, `sprd,multi-supply`, `sprd,voltage-step`, `sprd,temp-threshold`, `sprd,cluster-cpumask`, `sprd,pmic-type` |
| node stok | `/soc/aon/apcpu-dvfs` (`compatible = "sprd,cpufreq-v2"`) |

Sisi firmware sudah terbuka: `sprd_sip_svc` sekarang mengekspor
`dvfs_*` (ID `0x82000601`..`0x8200060c` dari
[RE-SPRD-SIP-SVC.md](RE-SPRD-SIP-SVC.md)). Yang belum ada: tabel OPP, arti
tiap properti DT di atas, dan urutan `dvfs_init`/`table_update` yang dipakai
driver. Butuh pembacaan `sprd_cpufreq_probe` + `sprd_cpufreq_dvfs_*`.

## 5. `sprd-cpufreq-public.ko` — API cpufreq untuk modul lain

5 fungsi, 1 ekspor: `sprd_cpufreq_update_opp`. Ini lapisan tipis di atas
`cpufreq_get_policy()` + `dev_pm_opp_*` yang dipanggil driver lain
(thermal/boost). Bisa ditulis setelah #4 ada, karena semantiknya bergantung
pada tabel OPP yang dikelola #4.

## 6. `sprd_7sreset.ko` — PMIC 7S reset

| Item | Nilai |
|---|---|
| ukuran / fungsi | 23.350 B / 15 |
| API | `dev_get_regmap`, `regmap_read`, `regmap_update_bits_base`, `kstrtoint`, `of_property_read_u32_index`, `sysfs_create_groups` |
| atribut sysfs | `module_disable`, `hard_mode`, `short_mode`, `keypad_mode`, `threshold_time` |
| compatible | `sprd,sc2720-7sreset`, `sc2721`, `sc2730`, `sc2731`, `ump9620-7sreset` |
| node stok | `/soc/aon/spi@32100000/pmic@0/hw-reset@1bc0`, `reg = <0x1bf8 0x1bc0>` |

Yang belum: alamat bit per mode di regmap PMIC dan arti dua nilai `reg`.
Tanpa itu, driver hanya bisa ditulis sebagai kerangka — jadi belum
dikerjakan.

## 7. `sprd_iq.ko` — camera IQ

15 fungsi, 32 KB, `depends = usb_f_vser,sipc-core`; node
`/reserved-memory/iq-mem@90000000` + `/sprd-iq`. Butuh `usb_f_vser`
(fungsi gadget vendor) yang juga belum ada di tree, jadi berantai.

## 8. `sprd_manufacturer_model.ko` — model/manufacturer SoC

5 fungsi, 7 simbol: `of_find_node_opts_by_path`, `of_property_read_string`,
`device_initialize`, `sysfs_create_group`, `sprintf`, `printk`.
String: `soc-manufacturer`, `soc_manufacturer`, `soc_model`, `soc0`,
`soc_model: %s`, `soc_manufacturer_show`, `soc_model_show`.

Namun **node `soc-manufacturer` tidak ada di `stock.dtb`** (`fdt_dump.py
--paths` tidak menemukannya); artinya path yang dicari driver bukan node DT
biasa, dan sysfs group-nya ditempel ke device yang dibuat sendiri
(`sprd_manufacturer_model_dev`). Tanpa tahu path/node itu, driver hanya
bisa menebak dari mana string dibaca.

## 9. `sprd_pdbg.ko` — power debug (irq domain)

| Item | Nilai |
|---|---|
| ukuran / fungsi | 40.854 B / 16 |
| depends | `sprd_sip_svc` (kini tersedia) |
| EKSPOR | `sprd_pdbg_msg_print` |
| node stok | `/debuglog`, properti `sprd,pdbg-irq-domain-{gic,gpio,ana,ana-eic,ap-eic-sync}` |
| SMC | `pwr_get_pdbg_info` = `0x82000502` (sudah dibungkus `sprd_sip_svc`) |

Blokir sebenarnya bukan simbol lagi: yang belum dipetakan adalah cara
`/debuglog` memakai nama irq-domain itu (dicocokkan ke `irq_domain` mana) dan
format keluaran sysfs-nya.

## 10. `sprd_power_stat.ko` — statistik tidur

| Item | Nilai |
|---|---|
| ukuran / fungsi | 42.670 B / 30 |
| depends | `sprd_sip_svc` (kini tersedia) |
| EKSPOR | `stat_info_register`, `stat_info_unregister` |
| node stok | `/debugstat`, properti `soc_slp_cnt_reg = <0x2 0x784 0xff>` |
| SMC | `pwr_get_wakeup_source` = `0x82000501` (sudah dibungkus) |

Sama seperti #9: ABI firmware-nya sudah tersedia, yang belum adalah format
node sysfs dan pemakaian `soc_slp_cnt_reg` (offset regmap 0x784, mask 0xff).

---

## Urutan yang paling murah lebih dulu

1. `sprd_power_stat` dan `sprd_pdbg` — SMC sudah ada; sisa pekerjaan RE
   terbatas pada format sysfs + pemetaan irq-domain.
2. `sprd-cpufreq-v2` lalu `sprd-cpufreq-public`.
3. `sprd_7sreset` (butuh peta bit regmap PMIC).
4. `sprd_manufacturer_model` (butuh asal string).
5. `sprd_iq` (berantai ke `usb_f_vser`).
6. `mmc_swcq` (paling besar, butuh sumber).
7. `core.ko` + `gpio.ko` (autotest) — ABI ioctl sudah terbaca, tinggal
   `gpio_test`.
