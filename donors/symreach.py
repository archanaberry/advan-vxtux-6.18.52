#!/usr/bin/env python3
"""symreach.py - matriks keterjangkauan simbol untuk 129 blob stok 5.4.210.

Menjawab satu pertanyaan untuk SETIAP undefined symbol di blob stok:
"di build 6.18.52 kita, simbol ini bisa dijangkau atau tidak, dan kalau
tidak, apa yang harus dilakukan?"

Sumber kebenaran:
  - donors/symvers_tick14.txt   : semua ekspor (vmlinux + 163 modul) build kita
  - donors/vmlinux_o_syms.txt   : nm --defined-only vmlinux.o build tick13
  - donors/stock_vendor_modules_all/*.ko : blob stok 5.4.210 (ELF aarch64)

Kelas:
  ok        diekspor vmlinux/modul kita
  ekspor    ada di vmlinux.o tapi tidak diekspor -> 1 baris EXPORT_SYMBOL
  rename    ada padanan 6.18 (printk->_printk, mutex_lock->*_noprof)
  makro     jadi makro/static inline/stub di header 6.18 -> source beres sendiri
  port      hanya disediakan blob stok lain -> modul itu wajib di-port
  instr     __cfi_*/__ubsan_* dari build Clang stok, bukan API kernel
  hilang    tidak ada di mana pun -> harus ditulis

    python donors/symreach.py
"""
import bisect
import collections
import json
import os
import re
import subprocess

from elftools.elf.elffile import ELFFile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBS = os.path.join(ROOT, "donors", "stock_vendor_modules_all")
SYMVERS = os.path.join(ROOT, "donors", "symvers_tick14.txt")
VMLINUX_SYMS = os.path.join(ROOT, "donors", "vmlinux_o_syms.txt")
TREE = os.path.join(ROOT, "6.18.52-VXTux-UMS512")
OUT_MD = os.path.join(ROOT, "docs", "UNDEF-REACHABILITY.md")
OUT_JSON = os.path.join(ROOT, "docs", "evidence-20261023", "symreach.json")
CACHE = os.path.join(ROOT, "donors", ".symreach_defs.json")

# __versions di 5.4 arm64: struct modversion_info { unsigned long crc; char name[56]; }
VER_REC, VER_NAMELEN = 64, 56

INSTR_PREFIX = ("__cfi_", "__ubsan_", "__sanitizer_", "__stack_chk_",
                "__ksan", "__kcsan", "__kasan", "__hwasan", "llvm.",
                "__tsan", "__msan", "__scs_")

ORDER = ("ok", "ekspor", "rename", "makro", "port", "instr", "hilang")

# Rename semantik 5.4 -> 6.18. Hanya dipakai kalau targetnya TERBUKTI ada di
# vmlinux.o kita; kalau tidak, entri dilewati (biar tidak jadi tebakan).
REMAP = {
    "del_timer": "timer_delete",
    "del_timer_sync": "timer_delete_sync",
    "__class_create": "class_create",
    "__class_register": "class_register",
    "no_llseek": "noop_llseek",
    "__memset_io": "memset_io",
    "__alloc_pages_nodemask": "__alloc_pages_noprof",
    "gpiochip_find": "gpio_device_find",
    "pwm_request": "pwm_get",
    "pwm_free": "pwm_put",
    "pwm_apply_state": "pwm_apply_might_sleep",
    "dma_request_slave_channel": "dma_request_chan",
    "kmem_cache_alloc_trace": "kmem_cache_alloc_noprof",
    "hrtimer_init": "hrtimer_setup",
}


def is_instrumentation(name):
    return name.startswith(INSTR_PREFIX)


def load_symvers(path):
    exp = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 4:
                continue
            exp.setdefault(parts[1], []).append(
                {"crc": parts[0], "provider": parts[2], "type": parts[3]})
    return exp


