@echo off
REM Sets up the 64-bit MSVC compiler, using whichever Visual Studio is installed.
REM build.bat, test.bat, build_server.bat, art.bat, run.bat and misc\land.bat
REM call this themselves when cl is not on PATH, so running it by hand is
REM optional: do it once in a console and later builds there skip the setup.
REM Does nothing when cl is already on PATH.
REM Labels, not ( ) blocks: the ")" in %ProgramFiles(x86)% ends a block early.
where cl >nul 2>nul
if not errorlevel 1 exit /b 0

set "ShellDir=%CD%"

REM A long PATH breaks vcvars ("'vswhere.exe' is not recognized"): cmd cannot
REM hold it once Visual Studio appends its folders. Past 3000 characters,
REM start from a short PATH that keeps git (land.bat) and g++ (test.bat).
if "%PATH:~3000,1%"=="" goto path_ready
set GitDir=
set GccDir=
for /f "delims=" %%g in ('where git 2^>nul') do if not defined GitDir set "GitDir=%%~dpg"
for /f "delims=" %%g in ('where g++ 2^>nul') do if not defined GccDir set "GccDir=%%~dpg"
set "PATH=%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\WindowsPowerShell\v1.0;%GitDir%;%GccDir%"
:path_ready

REM vcvars runs vswhere by name, so its folder goes on PATH too.
set "VsInstaller=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
set "PATH=%PATH%;%VsInstaller%"
if not exist "%VsInstaller%\vswhere.exe" goto no_vs
set VSDIR=
for /f "usebackq delims=" %%i in (`"%VsInstaller%\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto no_vs
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
REM vcvars may change the current folder
cd /d "%ShellDir%"
where cl >nul 2>nul
if errorlevel 1 goto no_vs
exit /b 0

:no_vs
echo shell_64: no Visual Studio with the C++ desktop tools found. Install "Desktop development with C++", or run from an "x64 Native Tools Command Prompt".
exit /b 1
