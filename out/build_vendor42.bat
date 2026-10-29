@echo off
setlocal
REM ===========================================================================
REM  artix42 -- siapkan vendor_boot_a berisi DT kita
REM
REM  Prasyarat: dump_dt42.bat sudah dijalankan (logs\vb42\vendor_boot_a.bin).
REM
REM  Langkah:
REM    1. ambil DT dasar ASLI dari vendor_boot_a  -> dtb_stock_vendorboot.dtb
REM       + decompile ke .dts (nama node sebenarnya di tablet ini)
REM    2. tempel DTB kita ke blok dtb yang sama (offset tidak bergeser)
REM    3. tulis logs\vb42\vendor_boot_patch42.img
REM
REM  Kenapa bukan dtb_a: dtb_a di tablet ini 8 MiB NOL.  LK menulis sendiri
REM    vendorbootimage: vendor dt offset is 0x15be000
REM    loading fix dtb ... offset = 0x15be040 ... Matches the corresponding dtb
REM  Jadi DT dasar ada di vendor_boot_a.
REM
REM  Pemakaian: build_vendor42.bat [berkas.dtb]
REM    tanpa argumen -> pakai out\dtb_artix41\ums512-1h10-vxtux.dtb
REM ===========================================================================

set OUT=%~dp0
set LOG=%OUT%logs\vb42
set PY=C:\Python314\python.exe
set DTB=%~1
if "%DTB%"=="" set DTB=%OUT%dtb_artix41\ums512-1h10-vxtux.dtb
set PATCH=%LOG%\vendor_boot_patch42.img

if not exist "%PY%" ( echo [X] python tidak ada: %PY% & pause & exit /b 1 )
if not exist "%LOG%\vendor_boot_a.bin" (
  echo [X] dump belum ada: %LOG%\vendor_boot_a.bin
  echo     Jalankan dump_dt42.bat dulu ^(tablet di Mode Download^).
  pause
  exit /b 1
)
if not exist "%DTB%" (
  echo [X] DTB sumber tidak ada: %DTB%
  pause
  exit /b 1
)

echo ============================================================
echo  [1/3] ambil + decompile DT dasar ASLI dari vendor_boot
echo ============================================================
"%PY%" "%OUT%extract_vendor_dtb.py" "%LOG%\vendor_boot_a.bin" "%LOG%"
if errorlevel 1 ( echo [X] ekstrak gagal & pause & exit /b 1 )

echo.
echo ============================================================
echo  [2/3] tempel DTB kita ke blok dtb vendor_boot
echo  DTB sumber: %DTB%
echo ============================================================
"%PY%" "%OUT%patch_vendor_boot.py" "%LOG%\vendor_boot_a.bin" "%DTB%" "%PATCH%"
if errorlevel 1 (
  echo [X] patch gagal. Kalau pesannya "lebih besar dari ruang", DTB kita
  echo     melebihi DT asli - kecilkan dulu (buang node tidak terpakai).
  pause
  exit /b 1
)

echo.
echo ============================================================
echo  [3/3] hasil
echo ============================================================
for %%A in ("%PATCH%") do echo  image patch : %%~fA  (%%~zA byte)
echo  backup asli : %LOG%\vendor_boot_a.bin
echo.
echo  Lanjut: flash_vendor42.bat   ^(backup + tulis vendor_boot_a + verifikasi^)
echo  Batal : tidak ada yang ditulis ke tablet sampai flash_vendor42.bat jalan.
pause
