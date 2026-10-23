# 10 blob yang belum tertangani — bukti dan langkah berikutnya

**Status 2026-10-23 (codespace, 235 `.ko`, rc=0): 6 dari 10 sudah diport.**

| # | blob | status |
|---|---|---|
| 1,2 | `core.ko`, `gpio.ko` | **selesai** — `sprd_autotest_core`, `sprd_autotest_gpio` |
| 6 | `sprd_7sreset.ko` | **selesai** — 5 atribut sysfs + node DT |
| 8 | `sprd_manufacturer_model.ko` | **selesai** — path root `/`, bukan node `soc-manufacturer` |
| 9 | `sprd_pdbg.ko` | **selesai** (commit `cb5e91d35`) |
| 10 | `sprd_power_stat.ko` | **selesai** sebagai `sprd_debugstat` (commit `8bf97026b`) |
| 3 | `mmc_swcq.ko` | belum — 40 fungsi, butuh pembacaan penuh |
| 4 | `sprd-cpufreq-v2.ko` | belum — 23 fungsi, disassembly tersedia |
| 5 | `sprd-cpufreq-public.ko` | belum — bergantung semantik #4 |
| 7 | `sprd_iq.ko` | belum — RE selesai, layout state/mailbox masih gelap |

Angka lama (build CI `39946023755`, commit `a0f2e70ce`, 229 `.ko`):

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

**SELESAI.** `gpio_test` sudah terbaca penuh: argumen u32 user (offset
bit 0..15, dir bit 16, value bit 17), chip dicocokkan lewat
`chip->ngpio == 256`, arah input membaca line dan menulis balik bit 17.
Port: `drivers/vxtux/soc/autotest/`. Rincian: [RE-SPRD-AUTOTEST.md](RE-SPRD-AUTOTEST.md).

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

3 fungsi (`sprd_cpufreq_update_opp` 476 B, `sprd_cpufreq_update_normal` dan
`sprd_cpufreq_update_common` masing-masing 148 B), 1 ekspor GPL:
`sprd_cpufreq_update_opp`. Undefined symbol-nya cuma
`cpufreq_get_policy`, `get_cpu_device`, `of_parse_phandle`, `strcmp`,
`memset`, `printk` — jadi modul ini memang tipis: ambil `policy->driver_data`
dari cluster 0 lalu panggil salah satu dari dua pointer fungsi di dalamnya
(offset 400 untuk nama node `cpufreq-cluster0`, offset 232 untuk
`cpufreq-clus0`), meneruskan dua argumen u32. Bergantung penuh pada #4: tanpa
`cpufreq-v2` tidak ada yang mengisi `driver_data`, sehingga port-nya tidak
berguna lebih dulu.

## 6. `sprd_7sreset.ko` — PMIC 7S reset

| Item | Nilai |
|---|---|
| ukuran / fungsi | 23.350 B / 15 |
| API | `dev_get_regmap`, `regmap_read`, `regmap_update_bits_base`, `kstrtoint`, `of_property_read_u32_index`, `sysfs_create_groups` |
| atribut sysfs | `module_disable`, `hard_mode`, `short_mode`, `keypad_mode`, `threshold_time` |
| compatible | `sprd,sc2720-7sreset`, `sc2721`, `sc2730`, `sc2731`, `ump9620-7sreset` |
| node stok | `/soc/aon/spi@32100000/pmic@0/hw-reset@1bc0`, `reg = <0x1bf8 0x1bc0>` |

**SELESAI.** Peta bit sudah dipulihkan: `module_disable` mask 0x2 bit 1,
`hard_mode` 0x1 bit 0, `short_mode` 0x100 bit 8, `keypad_mode` 0x200 bit 9
(aktif-rendah), `threshold_time` 0xf0 bit 4..7; dua sel `reg` = register
`keypad` (index 0) dan register kontrol 0x1bc0 (index 1); UMP9620 memakai
kunci 0x23d8. Port + node DT: [RE-SPRD-7SRESET.md](RE-SPRD-7SRESET.md).

## 7. `sprd_iq.ko` — camera IQ

15 fungsi, 32 KB, `depends = usb_f_vser,sipc-core`; node `/sprd-iq` dengan
properti `sprd,region` (phandle) dan opsional `sprd,mapping-offs`.

