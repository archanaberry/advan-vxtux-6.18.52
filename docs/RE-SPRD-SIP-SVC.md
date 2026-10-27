# RE `sprd_sip_svc.ko` — ID SMC per pembungkus

Dihasilkan `donors/sip_svc_ids.py` dari disassembly blob 5.4.210. Blob ini hanya mengekspor `sprd_sip_svc_get_handle()`;
`init_module` memanggil SMC `id` pertama lalu menyusun struct
fungsi, dan tiap fungsi di struct itu membungkus satu SMC.

| fungsi | ID SMC | argumen x1..x6 |
|---|---|---|
| `init_module` | `0x82000100` | 6 |
| `sprd_sip_svc_perf_set_freq` | `0x82000101` | 5 |
| `sprd_sip_svc_perf_get_freq` | `0x82000102` | 5 |
| `sprd_sip_svc_perf_set_div` | `0x82000103` | 5 |
| `sprd_sip_svc_perf_get_div` | `0x82000104` | 5 |
| `sprd_sip_svc_perf_set_parent` | `0x82000105` | 5 |
| `sprd_sip_svc_perf_get_parent` | `0x82000106` | 5 |
| `init_module` | `0x82000200` | 6 |
| `init_module` | `0x82000300` | 6 |
| `sprd_sip_svc_storage_ufs_crypto_enable` | `0x82000301` | 6 |
| `sprd_sip_svc_storage_ufs_crypto_disable` | `0x82000302` | 6 |
| `init_module` | `0x82000500` | 6 |
| `sprd_sip_svc_pwr_get_wakeup_source` | `0x82000501` | 6 |
| `sprd_sip_svc_pwr_get_pdbg_info` | `0x82000502` | 5 |
| `init_module` | `0x82000600` | 6 |
| `sprd_sip_svc_dvfs_enable` | `0x82000601` | 5 |
| `sprd_sip_svc_dvfs_table_update` | `0x82000602` | 5 |
| `sprd_sip_svc_dvfs_step_set` | `0x82000603` | 5 |
| `sprd_sip_svc_dvfs_margin_set` | `0x82000604` | 5 |
| `sprd_sip_svc_dvfs_freq_set` | `0x82000605` | 5 |
| `sprd_sip_svc_dvfs_freq_get` | `0x82000606` | 5 |
| `sprd_sip_svc_dvfs_pair_get` | `0x82000607` | 5 |
| `sprd_sip_svc_dvfs_pmic_set` | `0x82000608` | 5 |
| `sprd_sip_svc_dvfs_bin_set` | `0x82000609` | 5 |
| `sprd_sip_svc_dvfs_version_set` | `0x8200060a` | 5 |
| `sprd_sip_svc_dvfs_init` | `0x8200060b` | 5 |
| `sprd_sip_svc_dvfs_debug_init` | `0x8200060c` | 6 |
| `init_module` | `0x82000700` | 6 |
| `sprd_sip_svc_npu_set_freq` | `0x82000701` | 5 |
| `sprd_sip_svc_npu_disable_idle` | `0x82000702` | 6 |
| `sprd_sip_svc_npu_get_max_state` | `0x82000703` | 6 |
| `sprd_sip_svc_npu_get_opp` | `0x82000704` | 5 |
| `sprd_sip_svc_npu_set_volts` | `0x82000705` | 5 |
| `sprd_sip_svc_dbg_set_hang_hdl` | `0xc2000201` | 6 |
| `sprd_sip_svc_dbg_get_hang_ctx` | `0xc2000202` | 6 |

## Catatan

- `init_module` memanggil SMC `0x82000100` lebih dulu (ID SIP *fast call*, owner 0x02) dan mencatat hasilnya:
  `func_addr = 0x%lx, pdg = 0x%lx, level = 0x%lx`.
- `sprd_sip_svc_get_handle` cuma `adrp/add` ke objek `.bss` `sprd_sip_svc_handle` lalu `ret` — tidak ada alokasi, jadi
  handle harus sudah terisi saat `init_module` selesai.
- Versi tiap layanan dicetak dari string firmware:
  `SPRD SIP SVC {STORAGE,NPU,DBG,PERF,DVFS}:v%d.%d detected in firmware.`

## Peta tulis ke handle (dari `init_module`)

`sprd_sip_svc_get_handle` mengembalikan alamat `.bss` `sprd_sip_svc_handle`. `init_module` mengisinya dengan pola yang berulang per layanan: sepasang versi (`stp wN, wN`) lalu pointer fungsi. Offset yang terlihat di disassembly:

| offset | isi |
|---:|---|
| 0 | versi layanan pertama (major, minor) |
| 8 | dua pointer fungsi |
| 24 | dua pointer fungsi |
| 40 | dua pointer (basis string + 4) |
| 56 | versi layanan kedua |
| 64 | dua pointer fungsi |
| 80 | versi layanan ketiga |
| 88 | dua pointer fungsi |
| 208 | versi layanan keempat |
| 216 | dua pointer fungsi |
| 232 | dua pointer fungsi |
| 248 | satu pointer fungsi |

Urutan probe versi di `init_module` mengikuti angka layanan:
`0x82000100` (handle) → `0x82000200` (PERF) → `0x82000300`
(STORAGE) → `0x82000500` (PWR) → `0x82000600` (DVFS) →
`0x82000700` (NPU) → `0xc2000200` (DBG).

Konsekuensi untuk port: tabel ini adalah daftar ABI yang harus
direproduksi. Pengganti tidak boleh mengarang ID sendiri — kalau
satu ID salah, firmware akan menolak dan `get_handle` mengembalikan
struct berisi fungsi yang tidak pernah berhasil.
