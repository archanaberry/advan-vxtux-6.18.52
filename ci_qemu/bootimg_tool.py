#!/usr/bin/env python3
"""Parse / repack Android boot image (header v0..v4) untuk tablet UMS512.

Pakai:
  python bootimg_tool.py info  boot.img
  python bootimg_tool.py split boot.img outdir
  python bootimg_tool.py pack  outdir boot-new.img --cmdline "..." [--no-ramdisk]
"""
import os
import struct
import sys

BOOT_MAGIC = b"ANDROID!"


def parse(path):
    data = open(path, "rb").read()
    if data[:8] != BOOT_MAGIC:
        raise SystemExit("bukan boot image Android (magic salah)")
    (k_size, k_addr, r_size, r_addr, s_size, s_addr,
     tags_addr, page_size, os_ver, name) = struct.unpack("<10I", data[8:48])
    cmdline = data[64:64 + 512].split(b"\0")[0].decode("utf-8", "replace")
    img_id = data[576:608]
    hdr = {
        "kernel_size": k_size, "kernel_addr": k_addr,
        "ramdisk_size": r_size, "ramdisk_addr": r_addr,
        "second_size": s_size, "second_addr": s_addr,
        "tags_addr": tags_addr, "page_size": page_size,
        "os_version": os_ver, "name": data[48:64].split(b"\0")[0].decode("utf-8", "replace"),
        "cmdline": cmdline, "id": img_id.hex(), "header_version": 2,
        "dtb_size": 0, "dtb_addr": 0, "raw_hdr_size": 1632,
    }
    # header v1/v2: recovery_dtbo + header_version + dtb
    if len(data) >= 1664:
        (rec_dtbo_size,) = struct.unpack("<I", data[1632:1636])
        (hdr_size,) = struct.unpack("<I", data[1636:1640])
        (hdr_version,) = struct.unpack("<I", data[1644:1648])
        (dtb_size,) = struct.unpack("<I", data[1668:1672])
        (dtb_addr,) = struct.unpack("<Q", data[1672:1680])
        hdr.update(header_version=hdr_version, recovery_dtbo_size=rec_dtbo_size,
                   header_size=hdr_size, dtb_size=dtb_size, dtb_addr=dtb_addr,
                   raw_hdr_size=hdr_size if 1632 <= hdr_size <= 4096 else 1632)
    return data, hdr


def info(path):
    data, h = parse(path)
    print(f"file            : {path} ({len(data)} byte)")
    print(f"board/name      : {h['name']!r}")
    print(f"kernel_size     : {h['kernel_size']}  addr=0x{h['kernel_addr']:x}")
    print(f"ramdisk_size    : {h['ramdisk_size']}  addr=0x{h['ramdisk_addr']:x}")
    print(f"second_size     : {h['second_size']}")
    print(f"dtb_size        : {h['dtb_size']}  addr=0x{h.get('dtb_addr',0):x}")
    print(f"header_version  : {h['header_version']}")
    print(f"page_size       : {h['page_size']}")
    print(f"tags_addr       : 0x{h['tags_addr']:x}")
    print(f"cmdline         : {h['cmdline']!r}")
    print(f"id              : {h['id']}")
    return h


def align(n, page):
    return (n + page - 1) // page * page


def split(path, outdir):
    data, h = parse(path)
    os.makedirs(outdir, exist_ok=True)
    page = h["page_size"] if h["page_size"] in (2048, 4096, 8192, 16384) else 2048
    off = align(h["raw_hdr_size"], page)
    parts = {}
    for key, size in (("kernel", h["kernel_size"]), ("ramdisk", h["ramdisk_size"]),
                      ("second", h["second_size"])):
        if size:
            blob = data[off:off + size]
            open(os.path.join(outdir, key), "wb").write(blob)
            parts[key] = size
        off += align(size, page)
    if h.get("recovery_dtbo_size"):
        off += align(h["recovery_dtbo_size"], page)
    if h.get("dtb_size"):
        blob = data[off:off + h["dtb_size"]]
        open(os.path.join(outdir, "dtb"), "wb").write(blob)
        parts["dtb"] = h["dtb_size"]
    hs = h["raw_hdr_size"]
    open(os.path.join(outdir, "header.bin"), "wb").write(data[:hs])
    open(os.path.join(outdir, "cmdline.txt"), "w").write(h["cmdline"])
    for k, v in parts.items():
        print(f"  {k:8s} -> {outdir}/{k}  ({v} byte)")
    print(f"  header   -> {outdir}/header.bin ({hs} byte)")


