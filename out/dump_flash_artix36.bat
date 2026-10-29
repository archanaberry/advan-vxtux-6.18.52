@echo off
REM ===========================================================================
REM  artix36 -- dump log artix35 + flash boot_artix36.img + reset
REM
REM  APA YANG BERUBAH (artix36 = DIAGNOSA GANTUNG):
REM    Kernel artix35 terbukti (dump logs/artix35) menggantung SENYAP tepat
REM    setelah LK menyerahkan CPU ("[start_linux] PC:0x80080000"): tanpa
REM    panic, tanpa reboot, tanpa tulisan apa pun ke cache - semua kanal
REM    jejak ada di bawah titik gantung.  Diff artix34->artix35 cuma 2 baris
REM    config clock (=y), jadi pelakunya salah satu konsumen clock.
REM
REM    artix36 MENAMBAH penjejak initcall (CONFIG_VXTUX_INITTRACE=y):
REM    setiap initcall digambar NAMANYA ke panel LK (0x9e000000, RGB565
REM    1200x2000, font VGA8x16) SEBELUM dieksekusi.  Clock =y TETAP dipakai
REM    supaya hang tetap bereproduksi.  Banner muncul saat kernel mulai.
REM
REM  YANG DIHARAPKAN DI LAYAR (tunggu 40 detik sesudah reset):
REM    "VXTUX-ITRACE: kernel alive - watching initcalls, last line = hang"
REM         = tracer hidup, layar langsung hitam + teks putih
REM    "NNN <nama_initcall>" bertambah tiap baris
REM         = boot berjalan; KALAU BEKU: BARIS TERAKHIR = initcall macet
REM    banner tidak muncul sama sekali (layar tetap teks LK)
REM         = gantung SEBELUM early_param/initcall - foto saja, lalu dump
REM    layar dmesg fbcon biasa + "storage ready after" dst
REM         = lolos semua, lihat apakah eMMC akhirnya terdaftar
REM
REM  Yang diambil (dump pra-flash = state artix35, semua READ-ONLY):
REM    cache 100MB, uboot_log 4MB, sysdumpdb 10MB, misc 1MB, miscdata 1MB,
REM    boot_a 64MB (verifikasi prefix md5), lalu flash boot_a dan reset.
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix36
set IMG=%OUT%\6.18.52-VXTux-UMS512\boot_artix36.img

if not exist "%LOG%" mkdir "%LOG%"
if not exist "%IMG%" (
  echo [!] %IMG% belum ada -- pipeline ci_artix36_pipeline.sh belum selesai.
  pause
  exit /b 1
)

echo [1/1] dump log artix35 + flash boot_artix36.img + reset
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
echo [i] Setelah reset TUNGGU 40 DETIK lalu foto layarnya:
echo       "VXTUX-ITRACE: kernel alive" + baris "NNN ^<nama_initcall^>"
echo         = tracer hidup; KALAU BEKU baris TERAKHIR = initcall macet
echo       banner tidak muncul sama sekali = gantung sebelum initcalls
echo       "storage ready after" + "VXTUX-WRITE-TEST: OK" = lolos semua
echo [i] Verifikasi flash: md5 44.302.336 byte PERTAMA boot_a.bin harus =
echo       169a93d98e79a666f69d863c26cfcdb9 (auto-check di bat logonly)
pause
