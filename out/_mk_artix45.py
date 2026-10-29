#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""_mk_artix45.py -- paket artix45 (diagnostik "layar bisu") dari artix44.

Yang berubah di artix45 dibanding artix44 (semua di kernel/initramfs, bukan DT):

  * CONFIG_VXTUX_INITTRACE=y  -> tracer menggambar nama setiap initcall ke
    framebuffer LK SEBELUM initcall dijalankan.  Baris terakhir di layar =
    initcall yang membekukan kernel.  Ini menjawab keluhan "stuck di warning
    LOCK FLAG IS UNLOCK": layar itu milik LK, dan artix44 membuktikan LK sudah
    menjalankan [start_linux]; jadi yang mati adalah kernel, bukan LK.
  * vxtux_lkfb: device_initcall -> fs_initcall (level 5).  fbcon (level 4)
    tetap lebih dulu, tetapi konsol kini hidup SEBELUM drivers/mmc dan
    drivers/vxtux di level 6, jadi teks kernel tampil walau probe eMMC
    menggantung.
  * /vxtux/init mencetak identitas + "0/5 init alive" SEBELUM menunggu eMMC
    (dulu 180 detik tanpa satu byte pun di layar).

Karena DT tidak berubah, artix45 hanya menulis boot_a; vendor_boot_a sudah
berisi DT kita dari artix44.

Dijalankan dari root repo:  python out/_mk_artix45.py
"""
import io
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

FLASH_BODY = """\
REM  FLASH artix45  ->  boot_a saja  (boot_artix45.img)
REM
REM  Isi artix45 (build diagnostik untuk layar yang beku di teks LK):
REM   * VXTUX_INITTRACE=y -- tracer menggambar NAMA setiap initcall ke
REM     framebuffer LK sebelum initcall dijalankan.  Kalau boot berhenti,
REM     baris terakhir di layar adalah initcall pelakunya.
REM   * vxtux_lkfb pindah ke fs_initcall (level 5): fbcon (level 4) tetap
REM     lebih dulu, tetapi konsol hidup SEBELUM drivers/mmc / drivers/vxtux
REM     di level 6 -> teks kernel terlihat walau probe eMMC menggantung.
REM   * /vxtux/init mencetak "VXTUX-STEP: 0/5 init alive" SEBELUM menunggu
REM     eMMC (sebelumnya 180 detik tanpa satu byte pun di layar).
REM
REM  Skrip ini HANYA menulis boot_a.  vendor_boot_a tidak disentuh: DT kita
REM  sudah terpasang di sana sejak artix44 (flash_artix44.bat).
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Harus terbaca: "Write Part Done: boot_a"
REM   4. Cabut USB, nyalakan.  Yang harus muncul di layar, berurutan:
REM        VXTUX-ITRACE: kernel alive - watching initcalls, last line = hang
REM        <nomor> <nama initcall> ...        <- berhenti di sini = pelakunya
REM        vxtux-lkfb: LK fb ... fbcon active
REM        VXTUX ARTIX DIAG (initramfs) / VXTUX-STEP: 0/5 init alive
REM   5. Setelah itu baru jalankan dump_artix45_logonly.bat.
REM
REM  Kalau ingin kembali ke artix44: flash_artix44.bat.
"""

DUMP_BODY = """\
REM  artix45 -- DUMP SEMUA LOG (read-only, TANPA flash) -> logs\\artix45
REM
REM  Satu sesi spd_dump (read-only, tidak mengubah apa pun):
REM    cache 100MB | super 16MB | common_rs1_a 8MB | uboot_log 4MB
REM    sysdumpdb 10MB | misc 1MB | miscdata 1MB | boot_a 64MB
REM    dtb_a 8MB | dtbo_a 8MB
REM
REM  Yang dibaca dari hasil:
REM    uboot_log.bin  -> apakah LK benar-benar [start_linux] dan cmdline apa
REM                      yang dipakai (kalau tidak ada start_linux: masalah LK)
REM    cache.bin      -> /cache/vxtux-logs/* dan boot_log.txt dari init
REM    common_rs1_a   -> kepala "### VXTUXRS1" = bundle log mentah
REM    boot_a prefix md5 == boot_artix45.md5 -> artix45 benar-benar terpasang
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
    t = io.open(src, newline='').read().replace('artix44', 'artix45')
    if body:
        t = header_new(t, body)
    io.open(dst, 'w', newline='').write(t)
    print('OK %-34s %6d byte' % (dst, os.path.getsize(dst)))


# 1. pipeline -------------------------------------------------------------
src, dst = 'out/ci_artix44_pipeline.sh', 'out/ci_artix45_pipeline.sh'
s = io.open(src, newline='').read().replace('artix44', 'artix45')
io.open(dst, 'w', newline='').write(s)
print('OK %-34s %6d byte (sisa artix44: %d)' %
      (dst, os.path.getsize(dst), s.count('artix44')))

# 2. DTB sumber pipeline (tidak dibangun ulang: DTS tidak berubah) ---------
os.makedirs('out/dtb_artix45', exist_ok=True)
shutil.copy2('out/dtb_artix44/ums512-1h10-vxtux.dtb', 'out/dtb_artix45/')
print('OK out/dtb_artix45/ums512-1h10-vxtux.dtb')

# 3. skrip untuk kamu (flash + dump) --------------------------------------
convert('out/flash_artix44.bat', 'out/flash_artix45.bat', FLASH_BODY)
convert('out/dump_artix44_logonly.bat', 'out/dump_artix45_logonly.bat', DUMP_BODY)
# dump_flash_artix44.bat tidak pernah ada (commit sebelumnya hanya membuat
# flash/dump terpisah); artix45 juga cukup dua skrip itu.

print('\nSelesai.  Urutan pakai:')
print('  1. tunggu build CI (Image baru), lalu jalankan out/ci_artix45_pipeline.sh')
print('  2. out/flash_artix45.bat        (tablet di Mode Download, hanya boot_a)')
print('  3. lihat layar: baris VXTUX-ITRACE terakhir = initcall yang macet')
print('  4. out/dump_artix45_logonly.bat (dump cache/common_rs1_a/uboot_log)')
