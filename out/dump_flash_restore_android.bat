@echo off
REM ===========================================================================
REM  RESTORE ANDROID: dump state sekarang dulu, lalu flash super ORI Advan +
REM  boot ORI yang sudah di-patch Magisk, lalu reset.
REM
REM  ---------------------------------------------------------------------
REM  FILE YANG DIPAKAI (semuanya sudah dicek isinya hari ini):
REM    super  = E:\backupan-ori-advantabvxlite_t1.1\backup\super.bin
REM             5.872.025.600 byte (5600 MiB, pas dengan partisi super)
REM             md5 1f6e248a52f621d0ff5037590f230d61
REM             magic LPT "gDla" ada di offset 0x1000  -> super Android ASLI
REM    boot   = E:\spd_dump_logs\boot_a_dump.bin
REM             67.108.864 byte, md5 0a17b97edb355fd9e3d8e44662848a43
REM             magic "ANDROID!" header v4
REM             kernel IDENTIK dengan boot_a stock (38.085.120 byte),
REM             ramdisk beda (1.469.832 -> 1.297.306) + string "magisk"
REM             = boot stock device INI sendiri yang sudah Magisk patch
REM
REM  APA YANG DUMP (sebelum ada yang ditimpa, semuanya READ-ONLY):
REM    boot_a  64MB   -> boot_artix37 yang sekarang ter-flash (jaga-jaga)
REM    super   5600MB -> state super sekarang (set DUMP_SUPER=0 untuk
REM                       memotong +/- 10 menit; super tidak pernah kita
REM                       ubah, salinan ori-nya sudah ada di backupan-ori)
REM    uboot_log 4MB, misc 1MB -> jejak boot terakhir
REM
REM  APA YANG DITULIS:
REM    w super  <- super ori Android
REM    w boot_a <- boot ori + patch Magisk
REM    TIDAK disentuh: vbmeta_a/b, dtb, dtbo, vendor_boot, userdata, slot b.
REM
REM  PERKIRAAN WAKTU: dump boot+log +/- 1 menit; dump super +/- 5 menit;
REM  tulis super +/- 5 menit; tulis boot +/- 10 detik.
REM
REM  PAKAI: matikan tablet, tahan VOL- sambil colok USB (BROM), jalankan ini,
REM  lalu tekan tombol saat pause supaya spd_dump mulai menunggu koneksi.
REM  Tablet yang terkunci di teks LK pun tetap masuk BROM kalau pakai
REM  kombinasi VOL- + POWER 15 detik lalu colok USB.
REM
REM  CATATAN PENTING:
REM    * semua cek pakai goto, BUKAN blok if (... ) -- tanda kurung di
REM      dalam echo di dalam blok if menutup bloknya lebih awal, lalu
REM      perintah setelahnya bocor dan dieksekusi tanpa syarat (bug yang
REM      bikin script ini dulu cuma nampilin banner + pause).
REM    * dump super pake size hex 0x15E000000 (64-bit).  Kalau hasilnya
REM      berakhir KURANG dari 5.872.025.600 byte, berarti spd_dump memotong
REM      argumen size-nya.  Itu TIDAK berpengaruh ke flash (perintah w
REM      memakai ukuran FILE, bukan argumen) - cukup set DUMP_SUPER=0 dan
REM      kerjakan dump super terpisah nanti.  Read gagal tidak merusak apa
REM      pun.
REM    * bat lama flash_rollback_stock.bat melakukan hal yang sama tapi
REM      tanpa dump dan tanpa verifikasi; script ini menambah keduanya.
REM ===========================================================================
setlocal
set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=E:\vxtux_build_repo\out
set LOG=%OUT%\logs\android_pre
set SUPER=E:\backupan-ori-advantabvxlite_t1.1\backup\super.bin
set BOOT=E:\spd_dump_logs\boot_a_dump.bin
REM  ubah ke 0 kalau tidak mau nunggu dump super 5600MB:
set DUMP_SUPER=1

set SUPER_SIZE=5872025600
set SUPER_MD5=1f6e248a52f621d0ff5037590f230d61
set BOOT_SIZE=67108864
set BOOT_MD5=0a17b97edb355fd9e3d8e44662848a43
set ARTIX37_MD5=a475dc714c598c4ba348b8ffb63aa533

goto :preflight

:badsz_super
echo [X] ukuran super SALAH: %_SZ%  harus %SUPER_SIZE% = 5600 MiB
pause & exit /b 1

:badsz_boot
echo [X] ukuran boot SALAH: %_SZ%  harus %BOOT_SIZE%
pause & exit /b 1

:preflight

