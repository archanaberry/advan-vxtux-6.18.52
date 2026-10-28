#!/usr/bin/env python3
"""Bandingkan dua DTB (stok vs build kita) khusus jalur PMIC/ADI/EIC/I2C.

Tanpa dtc: parser FDT minimal, cukup untuk dump subtree + banding properti.
Pemakaian: dtdiff.py <stock.dtb> <built.dtb> [substring]
"""
import struct
import sys


def parse(path):
    blob = open(path, 'rb').read()
    magic, totalsize, off_struct, off_strings, off_rsvmap, version, \
        last_comp, boot_cpuid, size_strings, size_struct = struct.unpack_from(
            '>10I', blob, 0)
    if magic != 0xD00DFEED:
        raise SystemExit('%s: bukan FDT (magic 0x%08X)' % (path, magic))
    struct_blk = blob[off_struct:off_struct + size_struct]
    str_blk = blob[off_strings:off_strings + size_strings]

    def s(off):
        end = str_blk.index(b'\0', off)
        return str_blk[off:end].decode('ascii', 'replace')

    root = {'name': '', 'props': {}, 'children': []}
    stack = [root]
    pos = 0
    while pos < len(struct_blk):
        (tok,) = struct.unpack_from('>I', struct_blk, pos)
        pos += 4
        if tok == 1:                                    # FDT_BEGIN_NODE
            end = struct_blk.index(b'\0', pos)
            name = struct_blk[pos:end].decode('ascii', 'replace')
            pos = (end + 4) & ~3
            node = {'name': name, 'props': {}, 'children': []}
            stack[-1]['children'].append(node)
            stack.append(node)
        elif tok == 2:                                  # FDT_END_NODE
            stack.pop()
        elif tok == 3:                                  # FDT_PROP
            length, nameoff = struct.unpack_from('>II', struct_blk, pos)
            pos += 8
            val = struct_blk[pos:pos + length]
            pos = (pos + length + 3) & ~3
            stack[-1]['props'][s(nameoff)] = val
        elif tok == 4:                                  # FDT_NOP
            continue
        elif tok == 9:                                  # FDT_END
            break
        else:
            raise SystemExit('%s: token 0x%X pada offset %d' %
                             (path, tok, pos - 4))
    return root


def walk(node, path, out):
    p = path + '/' + node['name'] if node['name'] else path or '/'
    out[p] = node
    for c in node['children']:
        walk(c, p, out)


def strings(val):
    parts = [x for x in val.split(b'\0') if x]
    try:
        return [x.decode('ascii') for x in parts]
    except UnicodeDecodeError:
        return [repr(val)]


def show(val):
    if val and all(32 <= b < 127 or b == 0 for b in val):
        return ' '.join(strings(val))
    if len(val) == 4:
        v = struct.unpack('>I', val)[0]
        return '0x%08x (%d)' % (v, v)
    return '<%d byte: %s>' % (len(val), val[:24].hex())


def interesting(node):
    comps = ' '.join(strings(node['props'].get('compatible', b'')))
    return comps


def main():
    stock = parse(sys.argv[1])
    built = parse(sys.argv[2])
    needle = sys.argv[3] if len(sys.argv) > 3 else None

    sm, bm = {}, {}
    walk(stock, '', sm)
    walk(built, '', bm)

    if needle:
        def pick(m):
            out = []
            for p, n in m.items():
                hay = p + ' ' + interesting(n)
                if needle.lower() in hay.lower():
                    out.append(p)
            return out

        sp, bp = pick(sm), pick(bm)
        print('=== node cocok "%s" ===' % needle)
        print('  stok  : %d node' % len(sp))
        for p in sp:
            print('    %s' % p)
        print('  build : %d node' % len(bp))
        for p in bp:
            print('    %s' % p)

        for p in sp:
            n = sm[p]
            if needle.lower() in interesting(n).lower():
                print('\n---- STOK %s ----' % p)
                for k in sorted(n['props']):
                    print('   %-24s = %s' % (k, show(n['props'][k])))
                for c in n['children']:
                    cp = p + '/' + c['name']
                    print('   [child] %s  [%s]' % (cp, interesting(c)))
    else:
        only_s = sorted(set(sm) - set(bm))
        only_b = sorted(set(bm) - set(sm))
        print('node hanya di stok (%d):' % len(only_s))
        for p in only_s[:80]:
            print('   %s  [%s]' % (p, interesting(sm[p])))
        print('\nnode hanya di build (%d):' % len(only_b))
        for p in only_b[:80]:
            print('   %s  [%s]' % (p, interesting(bm[p])))


if __name__ == '__main__':
    main()
