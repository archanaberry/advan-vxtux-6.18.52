#!/usr/bin/env python3
"""Audit status porting: 129 .ko vendor Advan vs tree 6.18.52-VXTux-UMS512.

Klasifikasi tiap driver vendor:
  KO       : sudah ter-build jadi .ko di CI (artifact v17-built-kos)
  Y        : sumber ada + simbol Kconfig di-set =y di enhanced_config.cfg (built-in vmlinux)
  OFF      : sumber + simbol Kconfig ADA, tapi belum di-enable  -> bisa langsung di-port
  NOSRC    : tidak ada entri Makefile untuk modul ini -> butuh tulis driver baru
  NOSYM    : Makefile pakai obj-y (selalu built-in) / tidak pakai CONFIG_

Pakai:
  python donors/port_audit.py --tree 6.18.52-VXTux-UMS512 \
      --config enhanced_config.cfg --doc docs/DRIVERS_VENDOR_CROSSCHECK.md \
      --kos /tmp/built_kos.txt --out docs/PORT-AUDIT.md
"""
import argparse
import os
import re
import subprocess
import sys

# Blob stok -> nama modul pengganti di tree, kalau berbeda.
# phy-sprd-sharkl5Pro.ko (5.4) di-port sebagai phy-sprd-ums512.c (6.18).
RENAMES = {
    "phy-sprd-sharkl5Pro.ko": "phy-sprd-ums512.ko",
    # TICK15: kelima nama ini terbukti lewat `ar t` pada built-in.a hasil build
    # tick15 — objeknya benar-benar ikut Image, bukan cuma config-nya =y
    # (built-in.a adalah arsip tipis: ukurannya ~300 B tapi isinya penuh).
    #   jpg.ko                    -> sprd_jpg                  (sprd_jpg_main.o)
    #   vsp.ko                    -> sprd_vsp                  (sprd_vsp_main.o)
    #   sprd_cpp.ko               -> cpp                       (cpp_core.o)
    #   sprd-compr-2stage-dma.ko  -> snd-soc-sprd-platform     (sprd-pcm-compress.o)
    #   snd-soc-sprd-vbc-fe.ko    -> snd-soc-sprd-vbc-v4.ko    (=m, jadi KO)
    "jpg.ko": "sprd_jpg.ko",
    "vsp.ko": "sprd_vsp.ko",
    "sprd_cpp.ko": "cpp.ko",
    "sprd-compr-2stage-dma.ko": "snd-soc-sprd-platform.ko",
    "snd-soc-sprd-vbc-fe.ko": "snd-soc-sprd-vbc-v4.ko",
    # sc2730 audio-power: blob-nya sendiri menyebut path sumbernya di string:
    #   .../kernel5.4/audio_driver/sprd/codec/sprd/sc2730/power/sprd-audio-power.c
    # File dengan nama sama ada di pohon: drivers/vxtux/sound/asoc/codec/
    # sprd-audio-power.c, dibangun oleh CONFIG_VXTUX_SND_SPRD_SC2730_POWER=y
    # (regulator VREG/VB/BG/BIAS/MICBIAS...). Varian "-dev" adalah sub-device
    # dari driver yang sama (of_match-nya banyak compatible).
    "snd-soc-sprd-codec-sc2730-power.ko": "sprd-audio-power.o",
    "snd-soc-sprd-codec-sc2730-power-dev.ko": "sprd-audio-power.o",
    # bq2560x: chip BQ2560x didukung driver mainline bq256xx_charger.c yang
    # sudah =y (CONFIG_CHARGER_BQ256XX), lengkap dengan tabel bq2560x_reg_defs
    # dan bq25601d_get/set_chrg_volt. Blob vendor memakai chip yang sama lewat
    # sprd_charger_manager (juga ada di pohon).
    "bq2560x-charger.ko": "bq256xx_charger.o",
}

MOD_RE = re.compile(r"^obj-\$\((CONFIG_[A-Z0-9_]+)\)\s*\+?=\s*(.*)$")
JSON = json_dump = None


