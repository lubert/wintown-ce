@echo off
setlocal
call C:\Users\Lu\src\toolchain\setenv_mips.bat

set CFLAGS=-c -O2 -D_WIN32_WCE=211 -DUNDER_CE=211 -DWIN32 -DUNICODE -D_UNICODE -DMIPS -D_MIPS_ -DNEW32=1 -I. -IC:\Users\Lu\src\toolchain\wce211_mips\sdk\Emulation_Include_Files
set LFLAGS=/subsystem:windowsce,2.11 /machine:MIPS /entry:WinMainCRTStartup /nodefaultlib /libpath:C:\Users\Lu\src\toolchain\wce211_mips\sdk\Emulation_Libs\mips

echo Compiling WinTown C source files...
clmips.exe %CFLAGS% anim.c animtab.c assets.c budget.c cecompat.c charts.c disastr.c eval.c gdifix.c main.c mapgen.c newgame.c notify.c scenario.c sim.c sprite.c tiles.c tools.c
if errorlevel 1 goto error

echo Compiling Micropolis sim modules...
clmips.exe %CFLAGS% -Imicropolis\src\sim micropolis\src\sim\s_disast.c micropolis\src\sim\s_eval.c micropolis\src\sim\s_gen.c micropolis\src\sim\s_power.c micropolis\src\sim\s_scan.c micropolis\src\sim\s_traf.c micropolis\src\sim\s_zone.c
if errorlevel 1 goto error
clmips.exe %CFLAGS% -Imicropolis\src\sim /FIw_sim.h micropolis\src\sim\s_sim.c
if errorlevel 1 goto error

echo Compiling Resources...
rc.exe -r -d_WIN32_WCE=211 -dUNDER_CE=211 -IC:\Users\Lu\src\toolchain\wce211_mips\sdk\Emulation_Include_Files -I. wintown.rc
if errorlevel 1 goto error

echo Linking wintown_mips.exe...
link.exe %LFLAGS% /out:wintown_mips.exe anim.obj animtab.obj assets.obj budget.obj cecompat.obj charts.obj disastr.obj eval.obj gdifix.obj main.obj mapgen.obj newgame.obj notify.obj scenario.obj sim.obj sprite.obj tiles.obj tools.obj s_disast.obj s_eval.obj s_gen.obj s_power.obj s_scan.obj s_sim.obj s_traf.obj s_zone.obj wintown.res coredll.lib commctrl.lib corelibc.lib
if errorlevel 1 goto error

echo.
echo ===================================================
echo  SUCCESS: wintown_mips.exe built successfully!
echo ===================================================
goto end

:error
echo.
echo ===================================================
echo  BUILD FAILED!
echo ===================================================
exit /b 1

:end
