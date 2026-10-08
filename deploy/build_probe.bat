@echo off
REM Builds only the join probe (build\probe.exe) of the source tree this
REM file sits in. deploy/deploy.sh runs it on the tree it packages, so the
REM check it makes over the internet speaks the same protocol as the
REM server it just installed; a probe left over from an older build reads
REM a protocol change as a different game build.
call "%~dp0..\misc\shell_64.bat" || exit /b 1
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
pushd "%~dp0..\build"
cl -nologo -O2 -EHsc- -GR- -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=0 -DAPP_DEV=0 ..\code\server\probe_main.cpp -Feprobe.exe /link -incremental:no > probe_build.log 2>&1
set Result=%errorlevel%
if %Result% neq 0 type probe_build.log
popd
exit /b %Result%
