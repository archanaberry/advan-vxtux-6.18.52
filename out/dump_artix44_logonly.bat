@echo off
setlocal
REM ===========================================================================
REM  artix44 -- DUMP SEMUA LOG (read-only, TANPA flash)
REM
REM  KENAPA dtb_a / dtbo_a DITAMBAH:
REM    Boot image kita tidak memuat DTB board (hanya stub FDT 131 byte), dan
REM    Image tidak memuat string ums512-1h10/ldo-vddemmccore.  Jadi kernel
REM    memakai DTB dari partisi dtb_a.
REM    Tanpa membaca dtb_a kita tidak pernah bisa membandingkan DT yang
REM    BENAR-BENAR dipakai kernel dengan DTS kita.  Itu sebabnya tiga build
REM    berturut-turut "defer-nya sama saja": yang diubah bukan DT yang dipakai.
REM
REM  Satu sesi spd_dump (~40 detik):
REM    cache 100MB | super 16MB | common_rs1_a 8MB | uboot_log 4MB
REM    sysdumpdb 10MB | misc 1MB | miscdata 1MB | boot_a 64MB
REM    dtb_a 8MB | dtbo_a 8MB
REM
REM  Yang dibaca dari hasil:
REM    dtb_a.bin magic D0 0D FE ED  -> partisi memang FDT
REM    dtb_a.bin memuat "sharkl5pro-adi" / "dcdc-cpu" -> ini DT yang dipakai
REM    boot_a prefix md5           -> artix44 benar-benar terpasang
REM    VXTUX-DEFER[...]            -> nama device yang belum dapat driver
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\artix44
set PY=C:\Python314\python.exe

if not exist "%SPD%" ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )

REM -- rotasi: bukti boot sebelumnya tidak ditimpa ---------------------------
if exist "%LOG%\cache.bin" (
  if exist "%LOG%_prev" rmdir /s /q "%LOG%_prev"
  move "%LOG%" "%LOG%_prev" >nul 2>&1
  echo [i] dump sebelumnya -^> logs\artix44_prev
)
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  DUMP SEMUA LOG (read-only) -^> %LOG%
echo   cache 100MB ^| super 16MB ^| common_rs1_a 8MB ^| uboot_log 4MB
echo   sysdumpdb 10MB ^| misc 1MB ^| miscdata 1MB ^| boot_a 64MB
echo   dtb_a 8MB ^| dtbo_a 8MB
echo  Tablet harus sudah di Mode Download (VOL DOWN+POWER, colok USB).
echo ============================================================
pause

REM --- PENTING: jalankan spd_dump DENGAN CWD di folder log (drive E:) -----
REM  Tanpa ini spd_dump menulis pgpt.bin / partition_*.xml ke folder kerja
REM  (biasanya G:\UMS512\unlock\advan yang sudah 100% penuh) dan mati dengan
REM  "fwrite(dump) failed" SEBELUM satu pun read_part dijalankan -> tidak ada
REM  .bin sama sekali.  Perintah "path" tidak dipakai karena build ini tidak
REM  memakainya untuk file dump; CWD yang menentukan.
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
  read_part dtb_a        0 0x800000   dtb_a.bin ^
  read_part dtbo_a       0 0x800000   dtbo_a.bin ^
  reset > "%LOG%\spd_dump_session.log" 2>&1
set SPDRC=%ERRORLEVEL%
popd

echo.
echo === hasil dump ===
for %%F in ("%LOG%\*.bin") do echo   %%~nxF  %%~zF byte
if not exist "%LOG%\cache.bin" (
  echo.
  echo [X] TIDAK ADA .bin - spd_dump berhenti sebelum membaca partisi.
  echo     Cari baris ini di %LOG%\spd_dump_session.log :
  echo       "fwrite(dump) failed"  -^> folder kerja penuh / tidak bisa ditulis
  echo       "device removed"       -^> kabel/USB lepas, ulangi mode download
  echo       "incompatible partition" -^> FDL2 untuk chip lain
  echo     Baris terakhir transkrip:
  powershell -NoProfile -Command "if (Test-Path '%LOG%\spd_dump_session.log') { Get-Content '%LOG%\spd_dump_session.log' -Tail 8 }"
)

echo.
echo ============================================================
echo  analisa otomatis (tanpa mount)
echo ============================================================
if exist "%PY%" (
  "%PY%" "%OUT%analyze_dump.py" "%LOG%"
) else (
  findstr /C:"VXTUX" "%LOG%\cache.bin"
)

echo.
echo [i] DTB yang benar-benar dipakai kernel (dtb_a):
if exist "%PY%" (
  "%PY%" -c "import os;p=r'%LOG%\dtb_a.bin';d=open(p,'rb').read() if os.path.exists(p) else b'';print('  ukuran      :',len(d));print('  magic FDT   :',d[:4]==bytes.fromhex('d00dfeed'));print('  sharkl5pro  :',b'sharkl5pro' in d);print('  ums512-adi  :',b'ums512-adi' in d);print('  dcdc-cpu    :',b'dcdc-cpu' in d);print('  DCDC_CPU    :',b'DCDC_CPU' in d);print('  ldo-vddemmc :',b'ldo-vddemmccore' in d)"
)

echo.
echo [i] Apakah boot_a berisi artix44:
if exist "%LOG%\boot_a.bin" if exist "%PY%" (
  "%PY%" -c "import hashlib,os;m=r'%OUT%6.18.52-VXTux-UMS512\boot_artix44.md5';e=open(m).read().strip() if os.path.exists(m) else '';h=hashlib.md5(open(r'%LOG%\boot_a.bin','rb').read(os.path.getsize(r'%OUT%6.18.52-VXTux-UMS512\boot_artix44.img') if os.path.exists(r'%OUT%6.18.52-VXTux-UMS512\boot_artix44.img') else 0x4000000)).hexdigest();print('  prefix md5 =',h);print('  artix44 mendarat =',h==e)"
)

echo.
echo.
echo [i] bundle log (kanal mentah common_rs1_a):
if exist "%LOG%\common_rs1_a.bin" if exist "%PY%" (
  "%PY%" "%OUT%baca_bundle44.py" "%LOG%\common_rs1_a.bin"
)

echo  Laporan penuh : %LOG%\analysis.txt
echo  Transkrip spd : %LOG%\spd_dump_session.log
echo  Untuk flash   : flash pertama kali  -^>  flash_artix44.bat
echo                  dump + flash + reset -^>  dump_flash_artix44.bat
pause
