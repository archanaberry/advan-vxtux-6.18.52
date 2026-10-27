#!/usr/bin/env python3
"""Triage the vendor blobs PORT-AUDIT rates NOSRC.

PORT-AUDIT only knows what the Kconfig/Makefile wiring tells it: a blob is
NOSRC when no Makefile in the tree maps a config symbol to a module of that
name. That is not the same as "the source does not exist". This script
answers the two questions that actually matter per blob:

  1. does a plausible source file exist in 6.18.52-VXTux-UMS512?
  2. is that file wired into its directory's Makefile, i.e. would kbuild
     compile it at all once its CONFIG symbol is on?

plus the facts you can read straight off the stock blob: modinfo, how many
globals/locals/exports it has, and how many of its exported symbols already
exist in the build symbol universe.

    python donors/nosrc_triage.py          # -> docs/NOSRC-TRIAGE.md
    python donors/nosrc_triage.py --print  # stdout only
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
BLOBS = os.path.join(HERE, "stock_vendor_modules_all")
BUILT = os.path.join(HERE, "all_syms.txt")
AUDIT = os.path.join(ROOT, "docs", "PORT-AUDIT.md")
CONFIG_FINAL = os.path.join(ROOT, "docs", "evidence-20261023", "config.final")
OUT = os.path.join(ROOT, "docs", "NOSRC-TRIAGE.md")
TREE = os.path.join(ROOT, "6.18.52-VXTux-UMS512")

NM = (r"C:\Users\Administrator\scoop\apps\gcc-aarch64-none-linux-gnu"
      r"\15.2.rel1\bin\aarch64-none-linux-gnu-nm.exe")
STRINGS = (r"C:\Users\Administrator\scoop\apps\gcc-aarch64-none-linux-gnu"
           r"\15.2.rel1\bin\aarch64-none-linux-gnu-strings.exe")

# Blob stok -> nama modul pengganti kalau beda.
RENAMES = {"phy-sprd-sharkl5Pro.ko": "phy-sprd-ums512.ko"}
BUILT_LIST = os.path.join(ROOT, "docs", "evidence-20261023", "ko_list_tick13.txt")

RANK = {"KO": 0, "OBJ-Y": 1, "Y": 2, "OFF": 3, "OFF-DESIGN": 4,
        "NOKCONFIG": 5, "NOSRC": 6}

# Blob -> candidate sources. Checked against the tree; the report says for
# each whether the file exists and whether kbuild would compile it.
SOURCES = {
    "snd-soc-sprd-card.ko": [
        "drivers/vxtux/sound/asoc/card/sprd-card.c",
        "drivers/vxtux/sound/asoc/sprd-card.c",
    ],
    "hxchipset_83102e.ko": [
        "drivers/vxtux/touch/hx83102e/hxchipset/himax_ic_HX83102.c",
    ],
    "unisoc-iommu.ko": ["drivers/iommu/sprd-iommu.c"],
    "sipc-core.ko": ["drivers/soc/sprd/sipc/sipc.c"],
    "bq2560x-charger.ko": [
        "drivers/power/supply/bq2560x-charger.c",
        "drivers/vxtux/power/bq2560x-charger.c",
    ],
    "i2c-sprd-hw.ko": ["drivers/i2c/busses/i2c-sprd.c"],
    "ion_cma_heap.ko": ["drivers/dma-buf/heaps/cma_heap.c"],
    "jpg.ko": ["drivers/misc/sprd_jpg/sprd_jpg_main.c"],
    "misc_sprd_uid.ko": ["drivers/soc/sprd/soc_id/sprd_uid.c"],
    "mmc_swcq.ko": [
        "drivers/mmc/host/sdhci-sprd-swcq.c",
        "drivers/mmc/host/mmc_swcq.c",
    ],
    "musb_sprd.ko": ["drivers/usb/musb/musb_sprd_vxtux.c"],
    "phy-sprd-sharkl5Pro.ko": ["drivers/phy/sprd/phy-sprd-ums512.c"],
    "pinctrl-sprd-sharkl5Pro.ko": [
        "drivers/pinctrl/sprd/pinctrl-sprd-sharkl5pro.c",
    ],
    "sblock_bridge.ko": [
        "drivers/soc/sprd/sipc/sblock.c",
        "drivers/vxtux/sipc/sblock.c",
    ],
    "sbuf_bridge.ko": [
        "drivers/soc/sprd/sipc/sbuf.c",
        "drivers/vxtux/sipc/sbuf.c",
    ],
    "sha1-ce.ko": [
        "lib/crypto/arm64/sha1-ce-core.S",
        "lib/crypto/arm/sha1-ce-core.S",
    ],
    "snd-soc-sprd-codec-sc2730-power-dev.ko": [
        "drivers/vxtux/sound/asoc/codec_sc2730/sprd-audio-sc2730-power-dev.c",
    ],
    "snd-soc-sprd-codec-sc2730-power.ko": [
        "drivers/vxtux/sound/asoc/codec_sc2730/sprd-audio-sc2730-power.c",
    ],
    "snd-soc-sprd-dummy-codec.ko": [
        "drivers/vxtux/sound/asoc/codec/dummy-codec.c",
    ],
    "snd-soc-sprd-vbc-fe.ko": ["drivers/vxtux/sound/asoc/dai_v4/vbc-dai.c"],
    "sprd-compr-2stage-dma.ko": [
        "sound/soc/sprd/sprd-pcm-compress.c",
        "drivers/vxtux/sound/asoc/sprd-pcm-compress.c",
    ],
    "sprd-cpufreq-public.ko": [
        "drivers/cpufreq/sprd-cpufreq-public.c",
        "drivers/vxtux/cpufreq/sprd_cpufreq_public.c",
    ],
    "sprd-cpufreq-v2.ko": [
        "drivers/cpufreq/sprd-cpufreq-v2.c",
        "drivers/vxtux/cpufreq/sprd_cpufreq_v2.c",
    ],
    "sprd_7sreset.ko": [
        "drivers/soc/sprd/sprd_7sreset.c",
        "drivers/vxtux/soc/sprd_7sreset.c",
    ],
    "sprd_camera.ko": ["drivers/media/platform/sprd/pipeline/sprd_dcam.c"],
    "sprd_cpp.ko": ["drivers/media/platform/sprd/cpp/cpp_core.c"],
    "sprd_iq.ko": [
        "drivers/media/platform/sprd/isp/sprd_iq.c",
        "drivers/media/platform/sprd/sprd_iq.c",
    ],
    "sprd_manufacturer_model.ko": [
        "drivers/soc/sprd/sprd_manufacturer_model.c",
    ],
    "sprd_pdbg.ko": [
        "drivers/soc/sprd/sprd_pdbg.c",
        "drivers/vxtux/soc/sprd_pdbg.c",
    ],
    "sprd_power_stat.ko": ["drivers/soc/sprd/sprd_power_stat.c"],
    "sprd_sip_svc.ko": [
        "drivers/soc/sprd/sprd_sip_svc.c",
        "drivers/firmware/sprd_sip_svc.c",
    ],
    "sprd_vdsp.ko": ["drivers/vxtux/npu/vdsp/common/sprd_vdsp.c"],
    "vsp.ko": ["drivers/media/platform/sprd/vsp/sprd_vsp_main.c"],
}

# Cases where the tree already provides the function through another route.
NOTES = {
    "sha1-ce.ko": (
        "SUPERSEDED",
        "lib/crypto/sha1.c meng-include `$(SRCARCH)/sha1.h`; untuk arm64 "
        "`lib/crypto/arm64/sha1.h` memanggil `__sha1_ce_transform` di bawah "
        "`kernel_neon_begin()` begitu CPU punya fitur SHA1 (static key `have_ce`). "
        "Jadi shash \"sha1\" bawaan `crypto/sha1.c` SUDAH memakai Crypto "
        "Extensions; sha1-ce.ko stok tidak perlu di-port.",
    ),
}

TOKENS = {
    "bq2560x-charger": ["bq2560"],
    "mmc_swcq": ["swcq"],
    "sprd_7sreset": ["7sreset"],
    "sprd_iq": ["sprd_iq", "isp_iq"],
    "sprd_manufacturer_model": ["manufacturer_model"],
    "sprd_pdbg": ["pdbg"],
    "sprd_power_stat": ["power_stat"],
    "sprd_sip_svc": ["sip_svc"],
    "sprd-compr-2stage-dma": ["2stage", "pcm-compress"],
    "snd-soc-sprd-card": ["sprd-card", "sprd_card"],
    "snd-soc-sprd-codec-sc2730-power": ["sc2730-power", "sc2730_power"],
    "sprd-cpufreq": ["sprd-cpufreq", "sprd_cpufreq"],
}


def nosrc_names():
    """Names PORT-AUDIT rates NOSRC (a blob may appear on several rows)."""
    best = {}
    for line in open(AUDIT, encoding="utf-8"):
        if not line.startswith("| ") or "`" not in line:
            continue
        col = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(col) < 5:
            continue
        m = re.findall(r"`([^`]+)`", col[1])
        st = col[2].strip()
        if not m or st not in RANK:
            continue
        n = m[0]
        if n not in best or RANK[st] < RANK[best[n]]:
            best[n] = st
    return sorted(n for n, s in best.items() if s == "NOSRC")


def symbols(ko):
    """-> (globals, locals, exports) as sets of names."""
    out = subprocess.run([NM, "--defined-only", ko],
                         capture_output=True, text=True).stdout
    glob, loc, exp = set(), set(), set()
    for line in out.splitlines():
        p = line.split()
        if len(p) < 3:
            continue
        typ, name = p[-2], p[-1]
        if name.startswith("__ksymtab_"):
            exp.add(name[len("__ksymtab_"):])
        if typ in "TDBRWA":
            glob.add(name)
        elif typ in "tdbrwa":
            loc.add(name)
    return glob, loc, exp


def modinfo(ko):
    out = subprocess.run([STRINGS, "-a", ko],
                         capture_output=True, text=True).stdout
    info = {}
    for line in out.splitlines():
        for key in ("description", "author", "license", "vermagic",
                    "srcversion", "depends"):
            if line.startswith(key + "="):
                info[key] = line.split("=", 1)[1]
    return info


def tree_index():
    """-> (set of rel .c/.S paths, {dir: Makefile text})."""
    files, makes = set(), {}
    for base, dirs, names in os.walk(TREE):
        dirs[:] = [d for d in dirs if d not in (".git", "Documentation")]
        rel = os.path.relpath(base, TREE).replace("\\", "/")
        if "Makefile" in names:
            try:
                makes[rel] = open(os.path.join(base, "Makefile"),
                                  encoding="utf-8", errors="ignore").read()
            except OSError:
                pass
        for n in names:
            if n.endswith((".c", ".S")):
                files.add((rel + "/" + n).lstrip("./"))
    return files, makes


def wired(path, makes):
    """Would kbuild compile this file? Checks its own dir's Makefile."""
    d, name = path.rsplit("/", 1)
    stem = name.rsplit(".", 1)[0]
    text = makes.get(d, "")
    return re.search(r"(?<![\w.-])" + re.escape(stem) + r"\.o\b", text) is not None


