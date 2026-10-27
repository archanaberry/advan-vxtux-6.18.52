@echo off
REM ===========================================================================
REM  artix34 -- HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah partisi apa pun)
REM
REM  Dipakai SESUDAH boot boot_artix34.img (kanal tulis terinstrumentasi):
REM    * VXTUX-WRITE-TEST di layar: uji tulis-baca-balik cache (zona 6 MiB).
REM    * flightrec mencetak errno setiap kegagalan (open/write/readback).
REM    * laporan init diverifikasi baca-balik sebelum menyatakan OK.
REM    * daftar /proc/partitions tampil juga di layar.
REM
REM  Yang diambil (semuanya READ-ONLY, partisi tidak disentuh):
REM    cache      100MB  -> kunci utama:
REM                         offset 0x0        : zona pstore_blk (console/kmsg)
REM                         offset 0x400000   : laporan init (VXTUXLOG1)
REM                         offset 0x600000   : ZONA UJI artix34 (tanda tangan
REM                                              "VXTUXWRT" 16 byte -- kalau ini
REM                                              ada, kanal tulis eMMC HIDUP)
REM                         offset 0x800000   : flightrec kthread (VXTUXFR1)
REM    uboot_log    4MB  -> log LK boot ini
REM    sysdumpdb   10MB  -> minidump vendor
REM    misc         1MB  -> BCB
REM    miscdata     1MB  -> data misc
REM    boot_a      64MB  -> VERIFIKASI flash: md5 vs boot_artix34.img
REM    reset             -> tablet reboot ke kernel yang sedang ter-flash
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix34

REM dump_flash_artix34.bat menulis dump pra-flash ke logs\artix34 (state
REM artix33).  Aman-kan dulu supaya bukti "sebelum" tidak tertimpa; hasil
REM baru (sesudah boot artix34) masuk ke logs\artix34.
if exist "%LOG%\cache.bin" (
  if exist "%OUT%\logs\artix34_preflash" rmdir /s /q "%OUT%\logs\artix34_preflash"
  move "%LOG%" "%OUT%\logs\artix34_preflash" >nul
  echo [i] Dump lama sebelum flash artix34 dipindah -^> logs\artix34_preflash
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
echo [i] Yang dicari di cache.bin nanti:
echo       "VXTUXWRT"  @ 6MiB  -^> ZONA UJI: ada = kanal tulis eMMC HIDUP
echo       VXTUXFR1    @ 8MiB  -^> dmesg flightrec (kini diverifikasi baca-balik)
echo       VXTUXLOG1   @ 4MiB  -^> laporan init (kini diverifikasi baca-balik)
echo       pstore_blk  @ 0MiB  -^> catatan console/kmsg saat panic
echo       "VXTUX-WRITE-TEST:" -^> baris uji yang sama dari sisi layar
echo       "vxtux-fr: ... failed: -N" -^> errno kegagalan flightrec
echo [i] boot_a.bin dibandingkan dgn boot_artix34.img utk verifikasi flash:
echo       certutil -hashfile boot_a.bin MD5
echo       harus sama dgn md5 boot_artix34.img =
echo       2e918a88ca6e03ef623b540ea807d306  (44.126.208 byte)
echo [i] Foto layar sesudah reset (tunggu 40 detik):
echo     "VXTUX-WRITE-TEST: OK" = kunci utama tercapai
pause
