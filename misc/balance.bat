@echo off
REM Builds the dungeon balance probe (code\tools\dungeon_balance.cpp, about
REM three seconds) into this worktree's build\ and runs it. Arguments and
REM PROBE_* settings are the probe's own; see the top of that file.
REM   misc\balance.bat 20 3 7 32                 the crypt's last boss, 32 seeds
REM   set PROBE_MAP=vault& misc\balance.bat 20 3 7 32     the vault's last boss
REM   set PROBE_LEVELS=3& misc\balance.bat 200 3 2 24     crypt, depths and vault
REM Seeds run side by side, one process each, so 24 full runs take seconds.
setlocal
call "%~dp0shell_64.bat" >nul
pushd "%~dp0..\build"
cl -nologo -O2 -DAPP_DEV=1 -DAPP_SLOW=0 -DAPP_WIN32=1 ..\code\tools\dungeon_balance.cpp /link user32.lib Gdi32.lib Winmm.lib OpenGL32.lib > balance_build.log 2>&1
if errorlevel 1 (
    type balance_build.log
    popd
    exit /b 1
)
"%~dp0..\build\dungeon_balance.exe" %*
set "Result=%errorlevel%"
popd
exit /b %Result%
