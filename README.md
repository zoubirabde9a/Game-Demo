# Game-Demo
A Demo Game made in OpenGL from scratch using WIN32 api.

controls:

- Z, Q, S, D: move
- Space: jump (from the ground)
- Alt: dash (recharges in 0.8 s, shown by the small bar under health)
- Left click: fireball
- Right click: sword
- F3: tile editor

![alt text](https://i.ibb.co/VwQYmWv/1.jpg)
![alt text](https://i.ibb.co/Vvz0MMw/2.jpg)
![alt text](https://i.ibb.co/pzXDYHT/3.jpg)

## Building on Windows

Needs Visual Studio (2019 or later) with the C++ desktop tools.

```
misc\shell_64.bat
build.bat
```

`misc\shell_64.bat` finds your Visual Studio install and sets up the 64-bit compiler for the current console. `build.bat` writes `win32_app.exe` and `app.dll` into `build/`. Start the game from inside `build/`, because it loads `asset_1.zas` and `shaders/` from the current folder:

```
cd build
win32_app.exe
```
