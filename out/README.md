# Artefak boot: Artix Linux di partisi `super` (fisik, ext4 RW)

Dua sisi yang dipisah dan ditandai:

```
out/
├── artix-linux/                    ← SISI OS (userspace Linux)
│   ├── artix_super_5600mb.img      ← partisi super diisi ext4 RW (5.872.025.600 byte)
│   └── README.md                   ← isi rootfs, fstab, akun, catatan restore
└── 6.18.52-VXTux-UMS512/           ← SISI KERNEL (boot image)
    ├── boot_6.18.52-VXTux-UMS512-artix.img
    ├── Image                        ← kernel aarch64 mentah
    ├── modules/                     ← .ko dengan vermagic 6.18.52-VXTux-UMS512-artix
    └── README.md                   ← cmdline, config kunci, cara repack
```

## Pembagian tugas

| Sisi | Isi | Ditulis ke |
|---|---|---|
| **6.18.52-VXTux-UMS512** | kernel + cmdline (`root=PARTLABEL=super`) | partisi **`boot_a`** (64 MB) |
| **artix-linux** | rootfs Artix (ext4 **RW**, bukan EROFS/RO) | partisi **`super`** (5600 MiB) |

Partisi yang **tidak disentuh**: `vendor_boot_a/b`, `dtb_a/b`, `dtbo_a/b`,
`vbmeta_*`, `init_boot_*`, `userdata`, `metadata`, `cache`, dan seluruh partisi
modem/DSP/trustos.

## Kenapa ext4 RW (bukan EROFS)

fstab stok Android memakai `erofs ro` untuk `system/vendor/product/odm/vendor_dlkm`
(lihat [STOCK-FIRMWARE-UNPACK.md](../docs/STOCK-FIRMWARE-UNPACK.md) §3).
Untuk Linux kita buang seluruh LP metadata + EROFS dan menulis **satu filesystem
ext4 biasa** ke partisi `super` — bisa di-`mount -o rw`, bisa ditulis saat runtime.

## Cmdline boot

```
console=ttyS1,115200n8 console=tty0 root=PARTLABEL=super rootfstype=ext4 rw rootwait
```

`PARTLABEL=super` didukung kernel 6.18 (`block/early-lookup.c`), jadi tidak perlu
initramfs: kernel langsung mount ext4 RW dari partisi `super` sebagai `/`.

## Flash (JANGAN dijalankan tanpa perintah)

Lihat `flash_artix.bat` (menulis) dan `flash_rollback_stock.bat` (balik ke Android).

## Status verifikasi (2026-10-25)

| Item | Bukti |
|---|---|
| Image `super` dibuat | `mke2fs -d` di perangkat, **rc=0**, 5.872.025.600 byte, label `super`, journal 64 MB |
| Isi image | `e2fsck -fn` → **clean**, 93.744 file, 784.377/1.433.600 blok terpakai |
| Isi rootfs | `/sbin/init → dinit-init`, `dinit` + layanan `artix-adb`, `artix-sshd`, `getty@tty1..6`, `hyprland`, `wayvnc` |
| Salinan PC | `md5 0d1ec35a448d40c7af354d9c6d1d6ded` **identik** dengan perangkat (tarik kedua kali, 30,6 MB/s) setelah fstab diperbaiki |
| Kernel storage built-in | `CONFIG_MMC_SDHCI_SPRD=y` di `effective.config` artefak CI |
| Kernel fbcon | `CONFIG_FB=y`, `DRM_FBDEV_EMULATION=y`, `DRM_CLIENT_SELECTION=y`, `FRAMEBUFFER_CONSOLE=y` di `effective.config` run 40161696112 |
| Build kernel hijau | run **40163789668** (30m58s) + run 40163676567 (30m26s), commit `1aa43626d` |
| boot.img diracik | 41.883.232 byte (header v0, ramdisk kosong, cmdline `root=PARTLABEL=super ... rw init=/sbin/init`) |
| Isi `Image` | strings menunjukkan `sdhci_sprd_tuning`, `mmc_swcq_pump_requests`, `fbcon_startup`, `PARTLABEL=` — storage + fbcon built-in |
| Penanda varian | `CONFIG_LOCALVERSION="-VXTux-UMS512-artix"` (versi kernel akan terbaca `6.18.52-VXTux-UMS512-artix`) |
| Duplikat linker | `sdhci_sprd_send_command` dihapus dari `sdhci-sprd-ext.c`; run 40159821660 **hijau** |

Belum diverifikasi: boot di perangkat (belum ada perintah flash), perilaku panel
DRM, dan adbd/Artix di runtime.

## Perbaikan yang harus dilakukan sebelum boot hijau

1. **Himax hx83102e**: jalur `-DHX_CONFIG_FB` dimatikan (API notifier blank fb
   dihapus di 6.18). Touch jadi pakai jalur DRM seperti build sebelumnya.
2. **`/etc/fstab` di image**: baris root diganti `UUID=…,nofail` karena
   `/dev/block/by-name/` dibuat ueventd Android yang tidak ada di Linux murni.
3. **Guard CI**: `CONFIG_FONT_AUTOSELECT` dihapus dari fragmen (def_bool).
