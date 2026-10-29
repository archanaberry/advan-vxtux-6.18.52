#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ext4read.py -- pembaca ext4 read-only minimal (tanpa mount, tanpa root).

Dipakai untuk dua hal di proyek ini:

  1. membedah /metadata dari dump spd_dump (blob FBE: keymaster_key_blob),
  2. menarik /cache/vxtux-logs/* dari cache.bin tanpa perlu mount ext4.

Yang didukung: ext4 1K/2K/4K, fitur 64BIT, extent tree (kedalaman berapa pun),
direktori linear.  Tidak didukung: inline_data, htree (baca tetap jalan lewat
blok pertama), file terkompresi, dan jurnal replay (image yang di-dump saat
partisi bersih/konsisten; kalau kotor, baca "sebisanya").

Pemakaian:
    python ext4read.py <image> ls [path]
    python ext4read.py <image> cat <path>
    python ext4read.py <image> get <path> <keluaran>
    python ext4read.py <image> find <potongan-nama>
"""
import io
import struct
import sys

SB_OFF = 1024


class Ext4:
    def __init__(self, path):
        self.f = io.open(path, 'rb')
        self.f.seek(SB_OFF)
        sb = self.f.read(1024)
        if sb[0x38:0x3A] != b'\x53\xef':
            raise SystemExit('bukan ext4: magic superblock tidak ada')
        (self.inodes_count, self.blocks_count_lo, _r, _f, _fi,
         self.first_data_block, self.log_block_size, _cs,
         self.blocks_per_group, _cpg, self.inodes_per_group) = \
            struct.unpack_from('<11I', sb, 0)
        self.inode_size = struct.unpack_from('<H', sb, 0x58)[0]
        self.desc_size = struct.unpack_from('<H', sb, 0xFE)[0]
        feat_incompat = struct.unpack_from('<I', sb, 0x60)[0]
        self.is64 = bool(feat_incompat & 0x80)
        if self.desc_size == 0:
            self.desc_size = 64 if self.is64 else 32
        self.block_size = 1024 << self.log_block_size
        self.ngroups = (self.blocks_count_lo + self.blocks_per_group - 1) \
            // self.blocks_per_group

    def read_block(self, n, count=1):
        self.f.seek(n * self.block_size)
        return self.f.read(self.block_size * count)

    def _inode_table(self, group):
        gd_off = (self.first_data_block + 1) * self.block_size \
            + group * self.desc_size
        self.f.seek(gd_off)
        gd = self.f.read(self.desc_size)
        lo = struct.unpack_from('<I', gd, 8)[0]
        hi = struct.unpack_from('<I', gd, 0x28)[0] if self.is64 and \
            len(gd) >= 0x2C else 0
        return (hi << 32) | lo

    def inode(self, ino):
        if ino < 1:
            raise SystemExit('inode tidak valid: %d' % ino)
        group = (ino - 1) // self.inodes_per_group
        index = (ino - 1) % self.inodes_per_group
        off = self._inode_table(group) * self.block_size \
            + index * self.inode_size
        self.f.seek(off)
        raw = self.f.read(self.inode_size)
        mode = struct.unpack_from('<H', raw, 0)[0]
        size = struct.unpack_from('<I', raw, 4)[0]
        size_hi = struct.unpack_from('<I', raw, 0x6C)[0]
        if self.inode_size > 0x6C and size_hi:
            size |= size_hi << 32
        flags = struct.unpack_from('<I', raw, 0x20)[0]
        i_block = raw[0x28:0x28 + 60]
        return {'ino': ino, 'mode': mode, 'size': size, 'flags': flags,
                'i_block': i_block,
                'is_dir': (mode & 0xF000) == 0x4000,
                'is_reg': (mode & 0xF000) == 0x8000}

    # ---- extent tree -----------------------------------------------------
    def _extents(self, node, out, limit=100000):
        magic, entries, _max, depth = struct.unpack_from('<HHHH', node, 0)
        if magic != 0xF30A:
            return
        p = 12
        for _ in range(entries):
            if depth == 0:
                ee_block, ee_len, start_hi, start_lo = \
                    struct.unpack_from('<IHHI', node, p)
                length = ee_len & 0x7FFF
                if length > 32768:      # unwritten extent
                    length -= 32768
                out.append((ee_block, length, (start_hi << 32) | start_lo))
            else:
                _ei_block, leaf_lo, leaf_hi = \
                    struct.unpack_from('<IIH', node, p)
                child = self.read_block((leaf_hi << 32) | leaf_lo)
                self._extents(child, out)
            p += 12
            if len(out) > limit:
                return

    def read_file(self, ino):
        """Isi inode apa pun (berkas ATAU direktori); direktori dibaca mentah."""
        inode = self.inode(ino)
        if not (inode['is_reg'] or inode['is_dir']):
            return b''
        ext = []
        self._extents(inode['i_block'], ext)
        data = bytearray()
        for _lblk, length, phys in sorted(ext):
            data += self.read_block(phys, length)
        return bytes(data[:inode['size']])

    # ---- direktori -------------------------------------------------------
    def listdir(self, ino):
        inode = self.inode(ino)
        if not inode['is_dir']:
            raise SystemExit('inode %d bukan direktori' % ino)
        raw = self.read_file(ino)
        entries = []
        p = 0
        while p + 8 <= len(raw):
            child, rec_len, name_len, ftype = \
                struct.unpack_from('<IHBB', raw, p)
            if rec_len < 8:
                break
            name = raw[p + 8:p + 8 + name_len]
            if child and name not in (b'.', b'..'):
                entries.append((name.decode('utf-8', 'replace'), child, ftype))
            p += rec_len
        return entries

    def resolve(self, path):
        ino = 2                            # root
        parts = [x for x in path.split('/') if x]
        for part in parts:
            found = None
            for name, child, _t in self.listdir(ino):
                if name == part:
                    found = child
                    break
            if found is None:
                raise SystemExit('tidak ditemukan: /%s' % '/'.join(parts))
            ino = found
        return ino


FTYPE = {1: 'f', 2: 'd', 3: 'c', 4: 'b', 5: 'p', 6: 's', 7: 'l'}


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    fs = Ext4(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == 'ls':
        path = sys.argv[3] if len(sys.argv) > 3 else '/'
        ino = fs.resolve(path)
        rows = fs.listdir(ino)
        print('%-32s %-6s %10s  ino' % ('name', 'type', 'size'))
        for name, child, ftype in sorted(rows):
            try:
                size = fs.inode(child)['size']
            except SystemExit:
                size = 0
            print('%-32s %-6s %10d  %d' % (name, FTYPE.get(ftype, '?'),
                                          size, child))
        print('\n%d entri di %s' % (len(rows), path))
    elif cmd == 'cat':
        data = fs.read_file(fs.resolve(sys.argv[3]))
        sys.stdout.buffer.write(data)
    elif cmd == 'get':
        data = fs.read_file(fs.resolve(sys.argv[3]))
        io.open(sys.argv[4], 'wb').write(data)
        print('ditulis %s (%d byte)' % (sys.argv[4], len(data)))
    elif cmd == 'find':
        needle = sys.argv[3].encode()

        def walk(ino, prefix, depth=0):
            if depth > 6:
                return
            for name, child, ftype in fs.listdir(ino):
                full = prefix + '/' + name
                if needle in name.encode():
                    print(full)
                if ftype == 2:
                    walk(child, full, depth + 1)
        walk(2, '')
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
