#!/usr/bin/env python3
"""Generate the artix37 flash/dump .bat files with the real md5 + size.

Run AFTER ci_artix37_pipeline.sh produced boot_artix37.img:

    /c/Python*/python.exe out/gen_artix37_bats.py

Why a generator: the .bat files must quote the exact md5 of the image that
was actually built (and the exact byte count used for the prefix check), so
copy-pasting numbers by hand is how flash verification silently goes stale.
"""
import hashlib
import os

OUT = os.path.dirname(os.path.abspath(__file__))
IMG = os.path.join(OUT, "6.18.52-VXTux-UMS512", "boot_artix37.img")

# boot_a is 64 MiB; only the image's own bytes are compared.
PREFIX = 0x4000000


def md5_prefix(path, n):
    """Return (md5, bytes hashed) over the first n bytes of the file.

    If the file is shorter than n, everything is hashed; `bytes hashed` is
    counted from the chunks actually read (the original version derived it
    from getsize() minus unread bytes, which reported garbage once the file
    ran out - it claimed 21733376 for a 44421120-byte image).
    """
    h = hashlib.md5()
    left = n
    hashed = 0
    with open(path, "rb") as f:
        while left > 0:
            b = f.read(min(1 << 20, left))
            if not b:
                break
            h.update(b)
            left -= len(b)
            hashed += len(b)
    return h.hexdigest(), hashed


md5, hashed = md5_prefix(IMG, PREFIX)
size = os.path.getsize(IMG)
# boot_a is 64 MiB; when the image is smaller the whole file is the prefix,
# so the check compares exactly what the flash writes.
check_n = min(PREFIX, size)
assert hashed == size, (
    "md5 menghasilkan %d byte, file %d byte" % (hashed, size))
print("boot_artix37.img: %d byte (0x%X)" % (size, size))
print("  md5 penuh        : %s" % md5)
print("  md5 %d byte awal : %s" % (check_n, md5))
assert size < PREFIX, "image lebih besar dari partisi boot_a (64 MiB)"

HDR = """@echo off
REM ===========================================================================
REM  artix37 -- {title}
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
REM  boot_artix37.img : 0x{size:X} byte ({size})
REM  md5 {prefix_n} byte pertama : {md5}
REM ===========================================================================
"""


def dump_flash():
    return HDR.format(title="dump log artix37 + flash boot_artix37.img + reset",
                      size=size, prefix_n=check_n, md5=md5) + r"""set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
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
echo [i] Verifikasi flash: md5 __PREFIX__ byte PERTAMA boot_a.bin harus =
echo       __MD5__ (auto-check di bat logonly)
pause
""".replace("__PREFIX__", str(check_n)).replace("__MD5__", md5)


def logonly():
    return HDR.format(title="HANYA DUMP SEMUA LOG (tanpa flash, tanpa ubah partisi)",
                      size=size, prefix_n=check_n, md5=md5) + r"""set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
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
  read_part misc         0 0x100000   misc.bin ^  read_part miscdata     0 0x100000  miscdata.bin ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  read_part common_rs1_a 0 0x800000  common_rs1_a.bin ^
  reset

echo.
echo [i] Log tersimpan di %LOG%
echo [i] Yang dicari (semua lewat spd_dump read_part):
echo       cache.bin      -> mount -t ext4 /dev/mmcblkX  lalu lihat:
echo           /cache/boot_log.txt     dmesg awal SEBELUM pindai super
echo           /cache/vxtux_report.txt laporan penuh init
echo       common_rs1_a   -> teks "### VXTUXRS1" di offset 0 (128 KiB pertama)
echo                         = kanal mentah vendor, tetap terbaca walau ext4 gagal
echo       catatan: pstore_blk + flightrec DIMATIKAN (keduanya menulis mentah
echo               ke partisi ext4 -> merusak metadata filesystem cache)
echo [i] VERIFIKASI FLASH otomatis (__PREFIX__ byte PERTAMA boot_a.bin):
if not exist "%LOG%\boot_a.bin" goto :nosha
if exist C:\Python314\python.exe C:\Python314\python.exe -c "import hashlib;h=hashlib.md5(open(r'%LOG%\boot_a.bin','rb').read(__PREFIX__)).hexdigest();print('  boot_a prefix md5 =',h);print('  flash boot_artix37 mendarat =',h=='__MD5__')"
if not exist C:\Python314\python.exe echo       [!] C:\Python314\python.exe tidak ada -- hitung manual.
:nosha
echo [i] Bila prefix md5 BEDA __MD5SHORT__ -^> flash gagal; jalankan
echo       flash_artix37.bat lalu dump ulang.
pause
""".replace("__PREFIX__", str(check_n)).replace("__MD5SHORT__", md5[:8]).replace("__MD5__", md5)


