#!/usr/bin/env python3
"""Rekam ID SMC yang dipakai tiap wrapper di blob sprd_sip_svc.ko.

Blob itu cuma mengekspor `sprd_sip_svc_get_handle()`, yang mengembalikan
pointer ke struct fungsi. Tiap fungsi di struct itu adalah pembungkus tipis
`__arm_smccc_smc(id, ...)`. Skrip ini membongkar blob, lalu untuk setiap
fungsi mencatat:

  * ID SMC di w0 (mov + movk sebelum `bl __arm_smccc_smc`)
  * argumen x1..x6 yang diisi sebelum panggilan
  * jumlah pemanggilan SMC di fungsi itu

Output: docs/RE-SPRD-SIP-SVC.md

    python donors/sip_svc_ids.py
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
KO = os.path.join(HERE, "stock_vendor_modules_all", "sprd_sip_svc.ko")
OUT = os.path.join(ROOT, "docs", "RE-SPRD-SIP-SVC.md")
OBJDUMP = (r"C:\Users\Administrator\scoop\apps\gcc-aarch64-none-linux-gnu"
           r"\15.2.rel1\bin\aarch64-none-linux-gnu-objdump.exe")

FUNC = re.compile(r"^[0-9a-f]+ <([^>]+)>:$")


def parse(text):
    """-> {func: [insn, ...]} untuk section .text (tanpa header)."""
    funcs, cur = {}, None
    for line in text.splitlines():
        m = FUNC.match(line.strip())
        if m:
            cur = m.group(1)
            funcs[cur] = []
            continue
        if cur is None:
            continue
        body = line.split("\t")
        if len(body) >= 3:
            ins = body[1].strip() + " " + body[2].strip()
            funcs[cur].append(ins.split("//")[0].strip())
    return funcs


def id_before(insns, idx):
    """Nilai x0/w0 tepat sebelum panggilan SMC ke-idx, dilao�lui instruksi.

    Pola yang dipakai blob ini bukan cuma `mov w0,#id`, tapi juga
    `add x0, x21, #0x100` / `orr x0, x21, #0x600` dengan x21 = 0x82000000,
    jadi pelacakan harus berbasis register, bukan pola satu instruksi.
    """
    regs = {"xzr": 0, "wzr": 0}

    def put(dst, val):
        """Tulis ke wN juga mengisi xN (zero-extend), dan sebaliknya."""
        val &= 0xFFFFFFFFFFFFFFFF
        regs[dst] = val
        if dst.startswith("w") and dst[1:].isdigit():
            regs["x" + dst[1:]] = val & 0xFFFFFFFF
        elif dst.startswith("x") and dst[1:].isdigit():
            regs["w" + dst[1:]] = val & 0xFFFFFFFF

    for ins in insns[:idx]:
        parts = re.sub(r"\s+", " ", ins).split(" ")
        op = parts[0]
        if op in ("mov", "movz") and len(parts) >= 3:
            dst = parts[1].rstrip(",")
            src = parts[2].rstrip(",")
            if src.startswith("#"):
                put(dst, int(src[1:], 16))
            elif src in regs:
                put(dst, regs[src])
            elif src in ("xzr", "wzr"):
                put(dst, 0)
        elif op == "movk" and len(parts) >= 3:
            dst = parts[1].rstrip(",")
            val = int(parts[2].rstrip(",").lstrip("#"), 16)
            shift = 0
            m = re.search(r"lsl #(\d+)", ins)
            if m:
                shift = int(m.group(1))
            base = regs.get(dst, 0)
            put(dst, (base & ~(0xFFFF << shift)) | ((val & 0xFFFF) << shift))
        elif op in ("add", "orr") and len(parts) >= 4:
            dst = parts[1].rstrip(",")
            src = parts[2].rstrip(",")
            imm = parts[3].rstrip(",")
            if src in regs and imm.startswith("#"):
                v = int(imm[1:], 16)
                put(dst, regs[src] + v if op == "add" else (regs[src] | v))
            elif src in ("xzr", "wzr"):
                put(dst, 0)
        elif op in ("ldr", "ldp", "stp", "str"):
            # hasil muat tidak bisa dilacak; lupakan tujuan
            for p in parts[1:]:
                d = p.rstrip(",")
                if d.startswith(("x", "w")) and d[1:].isdigit():
                    regs.pop(d, None)
    for r in ("x0", "w0"):
        if r in regs:
            return regs[r] & 0xFFFFFFFF
    return None


def imm_of(insns, reg):
    """Nilai yang dirakit ke `reg` lewat mov/movk sebelum dipakai."""
    val = 0
    seen = False
    for ins in insns:
        m = re.match(rf"mov\s+{reg}, #(-?0x[0-9a-f]+)", ins)
        if m:
            val = int(m.group(1), 16) & 0xFFFFFFFF
            seen = True
            continue
        m = re.match(rf"movk\s+{reg}, #(0x[0-9a-f]+), lsl #(\d+)", ins)
        if m:
            shift = int(m.group(2))
            val = (val & ~(0xFFFF << shift)) | ((int(m.group(1), 16) & 0xFFFF) << shift)
            seen = True
            continue
        if re.match(rf"mov\s+{reg}, xzr", ins):
            val = 0
            seen = True
    return val if seen else None


def main():
    text = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", KO],
                          capture_output=True, text=True).stdout
    funcs = parse(text)
    rows = []
    for name, insns in funcs.items():
        smc_at = [i for i, ins in enumerate(insns) if ins.startswith("bl ") and "smccc_smc" in ins]
        if not smc_at:
            continue
        for idx in smc_at:
            before = insns[:idx]
            # ambil jendela 40 instruksi sebelum panggilan
            win = before[-40:]
            fid = id_before(insns, idx)
            args = 0
            for r in ("x1", "x2", "x3", "x4", "x5", "x6"):
                if any(re.match(rf"(mov|ldr|orr|add)\s+{r}[,\s]", i) for i in win):
                    args += 1
            rows.append((name, fid, args))

    rows.sort(key=lambda r: (r[1] is None, r[1] or 0, r[0]))
    L = ["# RE `sprd_sip_svc.ko` — ID SMC per pembungkus\n\n",
         "Dihasilkan `donors/sip_svc_ids.py` dari disassembly blob 5.4.210.",
         " Blob ini hanya mengekspor `sprd_sip_svc_get_handle()`;\n",
         "`init_module` memanggil SMC `id` pertama lalu menyusun struct\n",
         "fungsi, dan tiap fungsi di struct itu membungkus satu SMC.\n\n"]
    L.append("| fungsi | ID SMC | argumen x1..x6 |\n|---|---|---|\n")
    for name, fid, args in rows:
        ids = f"`0x{fid:08x}`" if fid is not None else "**tak terbaca**"
        L.append(f"| `{name}` | {ids} | {args} |\n")

    first = [r for r in rows if r[0] == "init_module"]
    L.append("\n## Catatan\n\n")
    if first:
        L.append(f"- `init_module` memanggil SMC `0x{first[0][1]:08x}` lebih dulu "
                 f"(ID SIP *fast call*, owner 0x02) dan mencatat hasilnya:\n"
                 "  `func_addr = 0x%lx, pdg = 0x%lx, level = 0x%lx`.\n")
    L.append("- `sprd_sip_svc_get_handle` cuma `adrp/add` ke objek `.bss` "
             "`sprd_sip_svc_handle` lalu `ret` — tidak ada alokasi, jadi\n"
             "  handle harus sudah terisi saat `init_module` selesai.\n")
    L.append("- Versi tiap layanan dicetak dari string firmware:\n"
             "  `SPRD SIP SVC {STORAGE,NPU,DBG,PERF,DVFS}:v%d.%d detected in firmware.`\n")
    L.append("\n## Peta tulis ke handle (dari `init_module`)\n\n")
    L.append("`sprd_sip_svc_get_handle` mengembalikan alamat `.bss` "
             "`sprd_sip_svc_handle`. `init_module` mengisinya dengan pola "
             "yang berulang per layanan: sepasang versi (`stp wN, wN`) lalu "
             "pointer fungsi. Offset yang terlihat di disassembly:\n\n")
    L.append("| offset | isi |\n|---:|---|\n")
    for off, what in [(0, "versi layanan pertama (major, minor)"),
                      (8, "dua pointer fungsi"),
                      (24, "dua pointer fungsi"),
                      (40, "dua pointer (basis string + 4)"),
                      (56, "versi layanan kedua"),
                      (64, "dua pointer fungsi"),
                      (80, "versi layanan ketiga"),
                      (88, "dua pointer fungsi"),
                      (208, "versi layanan keempat"),
                      (216, "dua pointer fungsi"),
                      (232, "dua pointer fungsi"),
                      (248, "satu pointer fungsi")]:
        L.append(f"| {off} | {what} |\n")
    L.append("\nUrutan probe versi di `init_module` mengikuti angka layanan:\n"
             "`0x82000100` (handle) → `0x82000200` (PERF) → `0x82000300`\n"
             "(STORAGE) → `0x82000500` (PWR) → `0x82000600` (DVFS) →\n"
             "`0x82000700` (NPU) → `0xc2000200` (DBG).\n")

    L.append("\nKonsekuensi untuk port: tabel ini adalah daftar ABI yang harus\n"
             "direproduksi. Pengganti tidak boleh mengarang ID sendiri — kalau\n"
             "satu ID salah, firmware akan menolak dan `get_handle` mengembalikan\n"
             "struct berisi fungsi yang tidak pernah berhasil.\n")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("".join(L))
    print(f"tulis {OUT} ({len(rows)} panggilan SMC)")
    for name, fid, args in rows[:6]:
        print(f"  {name:38s} 0x{(fid or 0):08x} args={args}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
