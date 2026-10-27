# RE `sprd_power_stat.ko` — ABI dan layout yang dipulihkan dari blob

Sumber: `donors/stock_vendor_modules_all/sprd_power_stat.ko` (42.670 B,
30 fungsi, vermagic `5.4.210`, Clang). Semua angka di bawah diambil dari
`nm` + `objdump` blob, bukan ditebak. Belum ditulis ulang jadi kode.

## Metadata

| item | nilai |
|---|---|
| MODULE_DESCRIPTION | `sprd debug stat driver` |
| depends | `sprd_sip_svc` |
| EXPORT (2) | `stat_info_register`, `stat_info_unregister` |
| DT compat | `sprd,debugstat` (node stok `/debugstat`) |
| properti DT | `soc_slp_cnt_reg = <0x2 0x784 0xff>` |

## Tabel fungsi

| alamat | nama |
|---|---|
| `0x0` | `init_module` — cuma `platform_driver_register(&driver)` |
| `0x30` | `stat_info_unregister` |
| `0x130` | `stat_info_register` |
| `0x290` | `sprd_debugstat_core_init` |
| `0x35c` | `slp_cnt_proc_open` |
| `0x388` | `slp_cnt_read` |
| `0x484` | `stat_proc_open` |
| `0x4b4` | `stat_read` |
| `0xcb8`.. | `stat_print_name`, `stat_print_total`, `stat_print_idle`, `stat_print_state`, `stat_print_sleep`, `stat_print_last_wakeup`, `stat_print_last_sleep`, `stat_print_reboot`, `stat_print_irq_count`, `stat_print_irq_to_ap`, `stat_print_wakeup_reason`, `stat_print_active_core` |
| `0x2240` | `sprd_debugstat_driver_probe` |
| `0x2484` | `sprd_debugstat_driver_remove` |
| `0x24c4` | `sprd_debugstat_suspend` |
| `0x251c` | `sprd_debugstat_resume` |
| `0x2590` | `ap_sleep_info_get` |
| `0x2694` | `soc_sleep_info_get` |

## API ekspor — `stat_info_register(const char *name, void *a, void *b)`

Dari `objdump` (`0x130`):

- `x0 = name`, `x1` disimpan di `entry+0x10`, `x2` di `entry+0x18`;
- duplikat dicek dengan `strcmp` terhadap `entry+0x00` — kalau sudah ada,
  cetak `The node(%s) is exsit` dan gagal;
- `entry` di-`kmalloc(0x30)`;
- `entry+0x08` di-`kmalloc(0x58)` — 88 byte data per node;
- `entry+0x20` adalah `struct list_head`, dimasukkan ke list global di
  offset `+0x20` dari objek driver;
- semua akses dilindungi mutex di `+0x440` dari objek driver.

`stat_info_unregister(const char *name)` (`0x30`): `strcmp`, `list_del`,
`kfree(entry+0x08)`, poison list, `kfree(entry)`. Tidak ketemu →
`The node(%s) is not found`, tetap `0`.

Jadi layout yang harus direproduksi:

```c
struct stat_info {          /* 0x30 byte */
	const char *name;   /* +0x00 */
	void *data;         /* +0x08  0x58 byte milik pemilik */
	void *arg1;         /* +0x10 */
	void *arg2;         /* +0x18 */
	struct list_head list;  /* +0x20 */
};
```

## `stat_print_*` — 12 printer, satu pola

Semua fungsi `0xcb8`..`0x2738` punya bentuk yang sama:

1. buffer 128 byte di stack (semua `stp xzr,xzr` di `sp+8..sp+120`);
2. `sprintf` pembuka dengan format `"%22s:"` atau `"%10s"`;
3. telusuri list yang sama, per node ambil `entry+0x08` (`ldur x2, [x24, #-24]`);
4. `sprintf` per node dengan format `"%10s"` atau `"%10u"`;
5. hasil buffer di-`seq_printf` ke `struct seq_file *` (argumen `x0`).

Tipe callback terkonfirmasi dari CFI type info blob:
`__typeid__ZTSFiP8seq_fileE` = `int (*)(struct seq_file *)`.

Yang **belum** dipulihkan per fungsi: offset persis di dalam blob 0x58
yang dicetak tiap printer, dan format mana dipakai tiap entry.

## Struktur privat driver (dari `*_sleep_info_get`)

Objek driver memuat sub-struktur pada `priv + 0x98`:

| offset | isi |
|---|---|
| `+0x10` | pointer `struct` yang memuat timestamp (lihat `ap_sleep_info_get`) |
| `+0x18` | pointer `struct subsys_sleep_info` |
| `+0x20` | `struct regmap *` |
| `+0x28` | `u32` offset register `soc_slp_cnt_reg` |
| `+0x2c` | `u32` mask `soc_slp_cnt_reg` |

### `soc_sleep_info_get` (`0x2694`)

```
regmap_read(regmap, offset, &val)
info->+64 = regmap_read_error ? -1 : (val & mask)
return info->+18
```

Dengan node stok `soc_slp_cnt_reg = <0x2 0x784 0xff>`: offset `0x784`,
mask `0xff`, dan `0x2` kemungkinan index parent/reg.

### `ap_sleep_info_get` (`0x2590`)

```
info = *(priv + 0x10)
info->+8  = ktime_get_boot_fast_ns() / 1e6   (magic reciprocal 0x44b82fa0a09b5a53, lsr 11)
info->+52 = number of online CPUs            (__cpu_online_mask)
(*handle->pwr)(6, 0, &val, 0, 0, 0)          (CFI slowpath, lalu blr)
info->+40 = (val >> 11 & 0x1fffe0) + (val & 0xffff)
return info
```

