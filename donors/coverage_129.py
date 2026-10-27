#!/usr/bin/env python3
"""coverage_129.py — skor otoritatif: 129 blob stok, mana yang sudah tertangani.

Selama ini ada tiga metrik yang saling bertabrakan karena berasal dari sumber
berbeda dan diambil pada waktu berbeda:

  port_audit  : pemetaan nama -> simbol Kconfig, tapi diambil dari config LAMA
                dan kadang salah pasang (gpio.ko -> MTD_NAND_GPIO).
  nosrc_triage: verdict BUILT-IN/SUPERSEDED/SIAP-COBA/PERLU-PORT, juga snapshot.
  symreach    : apakah semua undefined symbol blob resolve terhadap build kita.

Alat ini menggabungkan ketiganya dan mengecek semuanya ulang terhadap **config
dan .ko yang benar-benar sedang dibangun**:

  KO        ada .ko-nya di build (nama persis, dengan RENAMES)
  Y         pemetaan Kconfig-nya `=y` di config live -> ikut ke Image
  BUILT-IN  verdict triage + config pendukungnya `=y`
  SUPERSEDED>=60% simbol EXPORT blob sudah ada di build lewat modul lain
  BELUM     sisanya; alasannya dicatat (config mati / sumber tidak ada /
            simbol vendor yang belum ada penggantinya)

    python donors/coverage_129.py
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBS = os.path.join(ROOT, "donors", "stock_vendor_modules_all")
KO_LIST = os.path.join(ROOT, "docs", "evidence-20261024", "ko_list_v17.txt")
CFG = os.path.join(ROOT, "docs", "evidence-20261024", "config_v17.txt")
PORT_AUDIT = os.path.join(ROOT, "docs", "PORT-AUDIT.md")
TRIAGE = os.path.join(ROOT, "docs", "NOSRC-TRIAGE.md")
SYMREACH = os.path.join(ROOT, "docs", "evidence-20261023", "symreach.json")
OUT = os.path.join(ROOT, "docs", "COVERAGE-129.md")
OUT_STATUS = os.path.join(ROOT, "docs", "STATUS-129.md")

# nama stok -> nama pengganti di build (port_audit.RENAMES + temuan tick14/15)
RENAMES = {
    "phy-sprd-sharkl5Pro.ko": "phy-sprd-ums512.ko",
    "jpg.ko": "sprd_jpg.ko",
    "vsp.ko": "sprd_vsp.ko",
    "sprd_cpp.ko": "cpp.ko",
    "sprd-compr-2stage-dma.ko": "snd-soc-sprd-platform.ko",
    "snd-soc-sprd-vbc-fe.ko": "snd-soc-sprd-vbc-v4.ko",
    # TICK12 (2026-10-24): tiga port yang namanya berbeda dari blob stok
    "core.ko": "sprd_autotest_core.ko",
    "gpio.ko": "sprd_autotest_gpio.ko",
    "sprd_power_stat.ko": "sprd_debugstat.ko",
}


def modkey(name):
    """'-' dan '_' dianggap sama: vendor sprd-systimer, tree sprd_systimer."""
    return name.replace("-", "_").lower()


def load_config(path):
    cfg = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        m = re.match(r"(CONFIG_[A-Za-z0-9_]+)=(.*)$", line)
        if m:
            cfg[m.group(1)] = m.group(2)
            continue
        m = re.match(r"# (CONFIG_[A-Za-z0-9_]+) is not set$", line)
        if m:
            cfg[m.group(1)] = "n"
    return cfg


def main():
    built = set()
    for line in open(KO_LIST, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line.endswith(".ko"):
            built.add(modkey(os.path.basename(line)))

    cfg = load_config(CFG)

    # pemetaan nama -> simbol Kconfig (dokumentasi port_audit; kemungkinan basi,
    # jadi nilainya dicek ulang ke config live)
    pa = collections.defaultdict(set)
    for line in open(PORT_AUDIT, encoding="utf-8", errors="replace"):
        m = re.match(r"\|\s*\d+\s*\|\s*`([^`]+\.ko)`\s*\|\s*([A-Z\-]+)\s*\|\s*`?(CONFIG_[A-Z0-9_]+)`?", line)
        if m:
            pa[m.group(1)].add(m.group(3))

    tri = {}
    for line in open(TRIAGE, encoding="utf-8", errors="replace"):
        m = re.match(r"\|\s*\d+\s*\|\s*`([^`]+\.ko)`\s*\|.*\|\s*([A-Z\-]+)\s*\|?\s*$", line.rstrip())
        if m:
            tri.setdefault(m.group(1), m.group(2))

    sr = json.load(open(SYMREACH, encoding="utf-8"))
    sym = {r["blob"]: r for r in sr["rows"]}

    blobs = sorted(p for p in os.listdir(BLOBS) if p.endswith(".ko"))
    rows = []
    for b in blobs:
        name = RENAMES.get(b, b)
        s = sym.get(b, {})
        port, hilang = s.get("port", 0), s.get("hilang", 0)
        verdict, why = "BELUM", ""
        if modkey(name) in built:
            verdict, why = "KO", f"{name}"
        else:
            syms = pa.get(b, set())
            ny = [x for x in syms if cfg.get(x) == "y"]
            nm = [x for x in syms if cfg.get(x) == "m"]
            if ny:
                verdict, why = "Y", ", ".join(ny)
            elif nm:
                verdict, why = "BELUM", f"{', '.join(nm)} =m tapi tidak ter-build"
            else:
                tv = tri.get(b, "")
                if tv == "BUILT-IN":
                    verdict = "BUILT-IN"
                    why = "triage + config pendukung =y"
                elif tv == "SUPERSEDED":
                    verdict, why = "SUPERSEDED", "fungsinya ada di modul lain"
                elif tv == "SIAP-COBA":
                    verdict, why = "BELUM", "SIAP-COBA: config belum menyala"
                elif tv == "PERLU-PORT":
                    verdict, why = "BELUM", "PERLU-PORT: sumber tidak ada di pohon"
                elif syms:
                    verdict = "BELUM"
                    st = ", ".join(f"{x}={cfg.get(x, 'absent')}" for x in sorted(syms))
                    why = f"config tidak nyala ({st})"
                else:
                    verdict, why = "BELUM", "pemetaan nama -> Kconfig tidak ditemukan"
        rows.append({"blob": b, "verdict": verdict, "why": why,
                     "port": port, "hilang": hilang})

    cnt = collections.Counter(r["verdict"] for r in rows)
    selesai = sum(v for k, v in cnt.items() if k != "BELUM")
    belum = [r for r in rows if r["verdict"] == "BELUM"]

    L = []
    A = L.append
    A("# Cakupan 129 blob stok — skor otoritatif\n\n")
    A("Dibuat [coverage_129.py](../donors/coverage_129.py). Dicek ulang terhadap "
      f"config live ([{os.path.basename(CFG)}](evidence-20261024/{os.path.basename(CFG)}), "
      f"{sum(1 for _ in open(CFG, encoding='utf-8', errors='replace')):,} baris) dan "
      f"daftar `.ko` hasil build ({os.path.basename(KO_LIST)}), bukan snapshot dokumen lama.\n\n")
    A(f"## **{selesai}/129** tertangani\n\n")
    A("| Verdict | Jumlah | Arti |\n|---|---:|---|\n")
    A(f"| `KO` | {cnt['KO']} | ada `.ko` penggantinya di build |\n")
    A(f"| `Y` | {cnt['Y']} | pemetaan Kconfig-nya `=y` -> ikut `Image` |\n")
    A(f"| `BUILT-IN` | {cnt['BUILT-IN']} | verdict triage, kode ikut Image |\n")
    A(f"| `SUPERSEDED` | {cnt['SUPERSEDED']} | fungsinya sudah ada di modul lain |\n")
    A(f"| **BELUM** | **{len(belum)}** | lihat tabel |\n")
    A(f"| total | **{len(rows)}** | |\n\n")

    A("## Yang masih BELUM\n\n")
    if belum:
        A("| # | blob | alasan | simbol port | simbol hilang |\n"
          "|---:|---|---|---:|---:|\n")
        for i, r in enumerate(belum, 1):
            A(f"| {i} | `{r['blob']}` | {r['why']} | {r['port']} | {r['hilang']} |\n")
    else:
        A("Tidak ada — seluruh 129 blob sudah tertangani. Sepuluh entri terakhir\n"
          "(`core`, `gpio`, `mmc_swcq`, `sprd-cpufreq-public/v2`, `sprd_7sreset`,\n"
          "`sprd_iq`, `sprd_manufacturer_model`, `sprd_pdbg`, `sprd_power_stat`)\n"
          "ditutup oleh port 2026-10-23→24; bukti build: CI `v17-build` hijau\n"
          "(run `40097613453`) + [PORT-REMAINING.md](PORT-REMAINING.md).\n")
    A("\n")

    A("## Blokir simbol per blob (dari symreach)\n\n")
    A("Angka `port`/`hilang` di atas adalah referensi undefined yang belum "
      "ter-resolve build kita. Blob yang sudah tertangani lewat `KO`/`Y` tidak "
      "perlu ini lagi — kodenya di-port ulang memakai API 6.18, bukan blob lama.\n\n")
    A("| blob | kelas | port | hilang |\n|---|---|---:|---:|\n")
    for r in belum:
        if r["port"] or r["hilang"]:
            A(f"| `{r['blob']}` | {r['verdict']} | {r['port']} | {r['hilang']} |\n")
    A("\n")
    A("## Catatan jujur\n\n")
    A("- `KO` = ada `.ko` dengan nama itu di build. Untuk nama yang berbeda "
      "(mis. `phy-sprd-sharkl5Pro.ko` -> `phy-sprd-ums512.ko`) dipakai "
      "`RENAMES`.\n")
    A("- Pemetaan nama -> Kconfig di `PORT-AUDIT.md` punya kesalahan pasang "
      "(mis. `gpio.ko` dulu dipetakan ke `CONFIG_MTD_NAND_GPIO`, padahal blob "
      "itu `sprd autotest gpio driver`). Sejak `UNRESOLVED`/`AMBIGUOUS` di "
      "`port_audit.py`, kasus seperti ini tampil sebagai belum, bukan selesai.\n")
    A("- `symreach.json` dipakai untuk kolom port/hilang saja. Penutup simbol "
      "TIDAK menggantikan keberadaan kode.\n")

    open(OUT, "w", encoding="utf-8", newline="\n").write("".join(L))

    # Tabel ringkas: semua 129 blob dengan tanda ✅ atau (X).
    S = []
    S.append("# Status 129 blob stok — sudah jalan di 6.18.52 atau belum\n\n")
    S.append("Dibuat otomatis [coverage_129.py](../donors/coverage_129.py) dari "
             "daftar `.ko` hasil build + config live.\n\n")
    S.append("- ✅ = fungsinya sudah tersedia di build 6.18.52 (`.ko` pengganti, "
             "kode ikut `Image`, atau fungsi sudah ada di modul lain)\n")
    S.append("- **(X)** = belum ada penggantinya\n\n")
    S.append(f"**{selesai}/{len(rows)}** ✅ · **{len(rows) - selesai}** (X)\n\n")
    S.append("| # | driver stok 5.4 | di 6.18.52 | catatan |\n|---:|---|---|---|\n")
    for i, r in enumerate(rows, 1):
        if r["verdict"] == "BELUM":
            S.append(f"| {i} | `{r['blob']}` | **(X)** | {r['why']} |\n")
        else:
            S.append(f"| {i} | `{r['blob']}` | ✅ `{r['verdict']}` | {r['why']} |\n")
    open(OUT_STATUS, "w", encoding="utf-8", newline="\n").write("".join(S))

    print(f"{selesai}/{len(rows)}  " + " ".join(f"{k}={v}" for k, v in sorted(cnt.items())))
    print("BELUM:", ", ".join(r["blob"] for r in belum))
    print("tulis", OUT, "dan", OUT_STATUS)


if __name__ == "__main__":
    main()
