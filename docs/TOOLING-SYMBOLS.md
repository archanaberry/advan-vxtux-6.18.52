# Alat Bedah Undefined Symbol — Yang Dipakai, Yang Tidak Bisa

Permintaan: "alat yang bisa membedah total undefined symbol / yang tidak
terjangkau". Dokumen ini memisahkan **alat yang benar-benar dipakai di repo
ini** dari alat yang cuma terdengar bagus. Semua angka di sini berasal dari
[symreach.py](../donors/symreach.py) dan [UNDEF-REACHABILITY.md](UNDEF-REACHABILITY.md).

## Ringkas: alat mana untuk pertanyaan apa

| Pertanyaan | Alat yang benar | Status di mesin ini |
|---|---|---|
| Simbol apa saja yang di-UND sebuah `.ko`? | `nm -u`, `readelf -s`, atau `pyelftools` | **dipakai** (`pyelftools` 0.33 + `aarch64-linux-gnu-nm`) |
| Simbol itu diekspor kernel kita atau tidak? | `Module.symvers` + `modpost` | **dipakai** ([symvers_tick13.txt](../donors/symvers_tick13.txt)) |
| Kernel kita punya simbol itu (walau tak diekspor)? | `nm vmlinux` / `nm vmlinux.o` | **dipakai** ([vmlinux_o_syms.txt](../donors/vmlinux_o_syms.txt), 140k simbol) |
| Kenapa hilang: rename? inline? config mati? | `git grep` definisi + tabel `__versions` | **dipakai** (`tree_defs()` + CRC) |
| Siapa yang seharusnya menyediakan? | graf ekspor antar-`.ko` | **dipakai** (`provider_of` → kelas `port`) |
| ABI-nya masih cocok? | CRC modversions, `abidiff` | **CRC dipakai**; `abidiff` butuh DWARF (tidak ada) |
| Dari Image mentah, apa isi kallsyms-nya? | `vmlinux-to-elf` | tidak dipakai — kita punya `vmlinux.o` asli, lebih akurat |
| Validasi ulang independen | `check_symbol` (KernelSU) | konsepnya diadopsi, binernya tidak dipasang |

## Alat yang dipakai, dan kenapa

### 1. `pyelftools` — pembaca ELF utama
Tanpa ini, "undefined symbol" cuma tebakan dari `strings`. Yang dibaca:

- `.symtab` → simbol `SHN_UNDEF` (referensi keluar) dan `__crc_<nama>`
  (ekspor milik blob itu sendiri, cara paling andal karena tidak perlu
  mengurai `___ksymtab+<nama>`).
- `__versions` → 64 byte per entri di 5.4 arm64
  (`struct modversion_info { unsigned long crc; char name[56]; }`).
- `.modinfo` (`vermagic=`) → memastikan blob memang 5.4.210.

### 2. `nm --defined-only vmlinux.o`
Ini otoritas "kernel kita punya simbol ini atau tidak". Lebih baik daripada
ekstraksi kallsyms dari `Image`, karena `vmlinux.o` punya **semua** simbol
termasuk `t` (lokal). Uji kelengkapan: **14.291 dari 14.292** ekspor vmlinux
di `Module.symvers` ada di daftar ini — jadi kalau sebuah simbol tidak ada di
sini, dia benar-benar tidak ada di build kita.

Perintahnya (di codespace):

```bash
nm --defined-only --format=posix vmlinux.o | cut -d' ' -f1 | sort -u
```

### 3. `modpost` + `Module.symvers` + `KBUILD_EXTRA_SYMBOLS`
Ini alat resmi kernel, dan satu-satunya yang dipakai loader nanti:

```bash
# lihat semua unresolved sekaligus, jangan berhenti di yang pertama
make KBUILD_MODPOST_WARN=1 modules

# modul di luar pohon yang butuh ekspor modul lain
# (di Makefile modul):  KBUILD_EXTRA_SYMBOLS += /path/Module.symvers
```

`KBUILD_MODPOST_WARN=1` hanya untuk **melihat** daftar lengkap. `.ko` hasilnya
jangan dipakai: `insmod` tetap menolak simbol yang tidak ada.

### 4. `git grep` definisi kolom-0
Pembeda paling penting di proyek ini. Simbol yang tidak ada di `vmlinux.o`
bisa berarti tiga hal yang sangat berbeda:

| Bukti | Arti | Biaya perbaikan |
|---|---|---|
| `#define` / `static inline` di header | API 6.18 sudah inline | nol (recompile) |
| definisi ada, tapi Kconfig mati | fitur tidak ikut di-build | nyalakan config |
| tidak ada di pohon sama sekali | benar-benar dihapus | tulis/padankan kode |

