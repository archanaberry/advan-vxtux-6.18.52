#!/usr/bin/env python3
"""Dossier RE untuk blob yang sumbernya tidak ada di pohon 6.18.52.

Untuk tiap blob `PERLU-PORT` kita kumpulkan bukti yang bisa dibaca tanpa
decompiler penuh:

  * modinfo (description/author/depends/vermagic) - apa itu
  * simbol EXPORT       - ABI yang ditawarkan ke modul lain
  * simbol UNDEFINED    - API kernel yang dipakai (menentukan subsistem)
  * string penanda      - compatible DT, /proc, /sys, ioctl, nama register
  * jumlah fungsi       - ukuran pekerjaan port

Output: docs/RE-PORT-DOSSIER.md

    python donors/port_dossier.py
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
BLOBS = os.path.join(HERE, "stock_vendor_modules_all")
OUT = os.path.join(ROOT, "docs", "RE-PORT-DOSSIER.md")

BIN = (r"C:\Users\Administrator\scoop\apps\gcc-aarch64-none-linux-gnu"
       r"\15.2.rel1\bin")
NM = os.path.join(BIN, "aarch64-none-linux-gnu-nm.exe")
STRINGS = os.path.join(BIN, "aarch64-none-linux-gnu-strings.exe")

# 12 blob PERLU-PORT (hasil donors/nosrc_triage.py) + catatan subsistem.
TARGETS = [
    ("bq2560x-charger.ko", "power supply / charger"),
    ("mmc_swcq.ko", "MMC host queue"),
    ("snd-soc-sprd-codec-sc2730-power.ko", "ASoC codec power"),
    ("snd-soc-sprd-codec-sc2730-power-dev.ko", "ASoC codec power (device)"),
    ("sprd-cpufreq-public.ko", "cpufreq"),
    ("sprd-cpufreq-v2.ko", "cpufreq"),
    ("sprd_7sreset.ko", "SoC reset"),
    ("sprd_iq.ko", "camera ISP tuning"),
    ("sprd_manufacturer_model.ko", "SoC id"),
    ("sprd_pdbg.ko", "power debug"),
    ("sprd_power_stat.ko", "power debug"),
    ("sprd_sip_svc.ko", "firmware SIP service"),
]

STRONG = re.compile(
    r"(sprd,|unisoc,|^/proc/|^/sys/|^/dev/|^/vendor|_ioctl$|_probe$|_remove$"
    r"|^compatible$|^MODULE_|^sprd_|^sc27|^bq25|^mmc_|^sip_|^pm_|^clk_"
    r"|^regmap_|^devm_|^platform_|^of_|^crypto_|^cpufreq_|^thermal_"
    r"|^power_supply|^snd_|^soc_|^sched_|^reset_|^mfd_)")


def nm(ko, flag):
    return subprocess.run([NM, flag, ko], capture_output=True,
                          text=True).stdout.splitlines()


def syms(ko):
    defined, undef, funcs = [], set(), 0
    for line in nm(ko, "--defined-only"):
        p = line.split()
        if len(p) < 3:
            continue
        typ, name = p[-2], p[-1]
        if name.startswith("__ksymtab_"):
            defined.append((typ, name[len("__ksymtab_"):]))
        elif typ in "Tt":
            funcs += 1
    for line in nm(ko, "--undefined-only"):
        p = line.split()
        if len(p) >= 2:
            undef.add(p[-1])
    return defined, undef, funcs


def modinfo(ko):
    out = subprocess.run([STRINGS, "-a", ko], capture_output=True,
                         text=True).stdout
    info = {}
    for line in out.splitlines():
        for key in ("description", "author", "license", "vermagic",
                    "depends", "alias"):
            if line.startswith(key + "="):
                info.setdefault(key, []).append(line.split("=", 1)[1])
    return info


def markers(ko):
    """String yang biasanya menandai antarmuka driver."""
    out = subprocess.run([STRINGS, "-a", "-n", "4", ko],
                         capture_output=True, text=True).stdout
    keep = []
    for s in out.splitlines():
        s = s.strip()
        if not s or len(s) > 90:
            continue
        if s.startswith(("sprd,", "unisoc,", "compatible")):
            keep.append(s)
        elif s.startswith(("/proc", "/sys", "/dev", "/vendor")):
            keep.append(s)
        elif re.fullmatch(r"[a-z0-9_]+@[0-9a-f]+", s):
            keep.append(s)
    seen, uniq = set(), []
    for s in keep:
        if s not in seen:
            seen.add(s)
            uniq.append(s)
    return uniq[:14]


def sub_for(name):
    """Tebak subsistem dari nama simbol yang dipakai."""
    groups = {
        "regulator": ("regulator_",),
        "power_supply": ("power_supply_", "psy_"),
        "mmc": ("mmc_", "sdhci_"),
        "cpufreq": ("cpufreq_", "dev_pm_opp"),
        "ASoC/ALSA": ("snd_soc_", "snd_pcm_", "snd_ctl_"),
        "i2c/spi": ("i2c_", "spi_"),
        "sysfs/procfs": ("proc_", "sysfs_", "debugfs_"),
        "reset/regmap": ("reset_", "regmap_"),
        "platform/OF": ("platform_", "of_", "device_"),
        "misc/char": ("misc_register", "cdev_", "alloc_chrdev"),
        "firmware/SIP": ("arm_smccc_", "ffa_", "tee_"),
        "dma/ion": ("dma_", "ion_"),
    }
    hit = []
    joined = " ".join(sorted(name))
    for sub, keys in groups.items():
        if any(k in joined for k in keys):
            hit.append(sub)
    return hit


def dts_lookup(compat, dts_dir):
    """Cari compatible string ini di DTS/DTI pohon; -> daftar file."""
    hit = []
    for base, _dirs, names in os.walk(dts_dir):
        for n in names:
            if not n.endswith((".dts", ".dtsi")):
                continue
            p = os.path.join(base, n)
            try:
                text = open(p, encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            if compat in text:
                hit.append(os.path.relpath(p, os.path.dirname(dts_dir)))
    return hit


def main():
    L = ["# Dossier RE — 12 blob stok yang sumbernya tidak ada di pohon\n\n",
         "Dibuat oleh `donors/port_dossier.py` dari blob 5.4.210 itu sendiri\n",
         "(`nm` + `strings`), tanpa decompiler. Tujuannya: tiap blob punya\n",
         "spesifikasi awal yang cukup untuk menulis driver penggantinya —\n",
         "ABI yang harus disediakan, API kernel yang dipakai, dan nama-nama\n",
         "antarmuka (DT compatible, /proc, /sys) yang harus tetap sama.\n\n"]

    for ko, subsystem in TARGETS:
        path = os.path.join(BLOBS, ko)
        if not os.path.exists(path):
            L.append(f"\n## `{ko}`\n\nBlob tidak ada di hasil ekstrak.\n")
            continue
        defined, undef, funcs = syms(path)
        info = modinfo(path)
        mk = markers(path)
        subs = sub_for(undef)
        L.append(f"\n## `{ko}`\n\n")
        L.append(f"- ukuran: **{os.path.getsize(path)} B**, fungsi: **{funcs}**, "
                 f"simbol dipakai: **{len(undef)}**, EXPORT: **{len(defined)}**\n")
        L.append(f"- subsystem: {subsystem}\n")
        desc = (info.get("description") or ["-"])[0]
        auth = (info.get("author") or ["-"])[0]
        dep = (info.get("depends") or ["-"])[0]
        L.append(f"- description: `{desc}` · author: `{auth}` · depends: `{dep}`\n")
        if subs:
            L.append(f"- API kernel yang dipakai mengarah ke: {', '.join(subs)}\n")
        if defined:
            names = sorted(n for _, n in defined)
            L.append(f"- EXPORT ({len(names)}): " +
                     ", ".join(f"`{n}`" for n in names[:12]) + "\n")
        und = sorted(undef)
        L.append(f"- UNDEFINED ({len(und)}), 12 pertama: " +
                 ", ".join(f"`{n}`" for n in und[:12]) + "\n")
        if mk:
            L.append("- penanda antarmuka:\n")
            for s in mk:
                L.append(f"  - `{s}`\n")
        L.append("\n")

    # --- graf dependensi antar blob yang belum ada di pohon ---
    names = {ko[:-3]: ko for ko, _ in TARGETS}
    deps = {}
    for ko, _ in TARGETS:
        path = os.path.join(BLOBS, ko)
        if not os.path.exists(path):
            continue
        dep = (modinfo(path).get("depends") or [""])[0]
        want = [d.strip() for d in dep.split(",") if d.strip()]
        deps[ko] = [d for d in want if d in names]
    L.append("\n## Urutan port yang disarankan\n\n")
    L.append("Dihitung dari field `depends` di modinfo blob: modul yang tidak\n"
             "bergantung pada blob lain harus di-port lebih dulu.\n\n")
    ready = [ko for ko, d in deps.items() if not d]
    blocked = {ko: d for ko, d in deps.items() if d}
    L.append("**Tidak bergantung blob lain (bisa mulai sekarang):**\n\n")
    for ko in sorted(ready):
        L.append(f"- `{ko}`\n")
    if blocked:
        L.append("\n**Menunggu blob lain:**\n\n")
        for ko in sorted(blocked):
            L.append(f"- `{ko}` ← {', '.join('`' + d + '`' for d in blocked[ko])}\n")

    # --- apakah node DT-nya ada? ---
    dts_dir = os.path.join(ROOT, "6.18.52-VXTux-UMS512", "arch", "arm64",
                           "boot", "dts", "sprd")
    L.append("\n## Compatible DT: ada node-nya?\n\n")
    L.append("Blob hanya akan probe kalau `compatible`-nya ada di DTS. Kolom\n"
             "`ada di DTB stok` dibaca dari `donors/stock_initramfs/stock.dtb`:\n"
             "kalau **ya** tapi kolom DTS vxtux **tidak ada**, berarti node DT-nya\n"
             "ikut hilang dan harus di-port juga — bukan cuma kodenya.\n\n")
    dtb_path = os.path.join(HERE, "stock_initramfs", "stock.dtb")
    dtb = b""
    if os.path.exists(dtb_path):
        dtb = open(dtb_path, "rb").read()
    L.append("| blob | compatible | ada di DTB stok | ada di DTS vxtux |\n"
             "|---|---|---|---|\n")
    for ko, _ in TARGETS:
        path = os.path.join(BLOBS, ko)
        if not os.path.exists(path):
            continue
        for c in markers(path):
            if not c.startswith(("sprd,", "unisoc,")):
                continue
            hit = dts_lookup(c, dts_dir) if os.path.isdir(dts_dir) else []
            where = "`" + hit[0] + "`" if hit else "**tidak ada**"
            stock = "ya" if dtb and c.encode() in dtb else "tidak"
            L.append(f"| `{ko}` | `{c}` | {stock} | {where} |\n")

    L.append("\n## Cara memakai dossier ini\n\n")
    L.append("1. `UNDEFINED` menentukan subsistem: kalau isinya `regulator_*` dan\n"
             "   `power_supply_*`, driver baru harus mendaftar sebagai power supply,\n"
             "   bukan sebagai misc device.\n")
    L.append("2. `EXPORT` adalah kontrak ke modul lain — kalau blob lain memanggil\n"
             "   simbol itu, penggantinya wajib mengekspor nama yang sama, kalau\n"
             "   tidak `modprobe` modul pemakainya akan gagal.\n")
    L.append("3. Penanda `sprd,...` adalah `compatible` DT. Node DT di\n"
             "   `ums512-1h10-vxtux.dtb` harus cocok, kalau tidak driver tidak\n"
             "   pernah probe walau kodenya benar.\n")

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("".join(L))
    print(f"tulis {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