def flash_only():
    return r"""@echo off
setlocal
REM ============================================================
REM  FLASH HANYA boot_a  <-  boot_artix37.img
REM  (clock UMS512 =m lagi; modulnya dimuat /vxtux/init sesudah fbcon)
REM  Tidak menyentuh super/Artix/partisi lain.
REM
REM  artix37 = fix kenapa artix34 sampai fbcon sementara 35/36 hang:
REM  clock tree kembali =m (jalur 34 yang terbukti), lalu clk-sprd.ko +
REM  ums512-clk.ko dimuat dari initramfs SETELAH layar hidup, sehingga
REM  deferred probe tetap dipicu tapi tidak lagi dalam gelap.
REM
REM  Cara pakai:
REM   1. Matikan tablet, tahan VOL DOWN + POWER 15 detik,
REM      lepas POWER (VOL DOWN tetap ditekan), colok USB.
REM   2. Jalankan script ini, tekan apa pun saat pause.
REM   3. Harus terbaca: file size : 0x__SIZE__  lalu  Write Part Done: boot_a
REM   4. Setelah reset: JANGAN tahan tombol apa pun, biarkan boot normal.
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BOOT=%~dp06.18.52-VXTux-UMS512\boot_artix37.img

if not exist "%SPD%"  ( echo [X] spd_dump tidak ada: %SPD%  & pause & exit /b 1 )
if not exist "%BOOT%" ( echo [X] boot image tidak ada: %BOOT% & pause & exit /b 1 )

echo ============================================================
echo  boot_a  ^<-  %BOOT%
for %%A in ("%BOOT%") do echo  ukuran : %%~zA byte  (harus __SIZE__)
echo  md5 diharapkan : __MD5__
echo.
echo  Matikan tablet: tahan VOL DOWN + POWER 15 detik,
echo  lepas POWER (tetap tahan VOL DOWN), colok USB, tunggu 5 detik.
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w boot_a "%BOOT%" ^
  reset

echo.
if errorlevel 1 (
  echo [!] ada error -- cek baris "Write Part Done: boot_a" di atas
) else (
  echo [OK] selesai -- JANGAN tahan tombol apa pun saat tablet reboot,
  echo      biarkan boot normal +/- 40 detik, lalu foto layarnya.
  echo      "VXTUX-MOD: ... loaded" lalu "waiting for eMMC" = fix jalan.
  echo      "chroot OK, exec /sbin/init" = initramfs sudah menyerahkannya
  echo      ke dinit Artix (getty@tty1 -> CLI fbcon di layar).
)
pause
""".replace("__SIZE__", str(size)).replace("__MD5__", md5)


for name, body in (("dump_flash_artix37.bat", dump_flash()),
                   ("dump_artix37_logonly.bat", logonly()),
                   ("flash_artix37.bat", flash_only())):
    path = os.path.join(OUT, name)
    with open(path, "w", newline="\r\n") as f:
        f.write(body)
    print("dibuat: %s (%d baris)" % (name, body.count("\n") + 1))
