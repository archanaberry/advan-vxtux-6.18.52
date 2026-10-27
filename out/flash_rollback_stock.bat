@echo off
setlocal
REM ============================================================
REM  ROLLBACK: kembalikan Android stok (boot_a + super)
REM  Sumber: E:\backupan-ori-advantabvxlite_t1.1\backup
REM ============================================================

set SPD=E:\lindroid4_flash\spd_dump_new.exe
set FDL1=E:\lindroid4_flash\fdl1-dl.bin
set FDL2=E:\lindroid4_flash\fdl2-dl.bin
set STOK=E:\backupan-ori-advantabvxlite_t1.1\backup

echo ============================================================
echo  ROLLBACK ke Android stok:
echo    boot_a ^<- %STOK%\boot_a.bin
echo    super  ^<- %STOK%\super.bin
echo  (userdata TIDAK disentuh, data aplikasi tetap)
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%STOK%\boot_a.bin" ^
  w super "%STOK%\super.bin" ^
  reset

pause
