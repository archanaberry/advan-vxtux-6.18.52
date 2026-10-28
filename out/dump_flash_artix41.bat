@echo off
setlocal
REM ===========================================================================
REM  artix41 -- DUMP LENGKAP + FLASH boot_a + dtb_a + reset
REM
REM  Satu sesi spd_dump:
REM    read : cache 100MB, super 16MB, common_rs1_a 8MB, uboot_log 4MB,
REM           sysdumpdb 10MB, misc 1MB, miscdata 1MB, boot_a 64MB,
REM           dtb_a 8MB, dtbo_a 8MB
REM    write: boot_a <- boot_artix41.img, dtb_a <- dtb_artix41\*.dtb
REM    reset
REM
REM  BACKUP OTOMATIS: dtb_a lama dibaca ke logs\artix41\dtb_a_stock.bin
REM  SEBELUM ditulis.  Untuk kembali: restore_dtb41.bat
REM
REM  Kenapa dtb_a: boot image tidak memuat FDT; kernel memakai DTB dari
REM  partisi dtb_a.  Tanpa menulis dtb_a, sc2730.dtsi tidak pernah dipakai.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix41.img
set DTB=%~dp0dtb_artix41\ums512-1h10-vxtux.dtb
set LOG=%~dp0logs\artix41
set PY=C:\Python314\python.exe

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )
if not exist "%DTB%"  ( echo [X] DTB tidak ada: %DTB% & pause & exit /b 1 )

if exist "%LOG%\dtb_a_stock.bin" (
  echo [i] backup dtb_a lama dipertahankan: %LOG%\dtb_a_stock.bin
) else (
  if not exist "%LOG%" mkdir "%LOG%"
)

echo ============================================================
echo  DUMP + FLASH artix41
echo   boot_a 64MB ^| dtb_a 8MB ^| dtbo_a 8MB ^| cache 100MB
echo   super 16MB ^| common_rs1_a 8MB ^| uboot_log 4MB ^| sysdumpdb 10MB
echo  image : %BOOT%
for %%A in ("%BOOT%") do echo  ukuran: %%~zA byte
echo  dtb   : %DTB%
for %%A in ("%DTB%")  do echo  ukuran: %%~zA byte
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BOOT%';print('  boot md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
  "%PY%" -c "import hashlib;p=r'%DTB%';print('  dtb  md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
)
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  read_part dtb_a        0 0x800000   dtb_a_stock.bin ^
  read_part dtbo_a       0 0x800000   dtbo_a_stock.bin ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part super        0 0x1000000  super.bin ^
  read_part common_rs1_a 0 0x800000   common_rs1_a.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part sysdumpdb    0 0xA00000   sysdumpdb.bin ^
  read_part misc         0 0x100000   misc.bin ^
  read_part miscdata     0 0x100000   miscdata.bin ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  w boot_a "%BOOT%" ^
  w dtb_a  "%DTB%" ^
  reset > "%LOG%\spd_dump_session.log" 2>&1

echo.
echo === hasil dump ===
for %%F in ("%LOG%\*.bin") do echo   %%~nxF  %%~zF byte

echo.
echo === analisa otomatis ===
if exist "%PY%" "%PY%" "%OUT%analyze_dump.py" "%LOG%"

echo.
echo [i] verifikasi flash:
if exist "%PY%" (
  "%PY%" -c "import hashlib,os;m=r'%BOOT:~,-4%.md5';e=open(m).read().strip() if os.path.exists(m) else '';p=r'%LOG%\boot_a.bin';h=hashlib.md5(open(p,'rb').read(os.path.getsize(r'%BOOT%'))).hexdigest();print('  boot_a prefix md5:',h);print('  artix41 mendarat :',h==e)"
)
echo [i] dtb_a sekarang (seharusnya berisi DTB kita):
if exist "%PY%" (
  "%PY%" -c "import os;p=r'%LOG%\dtb_a.bin';d=open(p,'rb').read() if os.path.exists(p) else b'';print('  magic FDT :',d[:4]==bytes.fromhex('d00dfeed'));print('  dcdc-cpu  :',b'dcdc-cpu' in d);print('  DCDC_CPU  :',b'DCDC_CPU' in d)"
)

echo.
echo  Transkrip : %LOG%\spd_dump_session.log
echo  Backup    : %LOG%\dtb_a_stock.bin  (restore_dtb41.bat untuk kembali)
echo  Setelah reset, di layar cari: VXTUX-MOD / VXTUX-DEFER / storage ready / chroot OK
pause
