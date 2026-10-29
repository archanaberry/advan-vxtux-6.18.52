@echo off
setlocal
REM ===========================================================================
REM  artix42 -- FLASH vendor_boot_a (berisi DT kita) + verifikasi
REM
REM  Satu sesi spd_dump:
REM    1. read_part vendor_boot_a  -> logs\vb42\vendor_boot_a_backup.bin
REM    2. w vendor_boot_a          <- logs\vb42\vendor_boot_patch42.img
REM    3. read_part vendor_boot_a @0x15be000 0x4000 -> logs\vb42\vb_verify.bin
REM    4. reset
REM
REM  Kembali ke asli kalau tablet tidak mau boot:
REM       w vendor_boot_a dari logs\vb42\vendor_boot_a_backup.bin
REM       (pakai script restore_vendor42.bat)
REM
REM  Tidak menyentuh boot_a / super / dtb_a / partisi lain.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\vb42
set PATCH=%LOG%\vendor_boot_patch42.img
set PY=C:\Python314\python.exe

if not exist "%SPD%"   ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%PATCH%" ( echo [X] belum ada %PATCH% - jalankan build_vendor42.bat dulu & pause & exit /b 1 )
if not exist "%LOG%"   mkdir "%LOG%"
if not exist "%LOG%\vendor_boot_a_backup.bin" (
  echo [i] backup vendor_boot_a belum ada, akan dibuat di sesi ini ^(~1 menit^).
)

echo ============================================================
echo  vendor_boot_a  ^<-  %PATCH%
for %%A in ("%PATCH%") do echo   ukuran : %%~zA byte
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

pushd "%LOG%"
"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  read_part vendor_boot_a 0 0x6400000 vendor_boot_a_backup.bin ^
  w vendor_boot_a "%PATCH%" ^
  read_part vendor_boot_a 0x15be000 0x4000 vb_verify.bin ^
  reset > "%LOG%\spd_dump_flash.log" 2>&1
popd

echo.
echo === ringkasan tulis ===
findstr /C:"Write Part Done: vendor_boot_a" /C:"Read Part Done: vendor_boot_a" /C:"file size" "%LOG%\spd_dump_flash.log"

echo.
echo === verifikasi isi DTB di vendor_boot_a sekarang ===
if exist "%PY%" if exist "%LOG%\vb_verify.bin" (
  "%PY%" -c "import struct;d=open(r'%LOG%\vb_verify.bin','rb').read();i=d.find(bytes.fromhex('d00dfeed'));print('  FDT @0x%s' % format(i,'x') if i>=0 else '  TIDAK ada FDT di 0x15be000..0x15be040');t=struct.unpack('>I',d[i+4:i+8])[0] if i>=0 else 0;print('  totalsize :',t);print('  ada advan,t1030 :',b'advan,t1030' in d[:t]);print('  ada ums512-1h10 :',b'ums512-1h10' in d[:t]);print('  ada ldo-vddemmccore:',b'ldo-vddemmccore' in d[:t])"
)

echo.
echo  Setelah reset, di layar cari: VXTUX-MOD / VXTUX-DEFER / storage ready / chroot OK
echo  Kalau tablet tidak mau boot: restore_vendor42.bat
pause
