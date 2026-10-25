@echo off
setlocal
REM ============================================================
REM  DUMP partisi `cache` (100 MiB) -- berisi laporan boot dari
REM  initramfs diagnostik (/vxtux/init): dmesg kernel lengkap,
REM  daftar partisi, hasil pindai ext4, hasil mount root.
REM
REM  Cari teks "VXTUXLOG1" di hasil dump.
REM  Partisi ini TIDAK dipakai Artix, jadi aman dibaca/ditimpa.
REM ============================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set OUT=%~dp0logs\artix5
set IMG=%OUT%\cache.bin

if not exist "%OUT%" mkdir "%OUT%"

echo ============================================================
echo  cache [100 MiB]  -^>  %IMG%
echo  Matikan tablet, tahan VOL DOWN + POWER 15 detik,
echo  lepas POWER (tetap tahan VOL DOWN), colok USB.
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec path "%OUT%" ^
  read_part cache 0 0x6400000 cache.bin ^
  read_part uboot_log 0 0x400000 uboot_log.bin ^
  reset

echo.
if exist "%IMG%" (
  echo [OK] %IMG%
  echo.
  echo --- baris kunci di dump ---
  findstr /C:"VXTUXLOG1" /C:"VXTUX-STEP" /C:"Linux version" /C:"VFS:" /C:"No working init" /C:"Kernel panic" /C:"mmcblk" "%IMG%"
) else (
  echo [!] dump gagal, cek log di atas
)
pause
