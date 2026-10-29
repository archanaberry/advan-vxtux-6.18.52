#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""fbe_report.py -- bedah /metadata (dump spd_dump) untuk melihat keadaan FBE.

    python fbe_report.py logs/artix41/metadata.bin

Yang dilaporkan: versi kunci metadata, parameter stretching, ukuran blob
keymaster, ukuran kunci terbungkus, dan kesimpulan apakah userdata bisa
dibuka tanpa TEE.

Latar (dari AOSP, bukan tebakan):
  /metadata/vold/metadata_encryption/key/version          1 byte  "1"
  .../key/stretching                                      string  ("nopassword"
                                                          kalau tanpa lockscreen)
  .../key/encrypted_key       AES-GCM(metadata_key)       16 IV + 64 data + 12 tag
  .../key/keymaster_key_blob  kunci pembungkus dari TEE   bergantung Keymaster
  .../key/secdiscardable      salt HKDF 16 KiB

Artinya: kunci mentah TIDAK ada di eMMC.  Yang ada adalah kunci metadata yang
dibungkus (AES-GCM) oleh kunci milik TEE, dan kunci TEE itu sendiri dibungkus
oleh Keymaster TA di dalam Trusty (TOS), yang kuncinya berasal dari rahasia
perangkat (efuse/provisioning).  Tanpa TEE yang berjalan, blob itu hanya
derau acak -- ini batas kriptografis, bukan kekurangan tool.
"""
import hashlib
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
sys.path.insert(0, __file__.rsplit('\\', 1)[0])
from ext4read import Ext4   # noqa: E402

KEYDIR = '/vold/metadata_encryption/key'


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    img = sys.argv[1]
    fs = Ext4(img)
    print('=' * 66)
    print(' FBE REPORT  %s' % img)
    print('=' * 66)
    try:
        rows = fs.listdir(fs.resolve(KEYDIR))
    except SystemExit:
        print('[X] %s tidak ada: partisi metadata belum pernah dienkripsi FBE'
              % KEYDIR)
        return 1
    print('%-22s %8s  %s' % ('file', 'bytes', 'md5'))
    blobs = {}
    for name, ino, _t in sorted(rows):
        data = fs.read_file(ino)
        blobs[name] = data
        print('%-22s %8d  %s' % (name, len(data),
                                 hashlib.md5(data).hexdigest()))

    ver = blobs.get('version', b'')
    stretch = blobs.get('stretching', b'')
    enc = blobs.get('encrypted_key', b'')
    km = blobs.get('keymaster_key_blob', b'')

    print()
    print('version            : %r' % ver)
    print('stretching         : %r' % stretch)
    print('encrypted_key      : %d byte  (16 IV + 64 kunci + 12 tag = %s)'
          % (len(enc), 'cocok' if len(enc) == 92 else 'TIDAK cocok'))
    print('keymaster_key_blob : %d byte  head %s'
          % (len(km), km[:16].hex()))
    print()
    if stretch == b'nopassword':
        print('KESIMPULAN: metadata key TIDAK dilindungi lockscreen '
              '("nopassword"),')
        print('            jadi satu-satunya pembungkus adalah TEE Keymaster.')
    elif stretch:
        print('KESIMPULAN: metadata key dilindungi lockscreen '
              '(stretching=%r): butuh PIN/pattern.' % stretch)
    else:
        print('KESIMPULAN: stretching kosong (metadata belum terenkripsi / '
              'dump tidak konsisten).')
    print()
    print('Untuk membuka /data dari sistem lain dibutuhkan SALAH SATU:')
    print('  a) TEE Trusty + Keymaster TA yang berjalan (TOS di tablet ini),')
    print('  b) kunci fscrypt mentah yang diambil dari Android yang sedang')
    print('     hidup (root), atau')
    print('  c) salinan berkas dari Android yang sedang hidup (tanpa kripto).')
    print('Blob di eMMC saja tidak cukup: %d byte itu tidak bisa dibuka di PC.'
          % len(km))
    return 0


if __name__ == '__main__':
    sys.exit(main())
