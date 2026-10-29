"""Ganti DT dasar (multi-DTB) di dalam dump vendor_boot_a dengan DTB kita.

Kenapa vendor_boot: LK menulis sendiri di uboot_log tablet ini
    vendorbootimage: vendor ramdisk offset is 0x1000
    vendorbootimage: vendor dt size is 117599
    vendorbootimage: vendor dt offset is 0x15be000
    [_boot_load_kernel_ramdisk_image] loading fix dtb ...
    offset = 0x15be040 size=0x1cb1f
    Find kernel dtb prop name sprd,sc-id=..., g_fdt_blob sprd,sc-id=ums512 1h10 1000
    Matches the corresponding dtb in multiple DTB
Jadi DT yang dipakai kernel ada di vendor_boot_a (0x15be040), BUKAN di dtb_a
(dtb_a tablet ini seluruhnya nol).

Pemakaian:
    python patch_vendor_boot.py <vendor_boot_a.bin> <dtb_baru> <keluaran.img>

Blok dtb asli dibiarkan berada di offset yang sama persis (tidak menggeser
vendor_ramdisk_table / bootconfig), jadi tidak ada offset lain yang berubah.
"""
import os
import struct
import sys

FDT_MAGIC = b'\xd0\x0d\xfe\xed'
WD = 1024 * 1024


def find_fdt(b, lo, hi):
    """cari FDT pertama di [lo,hi) yang totalsize-nya masuk akal"""
    p = lo
    while True:
        p = b.find(FDT_MAGIC, p, hi)
        if p < 0:
            return None
        if p + 8 <= hi:
            total = struct.unpack('>I', b[p + 4:p + 8])[0]
            if 64 <= total <= (hi - p):
                return p
        p += 4


def fdt_total(b, off):
    return struct.unpack('>I', b[off + 4:off + 8])[0]


BEGIN_NODE, END_NODE, PROP, NOP, END = 1, 2, 3, 4, 9