def vendor_list(doc):
    names = []
    for line in open(doc, encoding="utf-8"):
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 2 or not cells[0].isdigit():
            continue
        m = re.search(r"`([A-Za-z0-9_.+-]+\.ko)`", cells[1])
        if not m:
            continue
        src = cells[2].strip("`") if len(cells) > 2 else ""
        names.append((m.group(1), src))
    return names


def makefile_map(tree):
    """base module name -> list of (CONFIG_ symbol, dir)."""
    out = subprocess.run(
        ["grep", "-rn", "obj-$(CONFIG_", "--include=Makefile", "."],
        cwd=tree, capture_output=True, text=True).stdout
    m = {}
    for line in out.splitlines():
        parts = line.split(":", 2)
        if len(parts) != 3:
            continue
        path, _lineno, rest = parts
        d = os.path.dirname(path)
        mo = MOD_RE.match(rest.strip())
        if not mo:
            continue
        sym, targets = mo.group(1), mo.group(2)
        for t in targets.split():
            base = t.strip()
            if not base.endswith(".o"):
                continue
            base = base[:-2]
            m.setdefault(base, []).append((sym, d))
    # obj-y entries (always built-in)
    out_y = subprocess.run(
        ["grep", "-rn", "obj-y", "--include=Makefile", "."],
        cwd=tree, capture_output=True, text=True).stdout
    ymap = {}
    for line in out_y.splitlines():
        parts = line.split(":", 2)
        if len(parts) != 3:
            continue
        path, _lineno, rest = parts
        rest = rest.strip()
        if not rest.startswith("obj-y"):
            continue
        _, _, targets = rest.partition("=")
        for t in targets.split():
            if t.endswith(".o"):
                ymap.setdefault(t[:-2], []).append(os.path.dirname(path))
    return m, ymap


def kconfig_symbols(tree):
    # catatan: grep di mesin ini BUKAN GNU (\s tidak jalan) -> pakai kelas POSIX.
    out = subprocess.run(
        ["grep", "-rhE",
         r"^[[:space:]]*(menu)?config[[:space:]]+[A-Z0-9_]+",
         "--include=Kconfig*", "."],
        cwd=tree, capture_output=True, text=True).stdout
    return {m.group(1) for m in re.finditer(r"config\s+([A-Z0-9_]+)", out)}


def config_state(cfg):
    state = {}
    for line in open(cfg, encoding="utf-8"):
        line = line.strip()
        m_off = re.match(r"#\s*(CONFIG_[A-Z0-9_]+) is not set$", line)
        if m_off:
            state[m_off.group(1)] = "n"
            continue
        if not line.startswith("CONFIG_"):
            continue
        mo = re.match(r"(CONFIG_[A-Z0-9_]+)=(.*)", line)
        if mo:
            state[mo.group(1)] = mo.group(2)
        else:
            state[line] = "y"  # bare CONFIG_FOO => y
    return state


