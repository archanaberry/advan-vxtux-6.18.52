@echo off
setlocal
REM ===========================================================================
REM  FLASH artix41  ->  boot_a  +  dtb_a   (dtb_a = kunci artix41)
REM
REM  KENAPA dtb_a IKUT DI-FLASH:
REM    boot_a TIDAK memuat DTB board - pemeriksaan seluruh byte menemukan
REM    hanya satu magic FDT berukuran 131 byte (stub root #address-cells),
REM    dan Image tidak memuat string ums512-1h10/ldo-vddemmccore.  Jadi DTB
REM    dibaca bootloader dari partisi dtb_a.  Selama ini yang di-flash hanya
REM    boot_a -> kernel selalu memakai DTB stok, sehingga sc2730.dtsi (nama
REM    rail, vmmc eMMC, ADI) tidak pernah sampai ke kernel.  Itulah sebabnya
REM    defer tidak pernah sembuh walau DTS diubah tiga kali.
REM
REM  Urutan satu sesi spd_dump:
REM    1. read_part dtb_a  -> logs\artix41\dtb_a_stock.bin  (BACKUP dulu)
REM    2. w boot_a  <- boot_artix41.img
REM    3. w dtb_a   <- out\dtb_artix41\ums512-1h10-vxtux.dtb
REM    4. reset
REM
REM  Tidak menyentuh super / Artix / partisi lain.
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap ditekan), colok USB.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Yang harus terbaca:
REM        read : "Read Part Done: dtb_a"
REM        write: "Write Part Done: boot_a"  lalu  "Write Part Done: dtb_a"
REM   4. Setelah reset jangan tahan tombol apa pun.
REM
REM  Kalau tablet tidak mau boot setelah ini, KEMBALIKAN DTB LAMA:
REM       restore_dtb41.bat   (menulis balik logs\artix41\dtb_a_stock.bin)
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
if not exist "%DTB%"  ( echo [X] DTB tidak ada: %DTB%  & echo     jalankan build_artix41.bat dulu. & pause & exit /b 1 )
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  boot_a ^<- %BOOT%
for %%A in ("%BOOT%") do echo   ukuran : %%~zA byte
echo  dtb_a  ^<- %DTB%
for %%A in ("%DTB%")  do echo   ukuran : %%~zA byte
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BOOT%';print('  boot md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
  "%PY%" -c "import hashlib;p=r'%DTB%';print('  dtb  md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"
)
echo.
echo  Tablet: tahan VOL DOWN + POWER 15 detik, lepas POWER
echo  (VOL DOWN tetap), colok USB, tunggu 5 detik.
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  read_part dtb_a 0 0x800000 dtb_a_stock.bin ^
  w boot_a "%BOOT%" ^
  w dtb_a  "%DTB%" ^
  reset

echo.
if errorlevel 1 (
  echo [!] ada error -- cek "Write Part Done: boot_a" dan "dtb_a" di atas
) else (
  echo [OK] boot_a + dtb_a tertulis.
  echo      Setelah reset, yang harus muncul di layar:
  echo        "VXTUX-MOD: /vxtux/ko/sc2730-regulator.ko loaded"
  echo        "VXTUX-DEFER[post-modules]: ..."  ^(daftar device tanpa driver^)
  echo        "storage ready after Ns"           ^(eMMC hidup^)
  echo        "chroot OK, exec /sbin/init"       ^(masuk Artix^)
  echo      Kalau layar tetap gelap: restore_dtb41.bat
)
pause
