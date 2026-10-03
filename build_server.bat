@echo off
REM Builds the dedicated server and its health-check probe. Run misc\shell_64.bat first.
REM   build\server.exe [port]          runs the server (default port 27015)
REM   build\probe.exe [address:port]   exits 0 if a server there lets a player in
set ServerFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 /FC /Z7
if not exist build mkdir build
pushd build
set Result=0
cl %ServerFlags% ..\code\server\server_main.cpp -Feserver.exe /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if %errorlevel% neq 0 set Result=1
cl %ServerFlags% ..\code\server\probe_main.cpp -Feprobe.exe /link -incremental:no
if %errorlevel% neq 0 set Result=1
popd
exit /b %Result%
