/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */

#include "render.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

/* Rendering: one pass at a time, through the render_context in render.h.
   A pass is begun (RenderBegin), filled with quads, rectangles and text,
   then drawn (RenderFlush). The batched renderer groups vertices by
   texture and program so a pass can be sorted back to front. */

#include "render/pass_setup.cpp"
#include "render/flush.cpp"
#include "render/shapes.cpp"
#include "render/text.cpp"
#include "render/textures.cpp"
#include "render/render_target.cpp"
