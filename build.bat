@echo off

REM TODO(zoubir): make a seprate file for x64 and x86
REM for executables and the ddls
REM x86 DEBUG
REM set OpenGlLink=../lib/x86/glew32d.lib OpenGL32.lib
REM set CommonLinkerFlags=-incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib %OpenGlLink%
REM x64 DEBUG
set OpenGlLink=OpenGL32.lib
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib %OpenGlLink%
REM x64 RELEASE
REM set OpenGlLink=../lib/x64/glew32.lib OpenGL32.lib
REM set CommonLinkerFlags=-incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib %OpenGlLink%

REM Set CommonCompilerFlags=-MT -nologo -Gm- -EHsc- -EHa- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7

REM DEBUG
Set CommonCompilerFlags=-MTd -nologo -Gm- -EHsc- -EHa- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=1 -DAPP_DEV=1 -DAPP_WIN32=1 /FC /Z7

REM RELEASE
REM set CommonCompilerFlags=-MT -nologo -Gm- -EHsc- -EHa- -GR- -Ox -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPP_SLOW=0 -DAPP_DEV=0 -DAPP_WIN32=1 /FC /Z7

REM subsystem:windows,5.02 x64
REM subsystem:windows,5.01 x86



pushd build
del *.pdb > NUL 2> NUL

REM Test File builder
cl %CommonCompilerFlags% ..\code\test_asset_builder.cpp /link %CommonLinkerFlags%

cl %CommonCompilerFlags% ..\code\app.cpp -Fmapp.map -LD /link -incremental:no -PDB:app%random%.pdb -opt:ref -subsystem:windows,5.02 -EXPORT:AppGetSoundSamples -EXPORT:AppUpdateAndRender %OpenGlLink%
cl %CommonCompilerFlags% ..\code\win32_app.cpp -Fmwin32_app.map /link -subsystem:windows,5.02 %CommonLinkerFlags%
popd


