#!/usr/bin/env python3
"""re.py — alat baca blob kernel (.ko) dengan relokasi sudah diterapkan.

`objdump -d` pada modul ELF mentah menampilkan `bl 0` untuk semua pemanggilan
luar karena targetnya belum dipatch (file baru dikompilasi ulang saat modul
di-load).  Alat ini memakai capstone + pyelftools untuk:

  * membaca `.symtab`, `.rela.*`, `.data`, `.rodata`, `.rodata.str1.1`
  * menerapkan relokasi AArch64 (CALL26, JUMP26, ADR_PREL_PG_HI21,
    ADD_ABS_LO12_NC, ADR_GOT_PAGE, LD64_GOT_LO12_NC, ABS64, PREL32)
  * menampilkan disassembly dengan panggilan/ramalan bertanda nama simbol
  * menampilkan tabel data berisi pointer fungsi bertanda nama
  * mencari siapa memakai suatu simbol (xref)

    python donors/blobre.py donors/stock_vendor_modules_all/sprd_power_stat.ko funcs
    python donors/blobre.py <blob> dis <nama|alamat>
    python donors/blobre.py <blob> xref <nama_simbol>
    python donors/blobre.py <blob> data [nama_seksi]
    python donors/blobre.py <blob> str [kata]
"""
import argparse
import re
import sys

from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN
from capstone.arm64 import ARM64_OP_IMM, ARM64_OP_REG
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

CS = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
CS.detail = False  # decoder manual lebih bisa dipakai untuk anotasi

# Relokasi yang memindahkan alamat instruksi (bukan data).
REL_BRANCH = {"CALL26", "JUMP26", "JUMP26_ADRP"}
# Relokasi yang menunjuk ke suatu simbol dari dalam kode / data.
REL_SYM = {"ABS64", "ABS32", "PREL32", "PREL64", "ADR_PREL_PG_HI21",
           "ADD_ABS_LO12_NC", "ADR_GOT_PAGE", "LD64_GOT_LO12_NC",
           "LD_PREL_LO19", "ADR_PREL_LO21"}

# Nomor tipe relokasi AArch64 (ELF for the Arm 64-bit Architecture).
AARCH64_REL = {
    257: "ABS64", 258: "ABS32", 259: "ABS16",
    260: "PREL64", 261: "PREL32", 262: "PREL16",
    274: "ADR_PREL_LO21", 275: "ADR_PREL_PG_HI21", 277: "ADD_ABS_LO12_NC",
    278: "LDST8_ABS_LO12_NC", 282: "JUMP26", 283: "CALL26",
    284: "LDST16_ABS_LO12_NC", 285: "LDST32_ABS_LO12_NC",
    286: "LDST64_ABS_LO12_NC", 299: "LDST128_ABS_LO12_NC",
    311: "ADR_GOT_PAGE", 312: "LD64_GOT_LO12_NC",
    1025: "GLOB_DAT", 1026: "JUMP_SLOT",
}


