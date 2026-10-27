#!/usr/bin/env python3
"""Ambil node DT stok untuk compatible milik blob yang belum ter-port.

Dossier RE (`docs/RE-PORT-DOSSIER.md`) menunjukkan compatible blob-blob itu
ada di DTB stok tetapi tidak ada di DTS vxtux. Skrip ini membuka
`donors/stock_initramfs/stock.dtb`, mencari node yang `compatible`-nya
cocok, lalu mencetak seluruh propertinya dalam bentuk DTS supaya node itu
bisa disalin ke DTS vxtux tanpa menebak nama/urutan properti.

    python donors/dt_nodes_for_blobs.py            # -> docs/STOCK-DT-NODES.md
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DTB = os.path.join(HERE, "stock_initramfs", "stock.dtb")
OUT = os.path.join(ROOT, "docs", "STOCK-DT-NODES.md")

sys.path.insert(0, HERE)
from fdt_dump import parse, fmt  # noqa: E402

COMPATIBLES = [
    ("sprd,cpufreq-v2", "sprd-cpufreq-v2.ko"),
    ("sprd,voltage-margin", "sprd-cpufreq-v2.ko"),
    ("sprd,debugstat", "sprd_power_stat.ko"),
    ("sprd,debuglog", "sprd_pdbg.ko"),
    ("sprd,pdbg-irq-domain-gpio", "sprd_pdbg.ko"),
    ("sprd,pdbg-irq-domain-gic", "sprd_pdbg.ko"),
    ("sprd,pdbg-irq-domain-ana", "sprd_pdbg.ko"),
    ("sprd,sc2730-7sreset", "sprd_7sreset.ko"),
    ("unisoc,sc2730-audio-codec", "snd-soc-sprd-codec-sc2730-power.ko"),
    ("sprd,iq", "sprd_iq.ko"),
    ("sprd,sc27xx-syscon", "bq2560x-charger.ko"),
]


def main():
    events = parse(open(DTB, "rb").read())
    # kumpulkan: path -> {prop: val}
    props, nodes = {}, []
    cur = None
    for kind, payload in events:
        if kind == "begin":
            cur = payload
            nodes.append(cur)
            props.setdefault(cur, {})
        elif kind == "end":
            cur = None
        elif kind == "prop":
            path, name, val = payload
            props.setdefault(path, {})[name] = val

    L = ["# Node DT stok untuk blob yang belum ter-port\n\n",
         "Sumber: `donors/stock_initramfs/stock.dtb`. Dibuat oleh\n",
         "`donors/dt_nodes_for_blobs.py`. Node di bawah **ada di stok** tetapi\n",
         "tidak ada di DTS vxtux (lihat `docs/RE-PORT-DOSSIER.md`), jadi harus\n",
         "ikut di-port bersama kodenya.\n\n"]
    seen = set()
    for compat, blob in COMPATIBLES:
        hits = [p for p, pr in props.items()
                if compat in pr.get("compatible", b"").decode("latin-1")]
        if not hits:
            L.append(f"## `{compat}` (untuk `{blob}`)\n\nTidak ditemukan di DTB stok.\n\n")
            continue
        for path in hits[:2]:
            if (compat, path) in seen:
                continue
            seen.add((compat, path))
            L.append(f"## `{compat}` (untuk `{blob}`)\n\n")
            L.append(f"path: `/sys/firmware/devicetree/base/{path}`\n\n```dts\n")
            L.append(f"{path.rsplit('/', 1)[-1]} {{\n")
            for name, val in props[path].items():
                L.append(f"\t{name} = {fmt(val)};\n")
            L.append("};\n```\n\n")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("".join(L))
    print(f"tulis {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