def blob_symbols(path):
    """-> (exports set, undef [(name, bind, crc)], vermagic)"""
    exports, undef, crcs = set(), [], {}
    with open(path, "rb") as f:
        elf = ELFFile(f)
        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            return set(), [], ""
        for s in symtab.iter_symbols():
            n = s.name
            if not n:
                continue
            if n.startswith("__crc_"):
                exports.add(n[len("__crc_"):])
            if s["st_shndx"] == "SHN_UNDEF":
                undef.append((n, s["st_info"]["bind"]))
        if not exports:
            for sec in elf.iter_sections():
                m = re.match(r"^___ksymtab[+_](.+)$", sec.name)
                if m:
                    exports.add(m.group(1))
        ver = elf.get_section_by_name("__versions")
        if ver is not None:
            data = ver.data()
            for off in range(0, len(data) - VER_REC + 1, VER_REC):
                crc = int.from_bytes(data[off:off + 8], "little")
                raw = data[off + 8:off + 8 + VER_NAMELEN]
                name = raw.split(b"\x00")[0].decode("utf-8", "replace")
                if name:
                    crcs[name] = "0x%08x" % (crc & 0xFFFFFFFF)
    with open(path, "rb") as f:
        raw = f.read()
    m = re.search(rb"vermagic=([^\x00]+)", raw)
    return exports, [(n, b, crcs.get(n)) for n, b in undef], (
        m.group(1).decode("utf-8", "replace") if m else "")


def tree_defs(names, tree, refresh=False):
    """Cari DEFINISI simbol di pohon 6.18 -> {name: 'file:line'}.

    Definisi kernel selalu mulai di kolom 0 (`int foo(...)`, `static inline
    ... foo(`, `#define foo`, `DEFINE_PER_CPU(..., cpu_number)`), sedangkan
    pemanggilan menjorok ke dalam fungsi. Batasan kolom 0 ini yang memisahkan
    definisi dari call-site palsu. Hasil di-cache karena git grep seluruh
    pohon butuh ~7 menit untuk 1.400 nama.
    """
    cache = {}
    if os.path.exists(CACHE) and not refresh:
        try:
            with open(CACHE, encoding="utf-8") as f:
                cache = json.load(f)
        except (OSError, ValueError):
            cache = {}
    todo = [n for n in names if len(n) > 2 and n not in cache]
    pats = (r"^[A-Za-z_#].*\b(%s)\s*\(",
            r"^#\s*define\s+(%s)\b",
            r"^[A-Za-z_#].*\b(%s)\b\s*[)=;\[]")
    for i in range(0, len(todo), 40):
        chunk = todo[i:i + 40]
        alt = "|".join(re.escape(n) for n in chunk)
        hit = {}
        for pat in pats:
            cmd = ["git", "grep", "-n", "-E", pat % alt, "--",
                   "*.c", "*.h", "*.S"]
            try:
                r = subprocess.run(cmd, cwd=tree, capture_output=True,
                                   text=True, errors="replace", timeout=600)
            except (subprocess.TimeoutExpired, FileNotFoundError):
                continue
            if r.returncode not in (0, 1):
                continue
            for line in r.stdout.splitlines():
                loc, _, rest = line.partition(":")
                ln, _, text = rest.partition(":")
                for n in chunk:
                    if n not in hit and re.search(r"\b%s\b" % re.escape(n), text):
                        hit[n] = f"{loc}:{ln}"
        for n in chunk:
            cache[n] = hit.get(n)
        try:  # simpan per chunk: grep bisa lama, jangan ulang dari nol
            with open(CACHE, "w", encoding="utf-8") as f:
                json.dump(cache, f, indent=0, sort_keys=True)
        except OSError:
            pass
    return {n: cache[n] for n in names if cache.get(n)}


def nearest(name, pool_sorted, pool_set):
    cands = [a for a in ("_" + name, name + "_noprof") if a in pool_set]
    if cands:
        return cands
    key = name[:10]
    i = bisect.bisect_left(pool_sorted, key)
    while i < len(pool_sorted) and pool_sorted[i].startswith(key) and len(cands) < 2:
        if pool_sorted[i] != name:
            cands.append(pool_sorted[i])
        i += 1
    return cands


