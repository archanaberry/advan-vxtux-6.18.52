@echo off
REM ===========================================================================
REM  artix35 -- FIX AKAR MASALAH eMMC: clock UMS512 builtin + init menunggu
REM             Dump log artix34, flash boot_artix35.img, reset.
REM
REM  APA YANG BERUBAH (commit 9d0f0848b):
REM    1. CONFIG_SPRD_COMMON_CLK=y dan CONFIG_SPRD_UMS512_CLK=y (dulu =m --
REM       modul tidak pernah dimuat di boot initramfs -> clock "sdio" dan
REM       "enable" host eMMC dari apapb-gate/ap-clk tidak pernah ada ->
REM       sdhci_sprd_probe() selalu -EPROBE_DEFER -> disk TIDAK PERNAH
REM       terdaftar -> inilah sebab 28 dump cache nol byte sejak 20 Okt).
REM    2. /vxtux/init: wait_for_storage() -- menunggu mmcblk* muncul maks
REM       180 detik, layar diperbarui tiap 10 detik (jumlah partisi +
REM       petunjuk dmesg sdio/mmc/-517/defer), lalu fallback mknod dari
REM       major/minor sebelum uji tulis/laporan/pindai ext4.
REM    3. flightrec: kegagalan sama dicetak ulang tiap 10 detik; bila disk
REM       mmcblk0 tak terdaftar, nama disk yang ada ikut dicetak.
REM
REM  YANG DIHARAPKAN DI LAYAR (detik 30 dst.):
REM    - "VXTUX-STEP: storage ready after Ns: 71 mmc partitions" = clock fix
REM      BEKERJA, eMMC terdaftar (bila muncul "waiting for eMMC" berulang +
REM      "disk mmcblk0 not registered" berarti fix belum cukup -- catat
REM      dmesg hints yang ikut tercetak).
REM    - "VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49"
REM    - "VXTUX-STEP: root=... (mounted)" lalu "chroot OK, exec" = Artix masuk.
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
echo       "storage ready after Ns: 71 mmc" = clock fix BEKERJA (eMMC hidup)
echo       "VXTUX-WRITE-TEST: OK open+write+readback" = kanal tulis eMMC HIDUP
echo       "VXTUX-STEP: root=... (mounted)" + "chroot OK, exec" = Artix masuk
echo       "waiting for eMMC" berulang + dmesg hints = clock belum cukup,
echo         catat baris hints-nya (sdio/mmc/-517/defer) utk analisa berikut
echo [i] Verifikasi flash: md5 boot_a.bin harus sama dgn:
echo       certutil -hashfile %IMG% MD5
echo       harus = 303c06b0a6a10e014a84035dd13708b9  (44.302.336 byte)
pause
