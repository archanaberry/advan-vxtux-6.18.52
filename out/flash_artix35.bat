@echo off
setlocal
REM ============================================================
REM  FLASH HANYA boot_a  <-  boot_artix35.img (fix clock UMS512
REM  builtin -> eMMC terdaftar + init menunggu storage)
REM  Tidak menyentuh super/Artix/partisi lain.
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap ditekan), colok USB.
REM   2. Jalankan script ini, tekan apa pun saat pause.
REM   3. Harus terbaca: file size : 0x2A40000  lalu  Write Part Done: boot_a
REM   4. Setelah reset: JANGAN tahan tombol apa pun, biarkan boot normal.
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix35.img

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )

echo ============================================================
echo  boot_a  ^<-  %BOOT%
for %%A in ("%BOOT%") do echo  ukuran : %%~zA byte  (harus 44302336)
echo  md5 diharapkan : 303c06b0a6a10e014a84035dd13708b9
echo.
echo  Matikan tablet: tahan VOL DOWN + POWER 15 detik,
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
if errorlevel 1 (
  echo [!] ada error -- cek baris "Write Part Done: boot_a" di atas
) else (
  echo [OK] selesai -- JANGAN tahan tombol apa pun saat tablet reboot,
  echo      biarkan boot normal +/- 40 detik, lalu foto layarnya:
  echo      "storage ready after" + "VXTUX-WRITE-TEST: OK" = target tercapai.
)
pause
