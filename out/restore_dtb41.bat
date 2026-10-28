@echo off
setlocal
REM ===========================================================================
REM  RESTORE dtb_a  <-  logs\artix41\dtb_a_stock.bin
REM
REM  Dipakai kalau setelah menulis dtb_a tablet tidak mau boot.
REM  flash_artix41.bat / dump_flash_artix41.bat sudah membaca dtb_a lama ke
REM  logs\artix41\dtb_a_stock.bin SEBELUM menulis, jadi berkas itu adalah DTB
REM  stok asli dari tablet ini.
REM
REM  Kalau berkas backup tidak ada, script berhenti - JANGAN menulis dtb_a
REM  kosong/tebakan.
REM ===========================================================================

set SPD=G:\UMS512\unlock\advan\spd_dump_new.exe
set FDL1=G:\UMS512\unlock\advan\fdl1-dl.bin
set FDL2=G:\UMS512\unlock\advan\fdl2-dl.bin
set BAK=%~dp0logs\artix41\dtb_a_stock.bin
set PY=C:\Python314\python.exe

if not exist "%SPD%" ( echo [X] spd_dump tidak ada: %SPD% & pause & exit /b 1 )
if not exist "%BAK%" (
  echo [X] backup tidak ada: %BAK%
  echo     Tanpa backup, dtb_a TIDAK boleh ditulis.  Jalankan dulu
  echo     dump_artix41_logonly.bat pada tablet yang masih bisa masuk
  echo     Mode Download, lalu cek logs\artix41\dtb_a.bin.
  pause
  exit /b 1
)

echo ============================================================
echo  RESTORE dtb_a ^<- %BAK%
for %%A in ("%BAK%") do echo  ukuran : %%~zA byte
if exist "%PY%" (
  "%PY%" -c "import hashlib;p=r'%BAK%';print('  md5   :',hashlib.md5(open(p,'rb').read()).hexdigest())"
)
echo  Tablet harus di Mode Download.
echo ============================================================
pause

"%SPD%" --wait 300 ^
  fdl "%FDL1%" 0x5500 ^
  fdl "%FDL2%" 0x9efffe00 ^
  exec ^
  w dtb_a "%BAK%" ^
  reset

echo.
if errorlevel 1 (
  echo [!] gagal - cek baris "Write Part Done: dtb_a"
) else (
  echo [OK] dtb_a dikembalikan ke DTB stok.  Tablet akan memakai DT asli lagi.
)
pause