def main():
    exports = load_symvers(SYMVERS)
    blobs = sorted(p for p in os.listdir(BLOBS) if p.endswith(".ko"))
    undef_all, exports_all, vermagic = {}, {}, {}
    for b in blobs:
        e, u, v = blob_symbols(os.path.join(BLOBS, b))
        exports_all[b], undef_all[b], vermagic[b] = e, [x for x in u if x[0]], v

    provider_of = collections.defaultdict(set)
    for b, ex in exports_all.items():
        for s in ex:
            provider_of[s].add(b)

    # vmlinux.o: "nama<TAB>tipe". Tipe huruf besar = global (bisa di-link modul),
    # huruf kecil = lokal (salinan out-of-line dari static inline) — bukan kandidat
    # EXPORT_SYMBOL, jadi harus dibedakan.
    vmlinux_all, vmlinux_global = set(), set()
    with open(VMLINUX_SYMS, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if not parts or not parts[0]:
                continue
            vmlinux_all.add(parts[0])
            if len(parts) > 1 and parts[1][:1].isupper():
                vmlinux_global.add(parts[0])
    vmlinux_set = vmlinux_all
    vmlinux_sorted = sorted(vmlinux_all)

    unique_undef = sorted({n for b in blobs for n, _x, _y in undef_all[b]})
    # hanya kandidat yang mungkin perlu dicari di pohon: sisanya sudah pasti
    kandidat = [n for n in unique_undef
                if n not in exports
                and n not in vmlinux_global
                and ("_" + n) not in vmlinux_global
                and (n + "_noprof") not in vmlinux_global
                and not is_instrumentation(n)]
    makro = (tree_defs(kandidat, TREE, refresh="--refresh" in os.sys.argv)
             if os.path.isdir(TREE) else {})

    remap = {k: v for k, v in REMAP.items() if v in vmlinux_global}

    def target_of(name):
        if ("_" + name) in vmlinux_global:
            return "_" + name
        if (name + "_noprof") in vmlinux_global:
            return name + "_noprof"
        return remap.get(name)

    def klass(name, blob):
        if name in exports:
            return "ok"
        if name in vmlinux_global:
            return "ekspor"
        if target_of(name):
            return "rename"
        if name in makro:
            return "makro"
        if any(p != blob for p in provider_of.get(name, ())):
            return "port"
        if is_instrumentation(name):
            return "instr"
        return "hilang"

    rows = []
    per_sym_blobs = collections.defaultdict(set)
    crc_beda = collections.Counter()
    for b in blobs:
        per = {"blob": b, "undef": 0, **{k: 0 for k in ORDER},
               "crc_beda": 0, "blokir_modul": collections.Counter(),
               "contoh": [], "map": {}}
        for name, bind, crc in undef_all[b]:
            per["undef"] += 1
            k = klass(name, b)
            per[k] += 1
            per_sym_blobs[name].add(b)
            if k == "ok":
                ours = exports[name][0]["crc"]
                if crc and ours and crc.lower() != ours.lower():
                    per["crc_beda"] += 1
                    crc_beda[name] += 1
            elif k == "rename":
                per["map"][name] = target_of(name)
            elif k == "port":
                for o in provider_of[name]:
                    if o != b:
                        per["blokir_modul"][o[:-3]] += 1
        for name in sorted(per["map"]):
            per["contoh"].append((name, [per["map"][name]]))
        for name in sorted(n for n in per_sym_blobs
                           if n in {x for x, _c, _y in undef_all[b]}
                           and klass(n, b) == "hilang")[:10]:
            if len(per["contoh"]) < 14:
                per["contoh"].append(
                    (name, nearest(name, vmlinux_sorted, vmlinux_set)))
        rows.append(per)

    leverage = collections.Counter()
    for r in rows:
        for m, c in r["blokir_modul"].items():
            leverage[m] += c

    t = {k: sum(r[k] for r in rows) for k in ORDER}
    t["undef"] = sum(r["undef"] for r in rows)
    t["crc_beda"] = sum(r["crc_beda"] for r in rows)
    siap = [r for r in rows if r["port"] == 0 and r["hilang"] == 0]
    terblokir = [r for r in rows if r["port"] or r["hilang"]]
    tanpa_undef = [r for r in rows if r["undef"] == 0]

    ekspor_syms = collections.Counter(
        {n: len(per_sym_blobs[n]) for n in per_sym_blobs if klass(n, "") == "ekspor"})
    rename_syms = collections.Counter()
    rename_target = {}
    for n in per_sym_blobs:
        if klass(n, "") == "rename":
            rename_syms[n] = len(per_sym_blobs[n])
            rename_target[n] = target_of(n)
    makro_syms = collections.Counter(
        {n: len(per_sym_blobs[n]) for n in per_sym_blobs if klass(n, "") == "makro"})
    instr_syms = collections.Counter(
        {n: len(per_sym_blobs[n]) for n in per_sym_blobs if klass(n, "") == "instr"})
    hilang_syms = collections.Counter(
        {n: len(per_sym_blobs[n]) for n in per_sym_blobs if klass(n, "") == "hilang"})

    L = []
    A = L.append
    A("# Keterjangkauan Simbol — 129 blob stok 5.4.210 vs build 6.18.52\n\n")
    A("Dihasilkan [symreach.py](../donors/symreach.py). Sumber: "
      "[symvers_tick14.txt](../donors/symvers_tick14.txt) "
      f"({len(exports):,} ekspor dari vmlinux + 163 modul), "
      f"`stock_vendor_modules_all/*.ko` ({len(blobs)} blob, vermagic "
      f"`{vermagic.get(blobs[0], '?')}`), dan "
      "[vmlinux_o_syms.txt](../donors/vmlinux_o_syms.txt) "
      f"({len(vmlinux_all):,} simbol dari `nm --defined-only vmlinux.o`, "
      f"{len(vmlinux_global):,} di antaranya global; "
      f"{len({n for n, v in exports.items() if any(e['provider'] == 'vmlinux' for e in v)} & vmlinux_all):,}"
      f" dari {len({n for n, v in exports.items() if any(e['provider'] == 'vmlinux' for e in v)}):,}"
      " ekspor vmlinux kita ada di situ).\n\n")
    A("Angka = **referensi** (`SHN_UNDEF` di `.symtab`), bukan simbol unik: "
      "satu simbol yang dipakai 60 blob dihitung 60 kali. "
      f"Simbol unik: {len(unique_undef):,}.\n\n")

    A("## Ringkasan\n\n")
    A("| Kelas | Referensi | Arti |\n|---|---:|---|\n")
    A(f"| `ok` | **{t['ok']:,}** "
      f"({100.0 * t['ok'] / max(t['undef'], 1):.1f}%) | diekspor build kita |\n")
    A(f"| `ekspor` | {t['ekspor']:,} | ada di vmlinux.o, tinggal `EXPORT_SYMBOL` |\n")
    A(f"| `rename` | {t['rename']:,} | padanan 6.18 ada, source menyesuaikan |\n")
    A(f"| `makro` | {t['makro']:,} | definisinya ADA di pohon 6.18 |\n")
    A(f"| `port` | {t['port']:,} | hanya disediakan blob stok lain |\n")
    A(f"| `instr` | {t['instr']:,} | runtime Clang CFI/UBSan di build stok |\n")
    A(f"| `hilang` | {t['hilang']:,} | tidak ada di mana pun, harus ditulis |\n")
    A(f"| **Total** | **{t['undef']:,}** | |\n")
    A(f"| CRC modversion beda | {t['crc_beda']:,} | dari "
      f"{t['ok']:,} referensi `ok` |\n")
    A(f"| Blob tanpa port | **{len(siap)}/{len(blobs)}** | tidak menunggu modul stok |\n")
    A(f"| Blob tanpa undefined (export-only) | {len(tanpa_undef)} | |\n\n")

    vendor_abi = ["cpu_hwcaps", "cpu_hwcap_keys", "arm64_const_caps_ready",
                  "kimage_vaddr", "cpu_number"]
    va_refs = sum(len(per_sym_blobs.get(n, ())) for n in vendor_abi)
    va_cfg = ""
    try:
        with open(os.path.join(ROOT, "docs", "evidence-20261023",
                               "config_tick13.txt"), encoding="utf-8",
                  errors="replace") as f:
            for line in f:
                if ("CONFIG_VXTUX_VENDOR_ABI" in line
                        and "CFG80211" not in line):
                    va_cfg = line.strip()
                    break
    except OSError:
        pass
    if va_refs:
        A(f"> **Catatan config.** {va_refs} referensi di kelas `makro` di atas "
          "adalah " + ", ".join(f"`{n}`" for n in vendor_abi) +
          ". Penyedianya sudah ada di pohon "
          "(`drivers/misc/sprd/vendor_abi/vendor_abi_arm64.c`), tetapi "
          f"`CONFIG_VXTUX_VENDOR_ABI` di `config_tick13` masih `{va_cfg or 'tidak diketahui'}`. "
          "Jadi untuk lima simbol ini yang kurang bukan kode "
          "dan bukan API: satu config.\n\n")

    A("## Blob yang tidak butuh port\n\n")
    A("| blob | undef | ok | ekspor | rename | makro | instr |\n"
      "|---|---:|---:|---:|---:|---:|---:|\n")
    for r in siap:
        A(f"| `{r['blob']}` | {r['undef']} | {r['ok']} | {r['ekspor']} | "
          f"{r['rename']} | {r['makro']} | {r['instr']} |\n")
    A("\n")

    A("## Blob terblokir\n\n")
    A("| blob | undef | ok | port | hilang | makro | modul stok penyedia |\n"
      "|---|---:|---:|---:|---:|---:|---|\n")
    for r in terblokir:
        top = ", ".join(f"`{m}`×{c}" for m, c in r["blokir_modul"].most_common(3))
        A(f"| `{r['blob']}` | {r['undef']} | {r['ok']} | {r['port']} | "
          f"{r['hilang']} | {r['makro']} | {top or '—'} |\n")
    A("\n")

    A("## Leverage: satu modul di-port, sekian referensi terbuka\n\n")
    A("| modul stok | referensi terbuka |\n|---|---:|\n")
    for m, c in leverage.most_common(15):
        A(f"| `{m}` | {c} |\n")
    A("\n")

    A("## `ekspor` — simbol GLOBAL ada di vmlinux kita, cuma tidak diekspor\n\n")
    A(f"{len(ekspor_syms)} simbol. Ini yang paling murah: satu baris "
      "`EXPORT_SYMBOL[_GPL]()` di file yang sudah ada di pohon. Simbol yang "
      "cuma muncul sebagai salinan **lokal** (`t`) dari `static inline` tidak "
      "dihitung di sini — itu masuk `makro`.\n\n")
    A("| simbol | blob |\n|---|---:|\n")
    for s, c in ekspor_syms.most_common(40):
        A(f"| `{s}` | {c} |\n")
    A("\n")

    A("## `rename` — padanan 6.18 ada\n\n")
    A("Semua padanan di bawah **diverifikasi** ada di `vmlinux.o` kita: aturan "
      "mekanis (`_nama`, `nama_noprof`) atau tabel `REMAP` di symreach.py \n      "
      "yang hanya dipakai kalau targetnya benar-benar ada.\n\n")
    A("| nama blob 5.4 | padanan | blob |\n|---|---|---:|\n")
    for s, c in rename_syms.most_common(40):
        A(f"| `{s}` | `{rename_target[s]}` | {c} |\n")
    A("\n")

    A("## `makro` — definisinya ada di pohon kita\n\n")
    A(f"{len(makro_syms)} simbol. Di blob 5.4 ini fungsi sungguhan; di 6.18 "
      "definisinya ada tapi **tidak jadi simbol**: `static inline`, `#define`, "
      "atau file-nya tidak ikut di-build karena CONFIG mati. Artinya ini urusan "
      "compile/config, **bukan** tulis kode baru. Bukti file:line dari pohon kita:\n\n")
    A("| simbol | blob | jadi makro di |\n|---|---:|---|\n")
    for s, c in makro_syms.most_common(40):
        A(f"| `{s}` | {c} | `{makro[s]}` |\n")
    A("\n")

    A("## `instr` — runtime Clang, bukan API kernel\n\n")
    A("| simbol | blob |\n|---|---:|\n")
    for s, c in instr_syms.most_common(12):
        A(f"| `{s}` | {c} |\n")
    A("\n")

    A("## `hilang` — harus ditulis / diganti kode\n\n")
    A(f"{len(hilang_syms)} simbol unik. Kolom kanan = kandidat terdekat di "
      "vmlinux 6.18.52.\n\n")
    A("| simbol | blob | kandidat |\n|---|---:|---|\n")
    for s, c in hilang_syms.most_common(40):
        cand = ", ".join(f"`{x}`" for x in nearest(s, vmlinux_sorted, vmlinux_set))
        A(f"| `{s}` | {c} | {cand or '—'} |\n")
    A("\n")

    A("## Lembar kerja per blob terblokir\n\n")
    for r in terblokir:
        A(f"**`{r['blob']}`** — undef {r['undef']}, ok {r['ok']}, "
          f"port {r['port']}, hilang {r['hilang']}\n\n")
        for name, cand in r["contoh"]:
            extra = f" → `{cand[0]}`" if cand else ""
            A(f"- `{name}`{extra}\n")
        A("\n")

    A("## Metodologi\n\n")
    A("1. `.symtab` setiap blob dibaca **pyelftools**: `SHN_UNDEF` = referensi "
      "keluar, `__crc_<nama>` = ekspor modul itu.\n")
    A("2. Urutan pencocokan: `Module.symvers` tick13 → `nm vmlinux.o` → padanan "
      "`_nama`/`nama_noprof` → `#define`/`static inline` di header (git grep) → "
      "ekspor blob stok lain → daftar sanitizer → `hilang`.\n")
    A("3. Seksi `__versions` (64 byte/entri: CRC 8 + nama 56) dibandingkan "
      "dengan CRC `Module.symvers`. Beda CRC = `insmod` menolak walau nama ada. "
      "Angka besarnya wajar: 5.4.210 → 6.18.52 mengubah banyak tipe.\n")
    A("4. `makro` dipisah dari `hilang` lewat `git grep` definisi di kolom 0 "
      "(`^[A-Za-z_].*nama(` atau `#define nama`), karena definisi kernel "
      "selalu di kolom 0 sedangkan pemanggilan menjorok. Simbol dengan "
      "definisi di pohon berarti masalah compile/config, bukan port kode.\n")

    with open(OUT_MD, "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(L))

    os.makedirs(os.path.dirname(OUT_JSON), exist_ok=True)
    with open(OUT_JSON, "w", encoding="utf-8") as f:
        json.dump({
            "vermagic": vermagic.get(blobs[0], ""),
            "exports_total": len(exports),
            "vmlinux_symbols": len(vmlinux_set),
            "unique_undef": len(unique_undef),
            "classes": t,
            "blobs": len(blobs), "blobs_no_port": len(siap),
            "leverage": dict(leverage.most_common(40)),
            "ekspor": dict(ekspor_syms.most_common(100)),
            "rename": {k: rename_target[k] for k, _ in rename_syms.most_common(100)},
            "definisi_pohon": {k: makro[k] for k, _ in makro_syms.most_common(200)},
            "instr": dict(instr_syms.most_common(50)),
            "hilang": dict(hilang_syms.most_common(200)),
            "crc_beda": dict(crc_beda.most_common(100)),
            "rows": [{k: (dict(v) if isinstance(v, collections.Counter) else v)
                      for k, v in r.items()} for r in rows],
        }, f, indent=1, default=str)

    print(f"undef={t['undef']} ok={t['ok']} ({100.0*t['ok']/max(t['undef'],1):.1f}%) "
          f"ekspor={t['ekspor']} rename={t['rename']} makro={t['makro']} "
          f"port={t['port']} instr={t['instr']} hilang={t['hilang']} "
          f"crc_beda={t['crc_beda']} siap={len(siap)}/{len(blobs)}")
    print("tulis", OUT_MD)


if __name__ == "__main__":
    main()
