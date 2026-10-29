@echo off
setlocal
REM ===========================================================================
REM  FLASH artix45  ->  boot_a saja  (boot_artix45.img)
REM
REM  Isi artix45 (build diagnostik untuk layar yang beku di teks LK):
REM   * VXTUX_INITTRACE=y -- tracer menggambar NAMA setiap initcall ke
REM     framebuffer LK sebelum initcall dijalankan.  Kalau boot berhenti,
REM     baris terakhir di layar adalah initcall pelakunya.
REM   * vxtux_lkfb pindah ke fs_initcall (level 5): fbcon (level 4) tetap
REM     lebih dulu, tetapi konsol hidup SEBELUM drivers/mmc / drivers/vxtux
REM     di level 6 -> teks kernel terlihat walau probe eMMC menggantung.
REM   * /vxtux/init mencetak "VXTUX-STEP: 0/5 init alive" SEBELUM menunggu
REM     eMMC (sebelumnya 180 detik tanpa satu byte pun di layar).
REM
REM  Skrip ini HANYA menulis boot_a.  vendor_boot_a tidak disentuh: DT kita
REM  sudah terpasang di sana sejak artix44 (flash_artix44.bat).
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Harus terbaca: "Write Part Done: boot_a"
REM   4. Cabut USB, nyalakan.  Yang harus muncul di layar, berurutan:
REM        VXTUX-ITRACE: kernel alive - watching initcalls, last line = hang
REM        <nomor> <nama initcall> ...        <- berhenti di sini = pelakunya
REM        vxtux-lkfb: LK fb ... fbcon active
REM        VXTUX ARTIX DIAG (initramfs) / VXTUX-STEP: 0/5 init alive
REM   5. Setelah itu baru jalankan dump_artix45_logonly.bat.
REM
REM  Kalau ingin kembali ke artix44: flash_artix44.bat.

REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0
set LOG=%OUT%logs\vb44
set BOOT=%OUT%6.18.52-VXTux-UMS512\boot_artix45.img
set VB=%LOG%\vendor_boot_patch44.img
set PY=C:\Python314\python.exe

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )
if not exist "%VB%"   ( echo [X] vendor_boot patch tidak ada: %VB% & pause & exit /b 1 )
if not exist "%LOG%"  mkdir "%LOG%"

echo ============================================================
echo  boot_a        ^<-  %BOOT%
for %%A in ("%BOOT%") do echo   ukuran : %%~zA byte
echo  vendor_boot_a ^<-  %VB%
for %%A in ("%VB%")   do echo   ukuran : %%~zA byte
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BOOT%';print('  boot md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
  "%PY%" -c "import hashlib;p=r'%VB%';d=open(p,'rb').read();print('  vb  md5  :',hashlib.md5(d).hexdigest());print('  vb  dtb  :',d[0x15be040:0x15be044]==bytes.fromhex('d00dfeed'))"
)
echo.
echo  Tablet harus sudah di Mode Download (VOL DOWN + POWER, colok USB).
echo ============================================================
pause

REM CWD di folder log (drive E:) selama spd_dump berjalan: kalau dijalankan
REM dari G:\UMS512\unlock\advan (penuh) spd_dump mati "fwrite(dump) failed"
REM sebelum satu perintah pun jalan.
pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%BOOT%" ^
  w vendor_boot_a "%VB%" ^
  reset
set SPDRC=%ERRORLEVEL%
popd

echo.
if not "%SPDRC%"=="0" (
  echo [!] ada error -- cari "Write Part Done: boot_a" /
  echo     "Write Part Done: vendor_boot_a" di atas.
) else (
  echo [OK] boot_a + vendor_boot_a tertulis.
  echo      Di layar cari:
  echo        "VXTUX-BLDB:"            pintu debug kedua, langkah ke-N
  echo        "VXTUX-LOG: ... 10 files"  bundle log sudah ditulis
  echo        "VXTUX-DEFER[post-modules]" device yang masih tanpa driver
  echo        "storage ready after Ns"   eMMC hidup
  echo        "chroot OK, exec /sbin/init" masuk Artix
  echo      Lalu jalankan dump_artix45_logonly.bat.
)
pause
