/* Drawing a pass: RenderFlush sorts the vertices or batches by depth when
   the pass asks for it and sends them to OpenGL. RenderMakeRoom flushes
   early when a batch pass runs out of room. */

internal bool
CompareVertexBackToFront(render_vertex *A, render_vertex *B)
{
    return A->Z >= B->Z;
}

internal bool
CompareVertexFrontToBack(render_vertex *A, render_vertex *B)
{
    return A->Z < B->Z;
}

internal void
SortVerticies(render_vertex *Verticies, u32 VerticiesCount,
              bool(* CompareFunction)(render_vertex *, render_vertex *))
{
    for(u32 FixedVertexIndex = 0;
        FixedVertexIndex < VerticiesCount - 1;
        FixedVertexIndex++)
    {
        render_vertex *FixedVertex = &Verticies[FixedVertexIndex];
        for(u32 CurrentVertexIndex = FixedVertexIndex;
            CurrentVertexIndex < VerticiesCount;
            CurrentVertexIndex++)
        {
            render_vertex *CurrentVertex = &Verticies[CurrentVertexIndex];
            if (CompareFunction(FixedVertex, CurrentVertex))
            {
                render_vertex tmp = *FixedVertex;
                *FixedVertex = *CurrentVertex;
                *CurrentVertex = tmp;
            }
                
        }
    }    
}

internal bool
CompareBatchBackToFront(render_batch *A, render_batch *B)
{
    return A->SortingValue >= B->SortingValue;
}

internal bool
CompareBatchFrontToBack(render_batch *A, render_batch *B)
{
    return A->SortingValue < B->SortingValue;
}

internal void
SortBatches(render_batch *Batches, u32 BatchCount,
            bool(* CompareFunction)(render_batch *, render_batch *))
{
    render_batch TemporaryBatch;
    for(u32 FixedBatchIndex = 0;
        FixedBatchIndex < BatchCount - 1;
        FixedBatchIndex++)
    {
        render_batch *FixedBatch = &Batches[FixedBatchIndex];
        for(u32 CurrentBatchIndex = FixedBatchIndex;
            CurrentBatchIndex < BatchCount;
            CurrentBatchIndex++)
        {
            render_batch *CurrentBatch = &Batches[CurrentBatchIndex];
            if (CompareFunction(FixedBatch, CurrentBatch))
            {
                TemporaryBatch = *FixedBatch;
                *FixedBatch = *CurrentBatch;
                *CurrentBatch = TemporaryBatch;
            }
        }
    }
}