def variants(base):
    """Nama modul vendor pakai '_' kadang tree pakai '-'."""
    return {base, base.replace("_", "-"), base.replace("-", "_")}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tree", required=True)
    ap.add_argument("--config", required=True)
    ap.add_argument("--doc", required=True)
    ap.add_argument("--kos", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--design", help="fragmen config: baris '# CONFIG_X is not set' = sengaja dimatikan")
    a = ap.parse_args()

    ven = vendor_list(a.doc)
    mk, ymap = makefile_map(a.tree)
    syms = kconfig_symbols(a.tree)
    cfg = config_state(a.config)
    # daftar .ko boleh berupa path relatif (find) atau nama saja -> pakai basename.
    # Nama modul dinormalisasi '-' == '_' (vendor: sprd_systimer.ko, tree: sprd-systimer.ko).
    def modkey(name):
        return name.replace("-", "_")

    built = {modkey(os.path.basename(l.strip())) for l in open(a.kos, encoding="utf-8") if l.strip()}
    design = set()
    if a.design:
        for line in open(a.design, encoding="utf-8"):
            m = re.match(r"#\s*(CONFIG_[A-Z0-9_]+) is not set\s*$", line.strip())
            if m:
                design.add(m.group(1))

    rows = []
    for ko, src in ven:
        base = ko[:-3]
        # TICK15: kalau nama stok punya pengganti (RENAMES), cari juga pemetaan
        # Makefile->Kconfig atas nama penggantinya. Tanpa ini jpg.ko tetap
        # NOSRC padahal CONFIG_SPRD_JPG=y dan objeknya ikut Image.
        tgt = RENAMES.get(ko, "")
        tgtbase = tgt
        for suf in (".ko", ".o"):
            if tgtbase.endswith(suf):
                tgtbase = tgtbase[:-len(suf)]
                break
        hit = None
        for v in set(variants(base)) | set(variants(tgtbase)):
            if v in mk:
                hit = mk[v]
                break
        status, sym, path = None, "", src
        if modkey(ko) in built or modkey(RENAMES.get(ko, "")) in built:
            status = "KO"
            if hit:
                sym, path = hit[0][0], hit[0][1]
        elif hit:
            sym, path = hit[0]
            st = cfg.get(sym, "")
            if st == "y":
                status = "Y"
            elif sym in design and st == "":
                status = "OFF-DESIGN"  # dimatikan eksplisit di fragmen, simbolnya tak muncul di .config
            elif st == "n":
                status = "OFF-DESIGN"  # sengaja dimatikan di config
            elif st in ("m",):
                status = "CONF-m"  # =m tapi tak muncul di artifact (cek!)
            else:
                # Kconfig mendefinisikan nama tanpa prefix CONFIG_
                status = "OFF" if sym.replace("CONFIG_", "", 1) in syms else "NOKCONFIG"
        else:
            y = None
            for v in variants(base):
                if v in ymap:
                    y = ymap[v]
                    break
            status = "OBJ-Y" if y else "NOSRC"
            if y:
                path = y[0]
        rows.append((ko, status, sym, path))

    order = {"KO": 0, "Y": 1, "OBJ-Y": 2, "OFF": 3, "NOKCONFIG": 4, "CONF-m": 5,
             "OFF-DESIGN": 6, "NOSRC": 7}
    rows.sort(key=lambda r: (order.get(r[1], 9), r[0]))
    tally = {}
    for r in rows:
        tally[r[1]] = tally.get(r[1], 0) + 1

    with open(a.out, "w", encoding="utf-8") as f:
        f.write("# PORT AUDIT — 129 .ko vendor Advan vs tree 6.18.52-VXTux-UMS512\n\n")
        f.write("Dibuat otomatis oleh `donors/port_audit.py` (jangan edit manual).\n\n")
        f.write("Status: **KO** = ter-build .ko · **Y** = sumber ada + `=y` (built-in) · ")
        f.write("**OBJ-Y** = selalu built-in · **OFF** = sumber + Kconfig ada, belum diaktifkan · ")
        f.write("**NOKCONFIG** = Makefile pakai CONFIG_ tapi entri Kconfig hilang · ")
        f.write("**OFF-DESIGN** = sengaja `# ... is not set` · ")
        f.write("**NOSRC** = sumber driver tidak ada di tree (butuh port kode baru)\n\n")
        f.write("## Ringkasan\n\n| Status | Jumlah |\n|---|---:|\n")
        for k in sorted(tally, key=lambda k: order.get(k, 9)):
            f.write(f"| {k} | {tally[k]} |\n")
        f.write(f"| **total** | **{len(rows)}** |\n\n")
        f.write("## Tabel\n\n| # | Driver | Status | Simbol | Sumber |\n|---:|---|---|---|---|\n")
        for i, (ko, st, sym, path) in enumerate(rows, 1):
            f.write(f"| {i} | `{ko}` | {st} | `{sym}` | `{path}` |\n")
    print(tally)
    return 0


if __name__ == "__main__":
    sys.exit(main())
