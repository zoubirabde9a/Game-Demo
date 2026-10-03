@echo off
REM Lands this worktree's branch on main, the way AGENTS.md section 5 asks:
REM rebase on main, run test.bat, build.bat and build.bat release, then
REM fast-forward main to the branch from the main checkout. If main moved
REM while the tests ran, it rebases and tests again (up to 3 times), so two
REM changes that pass alone are never merged untested together.
REM
REM Run from the root of your worktree, after committing:   misc\land.bat
REM Stops on the first failure and leaves main untouched. Sets up the
REM 64-bit compiler itself when cl is not on PATH.
setlocal EnableDelayedExpansion

for %%r in ("%~dp0..") do set "Root=%%~fr"
cd /d "%Root%"

for /f "delims=" %%b in ('git rev-parse --abbrev-ref HEAD') do set Branch=%%b
if /i "%Branch%"=="main" (
    echo land: run this from your own worktree branch, not main.
    exit /b 1
)
git diff --quiet HEAD
if errorlevel 1 (
    echo land: commit or stash your changes first.
    exit /b 1
)
for /f "delims=" %%d in ('git rev-parse --path-format^=absolute --git-common-dir') do set CommonDir=%%d
for %%p in ("%CommonDir%\..") do set MainDir=%%~fp

REM A long PATH breaks vcvars ("'vswhere.exe' is not recognized"), so
REM start from a short one that keeps git. Labels, not a ( ) block: the
REM ")" in %ProgramFiles(x86)% would end the block early.
where cl >nul 2>nul
if not errorlevel 1 goto compiler_ready
for /f "delims=" %%g in ('where git') do if not defined GitDir set "GitDir=%%~dpg"
REM g++, when installed, lets test.bat check the code the live server builds
for /f "delims=" %%g in ('where g++ 2^>nul') do if not defined GccDir set "GccDir=%%~dpg"
set "PATH=%SystemRoot%\system32;%SystemRoot%;%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%GitDir%;%GccDir%"
call "%Root%\misc\shell_64.bat" >nul
REM vcvars may change the current folder
cd /d "%Root%"
where cl >nul 2>nul
if errorlevel 1 (
    echo land: could not set up the compiler; run misc\shell_64.bat first.
    exit /b 1
)
:compiler_ready

REM Warn (never block) when this branch changes files another agent claimed.
powershell -NoProfile -ExecutionPolicy Bypass -File "%Root%\misc\claims.ps1" -Base main

if not exist build mkdir build
set Attempt=0
:again
set /a Attempt+=1
if %Attempt% gtr 3 (
    echo land: main kept moving; try again later.
    exit /b 1
)
git rebase main
if errorlevel 1 (
    echo land: rebase conflict. Resolve it, or git rebase --abort.
    exit /b 1
)
for /f "delims=" %%h in ('git rev-parse main') do set Base=%%h
echo land: testing %Branch% on main !Base:~0,8! (attempt %Attempt%)

call "%Root%\test.bat" >build\land_test.log 2>&1
if errorlevel 1 (
    echo land: test.bat failed, see build\land_test.log
    exit /b 1
)
call "%Root%\build.bat" >build\land_build.log 2>&1
if errorlevel 1 (
    echo land: build.bat failed, see build\land_build.log
    exit /b 1
)
call "%Root%\build.bat" release >build\land_release.log 2>&1
if errorlevel 1 (
    echo land: build.bat release failed, see build\land_release.log
    exit /b 1
)

for /f "delims=" %%h in ('git rev-parse main') do set Now=%%h
if not "!Now!"=="!Base!" (
    echo land: main moved while testing, again.
    goto again
)
git -C "%MainDir%" merge --ff-only %Branch%
if errorlevel 1 (
    echo land: fast-forward failed; main moved, again.
    goto again
)
echo land: main is now at
git log --oneline -1 main
exit /b 0
