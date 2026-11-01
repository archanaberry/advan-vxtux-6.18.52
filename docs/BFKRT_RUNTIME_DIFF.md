# BFKRT Runtime Diff — stok 5.4.210 vs port 6.18.52

Perbandingan perilaku runtime kernel via BFKRT, **tanpa flash**:
- **5.4.210 (stok, tablet hidup)** — modul BFKRT di-insmod di OS ori (sudah Live, taint OE).
- **6.18.52 (port)** — boot kernel port di **QEMU** (`qemu-bfkrt-diff.yml`), insmod 3 modul BFKRT, rekam `/proc/bfkrt_*`.

Run CI: `41317411128` (boot + insmod + dump sukses, gate false-negative), `41320771140` (gate hijau, arsip).
Sesi 5.4.210: `/e/ko54ci/sesi/stok/`; serial 6.18: `/e/ko54ci/qemu/seri2/serial-bfkrt.log`.

## Hasil (verdict)

| Aspek perilaku | 5.4.210 stok (tablet) | 6.18.52 port (QEMU) | Hasil |
|---|---|---|---|
| Boot health | hidup 199 hari, 0 panic/CFI boot ini | boot OK, 0 panic/oops/BUG/CFI | **PARITAS** |
| Load modul BFKRT (CFI-safe) | 3 Live, taint OE | 3x rc=0, taint W | **PARITAS** |
| Reentran (deadlock) | 0 | 0 | **PARITAS** |
| Kedalaman lock >8 | 0 | 0 | **PARITAS** |
| Deferred probe | 0 | 0 | **PARITAS** |
| Disiplin mutex | lock~unlock seimbang | 25 unlock, seimbang | **PARITAS** |
| Resolver simbol | jalan (`kallsyms_lookup_name` reachable) | "tidak tersedia" | **BEDA** (bukan cacat kernel) |
| Traffic PMIC/SPI | 190 `spi_sync` (PMIC asli) | 0 (`driver_probe_device` saja) | **CONFOUND HW-vs-QEMU** |

## Perbedaan yang terjelaskan (bukan regresi port)

1. **Resolver simbol mati di 6.18.** `kallsyms_lookup_name` **tidak di-export upstream
   sejak 5.7** (commit 0bd476e6c676). Meski `CONFIG_KALLSYMS=y`, simbol tak bisa
   di-link modul; di 5.4 masih exported. Fix = jalur kprobe (bukan bug kernel port).
2. **0 traffic SPI di QEMU.** Mesin `virt` QEMU tidak punya PMIC/SPI SPRD, jadi denyut
   `sprd_pmic_spi_read/write` tak pernah terjadi. Ini membandingkan HW vs
   virtualisasi, bukan stok vs port. Sinyal utama diff = boot-health, module-load,
   CFI-safety, disiplin mutex/deferred (semua PARITAS).

## Temuan CFI historis (5.4.210)

Pstore tablet berisi panic **`CFI failure (target: kallsyms_lookup_name)`** dari boot
LAMA (modul clang salah-build) — persis jebakan yang didokumentasikan skill
`cfi-kernel-module-loading` (jangan panggil fungsi kernel lewat function pointer
cast; pakai `register_kprobe`). Build clang r416183b sekarang lolos, dan modul BFKRT
identik masuk 6.18 tanpa CFI panic -> **CFI-safety port terjaga**.

## Catatan boot 6.18 (QEMU)

- `WARNING: genl_validate_ops` di `imsbr_netlink_init` (Tainted W) saat boot —
  WARN boot-time dari keluarga netlink IMS BR, bukan crash/CFI.
- `sprd_wdf: get public api error in sprd_wdt -19` — tak ada device watchdog di
  QEMU (tanpa PMIC); aman. Di HW asli `sprd_pmic_wdt` (deblob port) yang menyediakan.
- `platform gpio-keys: deferred probe pending: failed to get gpio` — probe deferred
  normal (GPIO tidak ada di QEMU); `deferred` tetap 0 (tidak ditambahkan dalam window).

## Gate CI (2 fix)

1. `make M=bfkrt_ko modules` gagal modpost ("undefined!") untuk SEMUA simbol ekspor
   (register_kprobe, _printk, memset, proc_create...) karena `Module.symvers` kosong
   (hanya `make Image` tidak mencatat ekspor). Fix: `make modules` (pohon penuh)
   dulu baru `M=bfkrt_ko`.
2. Gate false-negative: pola grep `INSMOD <m>.ko rc=0` (spasi tunggal) vs log yang
   memakai padding `%-32s`. Fix: hitung baris `INSMOD .* rc=0 OK` >= 3.