def pack(outdir, out, cmdline=None, no_ramdisk=False, no_dtb=False):
    hdr = open(os.path.join(outdir, "header.bin"), "rb").read()
    page = struct.unpack("<I", hdr[36:40])[0]
    if page not in (2048, 4096, 8192, 16384):
        page = 2048
    kernel = open(os.path.join(outdir, "kernel"), "rb").read()
    ramdisk = b"" if no_ramdisk else open(os.path.join(outdir, "ramdisk"), "rb").read()
    dtb = b"" if no_dtb else (open(os.path.join(outdir, "dtb"), "rb").read()
                              if os.path.exists(os.path.join(outdir, "dtb")) else b"")
    if cmdline is None:
        cmdline = open(os.path.join(outdir, "cmdline.txt")).read()
    cmd = cmdline.encode()[:511].ljust(512, b"\0")

    buf = bytearray(hdr)
    buf[8:48] = struct.pack("<10I", len(kernel),
                            struct.unpack("<I", hdr[12:16])[0],
                            len(ramdisk), struct.unpack("<I", hdr[20:24])[0],
                            0, struct.unpack("<I", hdr[28:32])[0],
                            struct.unpack("<I", hdr[32:36])[0], page,
                            struct.unpack("<I", hdr[40:44])[0],
                            struct.unpack("<I", hdr[44:48])[0])
    buf[64:576] = cmd
    # LK Unisoc mem-parse boot image dengan tata letak v3/v4 (header_version
    # di offset 40 = 4): offset 12 = ramdisk_size, BUKAN kernel_addr (v0).
    # Nilai 0x80080000 yang tertinggal di sana dibaca LK sebagai ramdisk
    # 2.1 GB (log: "generic ramdisk size is -2146959360", "locate to
    # 0xa15bc362") lalu dikirim ke kernel lewat linux,initrd-start/end.
    # Samakan makna offset 12 dengan versi yang diklaim header. Detail:
    # ci_qemu/mk_boot_artix.py (bedah dump out/logs/artix5, 2026-10-26).
    struct.pack_into("<I", buf, 12, len(ramdisk))
    if len(hdr) >= 1672:
        buf[1668:1672] = struct.pack("<I", len(dtb))

    outb = bytearray(buf)
    # Header WAJIB digenapkan ke page_size dulu. Tanpa ini kernel mulai di
    # offset 1632 (0x660) padahal bootloader membacanya di offset page_size
    # (0x1000) -> kernel tergeser 0x9A0 byte, entry mendarat di tengah kode,
    # LK menangkap brk #0x800 di EL2 (bootloader panic / bootloop).
    outb.extend(b"\0" * (align(len(buf), page) - len(buf)))
    def add(blob):
        outb.extend(blob)
        outb.extend(b"\0" * (align(len(blob), page) - len(blob)))
    add(kernel)
    assert page == 0 or len(outb) % page == 0
    add(ramdisk)
    if dtb:
        add(dtb)
    open(out, "wb").write(bytes(outb))
    print(f"ditulis {out}: {len(outb)} byte, kernel={len(kernel)}, ramdisk={len(ramdisk)}, dtb={len(dtb)}, page={page}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    cmd = sys.argv[1]
    if cmd == "info":
        info(sys.argv[2])
    elif cmd == "split":
        split(sys.argv[2], sys.argv[3])
    elif cmd == "pack":
        indir, out = sys.argv[2], sys.argv[3]
        kw = {}
        args = sys.argv[4:]
        i = 0
        while i < len(args):
            if args[i] == "--cmdline":
                kw["cmdline"] = args[i + 1]; i += 2
            elif args[i] == "--no-ramdisk":
                kw["no_ramdisk"] = True; i += 1
            elif args[i] == "--no-dtb":
                kw["no_dtb"] = True; i += 1
            else:
                i += 1
        pack(indir, out, **kw)
    else:
        raise SystemExit(__doc__)
