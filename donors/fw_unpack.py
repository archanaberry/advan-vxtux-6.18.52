#!/usr/bin/env python3
"""Bongkar image Android boot/vendor_boot T1030 tanpa butuh `unpack_bootimg`.

Mesin ini tidak punya unpack_bootimg/dtc, jadi pendekatannya langsung ke
struktur yang sebenarnya dipakai:

  1. Baca magic di offset 0. `ANDROID!` = boot image, `VNDRBOOT` = vendor_boot.
  2. Kalau header bisa diparse (v3/v4), pakai tabel ramdisk di header.
     Kalau tidak (header vendor terpotong / versi aneh), fallback: scan seluruh
     file untuk signature kompresi (gzip 1f 8b, xz fd 37 7a 58 5a, lz4 04 22 4d 18)
     dan dekompres tiap stream yang valid. Stream kompresi bersifat
     self-terminating, jadi cara ini aman untuk ramdisk yang tidak punya tabel.
  3. Hasil dekompresi yang diawali `070701`/`070702` adalah cpio newc ->
     daftar entri (dan ekstrak kalau diminta).

Pakai:
    python donors/fw_unpack.py <image> --list
    python donors/fw_unpack.py <image> --extract <dir>
"""
import argparse
import bz2
import gzip
import io
import lzma
import os
import struct
import sys

MAGICS = {
    b"\x1f\x8b\x08": "gzip",
    b"\xfd7zXZ\x00": "xz",
    b"\x04\x22\x4d\x18": "lz4-frame",
    b"\x02\x21\x4c\x18": "lz4-legacy",
    b"\x28\xb5\x2f\xfd": "zstd",
}


def find_streams(blob, min_len=64):
    """Yield (offset, kind) untuk setiap stream kompresi yang bisa didekompres."""
    for off in range(0, len(blob) - 8):
        head = blob[off:off + 6]
        for sig, kind in MAGICS.items():
            if head.startswith(sig):
                yield off, kind
                break


