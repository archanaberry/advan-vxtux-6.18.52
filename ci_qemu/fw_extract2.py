#!/usr/bin/env python3
"""Bongkar ramdisk stok (LZ4 legacy) + DTB dari backup firmware ADVAN VX Lite.

  python fw_extract2.py <dir-backup> <outdir>
"""
import os
import struct
import sys

import lz4.block

PAGE = 4096


def align(n):
    return (n + PAGE - 1) // PAGE * PAGE


def lz4_legacy_decode(blob):
    """Format 'lz4 -l' (legacy): magic 0x184C2102 lalu blok [u32 LE size][data]."""
    if blob[:4] != bytes.fromhex("02214c18"):
        raise ValueError("bukan lz4 legacy: " + blob[:4].hex())
    p, out, blocks = 4, bytearray(), 0
    while p + 4 <= len(blob):
        (sz,) = struct.unpack("<I", blob[p:p + 4])
        p += 4
        if sz == 0 or p + sz > len(blob):
            break
        block = blob[p:p + sz]
        p += sz
        blocks += 1
        dec = None
        for guess in (len(block) * 8, 1 << 20, 4 << 20, 8 << 20, 16 << 20, 32 << 20):
            try:
                dec = lz4.block.decompress(block, uncompressed_size=guess)
                break
            except Exception:
                continue
        if dec is None:
            raise ValueError(f"blok {blocks} gagal didekompresi")
        out += dec
    return bytes(out), blocks


def cpio_list(cpio, limit=40, prefix_filter=None):
    p, total = 0, 0
    names = []
    while p + 110 <= len(cpio):
        if cpio[p:p + 6] != b"070701":
            break
        vals = [int(cpio[p + 6 + i * 8:p + 14 + i * 8], 16) for i in range(13)]
        size, namesize = vals[6], vals[11]
        name = cpio[p + 110:p + 110 + namesize - 1].decode("utf-8", "replace")
        p += 110 + namesize
        p = (p + 3) & ~3
        p = (p + size + 3) & ~3
        if name == "TRAILER!!!":
            break
        total += 1
        names.append((name, size, vals[1]))
    print(f"    {total} entri cpio")
    shown = 0
    for name, size, mode in names:
        if prefix_filter and not name.startswith(prefix_filter):
            continue
        print(f"      {name:60s} {size:9d}")
        shown += 1
        if shown >= limit:
            print("      ...")
            break
    return names


def ko_summary(names):
    kos = [(n, s) for n, s, _m in names if n.endswith(".ko")]
    print(f"    modul .ko: {len(kos)}, total {sum(s for _n, s in kos) / 1048576:.1f} MB")
    for n, s in kos[:25]:
        print(f"      {os.path.basename(n):45s} {s / 1024:8.0f} KB")


def main(src, out):
    os.makedirs(out, exist_ok=True)
    b = open(os.path.join(src, "boot_a.bin"), "rb").read()
    ks, rs = struct.unpack("<II", b[8:16])
    hsize = struct.unpack("<I", b[20:24])[0]
    r_off = align(align(hsize) + align(ks))
    ram = b[r_off:r_off + rs]
    print(f"boot_a: kernel={ks} ramdisk={rs} @0x{r_off:x}")
    cpio, nb = lz4_legacy_decode(ram)
    open(os.path.join(out, "boot_ramdisk.cpio"), "wb").write(cpio)
    print(f"  dekompres {nb} blok -> {len(cpio)} byte; magic={cpio[:6]!r}")
    cpio_list(cpio, limit=25)

    v = open(os.path.join(src, "vendor_boot_a.bin"), "rb").read()
    vsize = struct.unpack("<I", v[24:28])[0]
    vrd = v[PAGE:PAGE + vsize]
    print(f"vendor_boot: vendor_ramdisk={vsize} @0x{PAGE:x}")
    vcpio, nb = lz4_legacy_decode(vrd)
    open(os.path.join(out, "vendor_ramdisk.cpio"), "wb").write(vcpio)
    print(f"  dekompres {nb} blok -> {len(vcpio)} byte; magic={vcpio[:6]!r}")
    names = cpio_list(vcpio, limit=15)
    ko_summary(names)

    dtb_off = v.find(b"\xd0\x0d\xfe\xed")
    dtb_sz = struct.unpack("<I", v[2100:2104])[0]
    print(f"DTB @0x{dtb_off:x} size={dtb_sz}")
    open(os.path.join(out, "stock.dtb"), "wb").write(v[dtb_off:dtb_off + dtb_sz])
    print(f"  ditulis {out}/stock.dtb ({dtb_sz} byte)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2])
