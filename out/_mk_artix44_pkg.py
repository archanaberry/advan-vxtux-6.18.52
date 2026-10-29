#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""_mk_artix44_pkg.py -- paket artix44: flash boot_a + vendor_boot_a.

Kenapa vendor_boot ikut: kernel di tablet ini TIDAK memakai DTB dari boot.img
(boot.img kita hanya berisi stub FDT 131 byte) dan partisi dtb_a seluruhnya
nol.  LK menulis sendiri di uboot_log:

    vendorbootimage: vendor dt size is 117599
    vendorbootimage: vendor dt offset is 0x15be000
    loading fix dtb ... offset = 0x15be040
    Matches the corresponding dtb in multiple DTB

Jadi DT yang benar-benar dipakai kernel ada di vendor_boot_a.  Selama
vendor_boot_a tidak ditulis, SEMUA perubahan DTS (nama rail SC2730, alias
i2c, audio unisoc,*, ramoops) tidak pernah sampai ke kernel -- dan itu
menjelaskan kenapa daftar deferred tidak berubah selama tiga build.

Dijalankan dari root repo:  python out/_mk_artix44_pkg.py
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

FLASH = r"""@echo off
setlocal
REM ===========================================================================
REM  FLASH artix44  ->  boot_a  +  vendor_boot_a
REM
REM  boot_a      : Image 6.18.52-VXTux + initramfs (init baru + bldbd + 10 log)
REM  vendor_boot_a: DT KITA (satu-satunya DT yang dibaca LK)
REM
REM  Kenapa vendor_boot WAJIB ikut (bukan dtb_a seperti artix41):
REM    boot.img kita hanya memuat stub FDT 131 byte, dan partisi dtb_a di
REM    tablet ini 8 MiB NOL.  uboot_log membuktikan LK mengambil DT dari
REM    vendor_boot_a: "vendor dt offset is 0x15be000" lalu
REM    "loading fix dtb ... offset = 0x15be040 ... Matches the corresponding
REM    dtb in multiple DTB".  Karena itu tiga build DTS sebelumnya tidak
REM    pernah mengubah apa pun: DT-nya tidak pernah ditulis.
REM
REM  Pembagian 3 keluaran dtc di DTB ini:
REM    * ramoops kini punya record/console/pmsg size -> pstore hidup, panic
REM      sebelum initramfs terekam dan dibaca boot berikutnya
REM    * root membawa sprd,sc-id = "ums512 1000 1000" (sama dengan stok)
REM      supaya LK benar-benar memilih DT ini di multi-DTB
REM    * rail SC2730 / alias i2c / audio unisoc,* tetap seperti artix43
REM
REM  Camera-cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik, lepas POWER
REM      (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Harus terbaca: "Write Part Done: boot_a" lalu
REM      "Write Part Done: vendor_boot_a".
REM   4. Reset, jangan tahan tombol apa pun, lalu jalankan
REM      dump_artix44_logonly.bat.
REM
REM  Kalau tablet tidak mau boot: restore_vendor44.bat (vendor_boot saja)
REM  dan flash_artix43.bat (boot_a lama) mengembalikan keadaan semula.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\vb44
set BOOT=%OUT%6.18.52-VXTux-UMS512\boot_artix44.img
set VB=%LOG%\vendor_boot_patch44.img
set PY=C:\Python314\python.exe

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )
if not exist "%VB%"   ( echo [X] vendor_boot patch tidak ada: %VB% & pause & exit /b 1 )
if not exist "%LOG%"  mkdir "%LOG%"

echo ============================================================
echo  boot_a        ^<-  %BOOT%
for %%A in ("%BOOT%") do echo   ukuran : %%~zA byte
echo  vendor_boot_a ^<-  %VB%
for %%A in ("%VB%")   do echo   ukuran : %%~zA byte
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BOOT%';print('  boot md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
  "%PY%" -c "import hashlib;p=r'%VB%';d=open(p,'rb').read();print('  vb  md5  :',hashlib.md5(d).hexdigest());print('  vb  dtb  :',d[0x15be040:0x15be044]==bytes.fromhex('d00dfeed'))"
)
echo.
echo  Tablet harus sudah di Mode Download (VOL DOWN + POWER, colok USB).
echo ============================================================
pause

REM CWD di folder log (drive E:) selama spd_dump berjalan: kalau dijalankan
REM dari G:\UMS512\unlock\advan (penuh) spd_dump mati "fwrite(dump) failed"
REM sebelum satu perintah pun jalan.
pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%BOOT%" ^
  w vendor_boot_a "%VB%" ^
  reset
set SPDRC=%ERRORLEVEL%
popd

echo.
if not "%SPDRC%"=="0" (
  echo [!] ada error -- cari "Write Part Done: boot_a" /
  echo     "Write Part Done: vendor_boot_a" di atas.
) else (
  echo [OK] boot_a + vendor_boot_a tertulis.
  echo      Di layar cari:
  echo        "VXTUX-BLDB:"            pintu debug kedua, langkah ke-N
  echo        "VXTUX-LOG: ... 10 files"  bundle log sudah ditulis
  echo        "VXTUX-DEFER[post-modules]" device yang masih tanpa driver
  echo        "storage ready after Ns"   eMMC hidup
  echo        "chroot OK, exec /sbin/init" masuk Artix
  echo      Lalu jalankan dump_artix44_logonly.bat.
)
pause
"""

