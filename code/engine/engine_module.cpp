/* engine: the game-independent layer under everything else. This file is
   the part only the game client compiles: batched sprite and line
   rendering (render.*) on OpenGL (opengl.*), the .zas asset pack and its
   threaded loader (asset.*, file_formats.h), the SSE sound mixer (audio.*)
   and the immediate-mode widget library (ui.*). The part the simulation
   and the server use (random numbers, string helpers) is engine_core.cpp;
   arenas (memory.h) and vector math (math.h) are headers.

   Nothing here knows about players, monsters or the network. The headers
   are included through app_platform.h and app.h; this file pulls in the
   implementations. A new engine file goes on its own line below. */

#include "asset.cpp"
#include "render.cpp"
#include "ui.cpp"
#include "opengl.cpp"
#include "audio.cpp"