def lz4_legacy_decompress(data):
    """LZ4 *legacy* frame (magic 02 21 4c 18) murni Python.

    Format legacy: magic 4 byte, lalu berulang [len u32-LE][blok terkompresi].
    Tidak ada penanda akhir; decode berhenti saat sisa < 4 byte atau len == 0.
    Blok LZ4 sendiri: token, literals, offset u16, match (boleh overlap).
    """
    if not data.startswith(b"\x02\x21\x4c\x18"):
        return None
    out = bytearray()
    p = 4
    n = len(data)
    while p + 4 <= n:
        (blen,) = struct.unpack("<I", data[p:p + 4])
        p += 4
        if blen == 0 or p + blen > n:
            break
        src = data[p:p + blen]
        p += blen
        i = 0
        m = len(src)
        while i < m:
            token = src[i]
            i += 1
            lit = token >> 4
            if lit == 15:
                while True:
                    b = src[i]
                    i += 1
                    lit += b
                    if b != 255:
                        break
            out += src[i:i + lit]
            i += lit
            if i + 2 > m:
                break
            off = src[i] | (src[i + 1] << 8)
            i += 2
            mlen = token & 0x0F
            if mlen == 15:
                while True:
                    b = src[i]
                    i += 1
                    mlen += b
                    if b != 255:
                        break
            mlen += 4
            start = len(out) - off
            if start < 0:
                raise ValueError("lz4: offset melebihi output")
            if off >= mlen:
                out += out[start:start + mlen]
            else:
                pattern = bytes(out[start:start + off])
                out += (pattern * (mlen // off + 1))[:mlen]
    return bytes(out)


def decompress(blob, off, kind, size=None):
    data = blob[off:off + size] if size else blob[off:]
    try:
        if kind == "gzip":
            return gzip.GzipFile(fileobj=io.BytesIO(data)).read()
        if kind == "xz":
            return lzma.LZMADecompressor().decompress(data)
        if kind == "bz2":
            return bz2.BZ2Decompressor().decompress(data)
        if kind == "lz4-legacy":
            return lz4_legacy_decompress(data)
    except Exception as e:  # noqa: BLE001 - dilaporkan, bukan ditelan
        print(f"  ! gagal dekompres {kind} @0x{off:x}: {e}")
        return None
    return None  # zstd: butuh library eksternal


def cpio_entries(raw):
    """Iterasi entri cpio newc: (name, mode, data)."""
    p = 0
    while p + 110 <= len(raw):
        if raw[p:p + 6] not in (b"070701", b"070702"):
            break
        fields = [int(raw[p + 6 + i * 8:p + 14 + i * 8], 16) for i in range(13)]
        mode = fields[1]
        size = fields[6]
        namesize = fields[11]
        name = raw[p + 110:p + 110 + namesize - 1].decode("utf-8", "replace")
        hdr = 110 + namesize
        hdr = (hdr + 3) & ~3
        data_off = p + hdr
        data = raw[data_off:data_off + size]
        yield name, mode, data
        p = data_off + size
        p = (p + 3) & ~3
        if name == "TRAILER!!!":
            break


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--extract")
    ap.add_argument("--grep", default=".ko")
    a = ap.parse_args()

    blob = open(a.image, "rb").read()
    magic = blob[:8]
    print(f"image    : {a.image}  ({len(blob):,} bytes)")
    print(f"magic    : {magic!r}")

    streams = []
    # vendor_boot v4: ramdisk mulai di page_size, panjangnya vendor_ramdisk_size
    # (header_size 2128 -> tabel vendor ramdisk menyusul setelah ramdisk).
    if magic == b"VNDRBOOT":
        page = struct.unpack("<I", blob[12:16])[0] or 4096
        size = struct.unpack("<I", blob[24:28])[0]
        head = blob[page:page + 8]
        for sig, kind in MAGICS.items():
            if head.startswith(sig):
                out = decompress(blob, page, kind, size)
                if out:
                    streams.append((page, kind, out))
                    print(f"stream   : off=0x{page:x} {kind:10s} -> {len(out):,} bytes "
                          f"(ramdisk header v4)")
                break

    if not streams:
        for off, kind in find_streams(blob):
            out = decompress(blob, off, kind)
            if out and len(out) > 1024:
                streams.append((off, kind, out))
                print(f"stream   : off=0x{off:x} {kind:10s} -> {len(out):,} bytes")
                if len(streams) >= 8:
                    break

    total_ko = 0
    for off, kind, raw in streams:
        ents = list(cpio_entries(raw))
        if not ents:
            continue
        ko = [(n, m, d) for n, m, d in ents if n.endswith(a.grep)]
        total_ko += len(ko)
        print(f"cpio     : off=0x{off:x} {len(ents)} entri, {len(ko)} cocok '{a.grep}'")
        if a.list:
            for n, m, d in ents:
                print(f"           {m:06o} {len(d):9d}  {n}")
        if a.extract:
            base = os.path.join(a.extract, f"ramdisk_{off:x}")
            skipped = 0
            for n, m, d in ents:
                if n == "TRAILER!!!":
                    continue
                rel = n[2:] if n.startswith("./") else n
                dest = os.path.join(base, rel)
                fmt = m & 0o170000
                try:
                    if fmt == 0o120000:  # symlink: isinya target, bukan file biasa
                        continue
                    if fmt == 0o040000 or rel.endswith("/"):
                        os.makedirs(dest, exist_ok=True)
                        continue
                    os.makedirs(os.path.dirname(dest) or base, exist_ok=True)
                    with open(dest, "wb") as f:
                        f.write(d)
                    os.chmod(dest, m & 0o7777)
                except (FileExistsError, NotADirectoryError, PermissionError, OSError):
                    # bentrok nama file<->direktori atau chmod tak didukung Windows
                    skipped += 1
            print(f"extract  : {base} (dilewati {skipped} entri bentrok/aneh)")
    print(f"TOTAL .ko: {total_ko}")


if __name__ == "__main__":
    sys.exit(main())
