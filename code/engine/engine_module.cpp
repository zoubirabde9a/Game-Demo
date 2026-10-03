/* engine: the game-independent layer under everything else. Arenas and
   temporary memory (memory.h), vector math (math.h), batched sprite and
   line rendering (render.*) on OpenGL (opengl.*), the .zas asset pack and
   its threaded loader (asset.*, file_formats.h), the SSE sound mixer
   (audio.*), the immediate-mode widget library used by the tile editor
   (ui.*), random numbers and small string helpers.

   Nothing here knows about players, monsters or the network. The headers
   are included through app_platform.h and app.h; this file pulls in the
   implementations. A new engine file goes on its own line below. */

#include "random.cpp"
#include "utility.cpp"
#include "asset.cpp"
#include "render.cpp"
#include "ui.cpp"
#include "opengl.cpp"
#include "audio.cpp"
