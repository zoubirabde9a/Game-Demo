/* Time warp: the rewinds' post-process. While one shows, the world pass
   is drawn into a texture (engine/render/render_target.cpp) and then put
   on the window through build/shaders/fx/time_warp.frag, which bends and
   recolours it inside each rewind's bubble (everywhere for a world
   rewind). Overlays and the HUD are drawn after, unbent.

   The shader takes up to REWIND_WARP_SLOTS rewinds as uniforms, each two
   vec4s:
     Rewinds[i]: centre x and y in window pixels from the bottom left, the
                 bubble's radius in pixels, the kind (rewind_kind)
     Looks[i]:   the phase (1 cast, 2 hold, 3 playback, 4 landing), its
                 progress 0..1, the strength 0..1, a seed
   and Screen: width, height, how many rewinds, the world zoom, all in the
   texture's pixels. */

#define REWIND_WARP_SLOTS 4

internal bool32
WantsTimeWarp(rewind_fx *Fx)
{
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        rewind_fx_cast *Cast = &Fx->Casts[Slot];
        if (Cast->Active || IsInRewindAfterglow(Cast, Fx->Clock))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): app.cpp, before the world pass: from here on the world
// draws into the texture when a rewind shows. False otherwise, or when the
// driver cannot draw into a texture (then there is no post-process)
internal bool32
BeginTimeWarp(render_context *RenderContext, app_state *AppState)
{
    rewind_fx *Fx = AppState->RewindFx;
    if (!Fx)
    {
        return false;
    }
    Fx->Capturing = WantsTimeWarp(Fx) && BeginRenderTarget(RenderContext, &Fx->Target);
    return Fx->Capturing;
}

// NOTE(zoubir): app.cpp, after the world pass is flushed: the world goes
// on the window through the shader
internal void
EndTimeWarp(render_context *RenderContext, app_state *AppState,
            memory_arena *TransientArena, mat4 *ScreenProjection,
            v3 CameraOffset, app_window *Window)
{
    rewind_fx *Fx = AppState->RewindFx;
    if (!Fx || !Fx->Capturing)
    {
        return;
    }
    Fx->Capturing = false;
    EndRenderTarget(RenderContext);

    float Width = (float)Window->Width;
    float Height = (float)Window->Height;
    // NOTE(zoubir): the shader works in the texture's pixels, which on a
    // scaled display are more than the window's game units
    float Pixels = Width > 0.f ? (float)Fx->Target.Width / Width : 1.f;
    float Zoom = Pixels * (AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f);
    float Rewinds[4 * REWIND_WARP_SLOTS] = {};
    float Looks[4 * REWIND_WARP_SLOTS] = {};
    u32 Count = 0;
    // NOTE(zoubir): the ones under way first, then the landings
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        for(u32 Slot = 0; Slot < MAX_PLAYERS && Count < REWIND_WARP_SLOTS; Slot++)
        {
            rewind_fx_cast *Cast = &Fx->Casts[Slot];
            bool32 Wanted = Pass == 0 ? Cast->Active : IsInRewindAfterglow(Cast, Fx->Clock);
            if (!Wanted)
            {
                continue;
            }
            float *Rewind = &Rewinds[4 * Count];
            float *Look = &Looks[4 * Count];
            Rewind[0] = Zoom * (Cast->Centre.X - CameraOffset.X);
            Rewind[1] = (float)Fx->Target.Height - Zoom * (Cast->Centre.Y - CameraOffset.Y);
            // NOTE(zoubir): a self rewind bends a little room around the
            // caster, a world rewind all of the screen
            float Radius = Cast->Kind == RewindKind_Bubble ? Cast->Radius :
                Cast->Kind == RewindKind_Self ? 54.f : 0.f;
            Rewind[2] = Cast->Kind == RewindKind_World ?
                2.f * Pixels * SquareRoot(Width * Width + Height * Height) : Zoom * Radius;
            Rewind[3] = (float)Cast->Kind;
            if (Cast->Active)
            {
                Look[0] = (float)Cast->Phase;
                Look[1] = RewindPhaseProgress(Cast);
            }
            else
            {
                Look[0] = 4.f;
                Look[1] = RewindClamp01((Fx->Clock - Cast->EndedAt) / REWIND_FX_AFTERGLOW);
            }
            Look[2] = 1.f;
            Look[3] = (float)Slot * 7.31f;
            Count++;
        }
    }

    open_gl *OpenGL = RenderContext->OpenGL;
    render_program *Program = &RenderContext->Programs[Shader_TimeWarp];
    bool32 OwnShader = Program->ID != RenderContext->TextureProgram.ID;
    if (OwnShader && OpenGL->glUniform4fv)
    {
        OpenGL->glUseProgram(Program->ID);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Rewinds"),
                             REWIND_WARP_SLOTS, Rewinds);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Looks"),
                             REWIND_WARP_SLOTS, Looks);
        float Screen[4] = {(float)Fx->Target.Width, (float)Fx->Target.Height,
                           (float)Count, Zoom};
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Screen"),
                             1, Screen);
        // NOTE(zoubir): the flush sends P and Time again on its own use
        RenderContext->AProgramIsUsed = false;
    }

    RenderSetProjection(RenderContext, ScreenProjection);
    SetupBatchRenderer(RenderContext, TransientArena, 4);
    RenderBegin(RenderContext, 24, RENDER_ORDER_NO_ORDER);
    if (OwnShader)
    {
        DrawShaderQuad(RenderContext, Shader_TimeWarp, 0.f, 0.f, Width, Height,
                       0xFFFFFFFF, RenderBlend_Alpha, Fx->Target.Texture);
    }
    else
    {
        // NOTE(zoubir): the warp shader did not build (the screen says
        // why): the world as it is, the texture's rows bottom up
        DrawTexturedQuad(RenderContext, Fx->Target.Texture, 0.f, 0.f, Width, Height,
                         V4(0.f, 0.f, 1.f, 1.f), 0xFFFFFFFF);
    }
    RenderFlush(RenderContext);
}
