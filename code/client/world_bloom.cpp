/* World bloom: the glow round bright, coloured things (fire, lava, magic,
   flashes), added over the graded world (world_grade.cpp). Made at a
   quarter of the screen's size by build/shaders/fx/bloom.frag: the bright
   parts of the world into one texture, blurred across into a second, then
   down back into the first, which is added over the window. A quarter
   costs a sixteenth of the pixels, so the glow can reach far for what
   the old one cost.

   The textures are the window's size and every pass draws into their
   bottom-left quarter (a quad over a quarter of the window), so no
   viewport change is needed and they remake themselves with the window. */

#define WORLD_BLOOM_STRENGTH 1.1f
// NOTE(zoubir): quarter pixels between blur taps; at 2 the glow reaches
// about 64 window pixels each way
#define WORLD_BLOOM_STEP 2.f

struct world_bloom
{
    render_target Bright;
    render_target Blurred;
};

// NOTE(zoubir): one pass of bloom.frag over Area of the window (in window
// units), reading Source; Pass as the shader takes it
internal void
RunBloomPass(render_context *RenderContext, memory_arena *TransientArena,
             mat4 *ScreenProjection, u32 Source, v2 TextureSize, v4 Area,
             v4 Pass, u32 Blend)
{
    open_gl *OpenGL = RenderContext->OpenGL;
    render_program *Program = &RenderContext->Programs[Shader_Bloom];
    OpenGL->glUseProgram(Program->ID);
    float Screen[4] = {TextureSize.X, TextureSize.Y, 0.f, 0.f};
    OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Screen"), 1, Screen);
    OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Pass"), 1, Pass.Data);
    RenderContext->AProgramIsUsed = false;

    RenderSetProjection(RenderContext, ScreenProjection);
    SetupBatchRenderer(RenderContext, TransientArena, 4);
    RenderBegin(RenderContext, 24, RENDER_ORDER_NO_ORDER);
    DrawShaderQuad(RenderContext, Shader_Bloom, Area.X, Area.Y, Area.Z, Area.W,
                   0xFFFFFFFF, Blend, Source);
    RenderFlush(RenderContext);
}

// NOTE(zoubir): the first three passes, after the world is drawn into
// World and before it is put on the window; false when the driver cannot
// (no shader, no render target), and the caller keeps its own glow
internal bool32
MakeWorldBloom(render_context *RenderContext, app_state *AppState,
               memory_arena *TransientArena, mat4 *ScreenProjection,
               app_window *Window, render_target *World)
{
    if (RenderContext->Programs[Shader_Bloom].ID == RenderContext->TextureProgram.ID ||
        !RenderContext->OpenGL->glUniform4fv)
    {
        return false;
    }
    if (!AppState->WorldBloom)
    {
        AppState->WorldBloom = AllocateStruct(&AppState->MemoryArena, world_bloom);
        *AppState->WorldBloom = {};
    }
    world_bloom *Bloom = AppState->WorldBloom;
    v2 Size = V2((float)World->Width, (float)World->Height);
    float W = (float)Window->Width;
    float H = (float)Window->Height;
    // NOTE(zoubir): the bottom-left quarter of the window, in window units
    // (Y down)
    v4 Quarter = V4(0.f, 0.75f * H, 0.25f * W, 0.25f * H);

    if (!BeginRenderTarget(RenderContext, &Bloom->Bright))
    {
        return false;
    }
    RunBloomPass(RenderContext, TransientArena, ScreenProjection, World->Texture,
                 Size, Quarter, V4(0.f, 0.f, 0.f, 0.f), RenderBlend_Alpha);
    if (!BeginRenderTarget(RenderContext, &Bloom->Blurred))
    {
        EndRenderTarget(RenderContext);
        return false;
    }
    RunBloomPass(RenderContext, TransientArena, ScreenProjection, Bloom->Bright.Texture,
                 Size, Quarter, V4(1.f, WORLD_BLOOM_STEP, 0.f, 0.f), RenderBlend_Alpha);
    BeginRenderTarget(RenderContext, &Bloom->Bright);
    RunBloomPass(RenderContext, TransientArena, ScreenProjection, Bloom->Blurred.Texture,
                 Size, Quarter, V4(1.f, 0.f, WORLD_BLOOM_STEP, 0.f), RenderBlend_Alpha);
    EndRenderTarget(RenderContext);
    return true;
}

// NOTE(zoubir): the last pass, after the graded world is on the window
internal void
AddWorldBloom(render_context *RenderContext, app_state *AppState,
              memory_arena *TransientArena, mat4 *ScreenProjection,
              app_window *Window)
{
    world_bloom *Bloom = AppState->WorldBloom;
    v2 Size = V2((float)Bloom->Bright.Width, (float)Bloom->Bright.Height);
    RunBloomPass(RenderContext, TransientArena, ScreenProjection, Bloom->Bright.Texture,
                 Size, V4(0.f, 0.f, (float)Window->Width, (float)Window->Height),
                 V4(2.f, 0.f, 0.f, WORLD_BLOOM_STRENGTH), RenderBlend_Additive);
}