Heuristik yang dipakai: definisi kernel **selalu mulai kolom 0**
(`int foo(...)`, `static inline ... foo(`, `#define foo`,
`DEFINE_PER_CPU(int, cpu_number)`), sedangkan pemanggilan selalu menjorok.
Tanpa batasan kolom ini, setiap call-site akan dianggap "simbolnya ada".

### 5. CRC `__versions` — pembeda "nama cocok, ABI beda"
Dengan `CONFIG_MODVERSIONS=y`, `insmod` membandingkan CRC tiap simbol yang
diimpor. Nama bisa ada semua dan modul tetap ditolak dengan pesan
`disagrees about version of symbol`. Membandingkan `__versions` blob 5.4
dengan `Module.symvers` 6.18 langsung menunjukkan ini.

## Alat populer yang **tidak** menyelesaikan masalah ini

### Ghidra
Bisa menampilkan `undefined _copy_from_user(void)` dan nama fungsi eksternal,
tapi tidak bisa memetakannya ke `vmlinux` target tanpa skrip
`ExternalSymbolResolver` buatan sendiri, dan tanpa DWARF semua signature
jadi `void`. Untuk pertanyaan "simbol ini ada atau tidak", Ghidra lebih
lambat dan lebih tidak pasti daripada `nm`. Tidak terpasang di mesin ini
(tidak ada Java), jadi tidak dipakai.

### radare2 / rabin2
`rabin2 -s` bagus untuk daftar impor/ekspor cepat. Tapi untuk analisis lanjut
ia harus dijalankan dengan `-e bin.cache=true` agar relokasi ter-resolve, dan
opsi itu **menghilangkan hasil analisis fungsi** (issue #20616). Jadi ia
bagus untuk inspeksi, bukan untuk audit menyeluruh.

### vmlinux-to-elf
Alat yang tepat ketika yang kamu punya hanya `Image` mentah: ia memindai
kallsyms terkompresi dan menghasilkan ELF untuk Ghidra/IDA. Di proyek ini
tidak diperlukan karena kita punya `vmlinux.o` hasil build sendiri — datanya
lebih lengkap daripada kallsyms. Simpan sebagai opsi kalau build ulang hilang.

### drgn / libabigail / Corelens / bpftrace
- `drgn`: debugger simbol berbasis Python; butuh host Linux + DWARF.
- `libabigail` (`abidiff`): membandingkan ABI dua ELF; butuh `CONFIG_DEBUG_INFO`.
- `Corelens`: di atas drgn, untuk crash dump tanpa DebugInfo.
- `bpftrace`/`perf-kallsyms`: untuk kernel **berjalan**, bukan untuk blob
  yang belum pernah dimuat.
Semuanya tidak bisa dijalankan di host Windows ini dan tidak menjawab
pertanyaan "simbol mana yang belum terjangkau" untuk blob yang belum dimuat.

### `check_symbol` (KernelSU PR #3035)
Utilitas C yang memvalidasi setiap UND sebuah `.ko` terhadap ekspor
`vmlinux`. Idenya sama dengan kolom `ok` di symreach: cocokkan UND ke daftar
ekspor. Tidak dipasang karena hasilnya sudah tercakup, tapi berguna sebagai
pembanding independen kalau angka kita dipertanyakan.

## Batas jujur

1. `vmlinux.o` adalah hasil link objek built-in, bukan `vmlinux` final
   (finalnya tidak ada di codespace tick13). Untuk daftar simbol, isinya
   setara.
2. `tree_defs` memakai heuristik kolom-0. Simbol data yang didefinisikan di
   dalam makro multi-baris bisa lolos dan tetap muncul sebagai `hilang`.
3. CRC `__versions` dihitung genksyms dengan tipe yang berbeda antar versi
   kernel; angka besar (3.223 dari 4.434 referensi `ok`) adalah **indikasi**
   drift ABI, bukan vonis per simbol.
4. Tidak ada satu pun blob yang dijalankan di perangkat. Semua ini analisis
   statis.

## Rujukan

- Kbuild / modpost: <https://docs.kernel.org/kbuild/modules.html>
- `check_symbol` KernelSU: <https://github.com/tiann/KernelSU/pull/3035>
- vmlinux-to-elf: <https://github.com/marin-m/vmlinux-to-elf>
- drgn: <https://github.com/osandov/drgn>
- libabigail: <https://sourceware.org/libabigail/>
- radare2 `bin.cache` vs analisis fungsi: <https://github.com/radareorg/radare2/issues/20616>
