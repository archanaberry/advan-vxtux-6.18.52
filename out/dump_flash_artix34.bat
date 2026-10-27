@echo off
REM ===========================================================================
REM  artix34 -- KANAL TULIS TERINSTRUMENTASI + SEMUA TEKS LAYAR BAHASA INGGRIS
REM             Dump log artix33, flash boot_artix34.img, reset.
REM
REM  APA YANG BERUBAH (ci_artix34):
REM    1. VXTUX-WRITE-TEST: /vxtux/init menulis tanda tangan 16 byte
REM       "VXTUXWRT" di zona 6 MiB partisi cache lalu membaca-balik.
REM       Hasil dicetak ke console (fbcon) DAN ke laporan:
REM         VXTUX-WRITE-TEST: OK open+write+readback on /dev/mmcblk0p49
REM         VXTUX-WRITE-TEST: FAIL <open|write|readback> ...: <errno>
REM    2. flightrec (vxtux_flightrec.c) kini mencetak errno LENGKAP untuk
REM       setiap kegagalan yang berbeda (open by devt, open /dev path,
REM       write, readback) -- sebab kegagalan terlihat langsung di fbcon.
REM       Dua strategi buka: dev_t gendisk, fallback /dev/mmcblk0p49.
REM       Setiap tulisan diverifikasi baca-balik 8 byte magic VXTUXFR1.
REM    3. write_report_to() O_RDWR + baca-balik kepala "### VXTUXLOG1"
REM       sebelum menyatakan OK; daftar /proc/partitions tampil di layar.
REM    4. Sisa teks Indonesia di initramfs -> Inggris penuh.
REM    5. Self-test pita & beacon kotak putih tetap MATI TOTAL (artix33).
REM
REM  YANG DIHARAPKAN DI LAYAR:
REM    - Layar hitam bersih + riwayat dmesg kernel (Inggris).
REM    - "VXTUX-WRITE-TEST: OK open+write+readback" = kanal tulis eMMC HIDUP.
REM    - "vxtux-fr: ... failed: -N" = errno persis kegagalan flightrec.
REM    - "VXTUX-STEP: chroot OK, exec ..." = rootfs Artix ke-mount.
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
  echo [!] %IMG% belum ada -- pipeline ci_artix34 belum selesai.
  exit /b 1
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
echo       "VXTUX-WRITE-TEST: OK open+write+readback" = kanal tulis eMMC HIDUP
echo       layar hitam + teks kernel Inggris  = fbcon bersih, target tercapai
echo       tidak ada kotak putih              = beacon mati
echo       tidak ada pita warna               = self-test mati
echo       "VXTUX-STEP: chroot OK, exec"       = rootfs Artix ke-mount
echo       "vxtux-fr: ... failed: -N"          = errno kegagalan flightrec
echo [i] Verifikasi flash: md5 boot_a.bin harus sama dgn:
echo       certutil -hashfile %IMG% MD5
echo       harus = 2e918a88ca6e03ef623b540ea807d306  (44.126.208 byte)
pause
