#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""baca_bundle44.py -- baca satu dump mentah common_rs1_a.bin.

init artix44 mengekspor seluruh bundle log (transkrip, tonggak, deferred,
error, device tanpa driver, partisi, dmesg) ke kanal mentah common_rs1_a
sebagai SATU berkas teks, dipisah baris "===== nama =====".

Jadi tidak perlu parser ext4: cukup

    spd_dump ... read_part common_rs1_a 0 0x800000 common_rs1_a.bin
    python baca_bundle44.py logs\\vb44\\common_rs1_a.bin

Pemakaian:
    python baca_bundle44.py <common_rs1_a.bin> [nama-bagian ...]

Tanpa argumen nama, semua bagian dicetak ringkas: jumlah baris + 40 baris
pertama tiap bagian.  Bagian yang disebut namanya dicetak penuh.
"""
import io
import re
import sys

MARK = re.compile(rb'^===== (\S+) =====$', re.M)


def sections(blob):
    marks = list(MARK.finditer(blob))
    out = []
    for i, m in enumerate(marks):
        end = marks[i + 1].start() if i + 1 < len(marks) else len(blob)
        out.append((m.group(1).decode('latin-1'), blob[m.end():end]))
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = sys.argv[1]
    want = [a.lower() for a in sys.argv[2:]]
    blob = io.open(path, 'rb').read()

    if b'### VXTUXRS1' not in blob[:64]:
        print('[X] bukan kanal BLDB: kepala bukan "### VXTUXRS1"')
        print('    (boot tidak pernah sampai init, atau partisi lain)')
    secs = sections(blob)
    if not secs:
        print('[X] tidak ada penanda "===== nama =====" di berkas ini')
        return 1

    # ringkasan dulu
    print('=' * 70)
    print(' BUNDLE LOG  %s  (%d byte, %d bagian)' % (path, len(blob), len(secs)))
    print('=' * 70)
    for name, data in secs:
        lines = data.count(b'\n')
        print('  %-22s %6d baris  %8d byte' % (name, lines, len(data)))
    print()

    for name, data in secs:
        text = data.decode('latin-1')
        full = (not want) or (name.lower() in want)
        print('=' * 70)
        print(' %s' % name)
        print('=' * 70)
        if full and not want:
            body = text.splitlines()
            for line in body[:40]:
                print(line)
            if len(body) > 40:
                print('   ... (%d baris lagi; sebutkan namanya untuk cetak penuh)'
                      % (len(body) - 40))
        else:
            print(text.rstrip())
        print()

    # ringkasan yang paling sering dicari
    joined = b'\n'.join(d for _, d in secs).decode('latin-1')
    print('=' * 70)
    print(' RINGKASAN')
    print('=' * 70)
    for label, key in (('deferred probe', 'deferred probe'),
                       ('probe pending', 'probe pending'),
                       ('-517 (EPROBE_DEFER)', '-517'),
                       ('-110 (timeout)', '-110'),
                       ('panic / Oops', 'Kernel panic'),
                       ('storage ready', 'storage ready'),
                       ('chroot OK', 'chroot OK'),
                       ('BLDB UDC', 'UDC found'),
                       ('BLDB no UDC', 'no UDC yet'),
                       ('DT model kita', 'Advan Tab VX Lite T1030')):
        print('  %-22s %d kali' % (label, joined.count(key)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
