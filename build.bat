@echo off
REM Builds the game into build\. Works from any folder and sets up the compiler if needed.
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

call "%~dp0misc\shell_64.bat" || exit /b 1
if not exist "%~dp0build" mkdir "%~dp0build"
pushd "%~dp0build"
del *.pdb > NUL 2> NUL
set Result=0

REM The game runs from build\ and loads its fonts from build\fonts; web\fonts is the one copy in git
xcopy /y /q /i ..\web\fonts fonts > NUL
REM The painted player skins (client\heroes\hero_skins.cpp) load from build\heroes; web\heroes is the copy in git
xcopy /y /q /i ..\web\heroes heroes > NUL

REM Packs art and sound into asset_1.zas
cl %CommonCompilerFlags% ..\code\tools\test_asset_builder.cpp /link %CommonLinkerFlags%
if %errorlevel% neq 0 set Result=1

REM Draws data\icon\game.ico; run it by hand after changing the icon (see its top comment)
cl %CommonCompilerFlags% ..\code\tools\icon_builder.cpp /link %CommonLinkerFlags%
if %errorlevel% neq 0 set Result=1

REM The game's icon (data\icon\game.ico), linked into win32_app.exe and launcher.exe
rc -nologo -fo game.res ..\code\platform\game.rc
if %errorlevel% neq 0 set Result=1

REM The game, hot-reloaded by win32_app.exe; a fresh pdb name lets it rebuild while running
cl %CommonCompilerFlags% ..\code\app.cpp -Fmapp.map -LD /link -incremental:no -PDB:app%random%.pdb -opt:ref -subsystem:windows,5.02 -EXPORT:AppGetSoundSamples -EXPORT:AppUpdateAndRender OpenGL32.lib
if %errorlevel% neq 0 set Result=1

REM The Windows platform layer
cl %CommonCompilerFlags% ..\code\platform\win32_app.cpp -Fmwin32_app.map game.res /link -subsystem:windows,5.02 %CommonLinkerFlags%
if %errorlevel% neq 0 set Result=1

REM The launcher players download: installs the game and keeps it on the server's build (deploy\publish_client.sh)
cl %CommonCompilerFlags% ..\code\platform\launcher_app.cpp -Felauncher.exe -Fmlauncher.map game.res /link -subsystem:windows,5.02 %CommonLinkerFlags% winhttp.lib bcrypt.lib ole32.lib shell32.lib
if %errorlevel% neq 0 set Result=1

popd
exit /b %Result%
