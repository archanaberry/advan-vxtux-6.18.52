# 10 blob — status port dan buktinya

**Status 2026-10-24: 10 dari 10 sudah diport.** Tiga terakhir (`mmc_swcq`,
`sprd-cpufreq-v2`, `sprd-cpufreq-public`) masuk bersama `sprd_iq`; kesepuluh
driver belum melewati build ulang sejak codespace berhenti bisa start
(HTTP 402), jadi angka `235 .ko` di bawah masih dari build CI 2026-10-23,
sebelum `sprd_iq` dan tiga driver terakhir.

| # | blob | status |
|---|---|---|
| 1,2 | `core.ko`, `gpio.ko` | **selesai** — `sprd_autotest_core`, `sprd_autotest_gpio` |
| 6 | `sprd_7sreset.ko` | **selesai** — 5 atribut sysfs + node DT |
| 8 | `sprd_manufacturer_model.ko` | **selesai** — path root `/`, bukan node `soc-manufacturer` |
| 9 | `sprd_pdbg.ko` | **selesai** (commit `cb5e91d35`) |
| 10 | `sprd_power_stat.ko` | **selesai** sebagai `sprd_debugstat` (commit `8bf97026b`) |
| 3 | `mmc_swcq.ko` | **selesai** — port dari sumber GPL vendor, [RE-MMC-SWCQ.md](RE-MMC-SWCQ.md) |
| 4 | `sprd-cpufreq-v2.ko` | **selesai** — `drivers/cpufreq/sprd-cpufreq-v2-*` |
| 5 | `sprd-cpufreq-public.ko` | **selesai** — `drivers/cpufreq/sprd-cpufreq-public.c` |
| 7 | `sprd_iq.ko` | **selesai** — `sprd_iq`, layout state dipulihkan dari relokasi |

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

**SELESAI.** Sumber GPL vendor ada di
[donors/gpl/mmc_swcq.c](../donors/gpl/mmc_swcq.c) (2.413 baris, + header),
dan itu bukan 5.4 mentah: `mmc_cqe_ops` sudah memakai nama 6.x, jadi
port-nya tipis. Hasil: `drivers/mmc/host/mmc_swcq.c` + `mmc_swcq.h`,
`CONFIG_MMC_SWCQ` (`drivers/mmc/host/Kconfig` + Makefile), dipilih saat
probe oleh `mmc_hsq_swcq_init()`.

Wiring didekode dari `sdhci-sprd.ko` (`mmc_hsq_swcq_init` @ 0x4af0): eMMC =
`(caps2 & (MMC_CAP2_NO_SDIO|MMC_CAP2_NO_SD)) == keduanya`. Probe
`sdhci_sprd_probe()` menyetel kedua bit itu untuk controller
`non-removable`, dan node eMMC `sdio3` di `ums512.dtsi` kini `non-removable`.
`sdhci_sprd_request_done()` memakai `mmc_swcq_finalize_request()` bila
`mmc->cqe_private` terpasang, runtime PM memakai `mmc_swcq_suspend()/resume()`.

Delta 6.18 yang diterapkan (rincian + bukti di
[RE-MMC-SWCQ.md](RE-MMC-SWCQ.md)): `mq->use_cqe` → `host->cqe_enabled`,
`from_timer()` → `timer_container_of()`, stash `android_kabi_reserved1/2`
dihapus, `proc_create()` → `struct proc_ops`, bit `MMC_CQE_QUEUE_FULL`
dihapus, `host->cqe_qdepth` diisi di `mmc_swcq_enable()` (tanpa itu tag set
6.18 jadi 0), include privat `mmc_sdhci_sprd.h` dibuang, dan definisi privat
`struct mmc_blk_data` disamakan dengan layout 6.18 (`kref`).

Verifikasi: `mmc_swcq.o`, `sdhci-sprd-ext.o`, `sdhci_sprd_main.o`, ketiga
objek cpufreq, dan DTB `ums512-1h10-vxtux.dtb` semuanya hijau di CI
`quick-cc` (ref `test1`). Link/modpost penuh (`v17-build`) dan boot QEMU
belum.

## 4. `sprd-cpufreq-v2.ko` — cpufreq DVFS

| Item | Nilai |
|---|---|
| ukuran / fungsi | 56.030 B / 23 |
| depends | `sprd_sip_svc` (kini tersedia) |
| penanda DT | `sprd,cpufreq-v2`, `sprd,cpufreq-boost`, `sprd,voltage-margin`, `sprd,multi-version`, `sprd,transition-delay`, `sprd,multi-supply`, `sprd,voltage-step`, `sprd,temp-threshold`, `sprd,cluster-cpumask`, `sprd,pmic-type` |
| node stok | `/soc/aon/apcpu-dvfs` (`compatible = "sprd,cpufreq-v2"`) |

