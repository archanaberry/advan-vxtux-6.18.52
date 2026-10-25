# SISI KERNEL — `6.18.52-VXTux-UMS512` (+ varian `-artix`)

Ditulis ke partisi **`boot_a`** (64 MB). Isi setelah build CI hijau
(run `40163789668`, commit `1aa43626d`, 30m58s):

```
6.18.52-VXTux-UMS512/
├── boot_6.18.52-VXTux-UMS512-artix.img   ← 41.885.696 byte, diflash ke boot_a
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

## Cmdline yang dipakai — DIPAKSA LURUS oleh `CONFIG_CMDLINE_FORCE`

> **PENTING (fix 2026-10-25, bedah `out/logs/artix3`)**: LK Unisoc
> mendeduplikasi cmdline per-kunci antara bootargs stok (dari DTB) dan
> cmdline boot.img, dan bootargs stok SELALU didahulukan → "yang pertama
> menang". Di log LK terlihat cmdline kita dibaca utuh
> (`root=PARTLABEL=super ... init=/sbin/init`) tapi `final bootargs` yang
> dikirim ke kernel hanya memuat milik stok: `init=/init root=/dev/ram0`.
> Karena boot.img tanpa ramdisk, `prepare_namespace()` lalu menunggu
> `/dev/ram0` selamanya → layar beku di logo (tanpa panic).
>
> Solusi: kernel di-build dengan `CONFIG_CMDLINE` + `CONFIG_CMDLINE_FORCE=y`
> (`enhanced_config.cfg`), sehingga `drivers/of/fdt.c:1099` melempar bootargs
> LK dan memakai cmdline bawaan build ini. Arm64 tidak punya
> `CONFIG_CMDLINE_EXTEND`, pilihannya hanya `FROM_BOOTLOADER` atau `FORCE`.

```
earlycon console=ttyS1,115200n8 loglevel=7
root=PARTLABEL=super rootfstype=ext4 rootwait rw init=/sbin/init
loop.max_part=7 printk.devkmsg=on
androidboot.console=ttyS1 androidboot.hardware=ums512_1h10
androidboot.slot_suffix=_a androidboot.serialno=0123456789ABCDEF
androidboot.selinux=permissive
```

* `root=PARTLABEL=super` → kernel 6.18 mencari partisi lewat
  `block/early-lookup.c`, jadi **tidak perlu initramfs** (ramdisk_size = 0).
* `rw` → ext4 dibuka read-write sejak awal (bukan `ro` seperti image lama).
* `rootfstype=ext4` → tidak ada erofs/LP sama sekali.
* `earlycon` tanpa argumen → pakai `stdout-path` DTB (`serial1` = ttyS1),
  alamat UART yang benar (`0x70100000`, bukan `0x71400000` yang lama —
  alamat itu bukan UART di UMS512).
* Cmdline di `mk_boot_artix.py` disinkronkan sebagai dokumentasi; yang
  menang adalah `CONFIG_CMDLINE` di kernel.

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
| `CONFIG_CMDLINE` + `_FORCE` | `y` | **fix diam di logo**: paksa cmdline build ini, buang bootargs stok LK (`root=/dev/ram0`)

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

## Perbaikan diam-di-logo 2026-10-25 (CMDLINE_FORCE)

Build CI `40210117913` (commit `2d0c28c15`, 16m58s) menghasilkan `Image` yang
membawa cmdline paksa di dalam binernya (diverifikasi via `strings`: teks
`root=PARTLABEL=super` ada di dalam `Image`). QEMU Boot Test run `40210117912`
lulus penuh (termasuk sesi CLI fbcon) — fragmen QEMU menetralkan CMDLINE
lewat `CONFIG_CMDLINE=""`, jadi QEMU tetap memakai `-append` sendiri.

Flash: hanya `boot_a` (super tidak berubah) — lihat `out/flash_boot_only.bat`.

## Perbaikan bootloop 2026-10-25 (padding header → page 4096)

Image hasil racikan sebelumnya menempelkan kernel tepat setelah header
1632 byte (**offset 0x660**) tanpa mem-kanal-kan header ke `page_size`
(4096). LK membaca kernel dari offset 4096 sesuai spesifikasi, sehingga
terjadi dua kali pemuatan kernel geser 0x9A0 byte: entry mendarat di pad
NOP, `head.S` terlewat, dan dalam ~60 ms kernel menabrak `brk #0x800`
(lokasi `elr 0x80177288`); LK menangkapnya di EL2 → `bootloader panic,
reboot` → loop (tercatat 21 sesi di `uboot_log`, semua identik).

Perbaikannya ada di `ci_qemu/mk_boot_artix.py`: tiap komponen kini di-
kanal-kan ke batas page **sebelum** ditulis (bukan hanya sesudahnya).
Layout baru diverifikasi: magic `ARM\x64` di 0x1038 (kernel mulai 0x1000,
sejajar page — sama persis dengan `boot_a_vxtux_arasan.img`), byte di
`boot[4096:4096+kernel_size]` identik dengan `Image`, dan word di titik
kegagalan lama (`4096+0xF7288`) kini bukan `brk` lagi.
