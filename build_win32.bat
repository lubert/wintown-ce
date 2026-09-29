@echo off
setlocal
set PATH=C:\MinGW\bin;%PATH%

echo Building WinTown for Win32 (Desktop Windows)...
mingw32-make.exe -f Makefile.mingw all
if errorlevel 1 goto error

echo.
echo ===================================================
echo  SUCCESS: wintown_win32.exe built successfully!
echo ===================================================
goto end

:error
echo.
echo ===================================================
echo  BUILD FAILED!
echo ===================================================
exit /b 1

:end
