"""Bedah boot image: header ANDROID! + semua FDT di dalamnya.

Dipakai untuk memutuskan apakah LK mengambil DTB dari boot_a (DTB tertanam)
atau dari partisi dtb_a terpisah.
"""
import os
import struct
import sys

MAG = bytes.fromhex('d00dfeed')


def fdt_info(d, off):
    if off + 40 > len(d):
        return None
    totalsize = struct.unpack_from('>I', d, off + 4)[0]
    off_dt_struct = struct.unpack_from('>I', d, off + 8)[0]
    off_strings = struct.unpack_from('>I', d, off + 12)[0]
    version = struct.unpack_from('>I', d, off + 20)[0]
    if off_dt_struct >= len(d) or off_strings >= len(d):
        return {'totalsize': totalsize, 'version': version, 'valid': False}
    # baca compatible pertama dari strings block + struktur
    comp = b''
    p = off + off_dt_struct
    end = off + off_dt_struct + min(totalsize, len(d) - off_dt_struct)
    depth = 0
    try:
        while p < end and len(comp) == 0:
            tok = struct.unpack_from('>I', d, p)[0]
            if tok == 1:  # BEGIN_NODE
                z = d.index(b'\0', p + 4)
                if depth == 0:
                    depth = 1
                p = (z + 4) & ~3
            elif tok == 2:  # END_NODE
                depth -= 1
                p += 4
                if depth <= 0:
                    break
            elif tok == 3:  # PROP
                ln, no = struct.unpack_from('>II', d, p + 4)
                p += 12
                name = d[off + off_strings + no:d.index(b'\0', off + off_strings + no)]
                val = d[p:p + ln]
                p = (p + ln + 3) & ~3
                if name == b'compatible':
                    comp = val.split(b'\0')[0].decode('ascii', 'replace')
                    break
            elif tok == 4:
                p += 4
            else:
                break
    except Exception as e:
        return {'error': str(e)}
    return {'totalsize': totalsize, 'version': version, 'valid': True,
            'compatible': comp}


def main(path):
    d = open(path, 'rb').read()
    print('=' * 74)
    print('%s  (%d byte)' % (os.path.basename(path), len(d)))
    print('=' * 74)
    if d[:8] == b'ANDROID!':
        (kernel_size, kernel_addr, ramdisk_size, ramdisk_addr, second_size,
         second_addr, tags_addr, page_size, header_version) = struct.unpack_from(
             '<9I', d, 8)
        print('  header ANDROID!')
        print('    kernel_size = %d (0x%X)' % (kernel_size, kernel_size))
        print('    ramdisk_size = %d (0x%X)' % (ramdisk_size, ramdisk_size))
        print('    page_size    = %d' % page_size)
        print('    header_ver   = %d' % header_version)
        # header v1 punya dtb_size di offset 0x10? (v2: dtb_size @0x10? cek)
        if header_version >= 1:
            # dtb_size di header v1 = offset 0x10 setelah id... sebenarnya v2+
            pass
        print('    kernel+ramdisk end ~ 0x%X' % (page_size + kernel_size + ramdisk_size))
    else:
        print('  bukan Android boot image:', d[:16])

    hits = []
    i = 0
    while True:
        i = d.find(MAG, i)
        if i < 0:
            break
        hits.append(i)
        i += 1
    print('  FDT magic ditemukan %d kali: %s' %
          (len(hits), ', '.join(hex(h) for h in hits[:8]) or '-'))
    for h in hits[:6]:
        info = fdt_info(d, h)
        if not info:
            continue
        print('    @0x%X  totalsize=%s ver=%s valid=%s compat=%r' %
              (h, info.get('totalsize'), info.get('version'),
               info.get('valid'), info.get('compatible', '?')))


for p in sys.argv[1:]:
    main(p)
