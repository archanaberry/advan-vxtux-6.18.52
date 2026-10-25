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
    "console=tty0 console=ttyS1,115200n8 loglevel=8 ignore_loglevel "
    "rdinit=/vxtux/init "
    "root=PARTLABEL=super rootfstype=ext4 rootwait rw init=/sbin/init "
    "loop.max_part=7 printk.devkmsg=on panic=15 androidboot.console=ttyS1 "
    "androidboot.hardware=ums512_1h10 androidboot.slot_suffix=_a "
    "androidboot.serialno=0123456789ABCDEF androidboot.selinux=permissive"
)


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
    rd = b""
    if ramdisk_dir:
        p = os.path.join(ramdisk_dir, "ramdisk")
        rd = open(p, "rb").read()
        print(f"ramdisk: {p} ({len(rd)} byte)")
    else:
        print("ramdisk: kosong (tanpa initramfs -> root langsung dari cmdline)")

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
