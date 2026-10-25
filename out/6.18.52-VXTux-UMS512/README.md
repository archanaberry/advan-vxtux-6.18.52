# SISI KERNEL — `6.18.52-VXTux-UMS512` (+ varian `-artix`)

Ditulis ke partisi **`boot_a`** (64 MB). Isi setelah build CI hijau
(run `40163789668`, commit `1aa43626d`, 30m58s):

```
6.18.52-VXTux-UMS512/
├── boot_6.18.52-VXTux-UMS512-artix.img   ← 41.883.232 byte, diflash ke boot_a
├── Image                                  ← 41.880.064 byte, kernel aarch64 mentah
├── System.map                             ← 5.991.872 byte
├── ums512-1h10-vxtux.dtb                  ← 53.882 byte (DTS vxtux, untuk QEMU/CI)
├── modules/                               ← 233 .ko, vermagic 6.18.52-VXTux-UMS512-artix
└── README.md                              ← berkas ini
```

Bukti dari isi `Image` (strings):

| Simbol di biner | Artinya |
|---|---|
| `Linux version 6.18.52-VXTux-UMS512-artix` | varian terpasang |
| `sdhci_sprd_tuning`, `sdhci_sprd_get_best_clk_sample` | driver eMMC **built-in** (modulnya tidak ada lagi di artefak .ko) |
| `mmc_swcq_pump_requests` | SW-CQ eMMC built-in |
| `fbcon_startup`, `fbcon_resize` | **fbcon ada di dalam kernel** |
| `PARTLABEL=` | dukungan root lewat label partisi GPT |

## Cmdline yang dipakai (header v0, sama seperti image yang sudah terbukti load)

```
console=ttyS0,115200n8 console=ttyS1,115200n8 earlycon=sprd,uart,0x71400000
androidboot.console=ttyS1 androidboot.hardware=ums512_1h10
androidboot.slot_suffix=_a androidboot.serialno=0123456789ABCDEF
root=PARTLABEL=super rootfstype=ext4 rootwait rw
init=/sbin/init androidboot.selinux=permissive
```

* `root=PARTLABEL=super` → kernel 6.18 mencari partisi lewat
  `block/early-lookup.c`, jadi **tidak perlu initramfs** (ramdisk_size = 0).
* `rw` → ext4 dibuka read-write sejak awal (bukan `ro` seperti image lama).
* `rootfstype=ext4` → tidak ada erofs/LP sama sekali.

## Config kunci (efektif, diverifikasi dari artefak CI)

| Simbol | Nilai | Kenapa |
|---|---|---|
| `CONFIG_MMC_SDHCI_SPRD` | `y` (dulu `m`) | eMMC harus hidup **sebelum** root di-mount |
| `CONFIG_EXT4_FS` | `y` | filesystem root |
| `CONFIG_DEVTMPFS` / `_MOUNT` | `y` | `/dev/console`, `/dev/mmcblk0*` otomatis ada |
| `CONFIG_DRM_SPRD` | `y` (dulu `m`) | panel/DSI, dipakai fbcon |
| `CONFIG_FB` | `y` (dulu `n`) | gerbang menu fbdev |
| `CONFIG_DRM_FBDEV_EMULATION` | `y` | client fbdev di atas KMS |
| `CONFIG_FRAMEBUFFER_CONSOLE` | `y` | **fbcon** — teks di panel |
| `CONFIG_VT` / `CONFIG_VT_CONSOLE` | `y` | tty1..tty6 + getty |
| `CONFIG_LOCALVERSION` | `-VXTux-UMS512-artix` | penanda: kernel ini untuk super ext4 |

Sebelumnya `CONFIG_FB=n` membuat `DRM_FBDEV_EMULATION` dibuang `olddefconfig`
(`depends on DRM && FB`), jadi fbcon tidak pernah ada — panel hanya bisa
menampilkan gambar lewat Android/SurfaceFlinger. Sekarang fbcon aktif.

## Cara repack boot image

```sh
python ci_qemu/mk_boot_artix.py <Image> out/6.18.52-VXTux-UMS512/boot_6.18.52-VXTux-UMS512-artix.img
python ci_qemu/bootimg_tool.py info out/6.18.52-VXTux-UMS512/boot_6.18.52-VXTux-UMS512-artix.img
```

Basis header diambil dari `out/base_boot/` (`header.bin` v0 dari
`boot_a_vxtux_arasan.img` yang sudah terbukti di-load bootloader UMS512):
page 4096, kernel addr `0x80080000`, tags `0x86000000`, dtb 0.
Ramdisk dikosongkan — root diambil langsung dari partisi `super`.
