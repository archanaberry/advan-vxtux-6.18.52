# Bongkar firmware stok ADVAN VX Lite (UMS512 / T618)

Sumber: `E:/backupan-ori-advantabvxlite_t1.1/backup` — **70 partisi**, dump lengkap
(`super.bin` 5.872.025.600 B, `vendor_boot_a/b` 100 MB, `boot_a/b` 64 MB, dtb/dtbo, vbmeta).

Alat: [ci_qemu/fw_unpack.py](../ci_qemu/fw_unpack.py), [ci_qemu/fw_extract2.py](../ci_qemu/fw_extract2.py),
[ci_qemu/bootimg_tool.py](../ci_qemu/bootimg_tool.py), [ci_qemu/cpio_tool.py](../ci_qemu/cpio_tool.py).

## 1. boot_a / boot_b — header v4

| Field | Nilai |
|---|---|
| magic | `ANDROID!` |
| header_version | **4** (header 1584 B, payload align 4096) |
| kernel | 38.085.120 B — Linux **5.4.210** `#1 SMP PREEMPT Tue Nov 7 11:50:19 CST 2023` |
| ramdisk | 1.469.832 B, **LZ4 legacy** → cpio 2.733.824 B (12 entri) |
| cmdline | kosong (vendor_boot yang mengisi) |

Ramdisk boot (GKI generik): `/init` 2.731.328 B, `system/etc/ramdisk/build.prop`,
`debug_ramdisk`, `metadata`, `mnt`, `proc`, `sys`, `system/...`.

## 2. vendor_boot_a / vendor_boot_b — header v4

| Field | Nilai |
|---|---|
| cmdline | `console=ttyS1,115200n8 buildvariant=user` |
| vendor_ramdisk | 22.791.010 B, LZ4 legacy → cpio **52.246.016 B, 639 entri** |
| DTB | **117.599 B** @ `0x15be040` (inilah "DTB arasan 115 KB" yang dipakai image kustom) |

Isi penting vendor ramdisk:

- `first_stage_ramdisk/fstab.ums512_1h10` — fstab stok (lihat §3)
- `system/bin/`: `e2fsck`, `fsck.f2fs`, `make_f2fs`, `sload_f2fs`, `snapuserd`, `linker64`
- `lib/modules/` → **129 modul `.ko`, 12,9 MB**, termasuk yang krusial:

| Modul | Ukuran | Fungsi |
|---|---|---|
| `sdhci-sprd.ko` | 145.726 B | host eMMC (wajib untuk mount root!) |
| `mmc_swcq.ko` | 72.670 B | software command queue MMC |
| `mmc_hsq.ko` | 21.790 B | hardware command queue MMC |
| `sprd-drm.ko` | 936.742 B | display/DRM (panel DSI) |

## 3. fstab stok — semuanya EROFS read-only

```
system      /system       erofs  ro wait,avb=vbmeta_system,logical,first_stage_mount,...
system      /system       ext4   ro,barrier=1 wait,avb=vbmeta_system,logical,...
system_ext  /system_ext   erofs  ro ... logical
vendor      /vendor       erofs  ro ... logical
odm         /odm          erofs  ro ... logical
product     /product      erofs  ro ... logical
vendor_dlkm /vendor_dlkm erofs  ro ... logical
/dev/block/by-name/metadata /metadata ext4 nodev,noatime,nosuid,errors=panic wait,...
```

Artinya partisi Android di dalam `super` memang **EROFS + logical (LP metadata)** —
persis yang tidak kita pakai: target kita **ext4 RW fisik** langsung di partisi `super`.

## 4. init_boot_a / init_boot_b

8 MB, **seluruhnya nol** — tidak dipakai di perangkat ini.

## 5. vbmeta

Semua `vbmeta_*`/`avbmeta_*`: magic `AVB0`, **big-endian**, `avb1.0`, `algo=2`
(SHA256_RSA2048), **`flags=0x0` → verifikasi AKTIF**, `release_string="avbtool 1.2.0"`.
`avbmeta_rs_a/b` seluruhnya nol.

Konsekuensi: kalau bootloader menegakkan AVB pada `boot_a`, image kernel kita bisa
ditolak. Perangkat sudah unlocked + Magisk, jadi dalam praktik bootloader mengabaikan —
tetapi jalan aman adalah menulis `vbmeta_a` dengan flag verifikasi dimatikan
(`avbtool make_vbmeta_image --flags 2`) **atau** mengandalkan mode unlocked.

## 6. super.bin

5.872.025.600 B (5600 MB). 4096 byte pertama nol, **tidak ada `LP_METADATA_GEOMETRY`
dalam 16 MB pertama**, data mulai di `0x1000` dan tidak berpola LP header.
Kesimpulan jujur: dump ini **bukan super berisi LP metadata yang bisa di-parse**
(kemungkinan diambil saat partisi sudah terhapus/tertimpa, atau dump parsial).
Untuk rencana kita ini tidak masalah — `super` akan **ditimpa total** oleh image ext4
Artix; untuk kembali ke Android cukup tulis ulang blok yang sama dari dump ini.

## 7. Artefak hasil ekstraksi

- `ci_qemu/fw/stock.dtb` — DTB stok 117.599 B
- `ci_qemu/fw/boot_ramdisk.cpio` — ramdisk GKI 2,7 MB
- `ci_qemu/fw/vendor_ramdisk.cpio` — vendor ramdisk 52 MB (modul + fstab + tools)

## 8. Rencana pakai (belum dieksekusi)

1. **initramfs Artix**: busybox + modul 6.18 (`sdhci-sprd.ko`, `mmc_hsq.ko`,
   `mmc_swcq.ko`, `sprd-drm.ko` dari `built_kos/`) + `/init` yang `insmod` lalu
   mount `super` **ext4 rw** dan `switch_root` ke Artix.
2. **boot_a**: repack basis `boot_a_vxtux_arasan.img` (kernel 6.18 terbukti load) +
   initramfs baru + cmdline `console=ttyS1,115200n8 console=tty0 rootwait rw`.
3. **super**: image ext4 **5600 MB, RW**, diisi rootfs Artix via `mke2fs -d` (di perangkat).
4. **flash**: `spd_dump ... exec w boot_a <img> w super <img> reset` — partisi vendor
   (`vendor_boot_a`, `dtb_a`, `vbmeta_*`, `userdata`) **tidak disentuh**.
