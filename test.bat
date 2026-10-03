@echo off
REM Builds and runs every test program in code\tests. Run misc\shell_64.bat first.
REM All programs are compiled first (about half a second each), then the
REM slow ones (simulation, server, soak) run at the same time, each into
REM build\<name>.log, and their output is printed in order once all are done.
REM Checks use "neq 0", not "errorlevel 1": a crashed test exits with a
REM negative code, which "errorlevel 1" treats as success.
setlocal EnableDelayedExpansion
set TestFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
set GameLibs=user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
if not exist build mkdir build
pushd build
set Result=0

cl %TestFlags% ..\code\tests\sim_tests.cpp /link -incremental:no %GameLibs%
if %errorlevel% neq 0 goto failed
cl %TestFlags% ..\code\tests\net_tests.cpp /link -incremental:no
if %errorlevel% neq 0 goto failed
cl %TestFlags% ..\code\tests\server_tests.cpp /link -incremental:no %GameLibs%
if %errorlevel% neq 0 goto failed
cl %TestFlags% ..\code\tests\simd_tests.cpp /link -incremental:no
if %errorlevel% neq 0 goto failed
cl %TestFlags% ..\code\tests\soak_tests.cpp /link -incremental:no %GameLibs%
if %errorlevel% neq 0 goto failed

REM The slow ones at once. The short soak is split in six parts, one per
REM game it plays (five soak games and the random-play run), which
REM together are exactly what "soak_tests.exe 1 2" plays; run
REM soak_tests.exe 10 8 by hand for a long one.
set Slow=sim_tests server_tests soak_tests_0 soak_tests_1 soak_tests_2 soak_tests_3 soak_tests_4 soak_tests_5
for %%t in (%Slow%) do del /q %%t.code 2>nul
start "" /b cmd /v:on /c ".\sim_tests.exe > sim_tests.log 2>&1 & echo ^!errorlevel^! > sim_tests.code"
start "" /b cmd /v:on /c ".\server_tests.exe > server_tests.log 2>&1 & echo ^!errorlevel^! > server_tests.code"
for /l %%k in (0,1,5) do start "" /b cmd /v:on /c ".\soak_tests.exe 1 2 %%k/6 > soak_tests_%%k.log 2>&1 & echo ^!errorlevel^! > soak_tests_%%k.code"

REM The quick ones meanwhile.
.\net_tests.exe
if %errorlevel% neq 0 set Result=1
.\simd_tests.exe
if %errorlevel% neq 0 set Result=1

:wait
for %%t in (%Slow%) do if not exist %%t.code goto sleep
goto collect
:sleep
ping -n 2 127.0.0.1 >nul
goto wait

:collect
REM A .code file can exist a moment before its number is written.
ping -n 2 127.0.0.1 >nul
for %%t in (%Slow%) do (
    type %%t.log
    set Code=
    set /p Code=<%%t.code
    set Code=!Code: =!
    if not "!Code!"=="0" (
        echo %%t exited with !Code!
        set Result=1
    )
)

popd
exit /b %Result%

:failed
popd
exit /b 1
