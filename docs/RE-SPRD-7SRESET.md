# RE `sprd_7sreset.ko` — PMIC 7s reset

Sumber: `donors/stock_vendor_modules_all/sprd_7sreset.ko` (15 fungsi,
23.350 B). Disassembly penuh dengan relokasi:
[sprd_7sreset.objdump.txt](evidence-20261023/sprd_7sreset.objdump.txt).

Port: `drivers/vxtux/soc/sprd_7sreset/sprd_7sreset.c` (Kconfig
`SPRD_7SRESET`), node DT `hw-reset@1bc0` di bawah `&sc2730_pmic`.

## Struct

```
struct sprd_7sreset (32 B, devm_kmalloc)
  +0x00 struct device *dev
  +0x08 struct regmap *regmap      dev_get_regmap(dev->parent, NULL)
  +0x10 u32 reg                    DT "reg" index 0
  +0x14 u32 reg_ctrl               DT "reg" index 1
  +0x18 const struct sprd_7sreset_data *data

struct sprd_7sreset_data (16 B, .data)
  +0x00 u32 chip_id
  +0x04 u32 reserved
  +0x08 u32 unlock_reg             hanya UMP9620 = 0x23d8
  +0x0c u32 reserved2
```

Tabel chip (relokasi `sprd_7sreset_of_match` → data):

| compatible | chip_id | unlock_reg |
|---|---|---|
| `sprd,sc2720-7sreset` | 0x2720 | 0 |
| `sprd,sc2721-7sreset` | 0x2721 | 0 |
| `sprd,sc2730-7sreset` | 0x2730 | 0 |
| `sprd,sc2731-7sreset` | 0x2731 | 0 |
| `sprd,ump9620-7sreset` | 0x9620 | 0x23d8 |

## Atribut sysfs

Semua store memakai `kstrtoint(buf, 10, &v)` lalu
`regmap_update_bits(regmap, reg_ctrl, mask, bit)`:

| atribut | mask | nilai tulis | show |
|---|---|---|---|
| `module_disable` | 0x002 | `(v != 0) << 1` | `(val >> 1) & 1` |
| `hard_mode` | 0x001 | `v != 0` | `val & 1` |
| `short_mode` | 0x100 | `(v != 0) << 8` | `(val >> 8) & 1` |
| `keypad_mode` | 0x200 | `(!v) << 9` (aktif-rendah) | `~((val >> 9) & 1)` |
| `threshold_time` | 0x0f0 | selalu `0xf0` | `(val >> 4) & 0xf` |

`show` memakai `regmap_read(reg_ctrl)` untuk semua atribut **kecuali**
`keypad_mode`, yang memakai `reg` bila `chip_id != 0x2731` dan `reg_ctrl`
bila `chip_id == 0x2731`. Bila `regmap_read` gagal, nilai error itulah yang
dicetak (`scnprintf("%d\n", ret)`), bukan 0.

## Unlock UMP9620

Bila `unlock_reg != 0`:

1. `regmap_update_bits(map, unlock_reg, 0x3991, 0x7fff)`
2. `regmap_read(map, unlock_reg, &v)`; bila `!(v & BIT(15))` → `-EINVAL`
3. tulis field utama
4. `regmap_update_bits(map, unlock_reg, 0x3990, 0x7fff)`
5. `regmap_read`; bila `v & BIT(15)` → `-EINVAL`

## Node DT

Blob stok: `/soc/aon/spi@32100000/pmic@0/hw-reset@1bc0` dengan
`reg = <0x1bf8 0x1bc0>` — dua sel itu **bukan** range, melainkan dua alamat:
index 0 = `reg` (keypad pada non-SC2731), index 1 = `reg_ctrl` (0x1bc0).

## Catatan port

- `sysfs_create_groups()` di 6.18 menerima `struct kobject *` (blob 5.4
  menerima `struct device *`); port memakai `&dev->kobj`.
- `platform_driver.remove` di 6.18 mengembalikan `void`.
- `platform_set_drvdata()` dipasang sebelum grup sysfs dibuat (blob
  sebaliknya) supaya pembacaan atribut tidak melihat drvdata NULL.
