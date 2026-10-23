# RE `sprd_pdbg.ko` — wakeup/sleep logging yang dipulihkan dari blob

Sumber: `donors/stock_vendor_modules_all/sprd_pdbg.ko` (40.854 B, 16
fungsi, vermagic `5.4.210`). Pembacaan memakai `donors/blobre.py`
(capstone + pyelftools, relokasi diterapkan). Disassembly penuh:
[pdbg_disasm.txt](evidence-20261023/pdbg_disasm.txt) (fungsi non-notifier)
dan [pdbg_notif.txt](evidence-20261023/pdbg_notif.txt) (probe, release,
kedua notifier). Semua angka di bawah dari blob — tanpa tebakan.

Port: `drivers/vxtux/soc/pdbg/sprd_pdbg.c` (Kconfig `SPRD_PDBG`,
compatible `sprd,debuglog`, node stok `/debuglog`).

## Metadata

| item | nilai |
|---|---|
| depends | `sprd_sip_svc` |
| EXPORT (1) | `sprd_pdbg_msg_print` |
| module_param | `sprd_pdbg_log_force` (bool, simbol `__param_sprd_pdbg_log_force`) |
| DT compat | `sprd,debuglog` |
| properti DT | 8 × `sprd,pdbg-irq-domain-*` (`gic`, `gpio`, `ana`, `ana-eic`, `ap-eic-sync`, +3) |
| SMC | `pwr_get_pdbg_info` = `0x82000502` (via `sprd_sip_svc`) |

## Layout `struct sprd_pdbg` (blob: `kzalloc` 0x538)

| ofset | isi | bukti disasm |
|---|---|---|
| `+0x00` | `task_struct *task` | `kthread_create_on_node`, `wake_up_process` |
| `+0x08` | `struct device *dev` | disimpan dari probe |
| `+0x10` | handle `sip_svc` | `sprd_sip_svc_get_handle` |
| `+0x18` | `pm_notifier` (0x18 byte) | `register_pm_notifier` |
| `+0x30` | `cpu_pm_notifier` | `cpu_pm_register_notifier` |
| `+0x48` | `list_head ws_list` | `__list_add_valid` di `+0x50` |
| `+0x58` | `delayed_work rescan` | `queue_delayed_work_on(system_wq, …, +0x58)` |
| `+0xe0` | `rwlock ws_lock` | `_raw_read_lock/+write_lock (x0+0xe0)` |
| `+0xf8` | `u64 regs[7][4]` | 4×`u64` per baris hasil SMC |
| `+0x148` | `const char *ws_names[8]` | 8 alamat berturut (0x148..0x180) |
| `+0x188` | `u32 poll_ms` (30) | default 30 |
| `+0x1b0` | `bool enabled` | gate `sprd_pdbg_msg_print` |
| `+0x1b2` | `char buf[0x380]` | buffer cetak |

Node `ws_list`: `+0x00` id domain (0..7), `+0x08` pointer
`irq_domain` (blob: `ldr x24,[desc,#0x50]` ⇒ `desc->domain` pada 5.4),
`+0x10` `list_head`.

## Tabel fungsi (addr dari blob)

| addr | nama | peran |
|---|---|---|
| `0x30` | `sprd_pdbg_msg_print` | EXPORT; `va_format`, prefiks `[SPRD_PDBG]`; gate `enabled` |
| `0x120` | `sprd_pdbg_probe` | isi struct, proc, kthread, notifier, `enabled=true` |
| `0x458` | `sprd_pdbg_release` | devm action: batal work, hapus proc, stop kthread, unregister |
| `0x568` | `sprd_pdbg_cpu_pm_notifier` | DEEP SLEEP START / WAKEUP SCENE END + SMC pdbg info |
| `0xaa4` | `sprd_pdbg_parse_irq_domain` | loop IRQ 0..511 (lihat bawah) |
| `0x930` | `sprd_pdbg_pm_notifier` | durasi suspend: `PM_SUSPEND_PREPARE`/`PM_POST_SUSPEND` |
| — | `regs_get_once`, `regs_get`, `regs_info` | baca + cetak info tidur |
| — | `sprd_pdbg_thread` | kthread periodik `sprd-pdbg-thread` |

