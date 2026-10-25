#!/usr/bin/env python3
"""Bongkar firmware stok ADVAN VX Lite (UMS512): boot / init_boot / vendor_boot
/ vbmeta (AVB) / super (LP metadata).

  python fw_unpack.py all  <dir-backup>  <outdir>
  python fw_unpack.py boot <img> <outdir>
  python fw_unpack.py vbmeta <img>
  python fw_unpack.py lp <super.img>
"""
import os
import struct
import sys
import zlib

PAGE_CANDIDATES = (2048, 4096, 8192, 16384)


def align(n, p):
    return (n + p - 1) // p * p


def u32(b, o):
    return struct.unpack("<I", b[o:o + 4])[0]


def u64(b, o):
    return struct.unpack("<Q", b[o:o + 8])[0]


def detect_comp(data):
    if data[:2] == b"\x1f\x8b":
        return "gzip"
    if data[:4] == b"\x04\x22\x4d\x18":
        return "lz4-frame"
    if data[:4] == b"\x02\x21\x4c\x18":
        return "lz4-legacy"
    if data[:3] == b"\x5d\x00\x00":
        return "lzma"
    if data[:4] == b"\xfd7zX":
        return "xz"
    if data[:2] == b"\x1f\x9e":
        return "gzip"
    if data[:5] == b"07070" or b"070701" in data[:64]:
        return "cpio"
    return "raw"


# ---------------------------------------------------------------- boot image
def parse_boot(path):
    b = open(path, "rb").read()
    if b[:8] != b"ANDROID!":
        return None
    hdr_ver = u32(b, 40)
    if hdr_ver >= 3:
        # layout v3/v4: kernel_size, ramdisk_size, os_version, header_size,
        # reserved[4], header_version, cmdline[1536]
        ks, rs, osver, hsize = u32(b, 8), u32(b, 12), u32(b, 16), u32(b, 20)
        page = 4096
        cmdline = b[44:44 + 1536].split(b"\0")[0].decode("utf-8", "replace")
        ka = ra = tags = ss = sa = 0
        rec_dtbo = dtb_sz = dtb_addr = 0
    else:
        (ks, ka, rs, ra, ss, sa, tags, page, osver, _n) = struct.unpack("<10I", b[8:48])
        if page not in PAGE_CANDIDATES:
            page = 4096
        cmdline = b[64:576].split(b"\0")[0].decode("utf-8", "replace")
        rec_dtbo = dtb_sz = dtb_addr = 0
        hsize = 1632
        if len(b) >= 1664:
            rec_dtbo = u32(b, 1632)
            hsize = u32(b, 1636) or 1632
            dtb_sz = u32(b, 1668)
            dtb_addr = u64(b, 1672)
    # payload mulai setelah header (v3/v4: tetap 4096; v0-v2: align page)
    off = align(hsize if hsize >= 1584 else 1632, page)
    parts = {}
    for name, size in (("kernel", ks), ("ramdisk", rs), ("second", ss)):
        if size:
            parts[name] = b[off:off + size]
            off = align(off + size, page)
    if rec_dtbo:
        off = align(off + rec_dtbo, page)
    if dtb_sz:
        parts["dtb"] = b[off:off + dtb_sz]
        off = align(off + dtb_sz, page)
    return {"page": page, "hdr_ver": hdr_ver, "hsize": hsize, "cmdline": cmdline,
            "name": b[48:64].split(b"\0")[0].decode("utf-8", "replace"),
            "kernel_addr": ka, "ramdisk_addr": ra, "tags_addr": tags,
            "parts": parts, "size": len(b)}


def boot_cmd(path, outdir):
    p = parse_boot(path)
    if not p:
        print("  bukan boot image")
        return
    print(f"  page={p['page']} hdr_ver={p['hdr_ver']} hdr_size={p['hsize']} "
          f"name={p['name']!r}")
    print(f"  cmdline: {p['cmdline']}")
    os.makedirs(outdir, exist_ok=True)
    for n, blob in p["parts"].items():
        open(os.path.join(outdir, n), "wb").write(blob)
        print(f"  {n:8s}: {len(blob)} byte ({detect_comp(blob)})")


