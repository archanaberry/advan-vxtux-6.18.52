@echo off
REM ===========================================================================
REM  PERBAIKI FILE RESTORE
REM
REM  Kenapa ada: dua script restore kadang terdeteksi 0 byte (tertimpa dari
REM  luar, di luar isi scriptnya - tercatat 2x, tepat sesudah edit terakhir).
REM  Kalau kamu jalankan dump_flash_restore_android.bat lalu keluar TANPA
REM  output sama sekali, file-nya kemungkinan 0 byte lagi.
REM
REM  Jalankan ini dulu, lalu jalankan script yang dimaksud:
REM
REM     E:\vxtux_build_repo\out\perbaiki_restore_bats.bat
REM     E:\vxtux_build_repo\out\dump_flash_restore_android.bat
REM ===========================================================================
setlocal
set SRC=E:\vxtux_build_repo\out\_backup_restore_bats
set DST=E:\vxtux_build_repo\out

if not exist "%SRC%\dump_flash_restore_android.bat" goto:nobak

echo [i] cek + pulihkan dari %SRC%
for %%F in (dump_flash_restore_android.bat dump_restore_android_logonly.bat) do call :fix %%F
goto :selesai

:nobak
echo [X] backup tidak ada di %SRC%
echo     restore manual: git -C E:\vxtux_build_repo checkout -- out/dump_flash_restore_android.bat out/dump_restore_android_logonly.bat
pause
exit /b 1

:selesai
echo.
echo [i] isi file sesudah perbaikan:
for %%F in (dump_flash_restore_android.bat dump_restore_android_logonly.bat) do call :size %%F
echo.
echo [i] Kalau masih ada yang 0 byte, restore manual:
echo       git -C E:\vxtux_build_repo checkout -- out/dump_flash_restore_android.bat out/dump_restore_android_logonly.bat
echo.
pause
exit /b 0

:fix
set NAME=%1
set _now=0
set _bak=0
if exist "%DST%\%NAME%" for %%A in ("%DST%\%NAME%") do set _now=%%~zA
if exist "%SRC%\%NAME%" for %%A in ("%SRC%\%NAME%") do set _bak=%%~zA
echo    %NAME% : sekarang=%_now%  backup=%_bak%
if "%_bak%"=="0" exit /b 1
if "%_now%"=="%_bak%" exit /b 0
echo [i] memulihkan %NAME% ...
attrib -r "%DST%\%NAME%" >nul 2>&1
copy /y "%SRC%\%NAME%" "%DST%\%NAME%" >nul
attrib +r "%DST%\%NAME%" >nul 2>&1
if exist "%DST%\%NAME%" for %%A in ("%DST%\%NAME%") do echo       -> jadi %%~zA byte
exit /b 0

:size
set NAME=%1
set _now=0
if exist "%DST%\%NAME%" for %%A in ("%DST%\%NAME%") do set _now=%%~zA
echo    %NAME% = %_now% byte
exit /b 0