echo ============================================================
echo  PRE-FLIGHT: cek file sebelum menyentuh tablet
echo ============================================================
if not exist "%SPD%"    ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%FDL1%"   ( echo [X] FDL1 tidak ada & pause & exit /b 1 )
if not exist "%FDL2%"   ( echo [X] FDL2 tidak ada & pause & exit /b 1 )
if not exist "%SUPER%"  ( echo [X] super ori tidak ada: %SUPER% & pause & exit /b 1 )
if not exist "%BOOT%"   ( echo [X] boot magisk tidak ada: %BOOT% & pause & exit /b 1 )

for %%A in ("%SUPER%") do set _SZ=%%~zA
if not "%_SZ%"=="%SUPER_SIZE%" goto :badsz_super
for %%A in ("%BOOT%") do set _SZ=%%~zA
if not "%_SZ%"=="%BOOT_SIZE%" goto :badsz_boot
echo [OK] ukuran super = %SUPER_SIZE% , boot = %BOOT_SIZE%

if not exist C:\Python314\python.exe goto :nopy1
echo [i] hitung md5 (super 5.87GB +/- 30 detik) ...
C:\Python314\python.exe -c "import hashlib;M=lambda p:(lambda f,h:([h.update(x) for x in iter(lambda:f.read(4194304),b'')],h.hexdigest())[1])(open(p,'rb'),hashlib.md5());s=M(r'%SUPER%');b=M(r'%BOOT%');print('  super md5  =',s);print('  super OK   =',s=='%SUPER_MD5%');print('  boot  md5  =',b);print('  boot  OK   =',b=='%BOOT_MD5%');print('  boot magic =',open(r'%BOOT%','rb').read(8));raise SystemExit(0 if (s=='%SUPER_MD5%' and b=='%BOOT_MD5%') else 1)"
if errorlevel 1 goto :md5fail
goto :done1
:nopy1
echo [i] python tidak ada - lewati cek md5, pakai ukuran saja.
goto :done1
:md5fail
echo [X] md5 super/boot TIDAK COCOK dengan nilai yang diharapkan.
echo     Jangan lanjut dulu - pastikan filenya benar (lihat header bat ini).
pause & exit /b 1
:done1
echo.
echo  Log dump  : %LOG%
echo  DUMP_SUPER= %DUMP_SUPER%   (0 = skip dump super)
echo ============================================================
echo  Hubungkan tablet BROM sekarang (VOL- + colok USB), lalu tekan
echo  tombol apa pun supaya spd_dump mulai menunggu 300 detik.
echo ============================================================
pause

if not exist "%LOG%" mkdir "%LOG%"

if "%DUMP_SUPER%"=="1" goto :with_super

echo [1/2] dump boot_a + log, lalu flash super + boot magisk + reset
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  size_part super ^
  size_part boot_a ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part misc         0 0x100000   misc.bin ^
  w super "%SUPER%" ^
  w boot_a "%BOOT%" ^
  reset
goto :after

:with_super
echo [1/2] dump boot_a + super + log, lalu flash super + boot magisk + reset
"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  path "%LOG%" ^
  size_part super ^
  size_part boot_a ^
  read_part boot_a       0 0x4000000  boot_a.bin ^
  read_part super        0 0x15E000000 super.bin ^
  read_part uboot_log    0 0x400000   uboot_log.bin ^
  read_part misc         0 0x100000   misc.bin ^
  w super "%SUPER%" ^
  w boot_a "%BOOT%" ^
  reset

:after
echo.
echo [2/2] verifikasi dump sebelum-restore
if not exist "%LOG%\boot_a.bin" goto :nodump
if not exist C:\Python314\python.exe goto :nopy2
C:\Python314\python.exe -c "import hashlib,os;M=lambda p:(lambda f,h:([h.update(x) for x in iter(lambda:f.read(4194304),b'')],h.hexdigest())[1])(open(p,'rb'),hashlib.md5());d=r'%LOG%';h=M(d+r'\boot_a.bin');print('  boot_a sebelum-restore md5 =',h);print('  = boot_artix37 (artix37 ter-flash) :',h=='%ARTIX37_MD5%');s=d+r'\super.bin';print('  super terdump                 :',os.path.exists(s),(str(os.path.getsize(s))+' byte') if os.path.exists(s) else '')"
goto :done2
:nopy2
echo [i] python tidak ada - cek manual: certutil -hashfile "%LOG%\boot_a.bin" MD5
:done2
:nodump
echo.
echo [i] Log + backup di %LOG%
echo [i] Syarat sukses di log spd_dump:
echo       "Write Part Done: super"  dan  "Write Part Done: boot_a"
echo     file size super = 0x15E000000 , boot = 0x4000000
echo [i] Setelah reset BIARKAN boot normal (jangan tahan tombol apa pun):
echo       animasi Advan masuk = Android hidup lagi
echo       Magisk = cek aplikasi Magisk setelah masuk Android
echo [i] Verifikasi ulang sesudah boot: jalankan
echo       dump_restore_android_logonly.bat
echo     lalu md5 boot_a.bin harus = %BOOT_MD5%
pause