# ----------------------------------------------------------- vendor boot
def vendor_cmd(path, outdir):
    b = open(path, "rb").read()
    if b[:8] != b"VNDRBOOT":
        print("  bukan vendor_boot")
        return
    hdr_ver = u32(b, 8)
    page = u32(b, 12)
    if page not in PAGE_CANDIDATES:
        page = 4096
    kaddr, raddr, vsize = u32(b, 16), u32(b, 20), u32(b, 24)
    cmdline = b[28:28 + 2048].split(b"\0")[0].decode("utf-8", "replace")
    name = b[2080:2096].split(b"\0")[0].decode("utf-8", "replace")
    hsize, dtb_sz, dtb_addr = u32(b, 2096), u32(b, 2100), u64(b, 2104)
    print(f"  hdr_ver={hdr_ver} page={page} name={name!r}")
    print(f"  vendor_ramdisk_size={vsize} dtb_size={dtb_sz} header_size={hsize}")
    print(f"  cmdline: {cmdline}")
    os.makedirs(outdir, exist_ok=True)
    off = align(hsize if hsize >= 2112 else 2112, page)
    vrd = b[off:off + vsize]
    open(os.path.join(outdir, "vendor_ramdisk"), "wb").write(vrd)
    print(f"  vendor_ramdisk: {len(vrd)} byte ({detect_comp(vrd)})")
    off = align(off + vsize, page)
    if dtb_sz:
        dtb = b[off:off + dtb_sz]
        open(os.path.join(outdir, "dtb"), "wb").write(dtb)
        print(f"  dtb: {len(dtb)} byte ({detect_comp(dtb)})")
        off = align(off + dtb_sz, page)
    if hdr_ver >= 4 and len(b) >= off + 12:
        tsize, tnum, tent, bsz = (u32(b, off), u32(b, off + 4),
                                  u32(b, off + 8), u32(b, off + 12))
        print(f"  table: size={tsize} entries={tnum} entry_size={tent} bootconfig={bsz}")
        tab = b[align(off + 16, page) if False else off + 16: off + 16 + tsize]
        for i in range(min(tnum, 32)):
            e = tab[i * tent:(i + 1) * tent]
            if len(e) < 108:
                break
            vs, vo, ss_, nm = struct.unpack("<IIII", e[:16])
            frag = e[16:16 + 32].split(b"\0")[0].decode("utf-8", "replace")
            print(f"    frag[{i}] name={frag!r} size={vs} off={vo} type={ss_}")


# --------------------------------------------------------------- vbmeta (AVB)
AVB_FLAG_HASHTREE_DISABLED = 1
AVB_FLAG_VERIFICATION_DISABLED = 2


def vbmeta(path):
    b = open(path, "rb").read(256)
    if b[:4] != b"AVB0":
        print(f"  {os.path.basename(path)}: bukan vbmeta (magic={b[:4]!r})")
        return
    maj, mnr = u32(b, 4), u32(b, 8)
    auth_sz, aux_sz = u64(b, 12), u64(b, 20)
    algo = u32(b, 28)
    flags = u32(b, 120)
    rb_idx = u64(b, 112)
    rb_loc = u32(b, 124)
    rel = b[128:128 + 48].split(b"\0")[0].decode("utf-8", "replace")
    fl = []
    if flags & AVB_FLAG_HASHTREE_DISABLED:
        fl.append("HASHTREE_DISABLED")
    if flags & AVB_FLAG_VERIFICATION_DISABLED:
        fl.append("VERIFICATION_DISABLED")
    print(f"  {os.path.basename(path)}: avb{maj}.{mnr} algo={algo} "
          f"auth={auth_sz} aux={aux_sz} rollback={rb_idx}@{rb_loc} "
          f"flags=0x{flags:x} {','.join(fl) or 'verifikasi AKTIF'} rel={rel!r}")


