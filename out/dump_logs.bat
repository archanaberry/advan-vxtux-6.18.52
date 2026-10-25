@echo off
setlocal
REM ============================================================
REM  DUMP LOG via spd_dump (mode Download / FDL2)
REM  Mengambil partisi log yang menumpuk di eMMC:
REM    uboot_log   4MB  = log bootloader/u-boot terakhir (persisten)
REM    sysdumpdb  10MB  = basis data crash/sysdump vendor
REM    misc        1MB  = bootloader control message (BCB)
REM    miscdata    1MB  = data misc vendor
REM  Hasil: *.bin di E:\vxtux_build_repo\out\logs\
REM  Mode Download: matikan tablet, Vol Down + Power 15 detik,
REM  lepas Power (tetap tahan Vol Down), colok USB, tunggu 5 detik.
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out\logs

if not exist "%OUT%" mkdir "%OUT%"

echo ============================================================
echo  Dump log ke %OUT%
echo  uboot_log 4MB + sysdumpdb 10MB + misc 1MB + miscdata 1MB
echo  (transkrip lengkap spd_dump disimpan ke spd_dump_session.log)
echo ============================================================
pause

"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%OUT%" ^
  read_part uboot_log 0 0x400000 uboot_log.bin ^
  read_part sysdumpdb 0 0xA00000 sysdumpdb.bin ^
  read_part misc 0 0x100000 misc_spd.bin ^
  read_part miscdata 0 0x100000 miscdata.bin ^
  reset > "%OUT%\spd_dump_session.log" 2>&1

echo.
echo === hasil ===
dir /b "%OUT%"
echo.
echo (transkrip: %OUT%\spd_dump_session.log)
pause