**RE selesai, driver belum ditulis.** Premis lama ("butuh `usb_f_vser` yang
tidak ada di tree") **salah**: `drivers/usb/gadget/function/f_vserial.c`
sudah ada, namanya `usb_f_vser.ko` dan mengekspor
`kernel_vser_register_callback`, `kernel_vser_set_pass_mode`,
`vser_pass_user_write`; API SIPC `shmem_ram_vmap_nocache_ex`/`_unmap_ex`
juga tersedia di `include/linux/soc/sprd/sipc.h`. Rincian lengkap (gate
bootargs `androidboot.mode=iq`, `misc` device `iq_mem`, tabel `ioctl`
0/1/0x80/0x82/0x83, kondisi `poll`, alur `iq_thread`, magic
`0x81005a00`/`0x81815a5a`/`"MPAD"`): [RE-SPRD-IQ.md](RE-SPRD-IQ.md).
Yang menahan port sekarang: layout 8 byte pertama state `iq` dan arah
transisi mode/mailbox belum terbaca pasti, dan salah tebak berarti menulis
ke region reserved modem.

## 8. `sprd_manufacturer_model.ko` — model/manufacturer SoC

5 fungsi, 7 simbol: `of_find_node_opts_by_path`, `of_property_read_string`,
`device_initialize`, `sysfs_create_group`, `sprintf`, `printk`.
String: `soc-manufacturer`, `soc_manufacturer`, `soc_model`, `soc0`,
`soc_model: %s`, `soc_manufacturer_show`, `soc_model_show`.

**SELESAI.** "Blokir" lama ternyata salah baca: blob memakai
`of_find_node_opts_by_path("/", NULL)` lalu membaca properti
`soc-module` dan `soc-manufacturer` dari **root** node, bukan dari node
bernama `soc-manufacturer`. Stok DTB memang punya keduanya
(`UMS512`, `Spreadtrum`). Port: [RE-SPRD-MANUFACTURER.md](RE-SPRD-MANUFACTURER.md).

## 9. `sprd_pdbg.ko` — power debug (irq domain) — **SELESAI (diport)**

| Item | Nilai |
|---|---|
| ukuran / fungsi | 40.854 B / 16 |
| depends | `sprd_sip_svc` (kini tersedia) |
| EKSPOR | `sprd_pdbg_msg_print` |
| node stok | `/debuglog`, properti `sprd,pdbg-irq-domain-{gic,gpio,ana,ana-eic,ap-eic-sync}` |
| SMC | `pwr_get_pdbg_info` = `0x82000502` (sudah dibungkus `sprd_sip_svc`) |

Blokir lama (cara `/debuglog` memakai nama irq-domain + format sysfs) sudah
terpecahkan lewat pembacaan blob penuh — ringkasan bukti dan keputusan port:
[RE-SPRD-PDBG.md](RE-SPRD-PDBG.md); disassembly lengkap
[pdbg_disasm.txt](evidence-20261023/pdbg_disasm.txt) dan
[pdbg_notif.txt](evidence-20261023/pdbg_notif.txt).

Port ada di `drivers/vxtux/soc/pdbg/` (Kconfig `SPRD_PDBG`):

- pemetaan: loop IRQ 0..511, `strcmp` nama domain (`irq_desc->irq_data.domain`
pada 6.18) terhadap 8 string DT, sekali per IRQ lalu nama di-null-kan;
- kthread `sprd-pdbg-thread` + `delayed_work` rescan (jeda dari `poll_ms`);
- pm_notifier (`PM_SUSPEND_PREPARE`/`PM_POST_SUSPEND`) dan cpu_pm_notifier
  (DEEP SLEEP START / WAKEUP SCENE END) — keduanya kini benar-benar di-set
  `.notifier_call`-nya;
- `/proc/sprd-pdbg/regs_info` (`regs_get_once`, 4 kata dari SMC
  `0x82000502` lewat varian baru `sprd_sip_svc_pwr_get_pdbg_info4`);
- module_param `sprd_pdbg_log_force` (level KERN_ERR, sama seperti blob).

Terverifikasi: `sprd_pdbg.o` rc=0 via `cs_quick.sh`, DTB `ums512-1h10-vxtux`
rc=0; build penuh untuk `.ko` — lihat status di commit.

## 10. `sprd_power_stat.ko` — statistik tidur — **SELESAI (diport)**

| Item | Nilai |
|---|---|
| ukuran / fungsi | 42.670 B / 30 |
| depends | `sprd_sip_svc` (kini tersedia) |
| EKSPOR | `stat_info_register`, `stat_info_unregister` |
| node stok | `/debugstat`, properti `soc_slp_cnt_reg = <0x2 0x784 0xff>` |
| SMC | `pwr_get_wakeup_source` = `0x82000501` (sudah dibungkus) |

Diport sebagai `sprd_debugstat` (commit `8bf97026b`):
`drivers/vxtux/soc/debugstat/`, Kconfig `SPRD_DEBUGSTAT`, kolom
`/proc/sprd-debugstat/*` dan pemakaian `soc_slp_cnt_reg` (regmap 0x784,
mask 0xff) mengikuti [RE-SPRD-POWER-STAT.md](RE-SPRD-POWER-STAT.md).

---

## Urutan yang paling murah lebih dulu

1. `sprd_power_stat` dan `sprd_pdbg` — SMC sudah ada; sisa pekerjaan RE
   terbatas pada format sysfs + pemetaan irq-domain.
2. `sprd-cpufreq-v2` lalu `sprd-cpufreq-public`.
3. `sprd_7sreset` (butuh peta bit regmap PMIC).
4. `sprd_manufacturer_model` (butuh asal string).
5. `sprd_iq` (RE selesai; butuh model state/mailbox yang pasti).
6. `mmc_swcq` (paling besar, butuh sumber).
7. `core.ko` + `gpio.ko` (autotest) — ABI ioctl sudah terbaca, tinggal
   `gpio_test`.
