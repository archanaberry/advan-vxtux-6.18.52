# SISI OS — `artix-linux` (partisi `super`, ext4 **RW**)

Berkas: `artix_super_5600mb.img`

| Properti | Nilai |
|---|---|
| Ukuran | **5.872.025.600 byte** = 5600 MiB — sama persis dengan partisi `super` di perangkat, **tidak diubah** |
| Filesystem | **ext4** (bukan EROFS, bukan read-only) |
| Label | `super` |
| UUID | `bcb89503-886c-49e3-b47f-464af345cfcd` |
| Block size | 4096 (1.433.600 blok) |
| Inode | 359.040, terpakai 93.744 |
| Journal | ada, 64 MB (ext4 RW normal) |
| Terpakai | 784.377 / 1.433.600 blok ≈ **3,06 GiB**, sisa ≈ 2,5 GiB bebas |
| md5 | `0d1ec35a448d40c7af354d9c6d1d6ded` (identik dengan salinan di perangkat) |

Dibuat **di perangkat** dengan `mke2fs -d` (bukan `mke2fs` kosong + tar),
lalu ditarik ke PC dan diverifikasi ulang: `e2fsck -fn` → **clean, 0 error**.

## Isi rootfs (Artix yang sudah diracik sebelumnya)

```
/sbin/init -> dinit-init          (dinit, BUKAN openrc/systemd)
/etc/dinit.d/artix-adb            adbd hidup lebih dulu (scripted service)
/etc/dinit.d/artix-sshd           sshd
/etc/dinit.d/artix-vnc            wayvnc
/etc/dinit.d/getty@tty1..tty6     getty CLI di VT (fbcon)
/etc/dinit.d/hyprland, sway       sesi grafis (belum divalidasi di perangkat)
/usr/bin/{dinit,dinitctl,bash,agetty,sshd,hyprland,sway,nano,vim}
/usr/local/bin/artix-adb-service  starter adbd
```

`/etc/fstab` sudah diperbaiki setelah pembuatan: baris root memakai
`UUID=bcb89503-…,nofail` karena `/dev/block/by-name/` hanya dibuat ueventd
Android; partisi tetap di-mount kernel lewat `root=PARTLABEL=super`.

Yang **dibuang** saat penyalinan (supaya masuk 5,6 GB): `root/src` (1,8 GB
source tree proot), `/tmp`, `usr/include`. Semua sisanya utuh.

## Kenapa ext4 RW, bukan EROFS

`fstab` stok Android memakai `erofs ro` untuk system/vendor/product/odm
(lihat [STOCK-FIRMWARE-UNPACK.md](../../docs/STOCK-FIRMWARE-UNPACK.md)).
Partisi `super` di sini **ditimpa total** oleh satu filesystem ext4 biasa:
tidak ada LP metadata, tidak ada super partition logis, tidak ada AVB —
bisa `mount -o rw` dan ditulis saat runtime.

## Cara membuat ulang

```sh
adb push out/mkfs_ext4.sh /data/local/tmp/
adb shell "su -c 'sh /data/local/tmp/mkfs_ext4.sh'"     # ~2 menit, log di /data/local/tmp/mkfs_ext4.log
adb pull /data/local/tmp/artix_super.img out/artix-linux/artix_super_5600mb.img
```

## Ditulis ke mana

Partisi **`super`** (5600 MB) — `spd_dump ... exec w super <img> reset`.
Partisi `userdata` (112 GB), `vendor_boot`, `dtb`, `vbmeta`, semua partisi
modem/DSP **tidak disentuh**.
