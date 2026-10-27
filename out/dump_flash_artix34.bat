@echo off
REM ===========================================================================
REM  artix34 -- KANAL TULIS TERINSTRUMENTASI (kenapa cache nol byte)
REM             Dump log artix33, flash boot_artix34.img, reset.
REM
REM  APA YANG BERUBAH (commit ec52f303c):
REM    1. VXTUX-WRITE-TEST DI LAYAR.  /vxtux/init kini menguji tulis-baca-
REM       balik SUNGGUHAN ke partisi cache (zona 6 MiB) SEBELUM hal lain:
REM         VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49
REM       atau FAIL open/write/readback lengkap dengan errno.  Keputusan
REM       apakah kanal tulis eMMC hidup atau mati kini TERLIHAT DI LAYAR,
REM       tanpa perlu dump 30 menit.
REM    2. FLIGHTREC BERSUARA.  vxtux_flightrec.c: setiap kegagalan berbeda
REM       (open by devt, open /dev path, write, readback) dicetak sekali ke
REM       dmesg dengan errno -> fbcon memutarnya kembali; ada fallback kedua
REM       lewat /dev/mmcblk0p49 (devtmpfs); setiap tulisan sukses diverifikasi
REM       baca-balik; pesan "write channel recovered" bila kanal pulih.
REM    3. LAPORAN INIT DIVERIFIKASI.  write_report_to() O_RDWR + baca-balik
REM       kepala "### VXTUXLOG1" sebelum menyatakan OK; daftar
REM       /proc/partitions ikut dicetak ke LAYAR.
REM    4. Semua teks Inggris (sisa "akhir dmesg"/"akhir /proc/partitions"
REM       diganti "end of ...").
REM
REM  YANG DIHARAPKAN DI LAYAR (tunggu 40 detik sesudah reset):
REM    - "VXTUX-STEP: 1/5 partition list" + daftar p1..p71  -> init jalan
REM    - "VXTUX-WRITE-TEST: OK ..."                          -> kanal tulis HIDUP
REM    - "VXTUX-WRITE-TEST: FAIL ...: No such device"        -> p49 tidak dibuat
REM    - "vxtux-fr: open ... failed: -19" (dsb)              -> sebab flightrec
REM    - "VXTUX-STEP: chroot OK, exec ..."                   -> rootfs Artix ke-mount
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix34
set IMG=%OUT%\6.18.52-VXTux-UMS512\boot_artix34.img

if not exist "%LOG%" mkdir "%LOG%"
if not exist "%IMG%" (
  echo [!] %IMG% belum ada -- pipeline ci_artix34_pipeline.sh belum selesai.
  exit /b 1
)

REM amankan dump lama (state artix33) sebelum tertimpa
if exist "%LOG%\cache.bin" (
  if exist "%OUT%\logs\artix34_preflash" rmdir /s /q "%OUT%\logs\artix34_preflash"
  move "%LOG%" "%OUT%\logs\artix34_preflash" >nul
  echo [i] Dump lama dipindah -^> logs\artix34_preflash
  mkdir "%LOG%"
)

echo [1/1] dump log artix33 + flash boot_artix34.img + reset
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
  w boot_a "%IMG%" ^
  reset

echo.
echo [i] Log tersimpan di %LOG%
echo [i] Setelah reset TUNGGU 40 DETIK lalu foto layarnya.
echo       "VXTUX-WRITE-TEST: OK ..."   = kanal tulis eMMC HIDUP (kunci!)
echo       "VXTUX-WRITE-TEST: FAIL ..." = ketahuan sebabnya langsung di layar
echo       "vxtux-fr: ... failed: -N"   = sebab flightrec (errno) di fbcon
echo       "VXTUX-STEP: chroot OK"      = rootfs Artix ke-mount
echo [i] Verifikasi flash: md5 boot_a.bin harus sama dgn:
echo       certutil -hashfile %IMG% MD5
echo       diharapkan = 2e918a88ca6e03ef623b540ea807d306 (44.126.208 byte)
pause