def resolve(blob, files, makes):
    """-> list of (path, wired) candidates found in the tree."""
    found = []
    for p in SOURCES.get(blob, []):
        if p in files:
            found.append(p)
    if found:
        return [(p, wired(p, makes)) for p in found]
    stem = blob[:-3]
    toks = TOKENS.get(stem) or TOKENS.get(stem.rsplit("-", 1)[0], [])
    if not toks:
        core = re.sub(r"^(snd-soc-sprd-|snd-soc-|sprd[-_])", "", stem)
        toks = [core]
    for p in sorted(files):
        low = p.lower()
        if any(t.lower() in low for t in toks):
            found.append((p, wired(p, makes)))
    # prefer same-name matches, keep the list short
    found.sort(key=lambda x: (not x[1], -len(x[0])))
    return found[:3]


def makefile_symbol(path, makes):
    """The obj-$(CONFIG_X) line that compiles `path`, if any."""
    d, name = path.rsplit("/", 1)
    stem = name.rsplit(".", 1)[0]
    text = makes.get(d, "")
    for line in text.splitlines():
        if re.search(r"(?<![\w.-])" + re.escape(stem) + r"\.o\b", line):
            m = re.search(r"obj-\$\((?:CONFIG_)?([A-Z0-9_]+)\)", line)
            if m:
                return "CONFIG_" + m.group(1)
            if re.match(r"\s*obj-y", line):
                return "obj-y"
    return ""


