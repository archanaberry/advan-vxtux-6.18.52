@echo off
setlocal
REM ===========================================================================
REM  artix42 -- KEMBALIKAN vendor_boot_a ASLI
REM
REM  Dipakai kalau setelah flash_vendor42.bat tablet tidak mau boot.
REM  Berkas yang ditulis: logs\vb42\vendor_boot_a_backup.bin
REM  (dibaca otomatis oleh flash_vendor42.bat SEBELUM menulis).
REM
REM  Kalau backup belum ada, script berhenti - JANGAN menulis vendor_boot
REM  kosong/tebakan.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set LOG=%~dp0logs\vb42
set BAK=%LOG%\vendor_boot_a_backup.bin

if not exist "%SPD%" ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%BAK%" (
  echo [X] backup tidak ada: %BAK%
  echo     Jalankan flash_vendor42.bat sekali ^(dia membaca backup dulu^),
  echo     atau dump_dt42.bat kalau vendor_boot_a masih asli.
  pause
  exit /b 1
)

echo ============================================================
echo  RESTORE vendor_boot_a  ^<-  %BAK%
for %%A in ("%BAK%") do echo   ukuran : %%~zA byte
echo  Tablet harus di Mode Download.
echo ============================================================
pause

pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w vendor_boot_a "%BAK%" ^
  reset
popd

echo.
if errorlevel 1 (
  echo [!] gagal - cek baris "Write Part Done: vendor_boot_a"
) else (
  echo [OK] vendor_boot_a dikembalikan ke aslinya.
)
pause