**SELESAI.** Port memakai sumber GPL vendor
([donors/gpl/sprd-cpufreq-v2-driver.c](../donors/gpl/sprd-cpufreq-v2-driver.c)
dan `sprd-cpufreq-common.h`) dengan sambungan firmware yang sudah terbuka:
`sprd_sip_svc` mengekspor `dvfs_*` (ID `0x82000601`..`0x8200060c`, lihat
[RE-SPRD-SIP-SVC.md](RE-SPRD-SIP-SVC.md)) dan `ARM_SPRD_CPUFREQ_V2`
`select SPRD_SIP_SVC`.

Hasil di `drivers/cpufreq/`: `sprd-cpufreq-v2-driver.c`,
`sprd-cpufreq-v2-debug.c`, `sprd-cpufreq-v2.h`, `sprd-cpufreq-common.h`,
`CONFIG_ARM_SPRD_CPUFREQ_V2=m`.

Tidak ikut diport (tidak dipakai jalur DVFS yang diport, tidak ada konsumen
in-tree): `sprd-cpufreq-common.c` (binning `dev_pm_opp_of_add_table_binning`,
cpuhp online/offline) dan `sprd-cpufreqsw.c` (gpu volt lock + slave notify,
modul terpisah di vendor). Bila GPU/devfreq kelak butuh, keduanya tinggal
ditambah sebagai file terpisah.

## 5. `sprd-cpufreq-public.ko` — API cpufreq untuk modul lain

**SELESAI.** `drivers/cpufreq/sprd-cpufreq-public.c`:
`sprd_cpufreq_update_normal()` (layout `cluster_info`, node
`cpufreq-cluster0`), `sprd_cpufreq_update_common()` (layout
`sprd_cpufreq_driver_data`, node `cpufreq-clus0`), dan
`sprd_cpufreq_update_opp()` dengan `EXPORT_SYMBOL_GPL`.

Satu delta API: blob memakai `cpufreq_get_policy(&policy, 0)` (4.14), yang
sudah tidak ada; port memakai `cpufreq_cpu_get(0)` + `cpufreq_cpu_put()` di
semua jalur keluar. `CONFIG_ARM_SPRD_CPUFREQ_PUBLIC=m` bergantung pada #4.

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

## 7. `sprd_iq.ko` — camera IQ — **SELESAI (diport)**

15 fungsi, 32 KB, `depends = usb_f_vser,sipc-core`; node `/sprd-iq` dengan
properti `sprd,region` (phandle) dan opsional `sprd,mapping-offs`.

Premis lama ("butuh `usb_f_vser` yang tidak ada di tree") **salah**:
`drivers/usb/gadget/function/f_vserial.c` sudah ada sebagai `usb_f_vser.ko`
dan mengekspor `kernel_vser_register_callback`, `kernel_vser_set_pass_mode`,
`vser_pass_user_write`; API SIPC `shmem_ram_vmap_nocache_ex`/`_unmap_ex` juga
tersedia.

Hambatan kedua ("layout state `iq` dan arah mailbox masih gelap") juga
**hilang**: relokasi `adrp`/`add`/`ldst` di
`.rela.text.unlikely.__cfi_check_fail` memberi offset absolut tiap akses
`.bss`, sehingga seluruh field state dan transisi magic-nya terbaca pasti
(`base` +0x08, `size` +0x10, `offs` +0x18, `va` +0x20, `chan` +0x30,
`thread` +0x38, `wait` +0x40). Rincian, tabel ioctl, handshake playback, dan
daftar bug blob yang tidak ditiru: [RE-SPRD-IQ.md](RE-SPRD-IQ.md); disassembly
terselesaikan: [evidence-20261023/sprd_iq.resolved.txt](evidence-20261023/sprd_iq.resolved.txt).

Port ada di `drivers/vxtux/soc/iq/` (Kconfig `SPRD_IQ`, `depends on
SPRD_SIPC && USB_F_VSERIAL`), `CONFIG_SPRD_IQ=m` di `enhanced_config.cfg`.
Hanya hidup dengan `androidboot.mode=iq`, jadi inert di perangkat normal.
Belum dikompilasi: codespace CI tidak bisa start (HTTP 402 billing).

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
5. `sprd_iq` — selesai, menunggu kompilasi ulang di lingkungan build.
6. `mmc_swcq` (paling besar, butuh sumber).
7. `core.ko` + `gpio.ko` (autotest) — ABI ioctl sudah terbaca, tinggal
   `gpio_test`.
