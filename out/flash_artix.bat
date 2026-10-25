@echo off
setlocal
REM ============================================================
REM  FLASH: Artix Linux (ext4 RW) ke super  +  kernel 6.18.52-VXTux-UMS512-artix ke boot_a
REM  Partisi vendor/userdata TIDAK disentuh.
REM  Jalankan hanya setelah backup & verifikasi selesai.
REM ============================================================

set SPD=E:\lindroid4_flash\spd_dump_new.exe
set FDL1=E:\lindroid4_flash\fdl1-dl.bin
set FDL2=E:\lindroid4_flash\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_6.18.52-VXTux-UMS512-artix.img
set SUPER=%~dp0artix-linux\artix_super_5600mb.img

if not exist "%SPD%"  ( echo [X] spd_dump_new.exe tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )
if not exist "%SUPER%" ( echo [X] image super tidak ada: %SUPER% & pause & exit /b 1 )

echo ============================================================
echo  boot_a  ^<- %BOOT%
echo  super   ^<- %SUPER%
echo.
echo  Matikan tablet. Tahan VOL DOWN + POWER 15 detik,
echo  lepas POWER (tetap tahan VOL DOWN), colok USB, tunggu 5 detik.
echo ============================================================
pause

REM urutan: super dulu (paling lama), boot_a terakhir.
REM kalau super gagal, boot_a belum berubah -> Android masih bisa boot.
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w super "%SUPER%" ^
  w boot_a "%BOOT%" ^
  reset

echo.
if errorlevel 1 ( echo [!] ada error, cek log di atas ) else ( echo [OK] selesai, tablet reboot )
pause