## Mekanisme yang dulu jadi "blokir" — kini terpetakan

**Pemetaan irq-domain.** `sprd_pdbg_parse_irq_domain` (0xaa4): loop
`irq = 0..0x200`, `irq_to_desc(irq)`, lalu `strcmp` nama
`irq_desc->domain->name` terhadap 8 string DT (`ws_names[0..7]`, alamat
+0x148..+0x180). Bila cocok: buat node `{id=i, domain, list}`, tambah ke
`ws_list` dengan `write_lock` di `+0xe0`, lalu **string DT di-null-kan**
(`str xzr,[x22]`) supaya tidak dicocokkan lagi. Hash kecil: index 0..7
dipakai sebagai id (`w26`, dari 0 s/d 7 tergantung slot mana yang cocok).
Setelah satu siklus penuh (0x200 IRQ), bila masih ada slot terisi,
`queue_delayed_work_on(system_wq, +0x58, delay 0x1f4)` — rescan
berikutnya dengan penghitung `+0x10` yang menaik dan dibatasi 10.

Catatan 6.18: `desc->domain` kini `desc->irq_data.domain`.

**Format keluaran.** `/proc/sprd-pdbg/regs_info` memanggil
`regs_get_once` → SMC `0x82000502` mengembalikan **4 kata** (0..3, bukan
1). Kata ke-0/1 adalah mask per-word (bit terpakai), bit dihitung per
`u16`; baris dicetak sebagai `value, mask` per word. Varian wrapper
4-kata `sprd_sip_svc_pwr_get_pdbg_info4()` ditambahkan ke
`sprd_sip_svc` (yang lama `..._pwr_get_pdbg_info` hanya memakai
`a0/a1` — tetap dipertahankan, tidak memutus pemakai lain).

**Durasi suspend.** `PM_SUSPEND_PREPARE` mencatat `ktime_get()`;
`PM_POST_SUSPEND` mencetak selisihnya — bila belum pernah masuk suspend,
cetak `The system has not yet entered sleep mode`.

**Cluster low-power.** `cpu_pm_notifier` mencetak `DEEP SLEEP START`
 saat cluster masuk dan `WAKEUP SCENE END` saat keluar, lalu mengambil
info tidur dari firmware (SMC) untuk dicetak.

**module_param `sprd_pdbg_log_force`.** Bila `true`, pesan dicetak
dengan `KERN_ERR` alih-alih `KERN_INFO` (di blob: prefiks level `"3"`
vs `"6"` pada `printk`).

## Keputusan port / perbedaan dari blob

- Keduanya notifier kini benar-benar di-set `.notifier_call` saat probe
  (pada blob diinisialisasi bersama struct; jalur eksplisit dipilih di
  sini).
- `PDE_DATA` → `pde_data()` (5.6+).
- Pengganti `pm_get_active_wakeup_sources()` (5.4) ditulis lokal —
  API itu tidak ada lagi di 6.18.

## Verifikasi

- `sprd_pdbg.o`: rc=0 lewat `donors/cs_quick.sh` (loop codespace, ~2 s).
- DTB `sprd/ums512-1h10-vxtux.dtb`: rc=0. Warning yang tersisa berasal
  dari `ums512.dtsi` lama (`reg_format`) dan padding dtc untuk properti
  `*-gpio` berisi string-list — keduanya tidak mempengaruhi isi node.
- `.ko` penuh: `make modules` di codespace → **`sprd_pdbg.ko` 487.688 B**
  (`depends=sprd_sip_svc`, export `sprd_pdbg_msg_print`, kthread
  `sprd-pdbg-thread`), `modules.order` 230 → **231**, RC=0 tanpa error.
