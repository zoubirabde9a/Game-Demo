@echo off
REM Builds the game and starts it. Works from any folder, or by double-clicking.
REM   run.bat            debug build
REM   run.bat release    release build
REM If the game is already running, rebuild with build.bat instead: the running
REM game loads the new app.dll by itself, and win32_app.exe cannot be rebuilt
REM while it is open.
setlocal
call "%~dp0build.bat" %1
if errorlevel 1 (
    echo run: the build failed, see the errors above.
    pause
    exit /b 1
)
REM The game loads asset_1.zas, shaders\ and fonts\ from the current folder.
start "" /d "%~dp0build" "%~dp0build\win32_app.exe"