class Blob:
    def __init__(self, path):
        self.path = path
        self.f = open(path, "rb")
        self.elf = ELFFile(self.f)
        self.sec = {}
        for s in self.elf.iter_sections():
            self.sec[s.name] = s
        # simbol: nama -> (alamat, section_index, ukuran, tipe)
        self.byname = {}
        self.byaddr = {}
        symtab = self.sec.get(".symtab")
        if symtab:
            for sym in symtab.iter_symbols():
                if not sym.name:
                    continue
                self.byname[sym.name] = sym
                if sym["st_shndx"] != "SHN_UNDEF":
                    self.byaddr.setdefault(sym["st_value"], sym.name)
        self.md = None

    # ---------- utilitas ----------
    def symname(self, value):
        """Alamat -> nama simbol, mencari simbol terdekat yang lebih kecil."""
        best = None
        for addr, name in self.byaddr.items():
            if addr <= value and (best is None or addr > best):
                best = addr
        if best is None:
            return f"0x{value:x}"
        sym = self.byname[self.byaddr[best]]
        off = value - best
        if off == 0:
            return sym.name
        return f"{sym.name}+0x{off:x}"

    def raw(self, secname):
        s = self.sec[secname]
        return s.data(), s["sh_addr"]

    def relocs(self, secname):
        """offset -> (tipe, nama_simbol, addend).

        Nama seksi relokasi tidak bisa ditebak: Clang memberi nama
        `.rela.text.unlikely.__cfi_check_fail` untuk `.text`, jadi petakan
        lewat `sh_info` (seksi yang direlokasi), bukan lewat nama.
        """
        out = {}
        for s in self.elf.iter_sections():
            if not isinstance(s, RelocationSection):
                continue
            if s["sh_info"] >= self.elf.num_sections():
                continue
            if self.elf.get_section(s["sh_info"]).name != secname:
                continue
            symtab = self.elf.get_section(s["sh_link"])
            for r in s.iter_relocations():
                sym = symtab.get_symbol(r["r_info_sym"])
                out[r["r_offset"]] = (AARCH64_REL.get(r["r_info_type"],
                                                      str(r["r_info_type"])),
                                      sym.name or f"sym#{r['r_info_sym']}",
                                      r["r_addend"])
        return out

    # ---------- decoder 16-bit untuk aliran kontrol ----------
    @staticmethod
    def _imm26(word):
        imm = word & 0x3FFFFFF
        if imm & (1 << 25):
            imm -= 1 << 26
        return imm << 2

    @staticmethod
    def _imm19(word):
        imm = (word >> 5) & 0x7FFFF
        if imm & (1 << 18):
            imm -= 1 << 19
        return imm << 2

    @staticmethod
    def _imm12(word):
        return word & 0xFFF

    # ---------- disassembly ----------
    def _rodata_string(self, addr):
        """Ambil string pendek di alamat rodata, atau None."""
        for name, sec in self.sec.items():
            if "rodata" not in name and not name.startswith(".rodata"):
                continue
            base = sec["sh_addr"]
            data = sec.data()
            if not (base <= addr < base + len(data)):
                continue
            off = addr - base
            end = data.find(b"\x00", off)
            if end < 0 or end - off > 120:
                return None
            try:
                txt = data[off:end].decode("ascii")
            except UnicodeDecodeError:
                return None
            if txt and all(32 <= ord(c) < 127 for c in txt):
                return txt
        return None

    def dis(self, secname=".text", start=None, count=None):
        data, base = self.raw(secname)
        rel = self.relocs(secname)
        insns = list(CS.disasm(data, base))
        lines = []
        pages = {}   # register -> alamat halaman, atau ("got", simbol)
        for ins in insns:
            word = int.from_bytes(data[ins.address - base:ins.address - base + 4],
                                  "little")
            note = ""
            # bl / b -> target absolut setelah patch
            if ins.mnemonic in ("bl", "b"):
                if ins.address in rel:
                    _t, s, a = rel[ins.address]
                    note = f"  ; -> {s}+{a:#x}"
                else:
                    target = ins.address + self._imm26(word)
                    note = f"  ; -> {self.symname(target)}"
            elif ins.mnemonic.startswith("b."):
                target = ins.address + self._imm19(word)
                note = f"  ; -> {self.symname(target)}"
            elif ins.mnemonic == "adrp":
                imm = (word >> 5) & 0x1FFFFF
                if imm & (1 << 20):
                    imm -= 1 << 21
                page = (ins.address & ~0xFFF) + (imm << 12)
                regs = ins.op_str.split(",")
                if ins.address in rel:
                    _t, s, a = rel[ins.address]
                    note = f"  ; -> {s}+{a:#x}"
                    pages[regs[0].strip()] = ("sym", s, a)
                else:
                    note = f"  ; page -> {self.symname(page)}"
                    pages[regs[0].strip()] = ("abs", page, 0)
            elif ins.mnemonic in ("add", "ldr") and ins.address in rel:
                _t, s, a = rel[ins.address]
                regs = [r.strip() for r in ins.op_str.split(",")]
                note = f"  ; {s}+{a:#x}"
                if ins.mnemonic == "ldr" and len(regs) >= 2:
                    src = regs[1].split("[")[-1].strip()
                    pg = pages.get(src)
                    if pg and pg[0] == "sym":
                        note = f"  ; -> {pg[1]}+{pg[2]:#x}"
                        pages[regs[0]] = ("abs", -1, 0)
                elif ins.mnemonic == "add" and len(regs) >= 3:
                    dst, src = regs[0], regs[1]
                    # Simbol di relokasi `add` inilah yang menentukan alamat;
                    # memakai simbol dari `adrp` salah kalau keduanya beda.
                    if s in self.byname:
                        addr = self.byname[s]["st_value"] + a
                    else:
                        # simbol seksi (sym#N) -> alamat = addend
                        addr = a
                    pages[dst] = ("abs", addr, 0)
                    # String hanya diambil kalau acuannya simbol seksi
                    # (rodata); simbol data bernama bukan teks.
                    txt = None if s in self.byname else self._rodata_string(addr)
                    note = (f"  ; -> {self.symname(addr)}  {txt!r}"
                            if txt else f"  ; -> {self.symname(addr)}")
            elif ins.address in rel:
                t, s, a = rel[ins.address]
                note = f"  ; {s}+{a:#x}"
            lines.append(f"{ins.address:10x}:  {ins.mnemonic:8s} {ins.op_str}{note}")
        return lines

    def funcs(self):
        symtab = self.sec.get(".symtab")
        out = []
        for sym in symtab.iter_symbols():
            if sym["st_info"]["type"] != "STT_FUNC":
                continue
            if not sym["st_size"]:
                continue
            out.append((sym["st_value"], sym["st_size"], sym.name))
        return sorted(out)

    def data_ptrs(self, secname=".data"):
        """Isi suatu seksi: pointer ke simbol ditandai nama."""
        data, base = self.raw(secname)
        rel = self.relocs(secname)
        lines = []
        for off in range(0, len(data) - 7, 8):
            word = int.from_bytes(data[off:off + 8], "little")
            tag = ""
            if off in rel:
                t, s, a = rel[off]
                tag = f"{s}+{a:#x}"
            elif word in self.byaddr:
                tag = self.byaddr[word]
            lines.append(f"{base + off:10x}: {word:016x}  {tag}")
        return lines

    def xref(self, name):
        """Cari pemanggil: instruksi yang berelokasi ke simbol ini."""
        sym = self.byname.get(name)
        if not sym:
            return [f"simbol {name} tidak ada di .symtab"]
        out = []
        for s in self.elf.iter_sections():
            if not isinstance(s, RelocationSection):
                continue
            secname = s.name.replace(".rela", "", 1)
            if secname not in self.sec:
                continue
            rel = self.relocs(secname)
            for off, (t, sn, a) in rel.items():
                if sn != name:
                    continue
                where = f"{self.symname(self.sec[secname]['sh_addr'] + off)}"
                out.append(f"{where}  <- {s.name} (tipe {t}, addend {a:#x})")
        return sorted(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("blob")
    ap.add_argument("cmd", choices=["funcs", "dis", "data", "xref", "str"])
    ap.add_argument("arg", nargs="?")
    a = ap.parse_args()
    b = Blob(a.blob)

    if a.cmd == "funcs":
        for addr, size, name in b.funcs():
            print(f"{addr:10x}  {size:6d}  {name}")
        return 0

    if a.cmd == "xref":
        print("\n".join(b.xref(a.arg)))
        return 0

    if a.cmd == "data":
        print("\n".join(b.data_ptrs(a.arg or ".data")))
        return 0

    if a.cmd == "str":
        pat = a.arg or ""
        for s in b.sec:
            if "rodata" not in s:
                continue
            data, base = b.raw(s)
            for m in re.finditer(rb"[\x20-\x7e]{4,}", data):
                txt = m.group().decode()
                if pat and pat not in txt:
                    continue
                print(f"{base + m.start():10x}  [{s}] {txt}")
        return 0

    if a.cmd == "dis":
        arg = a.arg or ""
        target = None
        limit = 0x4000  # batas atas jendela; fungsi terpanjang di blob ~3 KB
        for addr, size, name in b.funcs():
            if arg == name or arg.lower() in name.lower():
                target = addr
                limit = size
                print(f"; {name} @ 0x{addr:x} ({size} B)")
                break
        if target is None:
            target = int(arg, 16) if arg else b.sec[".text"]["sh_addr"]
        lines = b.dis(".text")
        started = False
        for ln in lines:
            addr = int(ln.split(":")[0], 16)
            if addr < target:
                continue
            started = True
            print(ln)
            if addr >= target + limit:
                break
        if not started:
            print("tidak ada kode di alamat itu")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
