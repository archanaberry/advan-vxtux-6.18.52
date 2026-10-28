@echo off
setlocal
REM ===========================================================================
REM  BUILD LOKAL artix41  ->  boot_artix41.img  +  out\dtb_artix41\*.dtb
REM
REM  KENAPA artix41 (defer belum sembuh di v39/v40):
REM
REM   A. DTB TIDAK PERNAH DIPAKAI.
REM      boot_artix40.img tidak berisi satu pun FDT (tidak ada magic D0 0D FE
REM      ED di page mana pun), dan boot_a stok juga tidak.  Kernel menerima
REM      DTB dari bootloader, yang membacanya dari partisi dtb_a.  Selama ini
REM      yang di-flash hanya boot_a, jadi sc2730.dtsi (rail eMMC, dll) tidak
REM      pernah sampai ke kernel - Image berubah, DT tetap DT stok.
REM      -> artix41 ikut membangun dan mem-flash dtb_a.
REM
REM   B. NAMA RAIL TIDAK COCOK of_match DRIVER.
REM      regulator_of_get_init_node() membandingkan child DT dengan
REM      desc->of_match ("dcdc-cpu", "ldo-vddemmccore"), sedangkan node
REM      memakai nama ROM asli (DCDC_CPU, LDO_VDDEMMCCORE).  devm_regulator_
REM      register() gagal -ENODEV pada deskriptor PERTAMA -> probe berhenti ->
REM      seluruh provider hilang -> vmmc eMMC, sdio, usb33, wifipa, codec
REM      defer selamanya.
REM      -> setiap rail sekarang punya regulator-compatible = of_match.
REM
REM   C. BUS ADI STOK BERNAMA LAIN.
REM      FDT stok: compatible = "sprd,sharkl5pro-adi" (driver hanya punya
REM      sc9860/sc9863/ums512).  Tanpa alias, tidak ada bus ADI -> PMIC,
REM      gpio-eic, adc, rtc semuanya tidak pernah probe.
REM      -> driver diberi alias sharkl5pro dengan data ums512.
REM
REM  SCRIPT INI OFFLINE: tidak ada gh/jaringan.  Image + .ko diambil dari
REM  out\ci_artix39 (artefak run CI), DTB dibangun dari DTS saat ini.
REM
REM  Gate yang harus lulus (kalau gagal -> FAILED=...):
REM    * 32/32 of_match punya child DT (out\regcheck.py)
REM    * DTB memuat rail dcdc-cpu, ldo-vddemmccore, ldo-vddsdcore, ldo-vddsdio
REM    * vmmc-supply host eMMC -> LDO_VDDEMMCCORE (out\dtcheck.py)
REM    * 9 .ko termuat, alias OF SC2730 ada, vermagic cocok
REM
REM  Hasil: boot_artix41.img + boot_artix41.md5/.size + dtb_artix41\dtb.md5
REM ===========================================================================

set OUT=E:\vxtux_build_repo\out
set REPO=E:\vxtux_build_repo
set IMG=%OUT%\6.18.52-VXTux-UMS512\boot_artix41.img
set MD5F=%OUT%\6.18.52-VXTux-UMS512\boot_artix41.md5
set SIZEF=%OUT%\6.18.52-VXTux-UMS512\boot_artix41.size
set DTBMD5=%OUT%\dtb_artix41\dtb.md5
set DTBSIZE=%OUT%\dtb_artix41\dtb.size
set LOGDIR=%OUT%\logs\artix41
set PY=C:\Python314\python.exe
set BASHRUN=
if exist "C:\Program Files\Git\bin\bash.exe" set BASHRUN=C:\Program Files\Git\bin\bash.exe
if not defined BASHRUN (
  where bash >nul 2>&1 && set BASHRUN=bash
)
if not defined BASHRUN (
  echo [X] Git Bash tidak ditemukan.
  pause
  exit /b 1
)

if not exist "%LOGDIR%" mkdir "%LOGDIR%"

echo ============================================================
echo  BUILD artix41  (boot image + DTB dtb_a)
echo  log penuh : %LOGDIR%\build.log
echo ============================================================
pushd "%REPO%"

echo.
echo [1/4] bangun DTB dari DTS saat ini ...
"%BASHRUN%" out/build_dtb41.sh > "%LOGDIR%\dtb.log" 2>&1
if errorlevel 1 (
  echo [X] build DTB gagal.  Lihat %LOGDIR%\dtb.log
  type "%LOGDIR%\dtb.log"
  popd
  pause
  exit /b 1
)
findstr /C:"[dtb] ditulis" "%LOGDIR%\dtb.log"

echo.
echo [2/4] gate rail driver vs DT ...
"%PY%" out\regcheck.py > "%LOGDIR%\regcheck.log" 2>&1
if errorlevel 1 (
  echo [X] regcheck GAGAL - probe regulator akan berhenti.
  type "%LOGDIR%\regcheck.log"
  popd
  pause
  exit /b 1
)
findstr /C:"OK:" /C:"ldo-vddemmccore" "%LOGDIR%\regcheck.log"

echo.
echo [3/4] pipeline image (init + 9 .ko) ...
"%BASHRUN%" out/ci_artix41_pipeline.sh > "%LOGDIR%\build.log" 2>&1
set RC=%ERRORLEVEL%
popd

echo [4/4] hasil penting:
findstr /C:"dtb OK" /C:"OK: 32/32" /C:"alias OF SC2730" /C:"INIT_LOCAL" /C:"ramdisk ditulis ulang" /C:"ditulis out" /C:"DTB=" /C:"DONE=1" /C:"FAILED=" "%LOGDIR%\build.log"

echo.
if not exist "%IMG%" (
  echo [X] image tidak terbentuk.  Lihat FAILED= di %LOGDIR%\build.log
  pause
  exit /b 1
)
for %%A in ("%IMG%") do echo   image : %%~zA byte  (file .size: %SIZEF%)
if exist "%PY%" (
  "%PY%" -c "import hashlib,os;p=r'%IMG%';h=hashlib.md5(open(p,'rb').read()).hexdigest();e=open(r'%MD5F%').read().strip() if os.path.exists(r'%MD5F%') else '';print('  md5   :',h);print('  cocok :',h==e)"
)
if exist "%PY%" (
  "%PY%" -c "import hashlib,os;p=r'%OUT%\dtb_artix41\ums512-1h10-vxtux.dtb';h=hashlib.md5(open(p,'rb').read()).hexdigest();e=open(r'%DTBMD5%').read().strip() if os.path.exists(r'%DTBMD5%') else '';print('  dtb md5:',h,'(',os.path.getsize(p),'byte )');print('  dtb cocok:',h==e)"
)

echo.
if not "%RC%"=="0" goto :fail
findstr /C:"DONE=1" "%LOGDIR%\build.log"
if errorlevel 1 goto :nodone
echo [OK] build selesai.
echo      boot_artix41.img  +  out\dtb_artix41\ums512-1h10-vxtux.dtb
echo      Lanjut: flash_artix41.bat  boot_a + dtb_a
echo      atau  : dump_flash_artix41.bat  dump + flash + reset
echo      atau  : dump_artix41_logonly.bat  dump read-only dulu
pause
exit /b 0

:nodone
echo [!] pipeline keluar 0 tapi DONE=1 tidak ada - cek build.log
pause
exit /b 1

:fail
echo [X] pipeline GAGAL  rc=%RC%   penyebab:
findstr /C:"FAILED=" "%LOGDIR%\build.log"
pause
exit /b %RC%