Panggilan ke-itu memakai fungsi pada offset `+0x10` dari objek di
`priv + 0x08`, dengan enam argumen dan `cmd = 6`. Dengan pemetaan
[RE-SPRD-SIP-SVC.md](RE-SPRD-SIP-SVC.md), `cmd` di `x0` untuk layanan PWR
adalah `sprd_sip_svc_pwr_get_wakeup_source` (`0x82000501`) — makanya
blob ini `depends = sprd_sip_svc` dan kini klien SMC-nya sudah ada.

## Node DT

```dts
debugstat {
	compatible = "sprd,debugstat";
	soc_slp_cnt_reg = <0x2 0x784 0xff>;
};
```

Belum ada di DTS vxtux; harus ikut di-port.

## Antarmuka proc yang dibuat

Direktori `sprd-debugstat` (string `sprd-debugstat` ada di `.rodata`),
dengan entri yang muncul dari string blob:

`current_status`, `slp_cnt`, `subsystem_reboot_count`, `active_core`,
`soc_sys`, `ap_sys`, `last_wakeup_duration`, `last_sleep_duration`,
`total_duration`, `idle_duration_total`, `sleep_duration_total`,
`wakeup_reason`, `internal_irq_count`, `subsystem_name`, `stat`.

Pesan error yang harus direproduksi (semua diawali level):
`Get debug device error`, `dbgstat_data alloc error`, `Node allock error`,
`Sleep info alloc error`, `The node(%s) is exsit`, `Proc file slp_cnt
create failed`, `Register soc sleep info get error`, `Parameter is error`,
`The node(%s) is not found`, `Get wakeup sip error`, `Get %s error`,
`sprd_debugstat_apsys_init error`, `sprd_debugstat_core_init error`,
`Proc file create failed`, `Proc dir create failed`,
`Register ap sleep info get error`.

## Kolom `/proc/sprd-debugstat/stat` (lengkap)

Diambil dari pasangan `sprintf` di tiap `stat_print_*` (`donors/blobre.py`
meresolusi string `.rodata` yang dirujuk `adrp/add`), lalu dicocokkan dengan
`ldr/ldur` yang membaca blob 0x58 milik tiap node.

| offset | field | printer | format |
|---:|---|---|---|
| `+0x00` | `char name[4]` | `stat_print_name` | `%10s` |
| `+0x08` | `total_duration` (u64) | `stat_print_total` | `%10llu` |
| `+0x10` | `sleep_duration_total` (u64) | `stat_print_sleep` | `%10llu` |
| `+0x18` | `idle_duration_total` (u64) | `stat_print_idle` | `%10llu` |
| `+0x20` | `state` | `stat_print_state` | `%10u` |
| `+0x24` | `subsystem_reboot_count` | `stat_print_reboot` | `%10u` |
| `+0x28` | `wakeup_reason` | `stat_print_wakeup_reason` | `%10u` |
| `+0x2c` | `last_wakeup_duration` | `stat_print_last_wakeup` | `%10u` |
| `+0x30` | `last_sleep_duration` | `stat_print_last_sleep` | `%10u` |
| `+0x34` | `active_core` | `stat_print_active_core` | `%#10x` |
| `+0x38` | `internal_irq_count` | `stat_print_irq_count` | `%10u` |
| `+0x3c` | `irq_to_ap_count` | `stat_print_irq_to_ap` | `%10u` |
| `+0x40` | `soc_slp_cnt` | `slp_cnt_read` | `%d\n` |

Header berkas: `"Subsys information:\n"`, tiap baris `"%22s:"` + nilai-nilai
node + `"\n"`.

## Pemanggil `stat_info_register` (dari `probe`)

| argumen | nilai |
|---|---|
| `name` | `"ap_sys"` / `"soc_sys"` |
| `x1` (getter) | `ap_sleep_info_get` / `soc_sleep_info_get` |
| `x2` (arg) | `&pdev->dev` (getter memanggil `dev_get_drvdata`) |

Blob mengisi ketiga field itu di `struct stat_info` (+0x00/+0x10/+0x18) dan
menyimpan `struct subsys_sleep_info` 0x58 byte di `+0x08`.

## `sprd_debugstat_core_init` (0x290)

```
proc_mkdir("sprd-debugstat", NULL)
proc_create("stat",    0444, dir, &stat_fops)     /* open -> single_open(stat_read) */
proc_create("slp_cnt", 0644, dir, &slp_cnt_fops)  /* open -> single_open(slp_cnt_read) */
```

Gagal → `Proc dir create failed` / `Proc file create failed` /
`Proc file slp_cnt create failed`, lalu `remove_proc_entry("sprd-debugstat")`
dan `-EINVAL`.

## Sudah diport

Hasil di atas dipakai langsung oleh
[drivers/vxtux/soc/debugstat/sprd_debugstat.c](../6.18.52-VXTux-UMS512/drivers/vxtux/soc/debugstat/sprd_debugstat.c)
(`CONFIG_SPRD_DEBUGSTAT=m`), termasuk node `/debugstat` di
`ums512-1h10-vxtux.dts`. Kompilasi diverifikasi lewat workflow `quick-cc`.

Satu perbedaan yang disengaja: blob mengakses handle SMC lewat offset tetap
(`handle+0x50` lalu `+0x10`), sedangkan port memakai field bernama
`handle->pwr_get_wakeup_source` dari
[include/linux/sprd_sip_svc.h](../6.18.52-VXTux-UMS512/include/linux/sprd_sip_svc.h).
SMC ID dan urutan argumennya sama (`0x82000501`).
