# RE kanal log/dump — kernel stok **5.4.210** → dipakai kernel 6.18.52

Tanggal bedah: 2026-10-28.  Target: Advan Tab VX Lite T1030 (Unisoc UMS512 / T618).

Tujuan: mencari **semua** mekanisme yang dipakai ROM & kernel stok untuk menyimpan
log antar-boot, lalu memilih kanal yang aman untuk kernel 6.18.52 — supaya boot
berikutnya tidak lagi "diam di logo" tanpa jejak.

---

## 1. Sumber kernel 5.4.210 (ditemukan di internet)

Identitas dari `backupan-ori-advantabvxlite_t1.1/backup/boot_a.bin` (kernel 38.085.120 byte):

```
Linux version 5.4.210 (wufeng@cube-NF5280M5) (Android (7284624, based on
  r416183b) clang version 12.0.0) ... 5.4.210 SMP preempt mod_unload modversions aarch64
```

Repo publik yang **cocok persis** (dicek lewat GitHub API, `Makefile` di cabang default):

| Repo | `VERSION/PATCHLEVEL/SUBLEVEL` |
| --- | --- |
| [realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source](https://github.com/realme-kernel-opensource/realme_C31_C33_C35_narzo50A-Prime-AndroidT-kernel-source) | **5 / 4 / 210** ✅ |

Repo ini adalah "induk" keluarga MocorDroid yang sudah tercatat di [README](../README.md)
donor (`iscle` 4.14.133, `marohinmark`/`Bocchi` 5.4.254) — dan sebelumnya memang
`Belum di-clone`.  Kode bootloader pembanding: [sprd-linux/lk](https://github.com/sprd-linux/lk)
(`app/sprdboot/include/minidump.h`).

Salinan file yang relevan diunduh ke **`donors/kernel_5.4.210/`**:
`sysdump.c` (51.980 B), `sysdumpdb.h`, `last_kmsg.c`, `sprd_sysdump.h`, 2× `Kconfig`.

---

## 2. Empat mekanisme, dengan bukti dari dump perangkat

| # | Mekanisme | Menulis ke | Pemicu | Pihak yang menulis |
| --- | --- | --- | --- | --- |
| 1 | `last_kmsg` | partisi **`common_rs1_a`** (mentah, tanpa fs) offset 0 (128 KiB) + offset 1 MiB (ylog) | `register_reboot_notifier` + `kmsg_dumper` (panic/oops/shutdown) | **kernel** |
| 2 | `sprd-sysdump` / minidump | **reserved memory** (DTS `sprd-sysdump`, bootarg `sysdump_magic=`) → partisi **`sysdumpdb`** | panic/crashkey → reset | kernel (isi) + **LK** (menyimpan ke partisi) |
| 3 | `ramoops` (`PSTORE_RAM`) | reserved memory, di-map sebagai `/sys/fs/pstore` | kmsg dump sesuai `max_reason` | kernel |
| 4 | `uboot_log` | partisi `uboot_log` 4 MiB | setiap boot LK | **LK** |

### 1) `last_kmsg` → `common_rs1_a` (inilah "dump log" yang dicari)

`drivers/soc/sprd/debug/sysdump/last_kmsg.c` (5.4.210):

```c
static const char devicename[] = "/dev/block/by-name/common_rs1_a";
#define LAST_KMSG_OFFSET        0
#define LAST_ANDROID_LOG_OFFSET (1 * 1024 * 1024)
#define KMSG_BUF_SIZE           (128 * 1024)
...
ret = write_data_to_partition(plog_file, kmsg_buf, KMSG_BUF_SIZE, LAST_KMSG_OFFSET);
ret = write_data_to_partition(plog_file, ylog_buffer, YLOG_BUF_SIZE, LAST_ANDROID_LOG_OFFSET, ...);
```

Bukti di dump PC (`backup/.../common_rs1_a.bin`, 8 MiB):

```
00000000  3c 31 34 3e 5b 20 20 20 37 31 2e 30 38 31 37 35  |<14>[   71.08175|
  → "<14>[   71.081754]c7 [    T1] init: Sending signal 15 to service ..."
```

`common_rs2_a.bin` (16 MiB) kosong — hanya slot `-a` yang dipakai.

**Kepentingan:** ini satu-satunya kanal stok yang menulis log *mentah* ke partisi
yang memang disediakan untuk itu (bukan filesystem).

### 2) `sprd-sysdump` → reserved memory → `sysdumpdb`

`drivers/soc/sprd/debug/sysdump/sysdump.c` (52 KB, 180 simbol `sprd-sysdump:` di
Image stok): menyalin register, `log_buf`, `ylog_buf`, `etb_data`, section
teks/rodata/pagetable ke region reserved (`sysdump_magic=0x...` di bootargs atau
`magic-addr` node DTS), lalu `minidump_save_extend_information()` mendaftarkan
potongan yang harus ikut.

`sysdumpdb.h` memuat format bagiannya:

```c
#define SYSDUMPDB_PARTITION_NAME "sysdumpdb"
#define UBOOT_MAGIC "U2.0"   /* uboot sudah menyimpan minidump */
#define KERNEL_MAGIC "K2.0"  /* data pengecualian kernel valid */
struct dumpdb_header { char uboot_magic[4]; char app_magic[4]; int dump_flag; ... };
```

Bukti di dump perangkat (`out/logs/artix37/sysdumpdb.bin`, 10 MiB) — **file tabel
rekaman yang ditulis bootloader**, bukan filesystem:

```
0x0100  MINIDUMP
0x0154  bootloader
0x0250  dump_report.txt
0x0290  -dump time is 1970-01-01_08-39-57
0x02d6  -reason is bootloader panic
0x02f2  -bootcause_cmdline is Reboot into bootloader panic
0x12ac  compress_record_file
0x136c  etb_data:32695 -1
0x137f  bootloader_last_log:103444 2649 -1
```

→ `sysdumpdb` hanya berisi data **sesudah** crash; boot normal tidak menulis apa pun di sini.

### 3) `ramoops`, bukan `pstore/blk`

Image stok memuat `ramoops.record_size`, `ramoops.pmsg_size`, `ramoops.console_size`,
`ramoops_pstore_write`, `ramoops: attached %s 0x%zx@0x%llx` — tetapi **0 kemunculan**
`pstore_blk`, `best_effort`, maupun `PSTORE_BLK`.  Jadi pstore stok memakai RAM yang
dipesan, tidak pernah menyentuh `cache`.

### 4) `uboot_log`

Dump `out/logs/artix37/uboot_log.bin` dimulai `[00000903] [c0] **  LK (1, restored from storage)` —
log LK, ditulis ulang tiap boot.

### 5) `cache` **bukan** kanal log

`cache.bin` (100 MiB) ber-**ext4**: magic `53 ef` di `1024+0x38`, berisi
`u:object_r:cache_file:s0`, berkas recovery, dsb.  Menulis byte mentah ke partisi ini
(= `CONFIG_PSTORE_BLK` / flight-recorder lama) merusak metadata filesystem.

---

## 3. Yang dipakai kernel 6.18.52 sekarang

Diubah di sesi ini (`ci_qemu/initramfs/init.c`, `enhanced_config.cfg`,
`ci_qemu/mk_boot_artix.py`, `out/ci_artix37_pipeline.sh`):

1. **Kanal utama — berkas di atas ext4 yang dipasang.**
   `mount_cache_partition()` mencari partisi berukuran persis 100 MiB lalu
   `mount -t ext4 /cache`; `save_early_dmesg()` menulis
   **`/cache/boot_log.txt`** (dmesg awal, *sebelum* partisi super dipindai) dan
   `write_report_to_cache()` menulis **`/cache/vxtux_report.txt`**.
   Uji tulis-baca-balik memakai berkas `/cache/.vxtux_write_test`
   (`VXTUX-WRITE-TEST: OK file write+readback on /cache`), bukan offset mentah.
2. **Kanal cadangan — meniru `last_kmsg` di `common_rs1_a`.**
   Karena mount ext4 bisa gagal (feature tak dikenal, journal kotor), init tetap
   menulis `### VXTUXRS1\n` + dmesg (maks 128 KiB, `RS1_MAX`) ke partisi mentah
   8 MiB yang berisi log stok.
   *Identifikasi dua syarat:* ukuran **8192 KiB** **DAN** isi offset 0 = tanda
   `### VXTUXRS1` atau baris khas stok `<` digit `[`.  Kenapa dua: **sembilan**
   partisi di perangkat ini berukuran 8 MiB persis (`dtb_a/b`, `dtbo_a/b`,
   `fbootlogo`, `logo`, `init_boot_a/b`, `common_rs1_a/b`), sedangkan isi hanya
   `common_rs1_a` yang berupa teks log.  Bila tidak ada yang cocok fungsi ini
   **menolak menulis** dan mencetak `... not found ... (skipped)` — lebih baik
   kehilangan log ketimbang menimpa dtb/logo.
3. **Backend yang merusak dimatikan:** `CONFIG_PSTORE_BLK` → `is not set`,
   `CONFIG_VXTUX_FLIGHTREC` → `is not set`, `pstore_blk.best_effort=1` dibuang dari
   `CONFIG_CMDLINE` **dan** dari `CMDLINE` di `ci_qemu/mk_boot_artix.py`.
   (`CONFIG_PSTORE=y` + `PSTORE_RAM=y` dibiarkan; tanpa node reserved-memory dia
   hanya membuang pesan `ramoops: NULL platform data`.)
4. **`/sbin/init` dulu, bukan `/bin/dinit-run`.** Scan mentah `artix_super_5600mb.img`
   **dan** `out/logs/android_pre/super.bin` (dump perangkat 2026-10-28 12:23):
   `dinit-run` = **0 kemunculan**, `dinit-init` = ada, `getty@tty1` = ada,
   `agetty` = ada.  Urutan kandidat kini
   `/sbin/init → /sbin/dinit-init → /sbin/dinit → /bin/dinit → /bin/sh`, plus
   `PATH/TERM/HOME` di-set (PID 1 mewarisi lingkungan kosong dari kernel).
   `getty@tty1..tty6` (fbcon) sudah tersedia di rootfs Artix → inilah CLI fbcon-nya.

### Catatan build

* Init dikompilasi **lokal** oleh pipeline
  (`aarch64-none-linux-gnu-gcc 15.2.1`, `-static -O2 -march=armv8-a -mabi=lp64`)
  lalu **disuntikkan ke ramdisk boot.img** — bukan memakai salinan lama di dalam Image.
  Alasan: cpio eksternal di-unpack **sesudah** initramfs bawaan dan file dengan nama
  sama dibuka `O_TRUNC` (`6.18.52-VXTux-UMS512/init/initramfs.c: do_populate_rootfs`
  dan `do_name()`, baris 333/384), jadi salinan baru selalu menang.
* Initramfs bawaan di dalam `Image` **tidak** dipatch: setelah gzip tepat 334.175 byte
  langsung ada data seksi lain (`00 5f 19 05 ...`), jadi memperbesarnya akan
  menimpa data lain.  Salinan bawaan akan menyusul otomatis pada build CI berikutnya.
* `/init -> /vxtux/init` (symlink) ikut disuntik sebagai cadangan bila `rdinit=`
  tidak dibaca LK.

---

## 4. Batas / yang belum diverifikasi

* Belum boot di perangkat: bukti di atas = bedah biner + dump partisi, bukan hasil boot.
* Tidak diketahui apakah `common_rs1_a` pernah ditulis ulang sejak perangkat terakhir
  jalan Android stok; kalau ternyata isinya sudah habis (nol byte), kanal cadangan
  otomatis melewati dirinya sendiri pada boot pertama — baris
  `raw vendor log partition not found (skipped)` adalah gejalanya.
* Belum ada backend pstore yang benar-benar hidup di 6.18.52 (ramoops butuh node
  reserved-memory di DTS kita; sengaja tidak ditambahkan sesi ini).
* Build CI `40662445403` (`test1@9d436c4dd`, 2026-10-28) **sudah** memakai config
  ini: `effective.config` keluar **0 baris** `CONFIG_PSTORE_BLK=y` dan **0 baris**
  `CONFIG_VXTUX_FLIGHTREC=y`, `CONFIG_CMDLINE` tanpa `pstore_blk.best_effort=1`,
  `SPRD_COMMON_CLK=m` + `SPRD_UMS512_CLK=m`, `VT_CONSOLE=y` +
  `FRAMEBUFFER_CONSOLE=y` (fbcon).  Initramfs bawaan Image juga sudah berisi
  `/init` -> `/vxtux/init` + `vxtux/init` 736.208 byte (dengan PATH/TERM/HOME).
  Artefak: `out/6.18.52-VXTux-UMS512/boot_artix37.img` 45.514.752 byte,
  md5 `016343fa9ea2ec00f6a12feebadb7503`; isinya diverifikasi terpisah —
  `vxtux/init` 3.368.384 byte md5 `9826f1c6dd4e` == binary hasil kompilasi lokal,
  `/init` symlink, `clk-sprd.ko` + `ums512-clk.ko` ikut.  Bat sudah digenerate
  ulang dari ukuran+md5 itu.
* Belum ada yang boot dengan artefak ini — verifikasi sejauh ini = bedah biner,
  dump partisi, kompilasi, dan pemeriksaan isi ramdisk, bukan hasil di layar.