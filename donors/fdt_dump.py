#!/usr/bin/env python3
"""Dump a subtree of a flattened device tree as pseudo-DTS.

No dtc on this machine, so walk the FDT structure directly and print
every node/property under a path prefix.

    python fdt_dump.py donors/stock_initramfs/stock.dtb /soc/aon/efuse@32240000
    python fdt_dump.py donors/stock_initramfs/stock.dtb --paths    # node paths only
"""
import struct
import sys

FDT_BEGIN_NODE = 1
FDT_END_NODE = 2
FDT_PROP = 3
FDT_NOP = 4
FDT_END = 9


def parse(dtb):
    (magic, totalsize, off_dt_struct, off_dt_strings, off_mem, ver,
     last_comp, boot_cpuid, size_dt_strings, size_dt_struct) = \
        struct.unpack(">10I", dtb[:40])
    assert magic == 0xD00DFEED, hex(magic)
    strings = dtb[off_dt_strings:off_dt_strings + size_dt_strings]
    p = off_dt_struct
    end = off_dt_struct + size_dt_struct
    stack = [""]
    events = []
    while p < end:
        (tok,) = struct.unpack(">I", dtb[p:p + 4])
        p += 4
        if tok == FDT_BEGIN_NODE:
            e = dtb.index(b"\x00", p)
            name = dtb[p:e].decode()
            p = (e + 4) & ~3
            stack.append(name)
            events.append(("begin", "/".join(stack[1:])))
        elif tok == FDT_END_NODE:
            stack.pop()
            events.append(("end", "/".join(stack[1:])))
        elif tok == FDT_PROP:
            (plen, nameoff) = struct.unpack(">II", dtb[p:p + 8])
            p += 8
            e = strings.index(b"\x00", nameoff)
            pname = strings[nameoff:e].decode()
            val = dtb[p:p + plen]
            p = (p + plen + 3) & ~3
            events.append(("prop", ("/".join(stack[1:]), pname, val)))
        elif tok == FDT_NOP:
            continue
        elif tok == FDT_END:
            break
    return events


def fmt(val):
    if val == b"":
        return ""
    txt = val.rstrip(b"\x00")
    try:
        s = txt.decode("utf-8")
        if s and all(32 <= ord(c) < 127 for c in s):
            return f'"{s}"'
    except UnicodeDecodeError:
        pass
    if len(val) >= 4 and len(val) % 4 == 0:
        cells = struct.unpack(">" + "I" * (len(val) // 4), val)
        return "<" + " ".join(f"0x{c:x}" for c in cells) + ">"
    return val.hex()


def under(path, prefix):
    return path == prefix or path.startswith(prefix + "/")


def main():
    dtb = open(sys.argv[1], "rb").read()
    args = sys.argv[2:]
    if args and args[0] == "--paths":
        for kind, payload in parse(dtb):
            if kind == "begin":
                print(payload)
        return
    prefix = args[0] if args else ""
    for kind, payload in parse(dtb):
        if kind == "begin":
            path = payload
            if prefix and not under(path, prefix):
                continue
            if prefix == "":
                depth = path.count("/")
            else:
                depth = path[len(prefix):].count("/")
            name = path.rsplit("/", 1)[-1] or "/"
            print("  " * depth + name + " {")
        elif kind == "end":
            path = payload
            if prefix and not under(path, prefix):
                continue
            if prefix == "":
                depth = path.count("/")
            else:
                depth = path[len(prefix):].count("/")
            print("  " * depth + "};")
        elif kind == "prop":
            path, pname, val = payload
            if prefix == "" or under(path, prefix):
                if prefix == "":
                    depth = path.count("/") + 1
                else:
                    depth = path[len(prefix):].count("/") + 1
                print("  " * depth + pname + " = " + fmt(val) + ";")


if __name__ == "__main__":
    main()
