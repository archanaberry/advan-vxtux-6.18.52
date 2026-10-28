@echo off
REM ===========================================================================
REM  artix37 -- HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah partisi)
REM
REM  APA YANG DIPERBAIKI (artix37 = kenapa 34 sampai fbcon, 35/36 tidak):
REM    artix34 : SPRD_COMMON_CLK + SPRD_UMS512_CLK =m dan modulnya TIDAK
REM              PERNAH dimuat -> seluruh konsumen clock (eMMC sdio, i2c,
REM              PMIC sc2730, ...) mendapat -EPROBE_DEFER selamanya -> tidak
REM              ada yang probe -> kernel santai jalan sampai device_initcall,
REM              vxtux_lkfb mendaftar, LAYAR HIDUP, init userspace mencetak.
REM    artix35/36 : clock diubah jadi =y -> provider clock terdaftar sejak
REM              initcall -> konsumen langsung probe untuk PERTAMA kali ->
REM              salah satunya menggantung senyap di bus yang belum siap,
REM              SEBELUM fbcon -> layar tetap teks LK, tanpa panic, tanpa
REM              tulisan ke cache (dump membuktikan cache byte-identik).
REM              sprd_clk_probe sendiri polos (hanya registrasi provider),
REM              jadi pelakunya konsumen, bukan driver clock-nya.
REM
REM    artix37 : clock kembali =m (jalur artix34 yang terbukti SELALU sampai
REM              fbcon), lalu clk-sprd.ko + ums512-clk.ko dimuat oleh
REM              /vxtux/init SETELAH fbcon hidup.  driver_register() memicu
REM              ulang deferred probe, jadi konsumen tetap dicoba - tapi
REM              sekarang sesudah layar bisa bicara, bukan dalam gelap.
REM              Tracer initcall artix36 dimatikan (find_font dari
REM              early_param selalu NULL: font baru terdaftar di
REM              device_initcall -> tracer tidak pernah menggambar apa pun).
REM
REM  YANG DIHARAPKAN (tunggu 40 detik sesudah reset):
REM    layar hitam + dmesg Inggris + "VXTUX-MOD: loading UMS512 clock..."
REM         = fbcon hidup dan modul sedang dimuat (ini yang artix35/36 tidak
REM           pernah capai)
REM    "VXTUX-MOD: /vxtux/ko/clk-sprd.ko loaded" lalu
REM    "VXTUX-MOD: /vxtux/ko/ums512-clk.ko loaded"
REM         = clock tree terdaftar, deferred probe dipicu ulang
REM    berhenti tepat sesudah salah satu baris VXTUX-MOD
REM         = konsumen clock menggantung SESUDAH layar hidup -> namanya
REM           tinggal dicari dari dmesg/cache, tidak buta lagi
REM    "VXTUX-MOD: ... FAILED: ..." = modul ditolak (vermagic/tanda tangan)
REM    "storage ready after Ns: 71 mmc partitions" = eMMC akhirnya terdaftar
REM    "VXTUX-WRITE-TEST: OK open+write+readback"  = kanal tulis hidup
REM
REM  boot_artix37.img : 0x2A5D000 byte (44421120)
REM  md5 44421120 byte pertama : a475dc714c598c4ba348b8ffb63aa533
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix37

REM dump_flash_artix37.bat menulis dump pra-flash ke logs\artix37 (state
REM artix36).  Aman-kan dulu supaya bukti "sebelum" tidak tertimpa.
if exist "%LOG%\cache.bin" (
  if exist "%OUT%\logs\artix37_preflash" rmdir /s /q "%OUT%\logs\artix37_preflash"
  move "%LOG%" "%OUT%\logs\artix37_preflash" >nul
  echo [i] Dump lama sebelum flash artix37 dipindah -^> logs\artix37_preflash
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
echo [i] Yang dicari:
echo       VXTUXFR1   @ 8MiB  -^> dmesg flightrec (isi dmesg s/d titik macet)
echo       VXTUXLOG1  @ 4MiB  -^> laporan init (apakah eMMC terdaftar / mount)
echo       VXTUXWRT   @ 6MiB  -^> zona uji tulis-balik eMMC
echo       pstore_blk @ 0MiB  -^> catatan console/kmsg saat panic
echo       "VXTUX-MOD" juga tercetak di cache lewat laporan init
echo [i] VERIFIKASI FLASH otomatis (44421120 byte PERTAMA boot_a.bin):
if not exist "%LOG%\boot_a.bin" goto :nosha
if exist C:\Python314\python.exe C:\Python314\python.exe -c "import hashlib;h=hashlib.md5(open(r'%LOG%\boot_a.bin','rb').read(44421120)).hexdigest();print('  boot_a prefix md5 =',h);print('  flash boot_artix37 mendarat =',h=='a475dc714c598c4ba348b8ffb63aa533')"
if not exist C:\Python314\python.exe echo       [!] C:\Python314\python.exe tidak ada -- hitung manual.
:nosha
echo [i] Bila prefix md5 BEDA a475dc71 -^> flash gagal; jalankan
echo       flash_artix37.bat lalu dump ulang.
pause
