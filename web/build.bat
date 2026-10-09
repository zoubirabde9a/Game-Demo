@echo off
REM Builds the browser game into web\build\: index.html (the page, from
REM web\shell.html), index.js, index.wasm and index.data (the files the game
REM reads: asset_1.zas and shaders\ from build\, fonts\ and the hero sheets
REM from web\). Open it through a web server, not as a file:
REM build\web_gateway.exe --files web\build serves it with online play at
REM http://127.0.0.1:27080/ (see code\server\web_gateway\web_gateway.h).
REM   web\build.bat          release: optimised, asserts and developer keys off
REM   web\build.bat debug    asserts and developer keys on, no optimisation
REM Works from any folder. Exits non-zero if the build fails.
setlocal
call "%~dp0..\misc\shell_emscripten.bat" || exit /b 1
pushd "%~dp0"

if /i "%1"=="debug" (
    set Flags=-O0 -g -DAPP_SLOW=1 -DAPP_DEV=1 -sASSERTIONS=1
) else (
    set Flags=-O2 -DAPP_SLOW=0 -DAPP_DEV=0
)

REM The files the game opens, laid out as in build\, packed into index.data
if exist build\files rmdir /s /q build\files
mkdir build\files
copy /y ..\build\asset_1.zas build\files > NUL
xcopy /e /y /q /i ..\build\shaders build\files\shaders > NUL
xcopy /y /q /i fonts build\files\fonts > NUL
xcopy /y /q /i heroes build\files\heroes > NUL
xcopy /y /q /i heroes_v2 build\files\heroes_v2 > NUL

REM -sMEMORY64=2: 8-byte pointers, as on the 64-bit desktop and server, so
REM structs have the same layout everywhere and the content id the server
REM checks (SimContentId, which hashes the monster tables' bytes) matches;
REM it still runs as ordinary 32-bit WebAssembly in every browser.
call em++ -std=c++14 ..\code\platform\emscripten_app.cpp %Flags% -sMEMORY64=2 ^
    -msimd128 -msse2 -Wno-write-strings -Wno-c++11-narrowing ^
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sENVIRONMENT=web ^
    -sINITIAL_MEMORY=256MB -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=8MB ^
    "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$UTF8ToString,$stringToUTF8" ^
    --preload-file build\files@/ --shell-file shell.html -o build\index.html
set Result=%errorlevel%
if exist build\files rmdir /s /q build\files
popd
exit /b %Result%
