
em++ -std=c++11 "code\emscripten_app.cpp" -msimd128 -msse2 -s USE_SDL=2 -s FULL_ES3=1 -DCOMPILER_EMSCRIPTEN -DAPP_DEV -Wno-write-strings --memory-init-file 0 -O1 -o web\build\plain.html
