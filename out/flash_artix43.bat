@echo off
setlocal
REM ===========================================================================
REM  FLASH artix43  ->  boot_a saja  (boot_artix43.img)
REM
REM  Isi artix43 (dibangun mengikuti FDT STOK vendor_boot_a, bukan tebakan):
REM   * i2c-sprd menerima "sprd,sharkl5pro-hw-i2c" -- compatible asli node
REM     i2c@32060000 di firmware.  Sebelumnya driver hanya mengenal
REM     "sprd,sc9860-i2c" -> bus i2c tidak pernah probe, semua konsumennya
REM     defer.  clock-names stok ("source","enable","i2c") sudah persis.
REM   * codec + VBC menerima prefiks rebrand "unisoc,..." (stok memakai
REM     unisoc,sc2730-audio-codec / unisoc,vbc-v4-codec-sc2730 /
REM     unisoc,sharkl5pro-vbc) -> i2s/kartu suara punya driver.
REM   * sc2731-regulator (built-in, peta register berbeda) kini MENOLAK bila
REM     induk PMIC = sc2730 -> modul sc2730-regulator yang memegang node.
REM     Ini pengaman hardware: menulis peta sc2731 ke chip sc2730 berisiko.
REM   * regu_name_eq (artix42): rail gaya stok DCDC_CPU cocok "dcdc-cpu".
REM   * BLDB (Berry Lab Foundation): ring 16 MiB + /dev/bldb0 sejak
REM     core_initcall, daemon /vxtux/bldbd, gadget ACM identitas palsu Berry
REM     1209:b1db -> pintu debug KEDUA selain adbd, tanpa solder.
REM
REM  Skrip ini TIDAK menyentuh dtb_a / dtbo_a / vendor_boot_a / super.
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap), colok USB, tunggu 5 detik.
REM   2. Jalankan skrip ini, tekan apa pun saat pause.
REM   3. Yang harus terbaca: "Write Part Done: boot_a"
REM   4. Setelah reset jangan tahan tombol apa pun, lalu jalankan
REM      dump_artix43_logonly.bat untuk membaca hasil boot.

REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix43.img
set DTB=%~dp0dtb_artix43\ums512-1h10-vxtux.dtb
set LOG=%~dp0logs\artix43
set PY=C:\Python314\python.exe

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )
if not exist "%DTB%"  ( echo [X] DTB tidak ada: %DTB%  & echo     jalankan build_artix43.bat dulu. & pause & exit /b 1 )
if not exist "%LOG%" mkdir "%LOG%"

echo ============================================================
echo  boot_a ^<- %BOOT%
for %%A in ("%BOOT%") do echo   ukuran : %%~zA byte
echo  (dtb_a TIDAK disentuh: LK membaca DT dari vendor_boot_a)
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BOOT%';print('  boot md5:',hashlib.md5(open(p,'rb').read()).hexdigest())"

)
echo.
echo  Tablet: tahan VOL DOWN + POWER 15 detik, lepas POWER
echo  (VOL DOWN tetap), colok USB, tunggu 5 detik.
echo ============================================================
pause

REM --- PENTING: CWD di folder log (drive E:) selama spd_dump berjalan -----
REM  Kalau dijalankan dari G:\UMS512\unlock\advan (drive itu penuh) spd_dump
REM  mati "fwrite(dump) failed" saat membaca daftar partisi, sebelum satu
REM  read_part / w dijalankan.  Jadi pindah dulu ke folder log di E:.
pushd "%LOG%"
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  read_part dtb_a 0 0x800000 dtb_a_stock.bin ^
  w boot_a "%BOOT%" ^
  reset
set SPDRC=%ERRORLEVEL%
popd

echo.
if not "%SPDRC%"=="0" (
  echo [!] ada error -- cek "Write Part Done: boot_a" di atas
) else (
  echo [OK] boot_a tertulis (dtb_a/dtbo_a/vendor_boot_a tidak disentuh).
  echo      Setelah reset, yang harus muncul di layar:
  echo        "VXTUX-BLDB:"                   ^(ring 16MiB + gadget ACM^)
  echo        "VXTUX-MOD: /vxtux/ko/sc2730-regulator.ko loaded"
  echo        "VXTUX-DEFER[post-modules]: ..."  ^(daftar device tanpa driver^)
  echo        "storage ready after Ns"           ^(eMMC hidup^)
  echo        "chroot OK, exec /sbin/init"       ^(masuk Artix^)
  echo      Setelah itu jalankan dump_artix43_logonly.bat.
)
pause
