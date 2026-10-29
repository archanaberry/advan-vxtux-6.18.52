@echo off
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
