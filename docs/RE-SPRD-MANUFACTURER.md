# RE `sprd_manufacturer_model.ko` — model/manufacturer SoC

Sumber: `donors/stock_vendor_modules_all/sprd_manufacturer_model.ko`
(5 fungsi, `.text` 0xb0 + `.init.text` 0xc0). Disassembly penuh dengan
relokasi: [sprd_manufacturer_model.objdump.txt](evidence-20261023/sprd_manufacturer_model.objdump.txt).

Port: `drivers/vxtux/soc/manufacturer_model/sprd_manufacturer_model.c`
(Kconfig `SPRD_MANUFACTURER_MODEL`).

## Fakta dari blob

| item | nilai |
|---|---|
| path DT | `/` (root) — `of_find_node_opts_by_path("/", NULL)` |
| properti 1 | `soc-module` → `sprd_soc_model` (`.bss+0x00`) |
| properti 2 | `soc-manufacturer` → `sprd_soc_manufacturer` (`.bss+0x08`) |
| device | `struct device sprd_manufacturer_model_dev` (`.bss+0x10`, 0x3a8 B) |
| sysfs | grup `default_attrs` (`.data+0x00`) → 2 atribut |
| format show | `.rodata+0x36` = `"soc_model: %s \n"` untuk **kedua** atribut |
| init print | `"Init manufacturer_model mudule successfully/failed"` (sic) |
| exit print | `"Exit manufacturer_model mudule..."` |

Nama atribut: `.rodata+0x8b` = `soc_model`, `.rodata+0x46` = `soc_manufacturer`
(keduanya `0444`, hanya show).

## Nilai di stok DTB

```
soc-module       = "UMS512"
soc-manufacturer = "Spreadtrum"
```

Diverifikasi dari `donors/stock_initramfs/stock.dtb` (root node). Karena itu
"blokir" pada PORT-REMAINING (node tidak ditemukan lewat
`fdt_dump.py --paths`) sebenarnya hanya soal path: driver membaca **root**,
bukan node bernama `soc-manufacturer`.

## Catatan port

- Device hanya `device_initialize()` lalu `sysfs_create_group()` — tidak ada
  `device_add()`, jadi grup sysfs tidak pernah terpasang dan blob mencetak
  `failed`. Perilaku itu dipertahankan; string tetap terbaca dan dipakai
  `show` bila atribut dibuka (mis. lewat pembuatan device sendiri di masa
  depan).
- Blob memberi tambahan `+0x18` pada pointer `[dev+0x20]` sebelum memanggil
  `sysfs_create_group`; dengan `struct device` 5.4 field itu bukan `kobj`,
  sehingga port memakai idiom baku `&dev->kobj`. Tidak ada efek yang bisa
  diamati pada keluaran karena device tidak pernah didaftarkan.
- Kedua atribut memakai format `"soc_model: %s \n"` (bug vendor, copy-paste).
  Dipertahankan 1:1 agar keluaran identik.
