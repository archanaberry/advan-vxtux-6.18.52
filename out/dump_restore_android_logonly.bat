@echo off
REM ===========================================================================
REM  RESTORE ANDROID -- HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah apa pun)
REM
REM  Dipakai:
REM    * sesudah dump_flash_restore_android.bat, untuk VERIFIKASI bahwa flash
REM      mendarat (md5 boot_a.bin harus = 0a17b97edb355fd9e3d8e44662848a43)
REM    * kalau boot setelah restore bermasalah, untuk mengambil bukti dulu
REM      sebelum menyentuh apa pun lagi.
REM
REM  Yang diambil (semuanya READ-ONLY):
REM    boot_a      64MB   -> isi boot yang benar-benar ada di device
REM    super       5600MB -> state super sesudah restore (set DUMP_SUPER=0
REM                           kalau tidak perlu, +/- 5 menit hemat)
REM    uboot_log    4MB   -> jejak LK boot terakhir
REM    misc         1MB   -> BCB (pesan boot mode Android)
REM    cache      100MB   -> sisa pstore/konsole Android
REM    metadata     16MB  -> metadata Android (cek: pernah ter-format?)
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini.
REM ===========================================================================
setlocal
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\android_post
set DUMP_SUPER=1
set BOOT_MD5=0a17b97edb355fd9e3d8e44662848a43

if exist "%LOG%\boot_a.bin" (
  if exist "%OUT%\logs\android_post_prev" rmdir /s /q "%OUT%\logs\android_post_prev"
  move "%LOG%" "%OUT%\logs\android_post_prev" >nul
  echo [i] Dump lama dipindah -^> logs\android_post_prev
)
if not exist "%LOG%" mkdir "%LOG%"

if "%DUMP_SUPER%"=="1" goto :with_super
echo [1/1] DUMP LOG (tanpa flash) -^> %LOG%
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  size_part super ^
  size_part boot_a ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part misc         0 0x100000   misc.bin ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part metadata     0 0x1000000  metadata.bin ^
  reset
goto :done

:with_super
echo [1/1] DUMP LOG + super (tanpa flash) -^> %LOG%
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  size_part super ^
  size_part boot_a ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  read_part super        0 0x15E000000 super.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part misc         0 0x100000   misc.bin ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part metadata     0 0x1000000  metadata.bin ^
  reset

:done
echo.
echo [i] Log tersimpan di %LOG%
if not exist "%LOG%\boot_a.bin" goto :noverify
if not exist C:\Python314\python.exe goto :nopy
C:\Python314\python.exe -c "import hashlib,os;d=r'%LOG%';h=hashlib.md5(open(d+r'\boot_a.bin','rb').read()).hexdigest();print('  md5 boot_a            =',h);print('  boot magisk mendarat  =',h=='%BOOT_MD5%');s=d+r'\super.bin';print('  super terdump         =',os.path.exists(s),(str(os.path.getsize(s))+' byte') if os.path.exists(s) else '');print('  super ori diharap     = 5872025600 byte, md5 1f6e248a52f621d0ff5037590f230d61')"
goto :done2
:nopy
echo [i] python tidak ada - hitung manual:
echo       certutil -hashfile "%LOG%\boot_a.bin" MD5
:done2
:noverify
echo [i] Kalau boot_a = %BOOT_MD5% DAN super 5.872.025.600 byte -^> restore sukses.
pause
