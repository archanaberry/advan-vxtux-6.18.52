import os
import struct
import subprocess
import sys

PAGE = 4096
DTC = r"..\6.18.52-VXTux-UMS512\scripts\dtc\dtc"


def align(x, p=PAGE):
    return (x + p - 1) // p * p


def build_vb(path, old_dtb):
    # tabel multi-DTB: header 32 + entri 32 + DTB (FDT di blob+0x40)
    table = struct.pack('>6I', 0xd7b7ab1e, 0, 32, 32, 1, 32)
    table += b'\0' * 8
    assert len(table) == 32, len(table)
    table += struct.pack('>4I', len(old_dtb), 0x40, 0x1100, 0)
    table += b'\0' * 16
    assert len(table) == 64, len(table)
    table += old_dtb

    hdr = b'VNDRBOOT'
    hdr += struct.pack('<5I', 4, PAGE, 0x80080000, 0x81000000, 12288)
    hdr += b'console=ttyS1 test'.ljust(2048, b'\0')      # 28..2076
    hdr += struct.pack('<I', 0x80000100)                  # 2076 tags_addr
    hdr += b'test-vendor-boot'.ljust(16, b'\0')           # 2080 name
    hdr += struct.pack('<2I', 2112, len(table))           # 2096 header_size, dtb_size
    hdr += struct.pack('<Q', 0x81000000)                  # 2104 dtb_addr
    assert len(hdr) == 2112, len(hdr)
    hdr = hdr.ljust(PAGE, b'\0')
    assert len(hdr) == PAGE

    img = hdr + b'\xaa' * 12288 + table + b'\0' * 4096
    open(path, 'wb').write(img)
    # pastikan layout sesuai yang diharapkan
    assert img[PAGE + 12288 + 64:PAGE + 12288 + 68] == b'\xd0\x0d\xfe\xed', img[PAGE:].hex()


def make_dts(path, text):
    src = path + '.dts'
    open(src, 'w').write(text)
    r = subprocess.run([DTC, '-I', 'dts', '-O', 'dtb', '-o', path, src],
                       capture_output=True, text=True)
    if r.returncode:
        print("dtc gagal:", r.stderr)
        sys.exit(1)
    return path


os.makedirs("_t_pvb", exist_ok=True)
big = make_dts("_t_pvb/old_big.dtb",
               '/dts-v1/;\n'
               '/ {\n'
               '  model = "Spreadtrum UMS512-1H10 SoC";\n'
               '  compatible = "sprd,ums512-1h10";\n'
               '  sprd,sc-id = "ums512 1h10 1000";\n'
               '  blob = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";\n'
               '};\n')
small = make_dts("_t_pvb/new_small.dtb", '/dts-v1/;\n/ { model = "VXTux test"; compatible = "advan,t1030"; };\n')

print("ukuran DTB lama :", os.path.getsize(big))
print("ukuran DTB baru :", os.path.getsize(small))

# --- kasus 1: masuk (DTB baru lebih kecil) -------------------------------
build_vb("_t_pvb/vb_fit.bin", open(big, 'rb').read())
r1 = subprocess.run([sys.executable, "patch_vendor_boot.py", "_t_pvb/vb_fit.bin", small,
                     "_t_pvb/vb_fit_out.img"], capture_output=True, text=True)
print("\n--- kasus 1: DTB baru muat ---")
print(r1.stdout.strip())
print("rc =", r1.returncode)

# --- kasus 2: DTB baru lebih besar dari blok -----------------------------
build_vb("_t_pvb/vb_tight.bin", open(small, 'rb').read())
r2 = subprocess.run([sys.executable, "patch_vendor_boot.py", "_t_pvb/vb_tight.bin", big,
                     "_t_pvb/vb_tight_out.img"], capture_output=True, text=True)
print("\n--- kasus 2: DTB baru tidak muat (harus DITOLAK) ---")
print(r2.stdout.strip())
print("rc =", r2.returncode)

ok = True
if r1.returncode != 0 or not os.path.exists("_t_pvb/vb_fit_out.img"):
    print("[X] kasus 1 seharusnya berhasil")
    ok = False
else:
    out = open("_t_pvb/vb_fit_out.img", 'rb').read()
    tbl = out[PAGE + 12288:PAGE + 12288 + 32]
    magic, = struct.unpack('>I', tbl[0:4])
    dsz, doff, did, rev = struct.unpack('>4I', out[PAGE + 12288 + 32:PAGE + 12288 + 48])
    fdt_here = out[PAGE + 12288 + 64:PAGE + 12288 + 68] == b'\xd0\x0d\xfe\xed'
    print("\nhasil kasus 1: magic tabel 0x%08x dt_size entri %d (baru %d) FDT di 0x40: %s"
          % (magic, dsz, os.path.getsize(small), fdt_here))
    if magic != 0xd7b7ab1e or dsz != os.path.getsize(small) or not fdt_here:
        print("[X] tabel/entri tidak diperbarui benar")
        ok = False
    # sisa ruang harus nol
    rest = out[PAGE + 12288 + 64 + dsz: PAGE + 12288 + 64 + len(open(big, 'rb').read())]
    if rest.strip(b'\0'):
        print("[X] sisa ruang tidak dinolkan")
        ok = False
    else:
        print("[OK] sisa ruang dinolkan")

if r2.returncode == 0:
    print("[X] kasus 2 seharusnya ditolak")
    ok = False
elif "lebih besar dari ruang" not in r2.stdout:
    print("[X] pesan penolakan tidak jelas")
    ok = False
else:
    print("[OK] kasus 2 ditolak dengan pesan jelas")

print("\nHASIL:", "LULUS" if ok else "GAGAL")
sys.exit(0 if ok else 1)
