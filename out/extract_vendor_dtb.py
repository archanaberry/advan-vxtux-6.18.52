"""Ambil DT dasar ASLI dari vendor_boot_a dan decompile ke .dts.

Ini DT yang benar-benar dipakai kernel di tablet ini (LK: "loading fix dtb",
"Matches the corresponding dtb in multiple DTB").  Dengan .dts hasil decompile
kita bisa lihat nama node sebenarnya (regulator, ADI, i2c, sdio, vbc) alih-alih
menebak dari ums512.dtsi.

Pemakaian:
    python extract_vendor_dtb.py <vendor_boot_a.bin> [folder_keluar]
"""
import os
import struct
import subprocess
import sys

FDT_MAGIC = b'\xd0\x0d\xfe\xed'
# di Windows binary-nya dtc.exe (bash tetap bisa memanggil "dtc")
DTC = os.path.join("..", "6.18.52-VXTux-UMS512", "scripts", "dtc", "dtc.exe")
if not os.path.exists(DTC):
    DTC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "..", "6.18.52-VXTux-UMS512", "scripts", "dtc", "dtc.exe")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    src = sys.argv[1]
    outdir = sys.argv[2] if len(sys.argv) > 2 else "logs/artix42"
    b = open(src, 'rb').read()
    if b[:8] != b'VNDRBOOT':
        print("[X] bukan vendor_boot (%r)" % b[:8])
        return 1
    ver, page, kern, ramd, vramdisk = struct.unpack('<5I', b[8:28])
    header_size, dtb_size = struct.unpack('<2I', b[2096:2104])
    dtb_off = ((page + vramdisk + page - 1) // page) * page
    print("header_version %d page %d vendor_ramdisk %d" % (ver, page, vramdisk))
    print("blok dtb     : 0x%x size %d" % (dtb_off, dtb_size))

    lo, hi = dtb_off, dtb_off + dtb_size
    p = b.find(FDT_MAGIC, lo, hi)
    if p < 0:
        print("[X] tidak ada FDT di blok dtb")
        return 1
    total = struct.unpack('>I', b[p + 4:p + 8])[0]
    print("prefix tabel : %d byte sebelum FDT (magic 0x%08x)"
          % (p - lo, struct.unpack('>I', b[lo:lo + 4])[0] if p - lo >= 4 else 0))
    print("DT dasar     : @0x%x totalsize %d" % (p, total))

    if not os.path.isdir(outdir):
        os.makedirs(outdir)
    dtb_path = os.path.join(outdir, "dtb_stock_vendorboot.dtb")
    open(dtb_path, 'wb').write(b[p:p + total])
    print("ditulis      :", dtb_path)

    dts_path = os.path.join(outdir, "dtb_stock_vendorboot.dts")
    if os.path.exists(DTC):
        r = subprocess.run([DTC, '-I', 'dtb', '-O', 'dts', '-o', dts_path, dtb_path],
                           capture_output=True, text=True)
        if r.returncode == 0:
            print("decompile    :", dts_path)
            txt = open(dts_path, encoding='utf-8', errors='replace').read()
            print("model        :", txt.split('model = ')[1].split(';')[0]
                  if 'model = ' in txt else '?')
            for key in ("sc2730-regulator", "ldo-vddemmccore", "DCDC_CPU",
                        "sharkl5pro-adi", "vbc@", "sdio@", "i2c@"):
                n = txt.count(key)
                print("  %-20s : %d" % (key, n))
            print("\nbaris regulator/adi pertama (cek nama node ASLI):")
            shown = 0
            for line in txt.splitlines():
                if ("regulator" in line or "-adi" in line or "supply" in line) and shown < 25:
                    print("   ", line.strip()[:110])
                    shown += 1
        else:
            print("[!] dtc gagal:", r.stderr.strip()[:200])
    else:
        print("[!] dtc tidak ada di", os.path.abspath(DTC))
    return 0


if __name__ == "__main__":
    sys.exit(main())
