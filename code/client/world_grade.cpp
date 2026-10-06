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
    bool32 RewindCaptures = AppState->RewindFx && AppState->RewindFx->Capturing;
    bool32 HasShader = RenderContext->Programs[Shader_WorldGrade].ID !=
        RenderContext->TextureProgram.ID;
    Grade->Capturing = !RewindCaptures && HasShader &&
        BeginRenderTarget(RenderContext, &Grade->Target);
    if (Grade->Capturing)
    {
        GatherWorldLights(AppState, CameraOffset, View, (float)Window->Height,
                          &Grade->Lights);
        Grade->WorldView[0] = CameraOffset.X;
        Grade->WorldView[1] = CameraOffset.Y;
        Grade->WorldView[2] = AppState->WorldZoom;
        Grade->WorldView[3] = (float)Window->Height;
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

    open_gl *OpenGL = RenderContext->OpenGL;
    render_program *Program = &RenderContext->Programs[Shader_WorldGrade];
    if (OpenGL->glUniform4fv)
    {
        OpenGL->glUseProgram(Program->ID);
        float Screen[4] = {(float)Grade->Target.Width, (float)Grade->Target.Height,
                           WORLD_GRADE_GLOW, WORLD_GRADE_STRENGTH};
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "Screen"),
                             1, Screen);
        OpenGL->glUniform4fv(OpenGL->glGetUniformLocation(Program->ID, "WorldView"),
                             1, Grade->WorldView);
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
}
