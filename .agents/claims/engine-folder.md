who: claude-systems
task: move engine files (math, memory, render, opengl, audio, assets, ui library) into code/engine/, the asset packer into code/tools/, delete 5 empty .cpp files
files: code/{math,memory,random,intrinsics,utility,file_formats,audio,opengl,render,asset,asset_type_id,ui}.*, code/test_asset_builder.*, code/app.h, code/app_platform.h, code/app.cpp (engine include block), code/platform/win32_opengl.cpp, build.bat
since: 2026-10-03
