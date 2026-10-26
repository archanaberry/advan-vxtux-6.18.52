#!/usr/bin/env python3
"""Racik boot.img untuk Artix dari header boot yang sudah terbukti load.

Pakai:
  python mk_boot_artix.py <Image> <boot_a_artix.img> [--base DIR] [--ramdisk DIR]

Isi image:
  header  : header v0 dari boot_a_vxtux_arasan.img (page 4096, addr kernel
            0x80080000, tags 0x86000000) -- dipakai apa adanya.
  kernel  : Image 6.18.52-VXTux-UMS512-artix (storage + DRM built-in).
  ramdisk : kosong (default) -> kernel langsung mount root dari cmdline.
            Bila --ramdisk DIR diberikan, DIR harus punya file 'ramdisk'
            (cpio/gzip) untuk disisipkan.
  cmdline : isi sama dengan CONFIG_CMDLINE di enhanced_config.cfg (lihat di
            bawah). Catatan: LK Unisoc mendeduplikasi cmdline per-kunci dan
            bootargs stok dari DTB selalu didahulukan, jadi root=/init
            dari sini bisa dibuang. Kernel Artix memakai
            CONFIG_CMDLINE_FORCE=y sehingga cmdline bawaan build yang menang;
            baris di bawah dipertahankan sinkron sebagai dokumentasi.

Partisi super TIDAK diubah ukurannya (5600MB); Artix di dalamnya ext4 RW.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bootimg_tool as bt

# Harus SINKRON dengan CONFIG_CMDLINE di ../enhanced_config.cfg.
CMDLINE = (
    "earlycon console=tty0 console=ttyS1,115200n8 loglevel=8 ignore_loglevel "
    "rdinit=/vxtux/init "
    "loop.max_part=7 printk.devkmsg=on panic=15 "
    "initcall_debug=1 panic_on_warn softlockup_panic=1 hung_task_panic=1 "
    "vxtuxfb=0x9d000000,0x9e000000 "
    "androidboot.console=ttyS1 "
    "androidboot.hardware=ums512_1h10 androidboot.slot_suffix=_a "
    "androidboot.serialno=0123456789ABCDEF androidboot.selinux=permissive"
)
# 2026-10-26: `earlycon` dikembalikan (DT stok: /chosen/stdout-path =
# "serial1:115200n8" -> alias serial1 = /soc/apb@70000000/serial@100000),
# dan `root=PARTLABEL=super rootwait` dibuang: PARTLABEL hanya resolve lewat
# symlink /dev/disk/by-partlabel milik udev, yang tidak ada di initramfs ->
# mount_block_root() menunggu tanpa batas. Root dicari sendiri oleh
# ci_qemu/initramfs/init.c (ext4 >1 GiB) sebelum chroot ke /sbin/init.


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    kernel_path, out = sys.argv[1], sys.argv[2]
    base = "out/base_boot"
    ramdisk_dir = None
    args = sys.argv[3:]
    i = 0
    while i < len(args):
        if args[i] == "--base":
            base = args[i + 1]
            i += 2
        elif args[i] == "--ramdisk":
            ramdisk_dir = args[i + 1]
            i += 2
        else:
            raise SystemExit(f"arg tak dikenal: {args[i]}")

    hdr = open(os.path.join(base, "header.bin"), "rb").read()
    if len(hdr) != 1632:
        raise SystemExit(f"header bukan v0 ({len(hdr)} byte) -- periksa basis")
    page = struct.unpack("<I", hdr[36:40])[0]
    kern = open(kernel_path, "rb").read()
    # ARM64 Image: magic "ARM\x64" di offset 0x38
    magic = struct.unpack("<I", kern[0x38:0x3C])[0] if len(kern) > 0x3C else 0
    if magic != 0x644D5241:
        print(f"PERINGATAN: magic ARM64 Image tidak cocok (0x{magic:08x}) -- "
              f"lanjut, tapi cek kernel-nya!")
    # --- RELAY 2MB (2026-10-26, bedah out/logs/artix13) -------------------
    # LK Unisoc TIDAK membaca text_offset dari Image; ia selalu memuat kernel
    # ke 0x80080000 (konstanta tetap, log: "[start_linux] start linux at
    # PC:0x80080000, dt_addr:0x9df00000").  Header Image kernel modern
    # (>=5.x) berisi text_offset = 0, dan
    # Documentation/arch/arm64/booting.rst:138 mewajibkan:
    #   "The Image must be placed text_offset bytes from a 2MB aligned base
    #    address anywhere in usable system RAM and called there."
    # Dengan text_offset 0, Image HARUS mendarat di alamat 2MB-aligned.
    # 0x80080000 = 0x80000000 + 0x80000 -> BUKAN 2MB-aligned, jadi pemetaan
    # awal/MMU salah 512KB dan kernel mati senyap: tidak ada satu baris pun
    # di layar maupun partisi cache (bukti: kernel_size kita 42209792 dibaca
    # benar 19x, nol jejak 6.18.52 di cache.bin).  Kernel stok 5.4 aman
    # karena Image-nya ber-text_offset 0x80000 (0x80000000 + 0x80000).
    #
    # Perbaikan tanpa menyentuh LK: awali blob kernel dengan satu instruksi
    # `b`.  LK menyalin blob ke 0x80080000, relay melompat ke 0x80200000, dan
    # Image asli ditaruh RELAY_OFF di dalam blob sehingga mendarat tepat di
    # 0x80200000 (2MB-aligned).  x0 (DTB) dan seluruh register tidak diubah.
    LK_LOAD = 0x80080000
    RELAY_AT = 0x80200000
    relay_off = RELAY_AT - LK_LOAD              # 0x180000 (1.5 MiB, page-aligned)
    relay_hdr = bytearray(kern[:64])
    struct.pack_into("<I", relay_hdr, 0,
                     0x14000000 | ((relay_off >> 2) & 0x03FFFFFF))  # b 0x80200000
    struct.pack_into("<I", relay_hdr, 4, 0xD503201F)                  # nop
    struct.pack_into("<Q", relay_hdr, 8, 0)                           # text_offset
    struct.pack_into("<Q", relay_hdr, 16, relay_off + len(kern))      # image_size
    struct.pack_into("<Q", relay_hdr, 24, 0xA)                        # flags
    struct.pack_into("<I", relay_hdr, 56, 0x644D5241)                 # "ARM\x64"
    kern = bytes(relay_hdr) + b"\0" * (relay_off - len(relay_hdr)) + kern
    print(f"relay: blob mulai `b 0x{RELAY_AT:x}` di 0x{LK_LOAD:x}; "
          f"Image asli di blob+0x{relay_off:x} -> 0x{RELAY_AT:x}")

    rd = b""
    if ramdisk_dir:
        p = os.path.join(ramdisk_dir, "ramdisk")
        rd = open(p, "rb").read()
        print(f"ramdisk: {p} ({len(rd)} byte)")
    if not rd:
        # LK Unisoc v4: ramdisk_size = 0 di offset 12 -> LK mencari ramdisk
        # di partisi init_boot.  Partisi init_boot perangkat ini TIDAK
        # berisi boot image v4 yang valid -> "bad init boot image header,
        # give up boot!" -> thread kernel_dtb_parse panic -> bootloop tanpa
        # pernah menjalankan kernel (bukti: uboot_log artix6/7, 21 panic
        # beruntun "parser init boot image header_v4 fail").  Karena itu
        # ramdisk WAJIB ada; pakai --ramdisk out/artix_ramdisk (init
        # diagnostik /vxtux/init, hasil compile ci_qemu/initramfs/init.c).
        raise SystemExit(
            "ramdisk kosong -- LK akan panic parse init_boot (bootloop). "
            "Berikan --ramdisk out/artix_ramdisk")

    buf = bytearray(hdr)
    buf[8:48] = struct.pack(
        "<10I", len(kern),
        struct.unpack("<I", hdr[12:16])[0],
        len(rd), struct.unpack("<I", hdr[20:24])[0],
        0, struct.unpack("<I", hdr[28:32])[0],
        struct.unpack("<I", hdr[32:36])[0], page,
        struct.unpack("<I", hdr[40:44])[0],
        struct.unpack("<I", hdr[44:48])[0])
    buf[64:576] = CMDLINE.encode()[:511].ljust(512, b"\0")

    # --- FIX header v4 (bedah dump out/logs/artix5, 2026-10-26) ------------
    # LK Unisoc mem-parse boot image ini dengan tata letak v3/v4, BUKAN v0
    # yang dipakai penulisan di atas.  Bukti dari uboot_log:
    #   * stok : header_version 4, kernel 38085120, ramdisk 1297306
    #            -> 1297306 ada di offset 12  (v4: ramdisk_size)
    #   * kita: header_version 4, ramdisk -2146959360 = 0x80080000
    #            -> nilainya KERNEL_ADDR kita, juga offset 12
    # Jadi offset 12 = kernel_addr (v0) di sini tertangkap LK sebagai
    # ramdisk_size 2.148.007.936 byte.  LK lalu "memuat" initrd fiktif
    # 2.1 GB ke 0xa15bc362 (0xa0000000 + vendor ramdisk) dan
    # meneruskannya ke kernel lewat linux,initrd-start/end -> kernel
    # membuang-buang waktu meng-unpack RAM acak (log LK: "boot_a ramdisk
    # read OK, size = -2146959360, locate to 0xa15bc362").
    # Samakan makna offset 12 dengan versi yang diklaim: ramdisk_size.
    # (kernel_addr di 0x80080000 terbukti TIDAK dipakai LK -- stok juga
    #  memuat kernel ke 0x80080000 padahal offset 12 stok = 1297306.)
    struct.pack_into("<I", buf, 12, len(rd))

    outb = bytearray(buf)
    # Header v0 hanya 1632 byte; bootloader membaca kernel mulai offset
    # page_size (4096). Tanpa padding ini kernel tergeser 0x9A0 byte.
    outb.extend(b"\0" * (bt.align(len(buf), page) - len(buf)))

    def add(blob):
        if not blob:
            return
        # Setiap komponen (header, kernel, ramdisk) harus mulai tepat di
        # batas page -- LK membaca kernel dari offset page_size (4096).
        # Tanpa padding INI, kernel menempel di offset 0x660 dan LK memuat
        # kernel geser 0x9A0 byte -> entry mendarat di pad NOP -> bootloop
        # (bukti: ELR 0x80177288 = brk #0x800 di uboot_log, 2026-10-25).
        outb.extend(b"\0" * (bt.align(len(outb), page) - len(outb)))
        outb.extend(blob)
        outb.extend(b"\0" * (bt.align(len(blob), page) - len(blob)))

    add(kern)
    add(rd)
    assert len(outb) % page == 0
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    open(out, "wb").write(bytes(outb))
    print(f"ditulis {out}: {len(outb)} byte "
          f"(kernel={len(kern)}, ramdisk={len(rd)}, page={page})")
    print(f"cmdline: {CMDLINE}")


if __name__ == "__main__":
    main()