def config_state(sym, final):
    """-> ('y'|'m'|'n'|'absent', raw line)."""
    if not sym or sym == "obj-y":
        return ("y" if sym == "obj-y" else "absent"), ""
    m = re.search(r"^CONFIG_" + re.escape(sym[7:]) + r"=([ym])", final, re.M)
    if m:
        return m.group(1), m.group(0)
    if re.search(r"^# " + re.escape(sym) + r" is not set", final, re.M):
        return "n", "# " + sym + " is not set"
    return "absent", ""


def verdict(srcs, exp, universe, final, makes, ko="", built=frozenset()):
    if ko in NOTES:
        return NOTES[ko][0], "", ""
    if RENAMES.get(ko, "") in built:
        return "KO-RENAME", "", ""
    if ko in built:
        return "KO", "", ""
    if not srcs:
        if exp and len(exp & universe) / len(exp) >= 0.60:
            return "SUPERSEDED", "", ""
        return "PERLU-PORT", "", ""
    path, w = srcs[0]
    sym = makefile_symbol(path, makes)
    st, _ = config_state(sym, final)
    if st == "y":
        return "BUILT-IN", sym, "y"
    if st == "m":
        return "MODUL", sym, "m"
    if w:
        return "SIAP-COBA", sym, st
    return "PERLU-WIRING", sym, st


