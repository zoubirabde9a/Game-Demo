/* World grade: the post-process the world goes through every frame
   (build/shaders/fx/world_grade.frag): a soft glow round bright pixels and
   a colour grade. The world pass draws into a texture
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
};

// NOTE(zoubir): app.cpp, before the world pass and after BeginTimeWarp
internal bool32
BeginWorldGrade(render_context *RenderContext, app_state *AppState)
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
