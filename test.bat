@echo off
REM Builds and runs every test program in code\tests.
REM All programs are compiled first (about half a second each), then the
REM slow ones (simulation, server, soak) run at the same time, each into
REM build\<name>.log, and their output is printed in order once all are done.
REM When g++ is on PATH, the server, tools and tests are also checked with
REM g++ -fsyntax-only (the live server is built with g++ on Linux, see
REM build_server.sh), as more of those parallel jobs (gcc_<name>.log).
REM Checks use "neq 0", not "errorlevel 1": a crashed test exits with a
REM negative code, which "errorlevel 1" treats as success.
setlocal EnableDelayedExpansion
set TestFlags=-MTd -nologo -Gm- -EHsc- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
set GameLibs=user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
call "%~dp0misc\shell_64.bat" || exit /b 1
if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"
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

REM The slow ones at once. The short soak is split in seven parts, one per
REM game it plays (five soak games, the random-play run, the bots run), which
REM together are exactly what "soak_tests.exe 1 2" plays; run
REM soak_tests.exe 10 8 by hand for a long one.
set Slow=sim_tests server_tests soak_tests_0 soak_tests_1 soak_tests_2 soak_tests_3 soak_tests_4 soak_tests_5 soak_tests_6
for %%t in (%Slow%) do del /q %%t.code 2>nul
start "" /b cmd /v:on /c ".\sim_tests.exe > sim_tests.log 2>&1 & echo ^!errorlevel^! > sim_tests.code"
start "" /b cmd /v:on /c ".\server_tests.exe > server_tests.log 2>&1 & echo ^!errorlevel^! > server_tests.code"
for /l %%k in (0,1,6) do start "" /b cmd /v:on /c ".\soak_tests.exe 1 2 %%k/7 > soak_tests_%%k.log 2>&1 & echo ^!errorlevel^! > soak_tests_%%k.code"

REM g++ syntax checks, if g++ is here: the server as the live build makes
REM it, the rest with the test flags.
where g++ >nul 2>nul
if %errorlevel% neq 0 goto no_gcc
REM g++ loads its libraries from PATH; Git for Windows' mingw64 folder has
REM older copies that make it exit 1 with no message, so its own folder goes first
for /f "delims=" %%g in ('where g++') do if not defined GccDir set "GccDir=%%~dpg"
set "PATH=%GccDir%;%PATH%"
set GccRelease=-std=c++11 -w -fsyntax-only -DAPP_SLOW=0 -DAPP_DEV=0
set GccDebug=-std=c++11 -w -fsyntax-only -DAPP_SLOW=1 -DAPP_DEV=1
REM A g++ that cannot compile an empty program is a broken install, not
REM broken code: say so and skip, rather than failing everyone's land.
echo int main() { return 0; } > gcc_works.cpp
g++ %GccRelease% gcc_works.cpp > gcc_works.log 2>&1
if %errorlevel% neq 0 goto broken_gcc
set Slow=%Slow% gcc_server gcc_probe gcc_bots gcc_replay gcc_net_tests gcc_sim_tests gcc_server_tests gcc_soak_tests
for %%t in (gcc_server gcc_probe gcc_bots gcc_replay gcc_net_tests gcc_sim_tests gcc_server_tests gcc_soak_tests) do del /q %%t.code 2>nul
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\server\server_main.cpp > gcc_server.log 2>&1 & echo ^!errorlevel^! > gcc_server.code"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\server\probe_main.cpp > gcc_probe.log 2>&1 & echo ^!errorlevel^! > gcc_probe.code"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\tools\bots_main.cpp > gcc_bots.log 2>&1 & echo ^!errorlevel^! > gcc_bots.code"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\tools\replay_main.cpp > gcc_replay.log 2>&1 & echo ^!errorlevel^! > gcc_replay.code"
for %%t in (net_tests sim_tests server_tests soak_tests) do start "" /b cmd /v:on /c "g++ %GccDebug% ..\code\tests\%%t.cpp > gcc_%%t.log 2>&1 & echo ^!errorlevel^! > gcc_%%t.code"
goto gcc_started
:no_gcc
echo test: g++ not found, skipping the g++ syntax checks
goto gcc_started
:broken_gcc
echo test: WARNING g++ at %GccDir% cannot compile an empty program, skipping the g++ syntax checks (see build\gcc_works.log)
:gcc_started

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
