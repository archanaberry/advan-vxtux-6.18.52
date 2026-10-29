@echo off
setlocal
REM ===========================================================================
REM  artix42 -- DUMP LOG (read-only, TANPA flash) -> logs\artix42_boot
REM
REM  Yang dibaca: cache 100MB | super 16MB | common_rs1_a 8MB | uboot_log 4MB
REM               sysdumpdb 10MB | misc 1MB | miscdata 1MB | boot_a 64MB
REM
REM  Yang dicari di hasil:
REM    cache.bin / common_rs1_a.bin : "sc2730-regulator", "VXTUX-DEFER",
REM                                   "storage ready", "chroot OK"
REM    boot_a.bin prefix md5       : artix42 benar-benar terpasang
REM    uboot_log.bin               : LK memilih DT yang mana (sprd,sc-id)
REM
REM  CATATAN PENTING: spd_dump dijalankan dengan CWD di folder log (drive E:).
REM  Kalau dijalankan dari G:\UMS512\unlock\advan (drive itu penuh) spd_dump
REM  mati "fwrite(dump) failed" saat baca daftar partisi -> tidak ada .bin.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\artix42_boot
set PY=C:\Python314\python.exe

if not exist "%SPD%" ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )

REM rotasi: bukti boot sebelumnya tidak ditimpa
if exist "%LOG%\cache.bin" (
  if exist "%LOG%_prev" rmdir /s /q "%LOG%_prev"
  move "%LOG%" "%LOG%_prev" >nul 2>&1
  echo [i] dump sebelumnya -^> logs\artix42_boot_prev
)
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  DUMP LOG (read-only) -^> %LOG%
echo   cache 100MB ^| super 16MB ^| common_rs1_a 8MB ^| uboot_log 4MB
echo   sysdumpdb 10MB ^| misc 1MB ^| miscdata 1MB ^| boot_a 64MB
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

pushd "%LOG%"
"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part super        0 0x1000000  super.bin ^
  read_part common_rs1_a 0 0x800000   common_rs1_a.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part sysdumpdb    0 0xA00000   sysdumpdb.bin ^
  read_part misc         0 0x100000   misc.bin ^
  read_part miscdata     0 0x100000   miscdata.bin ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  reset > "%LOG%\spd_dump_session.log" 2>&1
popd

echo.
echo === hasil dump ===
for %%F in ("%LOG%\*.bin") do echo   %%~nxF  %%~zF byte
if not exist "%LOG%\cache.bin" (
  echo.
  echo [X] TIDAK ADA .bin - spd_dump berhenti sebelum membaca partisi.
  echo     8 baris terakhir transkrip:
  powershell -NoProfile -Command "if (Test-Path '%LOG%\spd_dump_session.log') { Get-Content '%LOG%\spd_dump_session.log' -Tail 8 }"
  pause
  exit /b 1
)

echo.
echo ============================================================
echo  analisa otomatis (tanpa mount)
echo ============================================================
if exist "%PY%" (
  "%PY%" "%OUT%analyze_dump.py" "%LOG%"
) else (
  findstr /C:"sc2730" /C:"VXTUX" /C:"storage ready" /C:"chroot" "%LOG%\cache.bin"
)

echo.
echo [i] apakah boot_a berisi artix42:
if exist "%PY%" (
  "%PY%" -c "import hashlib,os;p=r'%LOG%\boot_a.bin';img=r'%OUT%6.18.52-VXTux-UMS512\boot_artix42.img';n=os.path.getsize(img) if os.path.exists(img) else 0x4000000;h=hashlib.md5(open(p,'rb').read(n)).hexdigest();m=r'%OUT%6.18.52-VXTux-UMS512\boot_artix42.md5';e=open(m).read().strip() if os.path.exists(m) else '';print('  prefix md5 =',h);print('  artix42 mendarat =',h==e)"
)

echo.
echo  Laporan penuh : %LOG%\analysis.txt
echo  Transkrip spd : %LOG%\spd_dump_session.log
pause
