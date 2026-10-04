@echo off
REM Builds the dedicated server and its health-check probe.
REM   build\server.exe [port]          runs the server (default port 27015)
REM   build\probe.exe [address:port] [content-id]   exits 0 if a server there lets a player in
REM   build\bots.exe address:port content-id [count] [seconds]   load test with bot players
set ServerFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 /FC /Z7
call "%~dp0misc\shell_64.bat" || exit /b 1
if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"
set Result=0
cl %ServerFlags% ..\code\server\server_main.cpp -Feserver.exe /link -incremental:no Winmm.lib
if %errorlevel% neq 0 set Result=1
cl %ServerFlags% ..\code\server\probe_main.cpp -Feprobe.exe /link -incremental:no
if %errorlevel% neq 0 set Result=1
cl %ServerFlags% ..\code\tools\bots_main.cpp -Febots.exe /link -incremental:no
if %errorlevel% neq 0 set Result=1
popd
exit /b %Result%
