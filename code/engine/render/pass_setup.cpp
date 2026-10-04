/* Setting up a pass: choosing the shader program and texture, picking
   the plain or batched renderer, and RenderBegin, which takes the vertex
   (and batch) arrays from the arena. */

inline void
RenderProgramUse(render_context *RenderContext,
                 render_program *Program)
{
    open_gl *OpenGL = RenderContext->OpenGL;
    if (!RenderContext->AProgramIsUsed ||
        RenderContext->LastUsedProgramID != Program->ID ||
        RenderContext->LastProjection != Program->ProjectionMatrix)
    {
        OpenGL->glUseProgram(Program->ID);
        for(u32 AttribIndex = 0;
            AttribIndex < Program->NumAttrib;
            AttribIndex++)
        {
            OpenGL->glEnableVertexAttribArray(AttribIndex);
        }
        
        if (Program->ProjectionLocation >= 0 && Program->ProjectionMatrix)
        {
            OpenGL->glUniformMatrix4fv(Program->ProjectionLocation, 1, GL_FALSE,
                                       Program->ProjectionMatrix->Data);
        }
        if (Program->TimeLocation >= 0)
        {
            OpenGL->glUniform1f(Program->TimeLocation, RenderContext->Time);
        }
        
        RenderContext->AProgramIsUsed = true;
        RenderContext->LastUsedProgramID = Program->ID;
        RenderContext->LastProjection = Program->ProjectionMatrix;
    }
        
}

// NOTE(zoubir): what is drawn from now on uses Projection (batches keep
// the one they were made with). The world draws zoomed, the UI does not:
// app.cpp switches between the two
internal void
RenderSetProjection(render_context *RenderContext, mat4 *Projection)
{
    for(u32 Index = 0; Index < Shader_Count; Index++)
    {
        RenderContext->Programs[Index].ProjectionMatrix = Projection;
    }
    RenderContext->TextureProgram.ProjectionMatrix = Projection;
    RenderContext->LineProgram.ProjectionMatrix = Projection;
}

// NOTE(zoubir): once at the top of each frame: every program draws with
// Projection, the clock moves on, and uniforms are sent again on first use
// (so a resized window gets its new projection)
internal void
RenderBeginFrame(render_context *RenderContext, mat4 *Projection, float DeltaTime)
{
    RenderContext->Time += DeltaTime;
    UpdateShaderLibrary(RenderContext, DeltaTime);
    RenderContext->AProgramIsUsed = false;
    RenderSetProjection(RenderContext, Projection);
}

inline void
RenderProgramUnuse(open_gl *OpenGL, render_program *Program)
{
    OpenGL->glUseProgram(0);                
    for(u32 AttribIndex = 0;
        AttribIndex < Program->NumAttrib;
        AttribIndex++)
    {
        OpenGL->glDisableVertexAttribArray(AttribIndex);
    }                
}

inline render_program *
GetTextureProgram(thread_context *Thread)
{
    return &Thread->RenderContext.TextureProgram;
};

internal void
RenderSetProgram(render_context *RenderContext, render_program Program)
{
#if APP_DEV
    if (RenderContext->RendererType == RENDERER_TYPE_DEFAULT)
    {
       Assert(RenderContext->Began == false);        
    }
#endif

    switch (RenderContext->RendererType)
    {
        case RENDERER_TYPE_DEFAULT:
        {
            RenderContext->Program = Program;
            break;
        }
    };
 }

internal void
RenderSetTexture(render_context *RenderContext, u32 TextureID)
{
#if APP_DEV
    if (RenderContext->RendererType == RENDERER_TYPE_DEFAULT)
    {
       Assert(RenderContext->Began == false);        
    }
#endif
    
    if (RenderContext->RendererType == RENDERER_TYPE_DEFAULT)
    {
        RenderContext->Texture = TextureID;
    }
    else if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        render_batch *CurrentBatch =
            &RenderContext->AllocatedBatches[RenderContext->BatchCount];
        CurrentBatch->TextureID = TextureID;
    }
}

inline void
SetupDefaultRenderer(render_context *RenderContext, memory_arena *Arena)
{
    RenderContext->RendererType = RENDERER_TYPE_DEFAULT;
    RenderContext->Arena = Arena;
}

internal void
SetupBatchRenderer(render_context *RenderContext,
                   memory_arena *Arena, u32 AllocatedBatchCount)
{
    Assert(AllocatedBatchCount > 0);
    RenderContext->Arena = Arena;
    RenderContext->RendererType = RENDERER_TYPE_BATCH;
    RenderContext->AllocatedBatchCount = AllocatedBatchCount;
    RenderContext->BatchCount = 0;
    RenderContext->BatchOpen = false;
    RenderContext->AllocatedBatches =
        AllocateArray(Arena, AllocatedBatchCount,
                             render_batch);
}

internal void
RenderBegin(render_context *RenderContext,
            u32 AllocatedVertexCount, u32 Tag)
{
    memory_arena *Arena = RenderContext->Arena;
    RenderContext->VertexCount = 0;
    RenderContext->AllocatedVertexCount = AllocatedVertexCount;
    RenderContext->OrderType = Tag;

    switch(RenderContext->RendererType)
    {
        case RENDERER_TYPE_DEFAULT:
        {
            RenderContext->AllocatedVerticies =
                AllocateArray(Arena, AllocatedVertexCount,
                                render_vertex);
            break;
        }
        case RENDERER_TYPE_BATCH:
        {
            //TODO(zoubir): make sure this value
            // is correct 'AllocatedVertexCount * 2'
            RenderContext->AllocatedVerticies =
                AllocateArray(Arena, AllocatedVertexCount,
                                render_vertex);
            RenderContext->TemporaryVerticies =
                AllocateArray(Arena, AllocatedVertexCount,
                                render_vertex);
//                RenderContext->AllocatedVerticies + RenderContext->AllocatedVertexCount;
            RenderContext->BatchCount = 0;
            break;
        }
    }
#if APP_DEV
    RenderContext->Began = true;
#endif
}
