import struct, sys

BOOT = "logs/artix41/boot_a.bin"
DTBO = "logs/artix41/dtbo_a_stock.bin"

def fdt_info(b, off):
    """kembalikan (totalsize, model, compatible) dari FDT di offset off."""
    if b[off:off + 4] != b'\xd0\x0d\xfe\xed':
        return None
    total = struct.unpack('>I', b[off + 4:off + 8])[0]
    off_struct = struct.unpack('>I', b[off + 8:off + 12])[0]
    off_strings = struct.unpack('>I', b[off + 12:off + 16])[0]
    size_strings = struct.unpack('>I', b[off + 32:off + 36])[0]
    strings = b[off + off_strings: off + off_strings + size_strings]
    # ambil beberapa string property penting dari blok struct
    blk = b[off + off_struct: off + off_struct + 400]
    out = {}
    for key in (b'model', b'compatible'):
        i = blk.find(key)
        if i < 0:
            continue
        # nama string prop + nilai (panjang 4B, offset 4B)
        j = i + len(key) + 1
        j = (j + 3) & ~3
        ln = struct.unpack('>I', blk[j:j + 4])[0]
        nm = struct.unpack('>I', blk[j + 4:j + 8])[0]
        if nm < size_strings:
            end = strings.find(b'\0', nm)
            val = strings[nm:end][:60]
            out[key.decode()] = val.decode('latin-1')
    return total, out.get('model', '?'), out.get('compatible', '?')

print("=" * 70)
print("HEADER boot_a.bin")
b = open(BOOT, 'rb').read()
magic = b[:8]
f = struct.unpack('<10I', b[8:48])
(kernel_size, kernel_addr, ramdisk_size, ramdisk_addr, second_size, second_addr,
 target_addr, tags_addr, page_size, header_version) = f
os_version, = struct.unpack('<I', b[48:52])
name = b[48 + 16:48 + 16 + 16].split(b'\0')[0]
cmdline = b[64:64 + 512].split(b'\0')[0]
extra = b[608:608 + 1024].split(b'\0')[0]
fields = dict(kernel_size=kernel_size, ramdisk_size=ramdisk_size, second_size=second_size,
              page_size=page_size, header_version=header_version)
print("magic            :", magic)
print("kernel_size      : 0x%x" % kernel_size)
print("ramdisk_size     : 0x%x" % ramdisk_size)
print("second_size      : 0x%x" % second_size)
print("page_size        : 0x%x" % page_size)
print("header_version   : %d" % header_version)
if header_version >= 1:
    rec_size, = struct.unpack('<I', b[1632:1636])
    rec_off, = struct.unpack('<Q', b[1636:1644])
    hdr_size, = struct.unpack('<I', b[1644:1648])
    print("recovery_dtbo    : size 0x%x offset 0x%x" % (rec_size, rec_off))
    print("header_size      : 0x%x" % hdr_size)
if header_version >= 2:
    dtb_size, = struct.unpack('<I', b[1648:1652])
    dtb_addr, = struct.unpack('<Q', b[1652:1660])
    print("dtb (v2)         : size 0x%x addr 0x%x" % (dtb_size, dtb_addr))
print("cmdline          :", cmdline.decode('latin-1'))
print("extra_cmdline    :", extra.decode('latin-1'))

print()
print("=" * 70)
print("SEMUA magic FDT di boot_a.bin")
i = 0
found = []
while True:
    i = b.find(b'\xd0\x0d\xfe\xed', i)
    if i < 0:
        break
    info = fdt_info(b, i)
    if info:
        found.append((i,) + info)
        print("  @0x%-10X totalsize=%-8d model=%-40s compat=%s" % (i, info[0], info[1], info[2]))
    i += 4
if not found:
    print("  (tidak ada)")

print()
print("=" * 70)
print("HEADER dtbo_a_stock.bin + entri")
d = open(DTBO, 'rb').read()
dm, ver, hsize, esize, ecount, eoff = struct.unpack('>6I', d[:24])
print("magic            : 0x%08x  (harus 0xd7b7ab1e)" % dm)
print("version          : 0x%x" % ver)
print("header_size      : %d" % hsize)
print("dt_entry_size    : %d" % esize)
print("dt_entry_count   : %d" % ecount)
print("dt_entries_offset: %d" % eoff)
for k in range(ecount):
    base = hsize + k * esize
    ds, doff, did, rev = struct.unpack('>4I', d[base:base + 16])
    info = fdt_info(d, doff)
    print("  entry %d: dt_size=0x%-6x dt_offset=0x%-6x id=0x%04x rev=0x%04x  FDT=%s"
          % (k, ds, doff, did, rev,
             ("totalsize=%d model=%s" % (info[0], info[1])) if info else "BUKAN FDT"))