def fdt_root_strings(b, off, keys=(b'model', b'compatible', b'sprd,sc-id')):
    """parse blok struktur FDT dan kembalikan properti node root saja"""
    if b[off:off + 4] != FDT_MAGIC:
        return {}
    total, off_struct, off_strings = struct.unpack('>3I', b[off + 4:off + 16])
    strings = b[off + off_strings:off + total]
    p = off + off_struct
    tk, = struct.unpack('>I', b[p:p + 4])
    p += 4
    if tk != BEGIN_NODE:
        return {}
    p = (b.find(b'\0', p) + 4) & ~3  # lewati nama node root ("")
    out = {}
    while p + 4 <= off + total:
        tk, = struct.unpack('>I', b[p:p + 4])
        p += 4
        if tk in (END_NODE, END):
            break
        if tk == NOP:
            continue
        if tk != PROP:
            break
        ln, nm = struct.unpack('>2I', b[p:p + 8])
        p += 8
        val = b[p:p + ln]
        p = (p + ln + 3) & ~3
        if nm > len(strings):
            continue
        e = strings.find(b'\0', nm)
        name = strings[nm:e if e >= 0 else len(strings)]
        if name in keys:
            out[name.decode()] = val.split(b'\0')[0].decode('latin-1', 'replace')
    return out


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    src, dtb, dst = sys.argv[1:4]
    b = bytearray(open(src, 'rb').read())
    new = open(dtb, 'rb').read()
    print("vendor_boot : %s (%d byte)" % (src, len(b)))
    print("DTB baru    : %s (%d byte)" % (dtb, len(new)))

    if b[:8] != b'VNDRBOOT':
        print("[X] magic bukan VNDRBOOT: %r" % bytes(b[:8]))
        return 1
    header_version, page, kern_addr, ramd_addr, vramdisk = struct.unpack('<5I', b[8:28])
    header_size, dtb_size = struct.unpack('<2I', b[2096:2104])
    print("header_version %d | page %d | vendor_ramdisk %d | dtb_size %d"
          % (header_version, page, vramdisk, dtb_size))

    # Layout vendor_boot (dikonfirmasi log LK): header 1 halaman, lalu
    # vendor_ramdisk, lalu blok dtb yang di-align ke page.
    dtb_off = ((page + vramdisk + page - 1) // page) * page
    print("header_size %d | dtb block offset: 0x%x (align(page+vendor_ramdisk))"
          % (header_size, dtb_off))

    lo, hi = dtb_off, dtb_off + dtb_size
    old_fdt = find_fdt(b, lo, hi)
    if old_fdt is None:
        print("[X] tidak ada FDT di rentang 0x%x..0x%x" % (lo, hi))
        return 1
    old_total = fdt_total(b, old_fdt)
    old_info = fdt_root_strings(b, old_fdt)
    new_info = fdt_root_strings(new, 0)
    print("DTB lama    : FDT @0x%x totalsize %d" % (old_fdt, old_total))
    print("   model     : %s" % old_info.get('model'))
    print("   compatible: %s" % old_info.get('compatible'))
    print("   sprd,sc-id: %s" % old_info.get('sprd,sc-id'))
    print("DTB baru    : totalsize %d" % fdt_total(new, 0))
    print("   model     : %s" % new_info.get('model'))
    print("   compatible: %s" % new_info.get('compatible'))
    print("   sprd,sc-id: %s" % new_info.get('sprd,sc-id'))

    if new[:4] != FDT_MAGIC:
        print("[X] berkas DTB baru tidak diawali magic FDT")
        return 1
    new_total = fdt_total(new, 0)
    if new_total != len(new):
        print("[!] totalsize (%d) != ukuran berkas (%d); dipakai totalsize" % (new_total, len(new)))

    # ruang yang tersedia: dari old_fdt sampai akhir blok dtb
    room = hi - old_fdt
    if new_total > room:
        print("[X] DTB baru (%d) lebih besar dari ruang (%d) - tidak bisa in-place." % (new_total, room))
        print("    Blok dtb harus diperbesar; itu menggeser ramdisk table/bootconfig.")
        return 1
    print("ruang tersedia di blok dtb : %d byte (dipakai %d, sisa %d)"
          % (room, new_total, room - new_total))

    # tulis DTB baru + nol sampai batas ruang (sisa blok tidak boleh sampah)
    b[old_fdt:old_fdt + new_total] = new[:new_total]
    b[old_fdt + new_total:hi] = b'\0' * (room - new_total)

    # kalau ada tabel multi-DTB (mkdtimg) sebelum FDT: perbarui dt_size entrinya
    if old_fdt - lo >= 64:
        magic = struct.unpack('>I', b[lo:lo + 4])[0]
        if magic == 0xd7b7ab1e:
            ver, hs, es, ec, eo = struct.unpack('>5I', b[lo + 4:lo + 24])
            print("tabel multi-DTB terdeteksi: version 0x%x header %d entry %d count %d"
                  % (ver, hs, es, ec))
            fixed = 0
            for k in range(ec):
                base = lo + hs + k * es
                dsz, doff, did, rev = struct.unpack('>4I', b[base:base + 16])
                if lo + doff == old_fdt:
                    struct.pack_into('>I', b, base, new_total)
                    fixed += 1
                    print("  entri %d (id 0x%04x): dt_size 0x%x -> 0x%x"
                          % (k, did, dsz, new_total))
            if fixed != 1:
                print("[!] entri yang cocok: %d (harusnya 1)" % fixed)
        else:
            print("prefix 0x%x byte sebelum FDT bukan tabel mkdtimg (magic 0x%08x)" % (old_fdt - lo, magic))

    # header vendor_boot: dtb_size tidak berubah karena blok tetap sebesar semula
    print("dtb_size di header tidak diubah (%d) - blok tetap sama besar." % dtb_size)

    open(dst, 'wb').write(bytes(b))
    print("\n[OK] ditulis: %s (%d byte)" % (dst, len(b)))

    # verifikasi ulang hasil
    chk = open(dst, 'rb').read()
    f = find_fdt(chk, lo, hi)
    print("verifikasi: FDT @0x%x totalsize %d model %s"
          % (f, fdt_total(chk, f), fdt_root_strings(chk, f).get('model')))
    return 0


if __name__ == "__main__":
    sys.exit(main())