def main():
    show = "--print" in sys.argv
    universe = {l.strip() for l in open(BUILT, encoding="utf-8") if l.strip()}
    final = open(CONFIG_FINAL, encoding="utf-8").read()
    built = set()
    if os.path.exists(BUILT_LIST):
        built = {l.strip().rsplit("/", 1)[-1]
                 for l in open(BUILT_LIST, encoding="utf-8") if l.strip()}
    print("index pohon...", file=sys.stderr)
    files, makes = tree_index()
    print(f"{len(files)} file .c/.S, {len(makes)} Makefile", file=sys.stderr)

    rows = []
    for ko in nosrc_names():
        path = os.path.join(BLOBS, ko)
        if not os.path.exists(path) or os.path.getsize(path) == 0:
            rows.append(dict(ko=ko, size=0, desc="(blob tidak ada)",
                             ng=0, nl=0, ne=0, nh=0, srcs=[], v="BLOB-HILANG",
                             sym="", cs=""))
            continue
        glob, loc, exp = symbols(path)
        info = modinfo(path)
        srcs = resolve(ko, files, makes)
        v, sym, st = verdict(srcs, exp, universe, final, makes, ko, built)
        rows.append(dict(ko=ko, size=os.path.getsize(path),
                         desc=info.get("description", "-"),
                         ng=len(glob), nl=len(loc), ne=len(exp),
                         nh=len(exp & universe),
                         srcs=srcs, v=v, sym=sym, cs=st))

    order = {"KO": 0, "KO-RENAME": 0, "BUILT-IN": 0, "MODUL": 1,
             "SUPERSEDED": 2, "SIAP-COBA": 3,
             "PERLU-WIRING": 4, "PERLU-PORT": 5, "BLOB-HILANG": 6}
    rows.sort(key=lambda r: (order.get(r["v"], 9), -r["nh"]))
    tally = {}
    for r in rows:
        tally[r["v"]] = tally.get(r["v"], 0) + 1

    L = []
    L.append("# Triage blob NOSRC — mana yang benar-benar hilang\n\n")
    L.append("Dibuat oleh `donors/nosrc_triage.py` dari blob stok itu sendiri "
             "(`nm` + `strings`) dan dari isi pohon 6.18.52-VXTux-UMS512. "
             "Bukan tebakan.\n\n")
    L.append("PORT-AUDIT menyebut blob **NOSRC** kalau tidak ada Makefile yang "
             "memetakan simbol CONFIG ke modul bernama itu. Itu **bukan** bukti "
             "sumbernya tidak ada. Kolom di bawah memisahkan tiga kasus:\n\n")
    L.append("- **BUILT-IN** — sumbernya ada dan simbolnya `=y` di `config.final`, "
             "jadi kodenya ikut ke `Image` (tidak muncul sebagai `.ko`).\n")
    L.append("- **MODUL** — sumber ada, simbol `=m`, harusnya jadi `.ko`.\n")
    L.append("- **SUPERSEDED** — tidak ada sumber dengan nama itu, tapi ≥60% simbol "
             "yang di-EXPORT blob sudah ada di build (fungsinya ada di modul lain).\n")
    L.append("- **SIAP-COBA** — file sumbernya ada *dan* tercantum di Makefile, "
             "tetapi simbol CONFIG-nya belum dinyalakan.\n")
    L.append("- **PERLU-WIRING** — sumbernya ada tapi tidak tercantum di "
             "Makefile/Kconfig direktori itu.\n")
    L.append("- **PERLU-PORT** — sumbernya memang tidak ada di pohon; harus port "
             "dari sumber vendor 5.4 atau ditulis ulang.\n\n")
    L.append("`overlap` = persen simbol yang di-EXPORT blob itu sudah ada di "
             "kumpulan simbol build (`donors/all_syms.txt`).\n\n")

    L.append("## Ringkasan\n\n| Verdict | Jumlah |\n|---|---:|\n")
    for k in ("KO", "KO-RENAME", "BUILT-IN", "MODUL", "SUPERSEDED", "SIAP-COBA",
              "PERLU-WIRING", "PERLU-PORT", "BLOB-HILANG"):
        if k in tally:
            L.append(f"| {k} | {tally[k]} |\n")
    L.append(f"| **total** | **{len(rows)}** |\n\n")

    L.append("## Tabel\n\n")
    L.append("| # | blob | bytes | desc | globals | exports | overlap | sumber di pohon | simbol | config | verdict |\n")
    L.append("|---:|---|---:|---|---:|---:|---:|---|---|---|---|\n")
    for i, r in enumerate(rows, 1):
        src = r["srcs"][0][0] if r["srcs"] else "—"
        pct = f"{r['nh']*100//r['ne']}%" if r["ne"] else "—"
        L.append(f"| {i} | `{r['ko']}` | {r['size']} | {r['desc']} | {r['ng']} | "
                 f"{r['ne']} | {r['nh']} ({pct}) | `{src}` | `{r['sym'] or '—'}` | "
                 f"{r['cs'] or '—'} | {r['v']} |\n")

    if NOTES:
        L.append("\n## Kasus yang sudah tertutup lewat jalur lain\n\n")
        for ko, (v, why) in sorted(NOTES.items()):
            L.append(f"- `{ko}` → **{v}**. {why}\n")

    L.append("\n## Kenapa blob stok tidak bisa dipakai apa adanya\n\n")
    L.append("Semua blob 5.4 di atas membawa vermagic "
             "`5.4.210 SMP preempt mod_unload modversions aarch64`, jadi "
             "`insmod` ke 6.18.52 **selalu** ditolak kernel. Penggantinya harus "
             "dibangun dari sumber — tidak ada jalan pintas memakai blob asli.\n")

    text = "".join(L)
    if show:
        sys.stdout.write(text)
    else:
        with open(OUT, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"tulis {OUT}")
        for k in ("KO", "KO-RENAME", "BUILT-IN", "MODUL", "SUPERSEDED",
                  "SIAP-COBA", "PERLU-WIRING", "PERLU-PORT", "BLOB-HILANG"):
            if k in tally:
                print(f"  {k:12s} {tally[k]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