RESTORE = r"""@echo off
setlocal
REM  Kembalikan vendor_boot_a ke isi STOK (backup dibaca dari dump 2026-10-29).
REM  Pakai ini kalau tablet tidak mau boot setelah flash_artix44.bat.
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\vb44
set STOCK=%LOG%\vendor_boot_a_stock.bin

if not exist "%STOCK%" ( echo [X] backup tidak ada: %STOCK% & pause & exit /b 1 )
echo ============================================================
echo  vendor_boot_a  ^<-  STOK  %STOCK%
for %%A in ("%STOCK%") do echo   ukuran : %%~zA byte
echo ============================================================
pause
pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w vendor_boot_a "%STOCK%" ^
  reset
popd
echo [i] vendor_boot_a dikembalikan ke stok.
pause
"""


def cro(p, text):
    with open(p, "w", newline="\r\n", encoding="utf-8") as f:
        f.write(text)
    print("OK %-34s %7d byte" % (p, os.path.getsize(p)))


cro("out/flash_artix44.bat", FLASH)
cro("out/restore_vendor44.bat", RESTORE)

# dump: turunan artix41 (sudah terbukti jalan), rename + tambahan pembaca bundle
src = open("out/dump_artix41_logonly.bat", newline="", encoding="utf-8").read()
t = src.replace("artix41", "artix44")
extra = (
    "\r\necho.\r\n"
    "echo [i] bundle log (kanal mentah common_rs1_a):\r\n"
    "if exist \"%LOG%\\common_rs1_a.bin\" if exist \"%PY%\" (\r\n"
    "  \"%PY%\" \"%OUT%baca_bundle44.py\" \"%LOG%\\common_rs1_a.bin\"\r\n"
    ")\r\n"
)
t = t.replace("\r\necho  Laporan penuh :", extra + "\r\necho  Laporan penuh :")
with open("out/dump_artix44_logonly.bat", "w", newline="", encoding="utf-8") as f:
    f.write(t)
print("OK %-34s %7d byte" % ("out/dump_artix44_logonly.bat",
                             os.path.getsize("out/dump_artix44_logonly.bat")))

print("\nSelesai. Urutan pakai:")
print("  1. python out/_mk_artix44_pkg.py        (sudah ini)")
print("  2. out/flash_artix44.bat                (tablet di Mode Download)")
print("  3. out/dump_artix44_logonly.bat         (baca hasil boot)")
print("  4. kalau gagal boot: out/restore_vendor44.bat")
