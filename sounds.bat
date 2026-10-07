@echo off
REM Writes every made sound effect into build\sounds\ as WAV, to listen to.
set ArtFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
call "%~dp0misc\shell_64.bat" || exit /b 1
if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"
cl %ArtFlags% ..\code\tools\sound_sheets.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if errorlevel 1 (
    popd
    exit /b 1
)
.\sound_sheets.exe
set Result=%errorlevel%
popd
exit /b %Result%
