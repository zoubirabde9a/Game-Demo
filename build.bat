@echo off
REM Builds the game into build\. Run misc\shell_64.bat first.
REM   build.bat           debug build: asserts on, developer keys, no optimisation
REM   build.bat release   release build: optimised, asserts and developer keys off
REM Exits non-zero if any program fails to compile.

set CommonLinkerFlags=-incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib OpenGL32.lib
set Warnings=-WX -W4 -wd4201 -wd4100 -wd4189 -wd4505

if /i "%1"=="release" (
    set CommonCompilerFlags=-MT -nologo -Gm- -EHsc- -EHa- -GR- -Ox -Oi %Warnings% -DAPP_SLOW=0 -DAPP_DEV=0 -DAPP_WIN32=1 /FC /Z7
) else (
    set CommonCompilerFlags=-MTd -nologo -Gm- -EHsc- -EHa- -GR- -Od -Oi %Warnings% -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7
)

if not exist build mkdir build
pushd build
del *.pdb > NUL 2> NUL
set Result=0

REM Packs art and sound into asset_1.zas
cl %CommonCompilerFlags% ..\code\tools\test_asset_builder.cpp /link %CommonLinkerFlags%
if %errorlevel% neq 0 set Result=1

REM The game, hot-reloaded by win32_app.exe; a fresh pdb name lets it rebuild while running
cl %CommonCompilerFlags% ..\code\app.cpp -Fmapp.map -LD /link -incremental:no -PDB:app%random%.pdb -opt:ref -subsystem:windows,5.02 -EXPORT:AppGetSoundSamples -EXPORT:AppUpdateAndRender OpenGL32.lib
if %errorlevel% neq 0 set Result=1

REM The Windows platform layer
cl %CommonCompilerFlags% ..\code\platform\win32_app.cpp -Fmwin32_app.map /link -subsystem:windows,5.02 %CommonLinkerFlags%
if %errorlevel% neq 0 set Result=1

popd
exit /b %Result%
