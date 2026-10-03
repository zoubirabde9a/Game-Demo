@echo off
REM Builds and runs every test program in code\tests. Run misc\shell_64.bat first.
REM Checks use "neq 0", not "errorlevel 1": a crashed test exits with a
REM negative code, which "errorlevel 1" treats as success.
set TestFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
if not exist build mkdir build
pushd build
set Result=0

cl %TestFlags% ..\code\tests\sim_tests.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if %errorlevel% neq 0 goto failed
.\sim_tests.exe
if %errorlevel% neq 0 set Result=1

cl %TestFlags% ..\code\tests\net_tests.cpp /link -incremental:no
if %errorlevel% neq 0 goto failed
.\net_tests.exe
if %errorlevel% neq 0 set Result=1

cl %TestFlags% ..\code\tests\server_tests.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if %errorlevel% neq 0 goto failed
.\server_tests.exe
if %errorlevel% neq 0 set Result=1

cl %TestFlags% ..\code\tests\simd_tests.cpp /link -incremental:no
if %errorlevel% neq 0 goto failed
.\simd_tests.exe
if %errorlevel% neq 0 set Result=1

REM Short soak here; run soak_tests.exe 10 8 by hand for a long one.
cl %TestFlags% ..\code\tests\soak_tests.cpp /link -incremental:no user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if %errorlevel% neq 0 goto failed
.\soak_tests.exe 1 2
if %errorlevel% neq 0 set Result=1

popd
exit /b %Result%

:failed
popd
exit /b 1
