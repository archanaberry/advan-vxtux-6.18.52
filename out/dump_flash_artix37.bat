@echo off
REM ===========================================================================
REM  artix37 -- dump log artix37 + flash boot_artix37.img + reset
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
REM
REM  KANAL LOG (bedah kernel stok 5.4.210 -- lihat docs/RE-LOG-DUMP-5.4.210.md):
REM    "VXTUX-STEP: mounted cache and saved early dmesg to /cache/boot_log.txt"
REM         = partisi cache (ext4!) terpasang, dmesg awal tersimpan sebagai berkas
REM    "VXTUX-WRITE-TEST: OK file write+readback on /cache"
REM         = kanal berkas ext4 hidup (bukan tulisan mentah yang merusak ext4)
REM    "VXTUX-STEP: raw vendor log saved to /dev/mmcblk0pN"
REM         = kanal mentah vendor common_rs1_a (8 MiB, offset 0) ikut terisi
REM    "VXTUX-STEP: raw vendor log partition not found ... (skipped)"
REM         = ukuran+isi tidak cocok -> kanal dilewati (aman, tidak asal tulis)
REM    "VXTUX-STEP: chroot OK, exec /sbin/init" = masuk ke dinit Artix
REM    "/init -> /vxtux/init" ada di ramdisk  = cadangan kalau rdinit tidak dibaca
REM
REM  boot_artix37.img : 0x2B68000 byte (45514752)
REM  md5 45514752 byte pertama : 016343fa9ea2ec00f6a12feebadb7503
REM ===========================================================================
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\artix37
set IMG=%OUT%\6.18.52-VXTux-UMS512\boot_artix37.img

if not exist "%LOG%" mkdir "%LOG%"
if not exist "%IMG%" (
  echo [!] %IMG% belum ada -- pipeline ci_artix37_pipeline.sh belum selesai.
  pause
  exit /b 1
)

echo [1/1] dump log artix37 + flash boot_artix37.img + reset
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  read_part cache        0 0x6400000  cache.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part sysdumpdb    0 0xA00000   sysdumpdb.bin ^
  read_part misc         0 0x100000   misc.bin ^  read_part miscdata     0 0x100000  miscdata.bin ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  w boot_a "%IMG%" ^
  read_part common_rs1_a 0 0x800000  common_rs1_a.bin ^
  reset

echo.
echo [i] Log tersimpan di %LOG%
echo [i] Setelah reset TUNGGU 40 DETIK lalu foto layarnya:
echo       "VXTUX-MOD: ... loaded"   = clock tree terdaftar sesudah fbcon
echo       "waiting for eMMC" dst    = deferred probe dipicu ulang
echo       "storage ready after Ns"  = eMMC akhirnya terdaftar
echo       "saved early dmesg"       = berkas /cache/boot_log.txt terbentuk
echo       "raw vendor log saved"    = kanal mentah common_rs1_a terisi
echo       "chroot OK, exec /sbin/init" = masuk ke dinit Artix
echo [i] Verifikasi flash: md5 45514752 byte PERTAMA boot_a.bin harus =
echo       016343fa9ea2ec00f6a12feebadb7503 (auto-check di bat logonly)
pause
