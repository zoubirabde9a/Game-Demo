@echo off
REM Puts Emscripten (em++) on PATH for web\build.bat, which calls this itself.
REM Does nothing when em++ is already on PATH. Otherwise it uses the emsdk in
REM %EMSDK%, then %USERPROFILE%\tools\emsdk, then w:\emscripten\emsdk.
REM To install one: git clone https://github.com/emscripten-core/emsdk.git
REM %USERPROFILE%\tools\emsdk, then in that folder: emsdk install latest and
REM emsdk activate latest.
where em++ >nul 2>nul
if not errorlevel 1 exit /b 0
set "ShellDir=%CD%"
set EmsdkDir=
if defined EMSDK if exist "%EMSDK%\emsdk_env.bat" set "EmsdkDir=%EMSDK%"
if not defined EmsdkDir if exist "%USERPROFILE%\tools\emsdk\emsdk_env.bat" set "EmsdkDir=%USERPROFILE%\tools\emsdk"
if not defined EmsdkDir if exist "w:\emscripten\emsdk\emsdk_env.bat" set "EmsdkDir=w:\emscripten\emsdk"
if not defined EmsdkDir goto no_emsdk
call "%EmsdkDir%\emsdk_env.bat" >nul 2>nul
cd /d "%ShellDir%"
where em++ >nul 2>nul
if errorlevel 1 goto no_emsdk
exit /b 0

:no_emsdk
echo shell_emscripten: no emsdk found. Install it as the top of misc\shell_emscripten.bat says, or set EMSDK to its folder.
exit /b 1