# ------------------------------------------------------------- super LP metadata
LP_GEOM_MAGIC = b"LP_METADATA_GEOMETRY"


def lp(path):
    with open(path, "rb") as f:
        b = f.read(16 * 1024 * 1024)
    geom = b.find(LP_GEOM_MAGIC)
    if geom < 0:
        nz = next((i for i, x in enumerate(b[:4 * 1024 * 1024]) if x), -1)
        print(f"  super: tidak ada LP metadata dalam 16MB pertama; "
              f"byte non-nol pertama @0x{nz:x} (awal: {b[:16].hex()})")
        return
    print(f"  LP geometry @0x{geom:x}")
    struct_size, chksum, meta_max, slots, lb_size = struct.unpack("<IIIII", b[geom + 16:geom + 36])
    print(f"  geometry: struct_size={struct_size} metadata_max={meta_max} "
          f"slots={slots} logical_block={lb_size}")
    hdr = -1
    for off in range(geom + 4096, min(len(b) - 4, geom + 128 * 1024), 4):
        if u32(b, off) == 0x414C5030:
            hdr = off
            break
    if hdr < 0:
        print("  header LP (magic 0x414C5030) tidak ditemukan")
        return
    major, minor, hsize, hchk, tsize, tchk = struct.unpack("<IIIIII", b[hdr + 4:hdr + 28])
    print(f"  header @0x{hdr:x}: versi {major}.{minor} size={hsize} tables={tsize}")
    desc = [struct.unpack("<III", b[hdr + 28 + i * 12: hdr + 40 + i * 12]) for i in range(4)]
    for nm, (o, num, esz) in zip(("partitions", "extents", "groups", "block_devices"), desc):
        print(f"    {nm}: {num} x {esz}B @ +{o}")
    po, pnum, pesz = desc[0]
    bo, bnum, besz = desc[3]
    for i in range(min(pnum, 32)):
        e = b[hdr + po + i * pesz: hdr + po + (i + 1) * pesz]
        if len(e) < 52:
            break
        pname = e[:36].split(b"\0")[0].decode("utf-8", "replace")
        attrs, first_ext, num_ext, group = struct.unpack("<IIII", e[36:52])
        print(f"      {pname:20s} attrs=0x{attrs:x} ext={first_ext}+{num_ext} group={group}")
    for i in range(min(bnum, 8)):
        e = b[hdr + bo + i * besz: hdr + bo + (i + 1) * besz]
        if len(e) < 44:
            break
        bname = e[:36].split(b"\0")[0].decode("utf-8", "replace")
        nsec = u64(e, 36)
        print(f"      dev {bname!r} sectors={nsec} ({nsec * 512 / 1048576:.0f} MB)")


def all_(src, outdir):
    os.makedirs(outdir, exist_ok=True)
    plan = [("boot_a.bin", "boot"), ("boot_b.bin", "boot"),
            ("init_boot_a.bin", "boot"), ("vendor_boot_a.bin", "vb")]
    for fn, kind in plan:
        p = os.path.join(src, fn)
        if not os.path.exists(p):
            continue
        print(f"=== {fn} ===")
        sub = os.path.join(outdir, fn.replace(".bin", ""))
        (boot_cmd if kind == "boot" else vendor_cmd)(p, sub)
    print("=== vbmeta ===")
    for fn in sorted(os.listdir(src)):
        if fn.startswith(("vbmeta", "avbmeta")):
            vbmeta(os.path.join(src, fn))
    print("=== super ===")
    lp(os.path.join(src, "super.bin"))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    cmd = sys.argv[1]
    if cmd == "all":
        all_(sys.argv[2], sys.argv[3])
    elif cmd == "boot":
        boot_cmd(sys.argv[2], sys.argv[3])
    elif cmd == "vb":
        vendor_cmd(sys.argv[2], sys.argv[3])
    elif cmd == "vbmeta":
        vbmeta(sys.argv[2])
    elif cmd == "lp":
        lp(sys.argv[2])
    else:
        raise SystemExit(__doc__)
