@echo off
REM ===========================================================================
REM  artix35 -- dump log artix34 + flash boot_artix35.img + reset
REM
REM  APA YANG BERUBAH di artix35 (fix utama: eMMC tak pernah terdaftar):
REM    1. CONFIG_SPRD_UMS512_CLK=y (builtin, bukan =m).  Clock controller
REM       ap-clk + apapb-gate milik ums512-clk.c kini ikut di Image; sejak
REM       commit 80146f0f6/42b8db22f (20 Okt) konfigurasi =m membuat boot
REM       initramfs tanpa userspace tak pernah memuat modul -> host eMMC
REM       sdio@71400000 gagal devm_clk_get -> -EPROBE_DEFER selamanya ->
REM       disk mmcblk0 tak pernah terdaftar -> 28 dump cache.bin nol byte.
REM    2. /vxtux/init: wait_for_storage() menunggu mmcblk* maks 180 detik
REM       (layar update tiap 10 dtk + petunjuk dmesg sdio/mmc/-517), lalu
REM       fallback mknod dari /proc/partitions sebelum uji tulis/laporan.
REM    3. flightrec: kegagalan dicetak ulang tiap 10 s + nama disk terdaftar.
REM
REM  YANG DIHARAPKAN DI LAYAR (tunggu 40 detik sesudah reset):
REM    "VXTUX-STEP: storage ready after Ns: 71 mmc partitions"
REM         = clock fix bekerja, eMMC terdaftar (pertama kali sejak 20 Okt)
REM    "VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49"
REM         = kanal tulis eMMC hidup
REM    "VXTUX-STEP: root=... (mounted)" + "chroot OK, exec"
REM         = rootfs Artix ke-mount, boot lanjut ke userspace
REM
REM  Yang diambil (dump pra-flash = state artix34, semua READ-ONLY):
REM    cache 100MB, uboot_log 4MB, sysdumpdb 10MB, misc 1MB, miscdata 1MB,
REM    boot_a 64MB (verifikasi: md5 vs boot_artix35.img), lalu flash boot_a
REM    dan reset.
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix35
set IMG=%OUT%\6.18.52-VXTux-UMS512\boot_artix35.img

if not exist "%LOG%" mkdir "%LOG%"
if not exist "%IMG%" (
  echo [!] %IMG% belum ada -- pipeline ci_artix35_pipeline.sh belum selesai.
  pause
  exit /b 1
)

echo [1/1] dump log artix34 + flash boot_artix35.img + reset
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
echo       "storage ready after Ns: 71 mmc partitions" = eMMC terdaftar
echo       "VXTUX-WRITE-TEST: OK"                      = kanal tulis hidup
echo       "VXTUX-STEP: ... (mounted)" + "chroot OK"   = rootfs Artix masuk
echo       masih "waiting for eMMC" berulang           = foto dmesg hints-nya
echo [i] Verifikasi flash: md5 44.302.336 byte PERTAMA boot_a.bin harus =
echo       303c06b0a6a10e014a84035dd13708b9 (md5 utuh boot_a.bin 64MB BEDA,
echo       jangan dibandingkan langsung -- ada auto-check di bat logonly)
pause
