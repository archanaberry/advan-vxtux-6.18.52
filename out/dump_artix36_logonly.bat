@echo off
REM ===========================================================================
REM  artix36 -- HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah partisi apa pun)
REM
REM  Dipakai SESUDAH boot boot_artix36.img (penjejak initcall di panel):
REM    * "VXTUX-ITRACE: kernel alive..." = banner tracer, layar hitam+teks.
REM    * Tiap initcall digambar "NNN <nama>" SEBELUM dieksekusi; saat boot
REM      beku, BARIS TERAKHIR di layar = initcall yang menggantung.
REM    * Kanal lama tetap ada: flightrec VXTUXFR1 @8MiB, laporan init
REM      VXTUXLOG1 @4MiB, zona uji VXTUXWRT @6MiB, pstore @0.
REM
REM  STATUS: artix35 terbukti gantung senyap setelah "[start_linux]" LK
REM  (tanpa panic/tanpa tulisan cache); artix36 menambah tracer untuk
REM  menunjuk titik macetnya.  Data UTAMA = foto layar; dump ini pelengkap.
REM
REM  Yang diambil (semuanya READ-ONLY, partisi tidak disentuh):
REM    cache      100MB  -> zona pstore @0 / VXTUXLOG1 @4MiB /
REM                         VXTUXWRT @6MiB / VXTUXFR1 @8MiB
REM    uboot_log    4MB  -> log LK boot ini (start_linux dll.)
REM    sysdumpdb   10MB  -> minidump vendor
REM    misc         1MB  -> BCB
REM    miscdata     1MB  -> data misc
REM    boot_a      64MB  -> VERIFIKASI flash: md5 44.302.336 byte PERTAMA
REM                         harus = 169a93d98e79a666f69d863c26cfcdb9
REM    reset             -> tablet reboot ke kernel yang sedang ter-flash
REM
REM  PAKAI (dari kondisi macet): tahan POWER 15 detik sampai tablet mati
REM  total, lalu tahan VOL- sambil colok USB (BROM), jalankan script ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix36

REM dump_flash_artix36.bat menulis dump pra-flash ke logs\artix36 (state
REM artix35).  Aman-kan dulu supaya bukti "sebelum" tidak tertimpa; hasil
REM baru (sesudah boot artix36) masuk ke logs\artix36.
if exist "%LOG%\cache.bin" (
  if exist "%OUT%\logs\artix36_preflash" rmdir /s /q "%OUT%\logs\artix36_preflash"
  move "%LOG%" "%OUT%\logs\artix36_preflash" >nul
  echo [i] Dump lama sebelum flash artix36 dipindah -^> logs\artix36_preflash
)
if not exist "%LOG%" mkdir "%LOG%"

echo [1/1] DUMP SEMUA LOG (tanpa flash) -^> %LOG%
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part sysdumpdb    0 0xA00000   sysdumpdb.bin ^
  read_part misc         0 0x100000   misc.bin ^
  read_part miscdata     0 0x100000   miscdata.bin ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  reset

echo.
echo [i] Log tersimpan di %LOG%
echo [i] DATA UTAMA = foto layar (baris terakhir tracer = initcall macet).
echo       "VXTUX-ITRACE"                -^> banner tracer artix36
echo       baris "NNN nama_initcall"      -^> progres; terakhir = macet
echo       tidak ada banner sama sekali   -^> gantung sebelum initcalls
echo [i] Isi dump ini (pelengkap):
echo       "VXTUXWRT"  @ 6MiB  -^> zona uji kanal tulis eMMC
echo       VXTUXFR1    @ 8MiB  -^> dmesg flightrec (baca-balik diverifikasi)
echo       VXTUXLOG1   @ 4MiB  -^> laporan init (baca-balik diverifikasi)
echo       pstore_blk  @ 0MiB  -^> catatan console/kmsg saat panic
echo [i] VERIFIKASI FLASH otomatis (44.302.336 byte PERTAMA boot_a.bin):
if not exist "%LOG%\boot_a.bin" goto :nosha
if exist C:\Python314\python.exe C:\Python314\python.exe -c "import hashlib;h=hashlib.md5(open(r'%LOG%\boot_a.bin','rb').read(44302336)).hexdigest();print('  boot_a prefix md5 =',h);print('  flash boot_artix36 mendarat =',h=='169a93d98e79a666f69d863c26cfcdb9')"
if not exist C:\Python314\python.exe echo       [!] C:\Python314\python.exe tidak ada -- hitung manual.
:nosha
echo [i] Bila prefix md5 BEDA 169a... -> flash gagal; jalankan
echo       flash_artix36.bat lalu dump ulang.
pause
