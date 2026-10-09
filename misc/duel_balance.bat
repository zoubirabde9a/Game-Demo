@echo off
REM Builds the duel balance probe (code\tools\duel_balance.cpp) into this
REM worktree's build\ and runs it. Arguments and PROBE_* settings are the
REM probe's own; see the top of that file.
REM   misc\duel_balance.bat 19 2 64                    random builds, 64 seeds
REM   set PROBE_BUILDS=fire,guard& misc\duel_balance.bat 19 2 64
setlocal
call "%~dp0shell_64.bat" >nul
pushd "%~dp0..\build"
cl -nologo -O2 -DAPP_DEV=1 -DAPP_SLOW=0 -DAPP_WIN32=1 ..\code\tools\duel_balance.cpp /link user32.lib Gdi32.lib Winmm.lib OpenGL32.lib > duel_balance_build.log 2>&1
if errorlevel 1 (
    type duel_balance_build.log
    popd
    exit /b 1
)
"%~dp0..\build\duel_balance.exe" %*
set "Result=%errorlevel%"
popd
exit /b %Result%
