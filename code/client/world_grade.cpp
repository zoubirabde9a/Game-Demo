/* World grade: the post-process the world goes through every frame
   (build/shaders/fx/world_grade.frag): the coloured light of fireballs and
   lava (world_lights.cpp), a soft glow round bright pixels and a colour
   grade. The world pass draws into a texture
   (engine/render/render_target.cpp) between BeginWorldGrade and
   EndWorldGrade, which then puts it on the window through the shader;
   overlays and the HUD draw after, ungraded.

   A time rewind has its own post-process (rewind_fx/time_warp.cpp); while
   one captures the world this stays out of the way. When the driver cannot
   draw into a texture or the shader did not build, the world draws
   straight to the window as before. */

#define WORLD_GRADE_GLOW 0.85f
#define WORLD_GRADE_STRENGTH 1.0f

struct world_grade
{
    render_target Target;
    bool32 Capturing;
    world_lights Lights;
    // NOTE(zoubir): camera x, y in world units, window pixels per world
    // unit, window height: lets the shader find the world point under a
    // pixel, for ground variation and cloud shadows that stay on the map
    float WorldView[4];
};

// NOTE(zoubir): app.cpp, before the world pass and after BeginTimeWarp
internal bool32
BeginWorldGrade(render_context *RenderContext, app_state *AppState,
                v3 CameraOffset, app_window *View, app_window *Window)
{
    if (!AppState->WorldGrade)
    {
        AppState->WorldGrade = AllocateStruct(&AppState->MemoryArena, world_grade);
        *AppState->WorldGrade = {};
    }
    world_grade *Grade = AppState->WorldGrade;
    // NOTE(zoubir): shadows (draw_entities/ground_contact.cpp) stretch out
    // under a low sun at dusk and dawn and fade at night; set before the
    // world is drawn, whether or not the grade pass runs
    float Day = Daylight(AppState);
    AppState->SunShadow = V2(4.f * Day * (1.f - Day), 1.f - Day);
    bool32 RewindCaptures = AppState->RewindFx && AppState->RewindFx->Capturing;
    bool32 HasShader = RenderContext->Programs[Shader_WorldGrade].ID !=
        RenderContext->TextureProgram.ID;
    Grade->Capturing = !RewindCaptures && HasShader &&
        BeginRenderTarget(RenderContext, &Grade->Target);
    if (Grade->Capturing)
    {
        // NOTE(zoubir): the target is the framebuffer, which on a display
        // scaled past 100% is bigger than the window; the shader's pixels
        // are the framebuffer's
        float Height = (float)Grade->Target.Height;
        float Zoom = AppState->WorldZoom * Height / (float)Maximum(Window->Height, 1u);
        GatherWorldLights(AppState, CameraOffset, View, Zoom, Height, &Grade->Lights);
        Grade->WorldView[0] = CameraOffset.X;
        Grade->WorldView[1] = CameraOffset.Y;
        Grade->WorldView[2] = Zoom;
        Grade->WorldView[3] = Height;
    }
    return Grade->Capturing;
}

// NOTE(zoubir): app.cpp, after the world pass is flushed and before
// EndTimeWarp
internal void
EndWorldGrade(render_context *RenderContext, app_state *AppState,
              memory_arena *TransientArena, mat4 *ScreenProjection,
              app_window *Window)
{
    world_grade *Grade = AppState->WorldGrade;
    if (!Grade || !Grade->Capturing)
    {
        return;
    }
    Grade->Capturing = false;
    EndRenderTarget(RenderContext);
    // NOTE(zoubir): the glow comes from the bloom when the driver can make
    // it, and from the grade shader's own short rings when not
    bool32 Bloomed = MakeWorldBloom(RenderContext, AppState, TransientArena,
                                    ScreenProjection, Window, &Grade->Target);

    open_gl *OpenGL = RenderContext->OpenGL;
    render_program *Program = &RenderContext->Programs[Shader_WorldGrade];
    if (OpenGL->glUniform4fv)
    {
        OpenGL->glUseProgram(Program->ID);
        float Screen[4] = {(float)Grade->Target.Width, (float)Grade->Target.Height,
                           Bloomed ? 0.f : WORLD_GRADE_GLOW, WORLD_GRADE_STRENGTH};
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Screen"),
                             1, Screen);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "WorldView"),
                             1, Grade->WorldView);
        // NOTE(zoubir): the map's own grade (map_moods.cpp)
        map_mood *Mood = MoodFor(AppState->World.MapId);
        // NOTE(zoubir): rain dims and greys it (weather.cpp)
        float Rain = RainAmount(AppState);
        // NOTE(zoubir): night dims, cools and greys it, short of too dark
        // to play (weather.cpp)
        float Day = Daylight(AppState);
        float Night = 1.f - Day;
        // NOTE(zoubir): dusk and dawn, at their strongest halfway between
        // day and night: warm highlights and long amber shafts
        float Twilight = 4.f * Day * Night;
        float MoodShadow[4] = {Mood->Shadow.X - 0.015f * Night, Mood->Shadow.Y,
                               Mood->Shadow.Z + 0.06f * Night,
                               Mood->Saturation * (1.f - 0.25f * Rain) * (1.f - 0.3f * Night)};
        // NOTE(zoubir): a lightning strike lights the world blue-white
        float Flash = LightningFlash(AppState);
        float MoodLight[4] = {Mood->Light.X + 0.05f * Flash + 0.07f * Twilight,
                              Mood->Light.Y + 0.07f * Flash + 0.025f * Twilight,
                              Mood->Light.Z + 0.12f * Flash - 0.05f * Twilight,
                              Mood->Exposure * (1.f - 0.2f * Rain) * (1.f + 0.9f * Flash) *
                              (1.f - 0.42f * Night)};
        // NOTE(zoubir): rain hides the sun
        float MoodShape[4] = {Mood->Contrast, Mood->Sky, Mood->SunShafts * (1.f - Rain) * (1.f - Night + 1.5f * Twilight), 0.f};
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "MoodShadow"), 1, MoodShadow);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "MoodLight"), 1, MoodLight);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "MoodShape"), 1, MoodShape);
        world_lights *Lights = &Grade->Lights;
        float LightCount[4] = {(float)Lights->Count, 0.f, 0.f, 0.f};
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "LightCount"),
                             1, LightCount);
        if (Lights->Count)
        {
            OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "LightSpot"),
                                 Lights->Count, Lights->Spot);
            OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "LightColor"),
                                 Lights->Count, Lights->Color);
        }
        // NOTE(zoubir): the flush sends P and Time again on its own use
        RenderContext->AProgramIsUsed = false;
    }

    RenderSetProjection(RenderContext, ScreenProjection);
    SetupBatchRenderer(RenderContext, TransientArena, 4);
    RenderBegin(RenderContext, 24, RENDER_ORDER_NO_ORDER);
    DrawShaderQuad(RenderContext, Shader_WorldGrade, 0.f, 0.f,
                   (float)Window->Width, (float)Window->Height,
                   0xFFFFFFFF, RenderBlend_Alpha, Grade->Target.Texture);
    RenderFlush(RenderContext);
    if (Bloomed)
    {
        AddWorldBloom(RenderContext, AppState, TransientArena, ScreenProjection, Window);
    }
}
