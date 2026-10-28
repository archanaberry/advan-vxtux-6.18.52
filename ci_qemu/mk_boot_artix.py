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

    # Boot image v3/v4 (perangkat ini: header_version 4) TIDAK punya field
    # page_size; ukuran halaman selalu 4096 dan header duduk di offset
    # 0..1584, jadi kernel mulai di offset 4096.  LK menentukan lokasi
    # kernel dari header_size (offset 20), bukan dari page_size.
    page = 4096
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

    # --- RELAY STUB: pass-through only --------------------------------------
    # The stub used to paint the bootloader framebuffer with diagnostic
    # bands -- one solid colour before that -- to prove that the handover
    # from LK to our code really happened.  Every one of those tests has
    # passed (ar: kernel reaches fbcon and initcalls), so the stub no longer
    # touches the panel at all: it branches straight to the kernel image.
    #
    # Leaving the framebuffer alone also means the first bootloader text the
    # panel loses is LK's own, cleared by vxtux_lkfb.c when the console takes
    # over - not a test pattern of ours.
    #
    # Requirements for this stub: the MMU may still be off, so it touches no
    # data; x0 (DTB pointer) is preserved; only x2 is clobbered.
    STUB_OFF = 0x40                         # right after the Image header (64 B)

    def _movz(rd, imm16, sf):
        return ((0xD2800000 if sf else 0x52800000) |
                ((imm16 & 0xFFFF) << 5) | rd)

    def _movk(rd, imm16, hw, sf):
        return ((0xF2800000 if sf else 0x72800000) | (hw << 21) |
                ((imm16 & 0xFFFF) << 5) | rd)

    def _mov_imm(rd, val, sf):
        """movz/movk pair for a 32-bit constant."""
        insns = [_movz(rd, val & 0xFFFF, sf)]
        hi = (val >> 16) & 0xFFFF
        if hi:
            insns.append(_movk(rd, hi, 1, sf))
        return insns

    def _passthrough_stub():
        """x2 = 0x80200000 ; br x2 -- two instructions, no painting."""
        return _mov_imm(2, RELAY_AT, 1) + [0xD61F0040]

    stub = _passthrough_stub()

    # Stub kini cuma 2 instruksi (8 B) -- taruh tepat setelah header (0x40)
    # dan arahkan cabang header ke sana (bukan 0x40 yang kini di-padding).
    STUB_REAL_OFF = 0x40
    relay_hdr = bytearray(kern[:64])
    # word0: b ke stub (offset 0x40 -> imm26 = 0x10); x0 (DTB) tidak disentuh.
    struct.pack_into("<I", relay_hdr, 0,
                     0x14000000 | ((STUB_REAL_OFF >> 2) & 0x03FFFFFF))
    struct.pack_into("<I", relay_hdr, 4, 0xD503201F)          # nop (pengaman)
    struct.pack_into("<Q", relay_hdr, 8, 0)                   # text_offset
    struct.pack_into("<Q", relay_hdr, 16, relay_off + len(kern))  # image_size
    struct.pack_into("<Q", relay_hdr, 24, 0xA)                # flags
    struct.pack_into("<I", relay_hdr, 56, 0x644D5241)         # "ARM\x64"
    relay_hdr += b"\0" * (STUB_OFF - len(relay_hdr))
    relay_hdr += b"".join(struct.pack("<I", w) for w in stub)
    assert len(relay_hdr) < relay_off
    kern = bytes(relay_hdr) + b"\0" * (relay_off - len(relay_hdr)) + kern
    print(f"relay: blob mulai `b 0x{LK_LOAD + STUB_REAL_OFF:x}` (stub pass-through) di "
          f"0x{LK_LOAD:x}, lalu `br` ke 0x{RELAY_AT:x}; "
          f"Image asli di blob+0x{relay_off:x}")
    print(f"stub: pass-through, {len(stub)} instructions - no framebuffer "
          f"painting at all")

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

    # --- HEADER v4 disintesis dari nol (bedah boot_a.bin stok, 2026-10-26) --
    # Dump backup stok (backupan-ori-advantabvxlite_t1.1/backup/boot_a.bin):
    #   0000: 41 4e 44 52 4f 49 44 21   "ANDROID!"
    #   0008: 00 22 45 02               kernel_size  = 38085120
    #   000c: 88 6d 16 00               ramdisk_size = 1469832
    #   0010: 76 01 00 1a               os_version   = 0x1a000176
    #   0014: 30 06 00 00               header_size  = 1584   <<-- KUNCI
    #   0018..0027: 00                  reserved[4]
    #   0028: 04 00 00 00               header_version = 4
    #   002c.. : 00                     cmdline[1536] KOSONG (stok memakai
    #                                   /chosen/bootargs dari DTB)
    #
    # Versi lama menyalin header v0 dari out/base_boot/header.bin, sehingga
    # offset 16/20/32/36 berisi sisa field v0 yang salah arti bagi v4:
    #   offset 16 = 293467 (ramdisk_size disalin dua kali -> os_version sampah)
    #   offset 20 = 0x84000000 (tags_addr lama -> jadi header_size!)
    #   offset 32 = 0x86000000, offset 36 = 4096 (reserved menyalah)
    # Stok punya header_size = 1584 = sizeof(boot_img_hdr_v3), sehingga LK
    # membaca kernel dari offset 4096.  Dengan 0x84000000 LK menghitung
    # offset kernel 2,2 GB -- di luar partisi boot_a (64 MB) -- dan yang
    # berakhir di 0x80080000 bukan kernel kita.  Itulah sebabnya tidak ada
    # satu instruksi pun yang pernah berjalan: layar diam di logo LK,
    # partisi cache tak tersentuh, walau LK mencetak
    # "boot_a kernel read OK, page size = 0x29c2000, locate to 0x80080000".
    V4_HEADER_SIZE = 1584
    buf = bytearray(V4_HEADER_SIZE)
    buf[0:8] = b"ANDROID!"
    struct.pack_into("<I", buf, 8, len(kern))          # kernel_size
    struct.pack_into("<I", buf, 12, len(rd))           # ramdisk_size
    struct.pack_into("<I", buf, 16, 0)                 # os_version
    struct.pack_into("<I", buf, 20, V4_HEADER_SIZE)    # header_size -> 1584
    struct.pack_into("<I", buf, 40, 4)                 # header_version = 4
    buf[48:64] = b"vxtux-arasan\0\0\0\0"               # name (v0, tak dipakai v4)
    # Cmdline ditaruh di offset 64 karena bukti uboot_log menunjukkan LK
    # membacanya di sana, bukan di offset 44 milik v4:
    #   "android kernel cmdline :earlycon console=tty0 console=ttyS1..."
    # Offset 44..64 sengaja dibiarkan nol supaya bila LK memakai field
    # cmdline v4 ia melihat string kosong -- persis seperti stok.  Kernel
    # sendiri memakai CONFIG_CMDLINE_FORCE=y, jadi cmdline image ini hanya
    # berpengaruh pada LK.
    buf[64:576] = CMDLINE.encode()[:511].ljust(512, b"\0")
    struct.pack_into("<I", buf, 1580, 0)               # signature_size (v4)

    outb = bytearray(buf)
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
