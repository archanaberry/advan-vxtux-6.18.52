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
set OUT=%~dp0
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix43.img
set DTB=%~dp0dtb_artix43\ums512-1h10-vxtux.dtb
set LOG=%~dp0logs\artix43
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
echo  DUMP + FLASH artix43
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

REM --- PENTING: CWD di folder log (drive E:) selama spd_dump berjalan -----
REM  Tanpa ini spd_dump menulis pgpt.bin/partition_*.xml ke folder kerja
REM  (G:\UMS512\unlock\advan, sudah 100% penuh) lalu mati "fwrite(dump)
REM  failed" SEBELUM read_part pertama -> tidak ada .bin dan tidak ada
REM  flash sama sekali.
pushd "%LOG%"
"%SPD%" --wait 300 --verbose 1 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
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
  reset > "%LOG%\spd_dump_session.log" 2>&1
popd

echo.
echo === hasil dump ===
for %%F in ("%LOG%\*.bin") do echo   %%~nxF  %%~zF byte

echo.
echo === analisa otomatis ===
if exist "%PY%" "%PY%" "%OUT%analyze_dump.py" "%LOG%"

echo.
echo [i] verifikasi flash:
if exist "%PY%" if exist "%LOG%\boot_a.bin" (
  "%PY%" -c "import hashlib,os;m=r'%BOOT:~,-4%.md5';e=open(m).read().strip() if os.path.exists(m) else '';p=r'%LOG%\boot_a.bin';h=hashlib.md5(open(p,'rb').read(os.path.getsize(r'%BOOT%'))).hexdigest();print('  boot_a prefix md5:',h);print('  artix43 mendarat :',h==e)"
) else (
  echo   [!] boot_a.bin tidak ada - dump gagal, lihat baris terakhir transkrip di bawah
)
echo [i] dtb_a sekarang (seharusnya berisi DTB kita):
if exist "%PY%" (
  "%PY%" -c "import os;p=r'%LOG%\dtb_a.bin';d=open(p,'rb').read() if os.path.exists(p) else b'';print('  magic FDT :',d[:4]==bytes.fromhex('d00dfeed'));print('  dcdc-cpu  :',b'dcdc-cpu' in d);print('  DCDC_CPU  :',b'DCDC_CPU' in d)"
)

echo.
echo  Transkrip : %LOG%\spd_dump_session.log
echo  Backup    : %LOG%\dtb_a_stock.bin  (restore_dtb41.bat untuk kembali)
echo  Setelah reset, di layar cari: VXTUX-BLDB / VXTUX-MOD / VXTUX-DEFER / storage ready / chroot OK
if not exist "%LOG%\dtb_a_stock.bin" (
  echo.
  echo  [!] dtb_a_stock.bin KOSONG/TIDAK ADA - backup dtb_a belum berhasil.
echo      Lihat 8 baris terakhir transkrip:
  powershell -NoProfile -Command "if (Test-Path '%LOG%\spd_dump_session.log') { Get-Content '%LOG%\spd_dump_session.log' -Tail 8 }"
)
pause
