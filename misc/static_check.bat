@echo off
rem Lists every static, global, local_persist and internal in code/.
pushd "%~dp0..\code"

echo STATICS FOUND:
findstr -s -n -i -l "static" *

echo ----- GLOBAL -----:
findstr -s -n -i -l "global_variable" *

echo ----- LOCAL -----:
echo STATICS FOUND:
findstr -s -n -i -l "local_persist" *

echo ----- INTERNAL -----:
echo STATICS FOUND:
findstr -s -n -i -l "internal" *
popd
