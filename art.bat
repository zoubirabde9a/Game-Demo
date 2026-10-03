@echo off
REM Draws every monster sprite sheet into build\monster_art\ as PNG for review. Run misc\shell_64.bat first.
set ArtFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
if not exist build mkdir build
pushd build
cl %ArtFlags% ..\code\tools\monster_sheets.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if errorlevel 1 (
    popd
    exit /b 1
)
.\monster_sheets.exe
set Result=%errorlevel%
popd
exit /b %Result%
