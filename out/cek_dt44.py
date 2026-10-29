#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cek_dt44.py -- periksa hasil dump artix44 tanpa perlu menafsirkan manual.

    python cek_dt44.py <folder-log>

Yang diperiksa:
  1. vb_dtb.bin  : FDT yang benar-benar ada di vendor_boot_a @0x15be040.
                   Harus memuat model kita, sprd,sc-id stok, dan record-size
                   (bukti DT kita dipakai LK, bukan DT stok).
  2. boot_a.bin  : prefix md5 harus sama dengan boot_artix44.md5
                   (bukti image artix44 mendarat, bukan image lama).
  3. metadata.bin: ada tidaknya blob FBE (dilaporkan terpisah oleh
                   fbe_report.py, jadi di sini hanya disebut ada/tidak).

Keluaran memakai kata PASS/GAGAL supaya bisa dibaca sekilas.
"""
import hashlib
import io
import os
import struct
import sys

FDT_MAGIC = b'\xd0\x0d\xfe\xed'


def root_props(blob, off):
    """Ambil properti node root dari FDT (model, compatible, sprd,sc-id)."""
    if blob[off:off + 4] != FDT_MAGIC:
        return {}
    total, off_struct, off_strings = struct.unpack_from('>3I', blob, off + 4)
    strings = blob[off + off_strings:off + total]
    p = off + off_struct
    tk, = struct.unpack_from('>I', blob, p)
    p += 4
    if tk != 1:
        return {}
    end = blob.find(b'\0', p)
    p = (end + 4) & ~3
    props = {}
    while p + 4 <= off + total:
        tk, = struct.unpack_from('>I', blob, p)
        p += 4
        if tk in (2, 9):
            break
        if tk == 4:
            continue
        if tk != 3:
            break
        ln, nm = struct.unpack_from('>2I', blob, p)
        p += 8
        val = blob[p:p + ln]
        p = (p + ln + 3) & ~3
        e = strings.find(b'\0', nm)
        name = strings[nm:e if e >= 0 else len(strings)]
        props[name.decode('latin-1', 'replace')] = val
    return props


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    log = sys.argv[1]
    ok = True

    print('=' * 62)
    print(' PEMERIKSAAN DUMP artix44  (%s)' % log)
    print('=' * 62)

    # ---- 1. DT di vendor_boot -------------------------------------------
    vb = os.path.join(log, 'vb_dtb.bin')
    if not os.path.exists(vb):
        print('[GAGAL] vb_dtb.bin tidak ada (read_part vendor_boot_a gagal?)')
        ok = False
    else:
        blob = io.open(vb, 'rb').read()
        offs = [i for i in range(0, max(1, len(blob) - 4))
                if blob[i:i + 4] == FDT_MAGIC]
        if not offs:
            print('[GAGAL] tidak ada magic FDT di vb_dtb.bin')
            ok = False
        else:
            props = root_props(blob, offs[0])
            model = props.get('model', b'').split(b'\0')[0].decode('latin-1')
            comp = props.get('compatible', b'').replace(b'\0', b' ').decode('latin-1')
            scid = props.get('sprd,sc-id', b'').split(b'\0')[0].decode('latin-1')
            print('  model      : %s' % model)
            print('  compatible : %s' % comp)
            print('  sprd,sc-id : %s' % scid)
            checks = [
                ('model adalah DT kita', 'VXTux' in model),
                ('sprd,sc-id = ums512 1000 1000', scid.strip() == 'ums512 1000 1000'),
                ('ramoops record-size ada', b'record-size' in blob),
                ('rail sc2730 ada', b'sprd,sc2730-regulator' in blob),
            ]
            for label, good in checks:
                print('  [%s] %s' % ('PASS' if good else 'GAGAL', label))
                ok = ok and good

    # ---- 2. boot_a mendarat? --------------------------------------------
    boot = os.path.join(log, 'boot_a.bin')
    md5f = sizef = ''
    probe = os.path.abspath(log)
    for _ in range(5):
        cand = os.path.join(probe, '6.18.52-VXTux-UMS512', 'boot_artix44.md5')
        if os.path.exists(cand):
            md5f = cand
            sizef = cand.replace('.md5', '.size')
            break
        probe = os.path.dirname(probe)
    if not os.path.exists(boot):
        print('[GAGAL] boot_a.bin tidak ada')
        ok = False
    else:
        want = ''
        if os.path.exists(md5f):
            want = io.open(md5f).read().strip().split()[0]
        size = 0
        if os.path.exists(sizef):
            try:
                size = int(io.open(sizef).read().strip())
            except ValueError:
                size = 0
        blob = io.open(boot, 'rb').read()
        if blob[:8] != b'ANDROID!':
            print('[GAGAL] boot_a.bin bukan Android boot image')
            ok = False
        else:
            if not size:
                ks, = struct.unpack_from('<I', blob, 8)
                size = ks + 4096
            got = hashlib.md5(blob[:size]).hexdigest()
            print('  boot md5 (prefix) : %s' % got)
            print('  boot_artix44.md5  : %s' % (want or '(tidak ada)'))
            good = bool(want) and got == want
            print('  [%s] artix44 mendarat' % ('PASS' if good else 'GAGAL'))
            ok = ok and good

    # ---- 3. metadata -----------------------------------------------------
    md = os.path.join(log, 'metadata.bin')
    if os.path.exists(md):
        blob = io.open(md, 'rb').read()
        has = b'keymaster_key_blob' in blob
        print('  [%s] metadata memuat blob FBE (%d byte)'
              % ('PASS' if has else 'INFO', len(blob)))
    print()
    print('HASIL: %s' % ('SEMUA PASS' if ok else 'ADA YANG GAGAL -- lihat di atas'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
