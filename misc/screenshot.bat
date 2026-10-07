@echo off
REM Runs the game from this worktree's build\, saves what it draws on one
REM frame to a PNG and quits. Run build.bat first.
REM   misc\screenshot.bat                  build\screenshot.png at frame 90
REM   misc\screenshot.bat out.png 300      out.png at frame 300 (5 seconds)
REM Works with the window covered, unlike a desktop capture. Set
REM GAME_OFFSCREEN=1 to open the window off screen, unfocused, so a shot
REM never pops up over what someone is doing on this machine. Plays offline
REM unless GAME_SERVER is set, so shots do not depend on the live server.
setlocal
if "%GAME_SERVER%"=="" set "GAME_SERVER=offline"
set "GAME_SCREENSHOT=%~dp0..\build\screenshot.png"
if not "%~1"=="" set "GAME_SCREENSHOT=%~f1"
if not "%~2"=="" set "GAME_SCREENSHOT_FRAME=%~2"
pushd "%~dp0..\build"
start /wait "" win32_app.exe
popd
echo %GAME_SCREENSHOT%
