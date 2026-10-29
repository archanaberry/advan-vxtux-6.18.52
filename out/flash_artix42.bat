@echo off
setlocal
REM ===========================================================================
REM  artix42 -- FLASH boot_a saja (boot_artix42.img)
REM
REM  Isi artix42: drivers/regulator/of_regulator.c sekarang mencocokkan nama
REM  rail DT tanpa peduli huruf besar/kecil dan menyamakan '_' dengan '-'.
REM  Sebabnya: DT yang benar-benar dipakai tablet ini datang dari vendor_boot_a
REM  (dtb_a seluruhnya nol) dan memakai nama node gaya vendor (DCDC_CPU,
REM  LDO_VDDEMMCCORE). Tanpa toleransi itu devm_regulator_register() gagal
REM  -ENODEV pada deskriptor pertama -> probe SC2730 berhenti -> vmmc eMMC,
REM  sdio, usb33, wifipa, codec defer selamanya.
REM
REM  Skrip ini TIDAK menyentuh dtb_a / dtbo_a / vendor_boot_a / super.
REM  Kembali ke artix41: flash_artix41.bat
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Harus terbaca: file size : 0x......  lalu  Write Part Done: boot_a
REM   4. Setelah reset jangan tahan tombol apa pun.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix42.img
set MD5F=%~dp06.18.52-VXTux-UMS512\boot_artix42.md5
set LOG=%~dp0logs\artix42
set PY=C:\Python314\python.exe

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image belum ada: %BOOT% & echo     build belum selesai. & pause & exit /b 1 )
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  boot_a  ^<-  %BOOT%
for %%A in ("%BOOT%") do echo   ukuran : %%~zA byte
if exist "%MD5F%" (
  echo   md5    : (dari boot_artix42.md5)
  type "%MD5F%"
)
if exist "%PY%" "%PY%" -c "import hashlib;print('   md5 berkas :',hashlib.md5(open(r'%BOOT%','rb').read()).hexdigest())"
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

REM CWD dipindah ke folder log (drive E:) supaya spd_dump tidak menulis
REM pgpt.bin/partition_*.xml ke G:\UMS512\unlock\advan yang penuh.
pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%BOOT%" ^
  reset > "%LOG%\spd_flash.log" 2>&1
popd

echo.
echo === ringkasan ===
findstr /C:"file size" /C:"Write Part Done: boot_a" "%LOG%\spd_flash.log"
echo.
findstr /C:"Write Part Done: boot_a" "%LOG%\spd_flash.log" >nul && (
  echo [OK] boot_a tertulis. Setelah reset cari di layar:
  echo    "sc2730-regulator"        ^(probe SC2730 tidak lagi -ENODEV^)
  echo    "VXTUX-DEFER[post-modules]" ^(daftar device tanpa driver^)
  echo    "storage ready after Ns"    ^(eMMC + partisi terbaca^)
  echo    "chroot OK, exec /sbin/init"
) || (
  echo [!] tidak ada "Write Part Done: boot_a" - lihat %LOG%\spd_flash.log
)
echo.
echo  Kalau perlu kembali ke artix41: flash_artix41.bat
pause
