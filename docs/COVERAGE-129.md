# Cakupan 129 blob stok — skor otoritatif

Dibuat [coverage_129.py](../donors/coverage_129.py). Dicek ulang terhadap config live ([config_tick15.txt](evidence-20261023/config_tick15.txt), 8.007 baris) dan daftar `.ko` hasil build, bukan snapshot dokumen lama.

## **119/129** tertangani

| Verdict | Jumlah | Arti |
|---|---:|---|
| `KO` | 57 | ada `.ko` penggantinya di build |
| `Y` | 48 | pemetaan Kconfig-nya `=y` -> ikut `Image` |
| `BUILT-IN` | 12 | verdict triage, kode ikut Image |
| `SUPERSEDED` | 2 | fungsinya sudah ada di modul lain |
| **BELUM** | **10** | lihat tabel |
| total | **129** | |

## Yang masih BELUM

| # | blob | alasan | simbol port | simbol hilang |
|---:|---|---|---:|---:|
| 1 | `gpio.ko` | config tidak nyala (CONFIG_MTD_NAND_GPIO=absent) | 2 | 0 |
| 2 | `mmc_swcq.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |
| 3 | `sprd-cpufreq-public.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |
| 4 | `sprd-cpufreq-v2.ko` | PERLU-PORT: sumber tidak ada di pohon | 1 | 1 |
| 5 | `sprd_7sreset.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |
| 6 | `sprd_iq.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |
| 7 | `sprd_manufacturer_model.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |
| 8 | `sprd_pdbg.ko` | PERLU-PORT: sumber tidak ada di pohon | 1 | 1 |
| 9 | `sprd_power_stat.ko` | PERLU-PORT: sumber tidak ada di pohon | 1 | 0 |
| 10 | `sprd_sip_svc.ko` | PERLU-PORT: sumber tidak ada di pohon | 0 | 0 |

## Blokir simbol per blob (dari symreach)

Angka `port`/`hilang` di atas adalah referensi undefined yang belum ter-resolve build kita. Blob yang sudah tertangani lewat `KO`/`Y` tidak perlu ini lagi — kodenya di-port ulang memakai API 6.18, bukan blob lama.

| blob | kelas | port | hilang |
|---|---|---:|---:|
| `gpio.ko` | BELUM | 2 | 0 |
| `sprd-cpufreq-v2.ko` | BELUM | 1 | 1 |
| `sprd_pdbg.ko` | BELUM | 1 | 1 |
| `sprd_power_stat.ko` | BELUM | 1 | 0 |

## Catatan jujur

- `KO` = ada `.ko` dengan nama itu di build. Untuk nama yang berbeda (mis. `phy-sprd-sharkl5Pro.ko` -> `phy-sprd-ums512.ko`) dipakai `RENAMES`.
- Pemetaan nama -> Kconfig di `PORT-AUDIT.md` punya kesalahan pasang (mis. `gpio.ko` dipetakan ke `CONFIG_MTD_NAND_GPIO`, padahal blob itu `sprd autotest gpio driver`). Kasus ini terbaca sebagai "pemetaan tidak ditemukan" sehingga tidak dihitung sebagai selesai.
- `symreach.json` dipakai untuk kolom port/hilang saja. Penutup simbol TIDAK menggantikan keberadaan kode.
