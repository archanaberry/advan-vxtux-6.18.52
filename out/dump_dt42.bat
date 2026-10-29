@echo off
setlocal
REM ===========================================================================
REM  artix42 -- DUMP SUMBER DTB YANG SEBENARNYA (read-only, TANPA flash)
REM
REM  KENAPA SCRIPT INI ADA (bukti dari dump artix41 di tablet ini):
REM    dtb_a  = 8388608 byte, SELURUHNYA NOL (md5 96995b58..., tidak ada
REM             satu pun magic FDT).  Jadi menulis dtb_a TIDAK ADA gunanya -
REM             LK tidak membacanya sebagai sumber DT.
REM    dtbo_a = berisi tabel overlay asli: magic d7b7ab1e, 2 entri
REM             id 0x1900 (dt_offset 0x60) dan id 0x1100 (dt_offset 0x76d9,
REM             size 0x704b) - ini yang di-merge LK di atas DT dasar.
REM
REM  Dari uboot_log.bin tablet ini, LK menulis sendiri:
REM    bootimage: generic kernel offset is 0x1000
REM    boot_a kernel read OK, page size = 0x29c1000, locate to 0x80080000
REM    [_boot_load_kernel_ramdisk_image] loading fix dtb ...
REM    Find kernel dtb prop name sprd,sc-id=ums512 1000 1000,
REM         g_fdt_blob sprd,sc-id=ums512 1h10 1000
REM    Matches the corresponding dtb in multiple DTB
REM    [_boot_load_kernel_ramdisk_image] loading dtbo ...
REM    [merge_dtbo] merging dtb/dtbo ...
REM    merge_dtbo : entry id is 0x1100, offset = 0x76d9 size=0x704b
REM
REM  Artinya: DT dasar datang dari blob "multiple DTB" milik LK
REM  (g_fdt_blob), bukan dari partisi dtb_a.  Kandidat pemiliknya:
REM  vendor_boot_a (Android vendor_boot menyimpan dtb multi-entri) atau
REM  uboot_a (LK).  Script ini membaca KEDUANYA plus dtbo_a dan boot_a,
REM  lalu langsung menyebutkan di mana blob itu berada.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\vb42
set PY=C:\Python314\python.exe

if not exist "%SPD%" ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  DUMP SUMBER DTB -^> %LOG%
echo   vendor_boot_a 100MB ^| uboot_a 3MB ^| dtbo_a 8MB ^| boot_a 64MB
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

pushd "%LOG%"
"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  read_part vendor_boot_a 0 0x6400000 vendor_boot_a.bin ^
  read_part uboot_a       0 0x300000  uboot_a.bin ^
  read_part dtbo_a        0 0x800000  dtbo_a.bin ^
  read_part boot_a        0 0x4000000 boot_a.bin ^
  reset > "%LOG%\spd_dump_session.log" 2>&1
popd

echo.
echo === hasil dump ===
for %%F in ("%LOG%\*.bin") do echo   %%~nxF  %%~zF byte
if not exist "%LOG%\vendor_boot_a.bin" (
  echo [X] dump gagal - lihat 8 baris terakhir transkrip:
  powershell -NoProfile -Command "if (Test-Path '%LOG%\spd_dump_session.log') { Get-Content '%LOG%\spd_dump_session.log' -Tail 8 }"
  pause
  exit /b 1
)

echo.
echo ============================================================
echo  bedah: di mana DT dasar yang dipakai kernel?
echo ============================================================
if exist "%PY%" (
  "%PY%" "%OUT%bedah_vendor_boot.py" "%LOG%"
) else (
  echo [!] python tidak ada, cek manual:
  findstr /C:"sprd,sc-id=ums512 1h10" "%LOG%\vendor_boot_a.bin" "%LOG%\uboot_a.bin"
)
pause
