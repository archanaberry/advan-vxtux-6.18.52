@echo off
REM ===========================================================================
REM  artix35 -- HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah partisi apa pun)
REM
REM  Dipakai SESUDAH boot boot_artix35.img (fix clock UMS512 builtin):
REM    * "storage ready after Ns: 71 mmc partitions" = eMMC terdaftar.
REM    * VXTUX-WRITE-TEST di layar: uji tulis-baca-balik cache (zona 6 MiB).
REM    * flightrec: errno dicetak ulang tiap 10 s + daftar disk terdaftar.
REM    * laporan init diverifikasi baca-balik (VXTUXLOG1 @ 4 MiB).
REM
REM  Yang diambil (semuanya READ-ONLY, partisi tidak disentuh):
REM    cache      100MB  -> kunci utama:
REM                         offset 0x0        : zona pstore_blk (console/kmsg)
REM                         offset 0x400000   : laporan init (VXTUXLOG1)
REM                         offset 0x600000   : zona uji "VXTUXWRT" 16 byte
REM                                              (ada = kanal tulis HIDUP)
REM                         offset 0x800000   : flightrec (VXTUXFR1, dmesg)
REM    uboot_log    4MB  -> log LK boot ini
REM    sysdumpdb   10MB  -> minidump vendor
REM    misc         1MB  -> BCB
REM    miscdata     1MB  -> data misc
REM    boot_a      64MB  -> VERIFIKASI flash: md5 44.302.336 byte PERTAMA
REM                         boot_a.bin harus = 303c06b0a6a10e014a84035dd13708b9
REM                         (md5 utuh 64MB TIDAK bisa dibandingkan langsung)
REM    reset             -> tablet reboot ke kernel yang sedang ter-flash
REM
REM  STATUS 28/10 07:30 -- tablet STUCK di layar LK "WARNING LOCK FLAG IS
REM  UNLOCK" (layar tidak pernah digantikan log kernel).  Dump ini
REM  menyelidiki 3 hal (semuanya READ-ONLY):
REM    (1) boot_a prefix md5 -> flash boot_artix35 mendarat atau tidak
REM    (2) uboot_log.bin     -> sampai di mana LK sebelum macet
REM    (3) cache.bin         -> apakah kernel sempat menulis apa pun
REM
REM  PAKAI (dari kondisi macet): tahan POWER 15 detik sampai tablet mati
REM  total, lalu tahan VOL- sambil colok USB (BROM), jalankan script ini.
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix35

REM dump_flash_artix35.bat menulis dump pra-flash ke logs\artix35 (state
REM artix34).  Aman-kan dulu supaya bukti "sebelum" tidak tertimpa; hasil
REM baru (sesudah boot artix35) masuk ke logs\artix35.
if exist "%LOG%\cache.bin" (
  if exist "%OUT%\logs\artix35_preflash" rmdir /s /q "%OUT%\logs\artix35_preflash"
  move "%LOG%" "%OUT%\logs\artix35_preflash" >nul
  echo [i] Dump lama sebelum flash artix35 dipindah -^> logs\artix35_preflash
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
echo       VXTUXFR1    @ 8MiB  -^> dmesg flightrec (baca-balik diverifikasi)
echo       VXTUXLOG1   @ 4MiB  -^> laporan init (baca-balik diverifikasi)
echo       pstore_blk  @ 0MiB  -^> catatan console/kmsg saat panic
echo       "storage ready after" -^> bukti eMMC terdaftar di boot ini
echo [i] VERIFIKASI FLASH otomatis (44.302.336 byte PERTAMA boot_a.bin):
if not exist "%LOG%\boot_a.bin" goto :nosha
if exist C:\Python314\python.exe C:\Python314\python.exe -c "import hashlib;h=hashlib.md5(open(r'%LOG%\boot_a.bin','rb').read(44302336)).hexdigest();print('  boot_a prefix md5 =',h);print('  flash boot_artix35 mendarat =',h=='303c06b0a6a10e014a84035dd13708b9')"
if not exist C:\Python314\python.exe echo       [!] C:\Python314\python.exe tidak ada -- hitung manual.
:nosha
echo [i] CARA BACA HASIL DUMP:
echo       prefix md5 SAMA 303c06b0... -> flash OK; yg macet isi kernelnya
echo       prefix md5 BEDA             -> flash gagal/parsial; jalankan
echo                                       flash_artix35.bat lalu dump ulang
echo       uboot_log.bin (strings)     -> baris TERAKHIR LK sebelum macet:
echo         ada "kernel read OK ... locate to 0x80080000" = LK sudah lompat,
echo                                  berarti kernel yg hang sebelum fbcon
echo         tidak ada baris itu      = LK gagal load image (masih di LK)
echo       cache.bin semua zona nol    -> wajar bila eMMC belum terdaftar
echo [i] Setelah reset layar dikunci di teks LK lagi = normal; yang penting
echo       file dump di atas, bukan layarnya -- kirim hasilnya ke chat.
pause
