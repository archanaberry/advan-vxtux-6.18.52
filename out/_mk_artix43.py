#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""_mk_artix43.py -- bangun paket artix43 dari keluarga artix41.

artix43 = build dari FDT stok (vendor_boot_a) + driver yang disamakan dengan
compatible asli firmware + rangkaian BLDB.  Skrip ini menyalin:

  out/ci_artix41_pipeline.sh  -> out/ci_artix43_pipeline.sh (rename path)
  out/dtb_artix41/*.dtb       -> out/dtb_artix43/      (DTB ikut pipeline)
  out/flash_artix41.bat       -> out/flash_artix43.bat  (header diganti)
  out/dump_artix41_logonly.bat-> out/dump_artix43_logonly.bat (header baru)
  out/dump_flash_artix41.bat  -> out/dump_flash_artix43.bat  (header baru)

Dijalankan dari root repo:  python out/_mk_artix43.py
"""
import io
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

FLASH_BODY = """\
REM  FLASH artix43  ->  boot_a saja  (boot_artix43.img)
REM
REM  Isi artix43 (dibangun mengikuti FDT STOK vendor_boot_a, bukan tebakan):
REM   * i2c-sprd menerima "sprd,sharkl5pro-hw-i2c" -- compatible asli node
REM     i2c@32060000 di firmware.  Sebelumnya driver hanya mengenal
REM     "sprd,sc9860-i2c" -> bus i2c tidak pernah probe, semua konsumennya
REM     defer.  clock-names stok ("source","enable","i2c") sudah persis.
REM   * codec + VBC menerima prefiks rebrand "unisoc,..." (stok memakai
REM     unisoc,sc2730-audio-codec / unisoc,vbc-v4-codec-sc2730 /
REM     unisoc,sharkl5pro-vbc) -> i2s/kartu suara punya driver.
REM   * sc2731-regulator (built-in, peta register berbeda) kini MENOLAK bila
REM     induk PMIC = sc2730 -> modul sc2730-regulator yang memegang node.
REM     Ini pengaman hardware: menulis peta sc2731 ke chip sc2730 berisiko.
REM   * regu_name_eq (artix42): rail gaya stok DCDC_CPU cocok "dcdc-cpu".
REM   * BLDB (Berry Lab Foundation): ring 16 MiB + /dev/bldb0 sejak
REM     core_initcall, daemon /vxtux/bldbd, gadget ACM identitas palsu Berry
REM     1209:b1db -> pintu debug KEDUA selain adbd, tanpa solder.
REM
REM  Skrip ini TIDAK menyentuh dtb_a / dtbo_a / vendor_boot_a / super.
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Yang harus terbaca: "Write Part Done: boot_a"
REM   4. Setelah reset jangan tahan tombol apa pun, lalu jalankan
REM      dump_artix43_logonly.bat untuk membaca hasil boot.
"""

DUMP_BODY = """\
REM  artix43 -- DUMP SEMUA LOG (read-only, TANPA flash) -> logs\\artix43
REM
REM  Satu sesi spd_dump (read-only, tidak mengubah apa pun):
REM    cache 100MB | super 16MB | common_rs1_a 8MB | uboot_log 4MB
REM    sysdumpdb 10MB | misc 1MB | miscdata 1MB | boot_a 64MB
REM    dtb_a 8MB | dtbo_a 8MB
REM
REM  Yang dibaca dari hasil:
REM    boot_a prefix md5 == boot_artix43.md5 -> artix43 benar-benar terpasang
REM    "VXTUX-BLDB:"   -> rangkaian pintu debug kedua; langkah ke-N mana yang
REM                        berhenti bila gadget USB belum menyala
REM    "VXTUX-MOD"/"VXTUX-DEFER" -> modul dimuat + device yang masih defer
REM    "i2c0 is ok" / sc2730-regulator / storage ready / chroot OK
REM    catatan: cache masih memuat log boot stok 5.4.210 -- pembanding asli
REM ==========================================================================="""


def header_new(old_text, new_body):
    """Ganti isi blok header (di antara dua baris REM ====) dengan new_body."""
    lines = old_text.split("\r\n")
    marks = [i for i, l in enumerate(lines) if l.startswith("REM ====")]
    if len(marks) >= 2:
        lines = lines[:marks[0] + 1] + new_body.split("\r\n") + lines[marks[1]:]
    else:
        print("[!] pola header tidak ditemukan, rename saja")
    return "\r\n".join(lines)


def convert(src, dst, body=None):
    t = io.open(src, newline='').read().replace('artix41', 'artix43')
    if body:
        t = header_new(t, body)
    io.open(dst, 'w', newline='').write(t)
    print('OK %-34s %6d byte' % (dst, os.path.getsize(dst)))


# 1. pipeline -------------------------------------------------------------
src, dst = 'out/ci_artix41_pipeline.sh', 'out/ci_artix43_pipeline.sh'
s = io.open(src, newline='').read().replace('artix41', 'artix43')
io.open(dst, 'w', newline='').write(s)
print('OK %-34s %6d byte (sisa artix41: %d)' %
      (dst, os.path.getsize(dst), s.count('artix41')))

# 2. DTB sumber pipeline --------------------------------------------------
os.makedirs('out/dtb_artix43', exist_ok=True)
shutil.copy2('out/dtb_artix41/ums512-1h10-vxtux.dtb', 'out/dtb_artix43/')
print('OK out/dtb_artix43/ums512-1h10-vxtux.dtb')

# 3. skrip untuk kamu (flash + dump) --------------------------------------
convert('out/flash_artix41.bat', 'out/flash_artix43.bat', FLASH_BODY)
convert('out/dump_artix41_logonly.bat', 'out/dump_artix43_logonly.bat', DUMP_BODY)
convert('out/dump_flash_artix41.bat', 'out/dump_flash_artix43.bat', FLASH_BODY)

print('\nSelesai.  Urutan pakai:')
print('  1. python out/_mk_artix43.py          (sudah ini)')
print('  2. jalankan out/flash_artix43.bat      (tablet di Mode Download)')
print('  3. jalankan out/dump_artix43_logonly.bat')
