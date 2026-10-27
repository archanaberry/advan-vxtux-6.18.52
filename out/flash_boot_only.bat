@echo off
setlocal
REM ============================================================
REM  FLASH boot_a SAJA — kernel 6.18.52-VXTux-UMS512-artix
REM  (varian CONFIG_CMDLINE_FORCE: root=PARTLABEL=super dipaksa,
REM   fix "diam di logo" karena LK membuang root= dari boot.img)
REM
REM  super TIDAK disentuh — Artix ext4 di dalamnya tetap.
REM  Jika super belum pernah diflash Artix, pakai flash_artix.bat.
REM
REM  Ubah SPD/FDL1/FDL2 bila tool berada di tempat lain
REM  (contoh lama: E:\lindroid4_flash\).
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_6.18.52-VXTux-UMS512-artix.img

if not exist "%SPD%"  ( echo [X] spd_dump_new.exe tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )

echo ============================================================
echo  boot_a ^<- %BOOT%
echo.
echo  Matikan tablet. Tahan VOL DOWN + POWER 15 detik,
echo  lepas POWER (tetap tahan VOL DOWN), colok USB, tunggu 5 detik.
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%BOOT%" ^
  reset

echo.
if errorlevel 1 ( echo [!] ada error, cek log di atas ) else ( echo [OK] selesai, tablet reboot )
pause
