# Cakupan 129 blob stok — skor otoritatif

Dibuat [coverage_129.py](../donors/coverage_129.py). Dicek ulang terhadap config live ([config_v17.txt](evidence-20261024/config_v17.txt), 8,010 baris) dan daftar `.ko` hasil build (ko_list_v17.txt), bukan snapshot dokumen lama.

## **129/129** tertangani

| Verdict | Jumlah | Arti |
|---|---:|---|
| `KO` | 68 | ada `.ko` penggantinya di build |
| `Y` | 47 | pemetaan Kconfig-nya `=y` -> ikut `Image` |
| `BUILT-IN` | 12 | verdict triage, kode ikut Image |
| `SUPERSEDED` | 2 | fungsinya sudah ada di modul lain |
| **BELUM** | **0** | lihat tabel |
| total | **129** | |

## Yang masih BELUM

Tidak ada — seluruh 129 blob sudah tertangani. Sepuluh entri terakhir
(`core`, `gpio`, `mmc_swcq`, `sprd-cpufreq-public/v2`, `sprd_7sreset`,
`sprd_iq`, `sprd_manufacturer_model`, `sprd_pdbg`, `sprd_power_stat`)
ditutup oleh port 2026-10-23→24; bukti build: CI `v17-build` hijau
(run `40097613453`) + [PORT-REMAINING.md](PORT-REMAINING.md).

## Blokir simbol per blob (dari symreach)

Angka `port`/`hilang` di atas adalah referensi undefined yang belum ter-resolve build kita. Blob yang sudah tertangani lewat `KO`/`Y` tidak perlu ini lagi — kodenya di-port ulang memakai API 6.18, bukan blob lama.

| blob | kelas | port | hilang |
|---|---|---:|---:|

## Catatan jujur

- `KO` = ada `.ko` dengan nama itu di build. Untuk nama yang berbeda (mis. `phy-sprd-sharkl5Pro.ko` -> `phy-sprd-ums512.ko`) dipakai `RENAMES`.
- Pemetaan nama -> Kconfig di `PORT-AUDIT.md` punya kesalahan pasang (mis. `gpio.ko` dulu dipetakan ke `CONFIG_MTD_NAND_GPIO`, padahal blob itu `sprd autotest gpio driver`). Sejak `UNRESOLVED`/`AMBIGUOUS` di `port_audit.py`, kasus seperti ini tampil sebagai belum, bukan selesai.
- `symreach.json` dipakai untuk kolom port/hilang saja. Penutup simbol TIDAK menggantikan keberadaan kode.