internal void
RenderFlush(render_context *RenderContext)
{
#if APP_DEV
    Assert(RenderContext->Began);
    RenderContext->Began = false;
#endif

    open_gl *OpenGL = RenderContext->OpenGL;
    u32 OrderType = RenderContext->OrderType;
    render_vertex* Verticies = RenderContext->AllocatedVerticies;
    render_vertex *VerticiesToRender = Verticies;
    
    switch(RenderContext->RendererType)
    {
        case RENDERER_TYPE_DEFAULT:
        {       
            if (OrderType == RENDER_ORDER_BACK_TO_FRONT)
            {
                SortVerticies(Verticies, RenderContext->VertexCount, CompareVertexBackToFront);
            }
            else if (OrderType == RENDER_ORDER_FRONT_TO_BACK)
            {
                SortVerticies(Verticies, RenderContext->VertexCount, CompareVertexFrontToBack);
            }    
    
            size_t VerticesMemoryBlockSize =
                RenderContext->VertexCount * sizeof(render_vertex); 
            OpenGL->glBindBuffer(GL_ARRAY_BUFFER, RenderContext->VBO);
            OpenGL->glBufferData(GL_ARRAY_BUFFER, VerticesMemoryBlockSize, 0, GL_DYNAMIC_DRAW);
            OpenGL->glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesMemoryBlockSize, VerticiesToRender);
            OpenGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
    
            OpenGL->glBindVertexArray(RenderContext->VAO);
            OpenGL->glBindTexture(GL_TEXTURE_2D, RenderContext->Texture);
    
            OpenGL->glDrawArrays(GL_TRIANGLES, 0, (GLsizei)RenderContext->VertexCount);
            break;
        }
        case RENDERER_TYPE_BATCH:
        {
            render_batch *Batches = RenderContext->AllocatedBatches;
            render_vertex *OutVerticies = RenderContext->TemporaryVerticies;
            if (RenderContext->BatchCount > 1)
            {
                VerticiesToRender = OutVerticies;
                if (OrderType == RENDER_ORDER_BACK_TO_FRONT)
                {                
                    SortBatches(Batches, RenderContext->BatchCount,
                                CompareBatchBackToFront);
                }
                else if (OrderType == RENDER_ORDER_FRONT_TO_BACK)
                {
                    SortBatches(Batches, RenderContext->BatchCount,
                                CompareBatchFrontToBack);
                }
            }
#if 1
            for (u32 CurrentBatchIndex = 0;
                 CurrentBatchIndex < RenderContext->BatchCount;
                 CurrentBatchIndex++)
            {
                render_batch *CurrentBatch = &Batches[CurrentBatchIndex];
                VerticiesToRender = CurrentBatch->Verticies;
                size_t VerticesMemoryBlockSize =
                    CurrentBatch->VertexCount * sizeof(render_vertex);
                OpenGL->glBindBuffer(GL_ARRAY_BUFFER, RenderContext->VBO);
                OpenGL->glBufferData(GL_ARRAY_BUFFER, VerticesMemoryBlockSize, 0, GL_DYNAMIC_DRAW);
                OpenGL->glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesMemoryBlockSize, VerticiesToRender);
                OpenGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
    
                OpenGL->glBindVertexArray(RenderContext->VAO);
                render_program *Program = &CurrentBatch->Program;

                RenderProgramUse(RenderContext, Program);
                
                switch(CurrentBatch->Type)
                {
                    case RENDER_BATCH_TYPE_TEXTURE:
                    {
                        OpenGL->glBindTexture(GL_TEXTURE_2D, CurrentBatch->TextureID);
                        OpenGL->glDrawArrays(GL_TRIANGLES, 0, (GLsizei)CurrentBatch->VertexCount);
                        break;
                    }
                    case RENDER_BATCH_TYPE_RECTANGLE:
                    {
                        OpenGL->glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)CurrentBatch->VertexCount);
                        break;
                    }
                    case RENDER_BATCH_TYPE_FILLED_RECTANGLE:
                    {
                        OpenGL->glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)CurrentBatch->VertexCount);
                        break;
                    }
                }
            }            
#endif
        }
    };

    // TODO(zoubir): have a draw call for each different
    // texture and program
}

// NOTE(zoubir): a batch pass's arrays are sized by a guess made before
// drawing. When they cannot take VertexCount more vertices (and a new
// batch, when NewBatch), this draws what is queued and empties them; an
// open batch carries on at the start of the emptied arrays. A full pass
// costs a draw call and, in a sorted pass, sort order across the break,
// never a crash or a write past the end.
internal void
RenderMakeRoom(render_context *RenderContext, u32 VertexCount, bool32 NewBatch)
{
    if (RenderContext->RendererType != RENDERER_TYPE_BATCH)
    {
        return;
    }
    u32 BatchesNeeded = RenderContext->BatchCount +
        ((NewBatch || RenderContext->BatchOpen) ? 1 : 0);
    if (RenderContext->VertexCount + VertexCount <=
        RenderContext->AllocatedVertexCount &&
        BatchesNeeded <= RenderContext->AllocatedBatchCount)
    {
        return;
    }

    render_batch Open = {};
    if (RenderContext->BatchOpen)
    {
        Open = RenderContext->AllocatedBatches[RenderContext->BatchCount];
        if (Open.VertexCount > 0)
        {
            RenderContext->BatchCount++;
        }
    }
    RenderFlush(RenderContext);
#if APP_DEV
    RenderContext->Began = true;
#endif
    RenderContext->BatchCount = 0;
    RenderContext->VertexCount = 0;
    if (RenderContext->BatchOpen)
    {
        Open.Verticies = RenderContext->AllocatedVerticies;
        Open.VertexCount = 0;
        RenderContext->AllocatedBatches[0] = Open;
    }
}
