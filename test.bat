@echo off
REM Builds and runs the simulation tests in code\tests. Run misc\shell_64.bat first.
set TestFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
if not exist build mkdir build
pushd build
cl %TestFlags% ..\code\tests\sim_tests.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if errorlevel 1 (
    popd
    exit /b 1
)
.\sim_tests.exe
set Result=%errorlevel%
popd
exit /b %Result%
