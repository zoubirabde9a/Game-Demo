@echo off
REM Builds and runs every test program in code\tests, as fast as the
REM machine allows. Everything that can run at once does:
REM  1. the layout check and the five test programs' compiles start together;
REM  2. once all five are built, every run starts together: sim_tests in
REM     SimParts parts and server_tests in ServerParts parts (each runs every
REM     Nth group of its list, code\tests\test_parts.h), the short soak in its
REM     seven games, net and simd tests, and the g++ syntax checks;
REM  3. their output is printed in order once all are done.
REM Each job writes build\<name>.log and, when it ends, build\<name>.code
REM with its exit code (written to a .tmp and renamed, so a .code file is
REM never read half written).
REM The tests are compiled with -O2 -Ob1 and APP_SLOW asserts on: the same
REM checks as a debug build at a quarter of the run time. -Ob1, not the full
REM -Ob2 inlining -O2 means: MSVC never finishes optimizing soak_tests.cpp
REM with full inlining. build.bat still makes the -Od game for the debugger.
REM When g++ is on PATH, the server, tools and tests are also checked with
REM g++ -fsyntax-only (the live server is built with g++ on Linux, see
REM build_server.sh).
REM Checks use "neq 0", not "errorlevel 1": a crashed test exits with a
REM negative code, which "errorlevel 1" treats as success.
setlocal EnableDelayedExpansion
set TestFlags=-MTd -nologo -Gm- -EHsc- -GR- -O2 -Ob1 -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
set GameLibs=user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
set SimParts=6
set ServerParts=4
call "%~dp0misc\shell_64.bat" || exit /b 1
if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"
set Result=0

REM 1. The layout rules (docs/architecture-plan.md) and the compiles.
set Builds=layout cl_sim_tests cl_net_tests cl_server_tests cl_simd_tests cl_soak_tests
for %%t in (%Builds%) do del /q %%t.code %%t.tmp 2>nul
start "" /b cmd /v:on /c "powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0misc\layout_check.ps1" > layout.log 2>&1 & echo ^!errorlevel^! > layout.tmp & move /y layout.tmp layout.code >nul"
for %%t in (sim_tests server_tests soak_tests) do start "" /b cmd /v:on /c "cl %TestFlags% ..\code\tests\%%t.cpp /link -incremental:no %GameLibs% > cl_%%t.log 2>&1 & echo ^!errorlevel^! > cl_%%t.tmp & move /y cl_%%t.tmp cl_%%t.code >nul"
for %%t in (net_tests simd_tests) do start "" /b cmd /v:on /c "cl %TestFlags% ..\code\tests\%%t.cpp /link -incremental:no > cl_%%t.log 2>&1 & echo ^!errorlevel^! > cl_%%t.tmp & move /y cl_%%t.tmp cl_%%t.code >nul"
set Wait=%Builds%
call :wait
set Built=1
for %%t in (%Builds%) do (
    set Code=
    set /p Code=<%%t.code
    set Code=!Code: =!
    if not "!Code!"=="0" (
        type %%t.log
        echo %%t exited with !Code!
        if "%%t"=="layout" (set Result=1) else (set Built=0)
    )
)
if "%Built%"=="0" goto failed
if "%Result%"=="1" type layout.log

REM 2. Every run at once. The short soak is split in seven parts, one per
REM game it plays (five soak games, the random-play run, the bots run), which
REM together are exactly what "soak_tests.exe 1 2" plays; run
REM soak_tests.exe 10 8 by hand for a long one.
set Runs=net_tests simd_tests
set /a LastSim=%SimParts% - 1
set /a LastServer=%ServerParts% - 1
for /l %%k in (0,1,%LastSim%) do set Runs=!Runs! sim_tests_%%k
for /l %%k in (0,1,%LastServer%) do set Runs=!Runs! server_tests_%%k
for /l %%k in (0,1,6) do set Runs=!Runs! soak_tests_%%k
for %%t in (%Runs%) do del /q %%t.code %%t.tmp 2>nul
for %%t in (net_tests simd_tests) do start "" /b cmd /v:on /c ".\%%t.exe > %%t.log 2>&1 & echo ^!errorlevel^! > %%t.tmp & move /y %%t.tmp %%t.code >nul"
for /l %%k in (0,1,%LastSim%) do start "" /b cmd /v:on /c ".\sim_tests.exe part %%k/%SimParts% > sim_tests_%%k.log 2>&1 & echo ^!errorlevel^! > sim_tests_%%k.tmp & move /y sim_tests_%%k.tmp sim_tests_%%k.code >nul"
for /l %%k in (0,1,%LastServer%) do start "" /b cmd /v:on /c ".\server_tests.exe part %%k/%ServerParts% > server_tests_%%k.log 2>&1 & echo ^!errorlevel^! > server_tests_%%k.tmp & move /y server_tests_%%k.tmp server_tests_%%k.code >nul"
for /l %%k in (0,1,6) do start "" /b cmd /v:on /c ".\soak_tests.exe 1 2 %%k/7 > soak_tests_%%k.log 2>&1 & echo ^!errorlevel^! > soak_tests_%%k.tmp & move /y soak_tests_%%k.tmp soak_tests_%%k.code >nul"

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
set Gcc=gcc_server gcc_probe gcc_bots gcc_replay gcc_net_tests gcc_sim_tests gcc_server_tests gcc_soak_tests
set Runs=%Runs% %Gcc%
for %%t in (%Gcc%) do del /q %%t.code %%t.tmp 2>nul
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\server\server_main.cpp > gcc_server.log 2>&1 & echo ^!errorlevel^! > gcc_server.tmp & move /y gcc_server.tmp gcc_server.code >nul"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\server\probe_main.cpp > gcc_probe.log 2>&1 & echo ^!errorlevel^! > gcc_probe.tmp & move /y gcc_probe.tmp gcc_probe.code >nul"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\tools\bots_main.cpp > gcc_bots.log 2>&1 & echo ^!errorlevel^! > gcc_bots.tmp & move /y gcc_bots.tmp gcc_bots.code >nul"
start "" /b cmd /v:on /c "g++ %GccRelease% ..\code\tools\replay_main.cpp > gcc_replay.log 2>&1 & echo ^!errorlevel^! > gcc_replay.tmp & move /y gcc_replay.tmp gcc_replay.code >nul"
for %%t in (net_tests sim_tests server_tests soak_tests) do start "" /b cmd /v:on /c "g++ %GccDebug% ..\code\tests\%%t.cpp > gcc_%%t.log 2>&1 & echo ^!errorlevel^! > gcc_%%t.tmp & move /y gcc_%%t.tmp gcc_%%t.code >nul"
goto gcc_started
:no_gcc
echo test: g++ not found, skipping the g++ syntax checks
goto gcc_started
:broken_gcc
echo test: WARNING g++ at %GccDir% cannot compile an empty program, skipping the g++ syntax checks (see build\gcc_works.log)
:gcc_started

REM 3. Wait for all of them, then print each one's output in order.
set Wait=%Runs%
call :wait
for %%t in (%Runs%) do (
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

REM Returns once every job named in Wait has written its .code file,
REM looking five times a second (ping waits 200 ms for an address that
REM never answers)
:wait
for %%t in (%Wait%) do if not exist %%t.code (
    ping -n 1 -w 200 192.0.2.1 >nul
    goto wait
)
exit /b 0
