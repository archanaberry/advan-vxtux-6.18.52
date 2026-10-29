import struct, sys, os

def parse(path):
    b = open(path, 'rb').read()
    print("=" * 72)
    print(path, "  (%d byte)" % len(b))
    print("  32 byte pertama:", b[:32].hex())
    if b[:8] != b'ANDROID!':
        print("  bukan boot image")
        return
    (kernel_size, kernel_addr, ramdisk_size, ramdisk_addr, second_size, second_addr,
     tags_addr, page_size, f8, os_version) = struct.unpack('<10I', b[8:48])
    name = b[48:64].split(b'\0')[0]
    cmdline = b[64:576].split(b'\0')[0]
    extra = b[608:1632].split(b'\0')[0]
    print("  kernel_size   : 0x%-9x (%d)" % (kernel_size, kernel_size))
    print("  kernel_addr   : 0x%x" % kernel_addr)
    print("  ramdisk_size  : 0x%-9x (%d)" % (ramdisk_size, ramdisk_size))
    print("  ramdisk_addr  : 0x%x" % ramdisk_addr)
    print("  second_size   : 0x%-9x (%d)" % (second_size, second_size))
    print("  tags_addr     : 0x%x" % tags_addr)
    print("  page_size     : 0x%x (%d)" % (page_size, page_size))
    print("  field@40      : 0x%x   (v1+: header_version | legacy: dt_size)" % f8)
    print("  os_version    : 0x%x" % os_version)
    print("  name          : %r" % name)
    print("  cmdline       : %r" % cmdline)
    print("  extra_cmdline : %r" % extra)
    if f8 >= 1:
        rec_size, = struct.unpack('<I', b[1632:1636])
        rec_off, = struct.unpack('<Q', b[1636:1644])
        hdr_size, = struct.unpack('<I', b[1644:1648])
        print("  recovery_dtbo : size 0x%x off 0x%x   header_size 0x%x" % (rec_size, rec_off, hdr_size))
        if f8 >= 2:
            dtb_size, = struct.unpack('<I', b[1648:1652])
            dtb_addr, = struct.unpack('<Q', b[1652:1660])
            print("  dtb field     : size 0x%x addr 0x%x" % (dtb_size, dtb_addr))
    # halaman + posisi tiap bagian (legacy: 1 halaman header)
    if page_size:
        off = page_size
        print("  --> kernel  @0x%x .. 0x%x" % (off, off + kernel_size))
        off += (kernel_size + page_size - 1) // page_size * page_size
        if ramdisk_size:
            print("  --> ramdisk @0x%x .. 0x%x" % (off, off + ramdisk_size))
        off += (ramdisk_size + page_size - 1) // page_size * page_size
        if second_size:
            print("  --> second  @0x%x .. 0x%x" % (off, off + second_size))

for p in ("logs/artix41/boot_a.bin",
          "6.18.52-VXTux-UMS512/boot_artix41.img",
          "6.18.52-VXTux-UMS512/boot_artix40.img",
          "6.18.52-VXTux-UMS512/boot_artix37.img"):
    if os.path.exists(p):
        parse(p)
    else:
        print("=" * 72)
        print(p, "-> tidak ada")
