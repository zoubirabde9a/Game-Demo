@echo off
REM Builds the dedicated server into build\server.exe. Run misc\shell_64.bat first.
REM Run it with: build\server.exe [port]   (default port 27015)
set ServerFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 /FC /Z7
if not exist build mkdir build
pushd build
cl %ServerFlags% ..\code\server\server_main.cpp -Feserver.exe /link -incremental:no
set Result=%errorlevel%
popd
exit /b %Result%
