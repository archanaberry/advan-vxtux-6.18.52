@echo off
setlocal
REM ============================================================
REM  FLASH HANYA boot_a  <-  boot_artix34.img
REM  Tidak menyentuh super/Artix/partisi lain.
REM
REM  artix34 = kanal tulis terinstrumentasi:
REM   * VXTUX-WRITE-TEST: uji tulis-baca-balik cache, hasil di layar
REM   * flightrec: errno tiap kegagalan (open/write/readback) di fbcon
REM   * laporan init diverifikasi baca-balik; teks Inggris penuh
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap ditekan), colok USB.
REM   2. Jalankan script ini, tekan apa pun saat pause.
REM   3. Harus terbaca: file size : 0x2A15000 (atau ukuran img baru)
REM      lalu  Write Part Done: boot_a
REM   4. Setelah reset: JANGAN tahan tombol apa pun, biarkan boot normal.
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix34.img

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )

echo ============================================================
echo  boot_a  ^<-  %BOOT%
for %%A in ("%BOOT%") do echo  ukuran : %%~zA byte  (harus 44126208)
echo  md5 diharapkan : 2e918a88ca6e03ef623b540ea807d306
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
  echo      "VXTUX-WRITE-TEST: OK" = kanal tulis eMMC hidup.
)
pause
