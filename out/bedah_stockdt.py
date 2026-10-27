#!/usr/bin/env python3
"""Bedah stock.dtb: cetak node i2c & sdio (eMMC) + semua properti clocks/regulators
beserta target phandle-nya (untuk mengidentifikasi pemasok clock yang hilang)."""
import struct, sys, re

DTB = sys.argv[1] if len(sys.argv) > 1 else 'ci_qemu/fw/stock.dtb'
d = open(DTB, 'rb').read()

magic, total, off_struct, off_strings, version, _, _, boot_cpuid, size_strings, size_struct = \
    struct.unpack('>10I', d[:40])
assert magic == 0xd00dfeed, hex(magic)

FDT_BEGIN_NODE = 1
FDT_END_NODE = 2
FDT_PROP = 3
FDT_NOP = 4
FDT_END = 9

def cstr(b, o):
    e = b.index(b'\0', o)
    return b[o:e].decode('latin1'), e + 1

props = {}           # path -> {name: bytes}
cur = []
p = off_struct
phandles = {}        # phandle int -> path

def align4(x): return (x + 3) & ~3

while True:
    tok, = struct.unpack('>I', d[p:p+4]); p += 4
    if tok == FDT_BEGIN_NODE:
        name, p = cstr(d, p)
        cur.append(name)
        p = align4(p)
    elif tok == FDT_END_NODE:
        cur.pop()
    elif tok == FDT_PROP:
        length, nameoff = struct.unpack('>II', d[p:p+8]); p += 8
        pname, _ = cstr(d, off_strings + nameoff)
        val = d[p:p+length]; p = align4(p + length)
        path = '/'.join(cur)
        props.setdefault(path, {})[pname] = val
        if pname == 'phandle' and length == 4:
            phandles[struct.unpack('>I', val)[0]] = path
    elif tok == FDT_NOP:
        continue
    elif tok == FDT_END:
        break

def fmt(val):
    # 4-byte cells / strings / u32
    if val is None: return '?'
    if b'\0' in val[:-1] or (val and val[-1:] != b'\0' and all(32 <= c < 127 for c in val[:1])):
        try:
            return val.decode().strip('\0')
        except Exception:
            pass
    if len(val) % 4 == 0 and len(val) > 0:
        cells = struct.unpack('>%dI' % (len(val)//4), val)
        return ' '.join('0x%x' % c for c in cells)
    return repr(val[:40])

def refs(val):
    """phandle referensi dalam properti cells"""
    out = []
    if len(val) % 4 == 0:
        cells = struct.unpack('>%dI' % (len(val)//4), val)
        i = 0
        while i < len(cells):
            ph = cells[i]
            if ph in phandles:
                # tebak jumlah args: 1 arg default
                out.append((i, phandles[ph]))
            i += 1
    return out

pat = re.compile(sys.argv[2] if len(sys.argv) > 2 else r'i2c|sdio|mmc')
for path in sorted(props):
    node = path.split('/')[-1]
    if not pat.search(path):
        continue
    print('=' * 70)
    print('NODE', path)
    for pname in ('compatible', 'clock-names', 'clocks', 'regulators', 'supplies',
                  'mmc-ddr-1_8v', 'vqmmc-supply', 'vmmc-supply', 'power-domains',
                  'resets', 'reset-names', 'pinctrl-names', 'status'):
        if pname in props[path]:
            val = props[path][pname]
            print('  %-16s = %s' % (pname, fmt(val)))
            for i, tgt in refs(val):
                # tampilkan clocks dari node target bila ada nama
                print('      phandle[%d] -> %s' % (i, tgt))
print('=' * 70)
print('PROVIDER CANDIDATE (node dengan compatible sprd clk/pmic):')
for path, ps in sorted(props.items()):
    if 'compatible' in ps:
        c = fmt(ps['compatible'])
        if re.search(r'clk|clock|gate|sc2730|pmic|eic', c, re.I):
            print('  %-60s %s' % (path, c))
