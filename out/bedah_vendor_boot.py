import os, struct, sys

FDT_MAGIC = b'\xd0\x0d\xfe\xed'
BEGIN_NODE, END_NODE, PROP, NOP, END = 1, 2, 3, 4, 9


def root_props(b, off):
    """baca properti node root dari FDT di offset off"""
    if b[off:off + 4] != FDT_MAGIC:
        return None, {}
    total, off_struct, off_strings = struct.unpack('>3I', b[off + 4:off + 16])
    if total < 64 or total > (1 << 24):
        return None, {}
    p = off + off_struct
    strings = b[off + off_strings:off + total]
    props = {}
    tk, = struct.unpack('>I', b[p:p + 4])
    p += 4
    if tk != BEGIN_NODE:
        return total, {}
    # nama node root (string nul-terminated)
    e = b.find(b'\0', p)
    p = (e + 4) & ~3
    while p + 4 <= off + total:
        tk, = struct.unpack('>I', b[p:p + 4])
        p += 4
        if tk == END_NODE or tk == END:
            break
        if tk == NOP:
            continue
        if tk != PROP:
            break
        ln, nm = struct.unpack('>2I', b[p:p + 8])
        p += 8
        val = b[p:p + ln]
        p = (p + ln + 3) & ~3
        key = strings[nm:strings.find(b'\0', nm)].decode('latin-1')
        props[key] = val
    return total, props


def show(b, off, tag):
    total, props = root_props(b, off)
    if total is None:
        return None
    model = props.get('model', b'?').split(b'\0')[0].decode('latin-1')
    comp = props.get('compatible', b'?').split(b'\0')[0].decode('latin-1')
    scid = props.get('sprd,sc-id', b'-').split(b'\0')[0].decode('latin-1')
    print("   %-10s @0x%-8X totalsize=%-7d model=%-38s sc-id=%s"
          % (tag, off, total, model, scid))
    return total


def scan_multi(b, start, limit=None, tag="dtb", page=4096):
    """iterasi blob multi-DTB (FDT bersambung, boleh page-aligned)"""
    out = []
    p = start
    end = len(b) if limit is None else min(len(b), start + limit)
    while p < end - 4:
        if b[p:p + 4] == FDT_MAGIC:
            t = show(b, p, tag)
            if t:
                out.append((p, t))
                p = p + max(t, 4)
                p = (p + page - 1) // page * page if page else p
                continue
        nxt = b.find(FDT_MAGIC, p + 1, end)
        if nxt < 0:
            break
        p = nxt
    return out


def parse_vendor_boot(path):
    b = open(path, 'rb').read()
    print("=" * 74)
    print(os.path.basename(path), " (%d byte)" % len(b))
    if b[:8] != b'VNDRBOOT':
        print("   bukan VNDRBOOT (magic=%r)" % b[:8])
        return None
    (ver, page, kern_addr, ramd_addr, vramdisk_size) = struct.unpack('<5I', b[8:28])
    cmdline = b[28:28 + 2048].split(b'\0')[0].decode('latin-1', 'replace')
    name = b[2080:2096].split(b'\0')[0].decode('latin-1')
    header_size, dtb_size = struct.unpack('<2I', b[2096:2104])
    dtb_addr, = struct.unpack('<Q', b[2104:2112])
    print("   header_version : %d" % ver)
    print("   page_size      : %d" % page)
    print("   vendor_ramdisk : %d byte" % vramdisk_size)
    print("   name           : %r" % name)
    print("   header_size    : %d" % header_size)
    print("   dtb_size       : %d   dtb_addr 0x%x" % (dtb_size, dtb_addr))
    dtb_off = ((page + vramdisk_size + page - 1) // page) * page
    print("   --> dtb blob di 0x%x (%d byte)" % (dtb_off, dtb_size))
    prefix = struct.unpack('>I', b[dtb_off:dtb_off + 4])[0] if dtb_size >= 4 else 0
    if dtb_off > 0 and b[dtb_off:dtb_off + 4] == b'\xd7\xb7\xab\x1e':
        tver, ths, tes, tec, teo = struct.unpack('>5I', b[dtb_off + 4:dtb_off + 24])
        print("   tabel multi-DTB: version 0x%x header %d entry %d count %d"
              % (tver, ths, tes, tec))
        for k in range(tec):
            base = dtb_off + ths + k * tes
            dsz, doff, did, rev = struct.unpack('>4I', b[base:base + 16])
            print("     entri %d: id=0x%04x size=0x%-6x off=0x%-6x (FDT @0x%x)"
                  % (k, did, dsz, doff, dtb_off + doff))
    else:
        print("   prefix 0x%08x (bukan tabel mkdtimg)" % prefix)
    print("   isi multi-DTB:")
    entries = scan_multi(b, dtb_off, dtb_size, "dtb", page)
    if not entries:
        print("     (tidak ada FDT di rentang dtb)")
    return entries


def parse_dtbo(path):
    b = open(path, 'rb').read()
    print("=" * 74)
    print(os.path.basename(path), " (%d byte)" % len(b))
    if b[:4] != b'\xd7\xb7\xab\x1e':
        print("   bukan tabel dtbo")
        return
    m, ver, hsize, esize, ecount, eoff = struct.unpack('>6I', b[:24])
    print("   entri: %d (entry_size %d)" % (ecount, esize))
    for k in range(ecount):
        base = hsize + k * esize
        ds, doff, did, rev = struct.unpack('>4I', b[base:base + 16])
        total, props = root_props(b, doff)
        model = props.get('model', b'?').split(b'\0')[0].decode('latin-1') if props else '?'
        scid = props.get('sprd,sc-id', b'-').split(b'\0')[0].decode('latin-1') if props else '-'
        print("   entry %d: id=0x%04x size=0x%-6x off=0x%-6x FDT=%-6s model=%-30s sc-id=%s"
              % (k, did, ds, doff, total, model, scid))


def scan_plain(path, tag):
    b = open(path, 'rb').read()
    print("=" * 74)
    print(os.path.basename(path), " (%d byte)" % len(b))
    n = b.count(FDT_MAGIC)
    print("   jumlah magic FDT : %d" % n)
    print("   kunci 'sprd,sc-id'      :", b'sprd,sc-id' in b)
    print("   kunci 'ums512 1h10'     :", b'ums512 1h10' in b)
    print("   kunci 'ldo-vddemmccore' :", b'ldo-vddemmccore' in b)
    print("   kunci 'DCDC_CPU'        :", b'DCDC_CPU' in b)
    print("   kunci 'sharkl5pro'      :", b'sharkl5pro' in b)
    if n:
        scan_multi(b, 0, None, tag, 0)


d = sys.argv[1] if len(sys.argv) > 1 else "logs/artix42"
if not os.path.isdir(d):
    d = os.path.join(os.path.dirname(os.path.abspath(__file__)), d)
print("direktori:", d)
vb = os.path.join(d, "vendor_boot_a.bin")
ub = os.path.join(d, "uboot_a.bin")
db = os.path.join(d, "dtbo_a.bin")
bb = os.path.join(d, "boot_a.bin")

if os.path.exists(vb):
    parse_vendor_boot(vb)
if os.path.exists(db):
    parse_dtbo(db)
if os.path.exists(ub):
    scan_plain(ub, "uboot")
if os.path.exists(bb):
    scan_plain(bb, "boot")
