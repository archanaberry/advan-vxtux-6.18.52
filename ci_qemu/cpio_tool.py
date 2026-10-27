#!/usr/bin/env python3
"""cpio newc (070701) lister/extractor/packer -- untuk initramfs VXTux.

  python cpio_tool.py list  ramdisk
  python cpio_tool.py extract ramdisk outdir
  python cpio_tool.py pack  indir out.cpio
"""
import os
import stat
import sys

MAGIC = b"070701"


def _fields(buf, off):
    hdr = buf[off:off + 110]
    if len(hdr) < 110 or hdr[:6] != MAGIC:
        return None
    vals = [int(hdr[6 + i * 8:14 + i * 8], 16) for i in range(13)]
    return vals  # ino, mode, uid, gid, nlink, mtime, filesize, devmaj, devmin, rdevmaj, rdevmin, namesize, check


def list_archive(path, verbose=True):
    buf = open(path, "rb").read()
    off, entries = 0, 0
    while off + 110 <= len(buf):
        v = _fields(buf, off)
        if v is None:
            break
        namesize, filesize = v[11], v[6]
        name = buf[off + 110:off + 110 + namesize - 1].decode("utf-8", "replace")
        off += 110 + namesize
        off = (off + 3) & ~3
        data_off = off
        off = (off + filesize + 3) & ~3
        if name == "TRAILER!!!":
            break
        entries += 1
        if verbose:
            mode = v[1]
            perms = stat.filemode(mode)
            print(f"{perms:11s} {filesize:9d}  {name}")
    print(f"-- {entries} entri, {len(buf)} byte --")
    return entries


def extract(path, outdir):
    buf = open(path, "rb").read()
    off, count = 0, 0
    while off + 110 <= len(buf):
        v = _fields(buf, off)
        if v is None:
            break
        (ino, mode, uid, gid, nlink, mtime, filesize, dmaj, dmin,
         rmaj, rmin, namesize, chk) = v
        name = buf[off + 110:off + 110 + namesize - 1].decode("utf-8", "replace")
        off += 110 + namesize
        off = (off + 3) & ~3
        data = buf[off:off + filesize]
        off = (off + filesize + 3) & ~3
        if name == "TRAILER!!!":
            break
        count += 1
        target = os.path.join(outdir, name.lstrip("/"))
        os.makedirs(os.path.dirname(target) or outdir, exist_ok=True)
        if stat.S_ISDIR(mode):
            os.makedirs(target, exist_ok=True)
        elif stat.S_ISLNK(mode):
            try:
                os.symlink(data.decode(), target)
            except OSError:
                pass
        elif stat.S_ISREG(mode):
            with open(target, "wb") as f:
                f.write(data)
            os.chmod(target, stat.S_IMODE(mode) | 0o600)
    print(f"diekstrak {count} entri ke {outdir}")


def _pad(s, n=4):
    return s + b"\0" * ((n - len(s) % n) % n)


def pack(indir, out):
    entries = []
    for root, dirs, files in os.walk(indir):
        for name in dirs + files:
            full = os.path.join(root, name)
            rel = os.path.relpath(full, indir)
            arc = "./" + rel.replace(os.sep, "/")
            entries.append((arc, full))
    entries.sort()
    entries.insert(0, (".", indir))
    blob = bytearray()
    ino = 1
    for arc, full in entries:
        st = os.lstat(full)
        data = b""
        mode = st.st_mode
        if stat.S_ISREG(mode):
            data = open(full, "rb").read()
        elif stat.S_ISLNK(mode):
            data = os.readlink(full).encode()
        name = arc.encode() + b"\0"
        hdr = MAGIC + b"".join(f"{x:08x}".encode() for x in (
            ino, mode, 0, 0, 1, int(st.st_mtime), len(data),
            0, 0, 0, 0, len(name), 0))
        blob += _pad(hdr + name)
        blob += _pad(bytes(data))
        ino += 1
    name = b"TRAILER!!!\0"
    hdr = MAGIC + b"".join(f"{x:08x}".encode() for x in (0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, len(name), 0))
    blob += _pad(hdr + name)
    open(out, "wb").write(bytes(blob))
    print(f"pack {len(entries)} entri -> {out} ({len(blob)} byte)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    if sys.argv[1] == "list":
        list_archive(sys.argv[2])
    elif sys.argv[1] == "extract":
        extract(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == "pack":
        pack(sys.argv[2], sys.argv[3])
