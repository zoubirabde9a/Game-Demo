/* Adding to a pass: single vertices, textured quads (straight or
   rotated), glyph quads, outlined and filled rectangles, and opening and
   closing a textured batch. */

inline void
RenderVertex(render_context *RenderContext, render_vertex *Vertex)
{
    Assert(RenderContext->VertexCount < RenderContext->AllocatedVertexCount);
    //NOTE(zoubir) to debug see assembly
    RenderContext->AllocatedVerticies[RenderContext->VertexCount++] =
        *Vertex;
}

inline void
RenderVertex(render_context *RenderContext,
             float X, float Y, float Z,
             u32 Color, float U, float V)
{
    render_vertex *CurrentVertex =
        RenderContext->AllocatedVerticies + RenderContext->VertexCount;
    CurrentVertex->X = X;
    CurrentVertex->Y = Y;
    CurrentVertex->Z = Z;
    CurrentVertex->Color = Color;
    CurrentVertex->U = U;
    CurrentVertex->V = V;
    RenderContext->VertexCount++;
}

inline void
RenderVertex(render_context *RenderContext,
             float X, float Y, float Z, u32 Color)
{
    render_vertex *CurrentVertex =
        RenderContext->AllocatedVerticies + RenderContext->VertexCount;
    CurrentVertex->X = X;
    CurrentVertex->Y = Y;
    CurrentVertex->Z = Z;
    CurrentVertex->Color = Color;
    CurrentVertex->U = X;
    CurrentVertex->V = Y;
    RenderContext->VertexCount++;
}

internal void
RenderQuadTexture(render_context *RenderContext, float X, float Y,
                  float Width, float Height, v4 Uvs,
                  u32 Color, float Depth)
{
    RenderMakeRoom(RenderContext, 6, false);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];

    // Bottom Left
    RenderVertex(RenderContext, X, Y + Height, Depth, Color, Uvs.X, Uvs.Y);
    // Bottom Right
    RenderVertex(RenderContext, X + Width, Y + Height, Depth, Color, Uvs.Z, Uvs.Y);
    // Top Right
    RenderVertex(RenderContext, X + Width, Y, Depth, Color, Uvs.Z, Uvs.W);
    // Top Right
    RenderVertex(RenderContext, X + Width, Y, Depth, Color, Uvs.Z, Uvs.W);
    // Top Left
    RenderVertex(RenderContext, X, Y, Depth, Color, Uvs.X, Uvs.W);
    // Bottom Left
    RenderVertex(RenderContext, X, Y + Height, Depth, Color, Uvs.X, Uvs.Y);
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        render_batch *NewBatch =
            &RenderContext->AllocatedBatches[RenderContext->BatchCount];
        NewBatch->VertexCount += 6;
        int EndHere = 4;
    }
}

inline v2
RotatePoint(v2 Point, float Angle)
{
    v2 Result;
    
    float SinValue = Sin(Angle);
    float CosValue = Cos(Angle);

    Result.X = (Point.X * CosValue) - (Point.Y * SinValue);
    Result.Y = (Point.X * SinValue) + (Point.Y * CosValue);

    return Result;
}

internal void
RenderQuadTexture(render_context *RenderContext, float X, float Y,
                  float Width, float Height, v4 Uvs,
                  u32 Color, float Depth, float Angle)
{
    RenderMakeRoom(RenderContext, 6, false);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];

    v2 HalfDims = {Width * 0.5f, Height * 0.5f};
    
    v2 BottomLeft = {-HalfDims.X, HalfDims.Y};
    v2 BottomRight = {HalfDims.X, HalfDims.Y};
    v2 TopRight = {HalfDims.X, -HalfDims.Y};
    v2 TopLeft = {-HalfDims.X, -HalfDims.Y};

    BottomLeft = RotatePoint(BottomLeft, Angle);
    BottomRight = RotatePoint(BottomRight, Angle);
    TopRight = RotatePoint(TopRight, Angle);
    TopLeft = RotatePoint(TopLeft, Angle);
    
    v2 Offset = HalfDims + V2(X, Y);
    BottomLeft += Offset;
    BottomRight += Offset;
    TopRight += Offset;
    TopLeft += Offset;
    
    // Bottom Left
    RenderVertex(RenderContext, BottomLeft.X, BottomLeft.Y, Depth, Color, Uvs.X, Uvs.Y);
    // Bottom Right
    RenderVertex(RenderContext, BottomRight.X, BottomRight.Y, Depth, Color, Uvs.Z, Uvs.Y);
    // Top Right
    RenderVertex(RenderContext, TopRight.X, TopRight.Y, Depth, Color, Uvs.Z, Uvs.W);
    // Top Right
    RenderVertex(RenderContext, TopRight.X, TopRight.Y, Depth, Color, Uvs.Z, Uvs.W);
    // Top Left
    RenderVertex(RenderContext, TopLeft.X, TopLeft.Y, Depth, Color, Uvs.X, Uvs.W);
    // Bottom Left
    RenderVertex(RenderContext, BottomLeft.X, BottomLeft.Y, Depth, Color, Uvs.X, Uvs.Y);
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        render_batch *NewBatch =
            &RenderContext->AllocatedBatches[RenderContext->BatchCount];
        NewBatch->VertexCount += 6;
        int EndHere = 4;
    }
}

internal void
RenderGlyph(render_context *RenderContext, float X, float Y, float Width,
              float Height, float UX, float VX, float UY, float VY,
                  u32 Color, float Depth)
{
    RenderMakeRoom(RenderContext, 6, false);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];

    // Top Right
    RenderVertex(RenderContext, X + Width, Y + Height, Depth, Color, UY, VY);
    // Top Left
    RenderVertex(RenderContext, X, Y + Height, Depth, Color, UX, VY);
    // Bottom Left
    RenderVertex(RenderContext, X, Y, Depth, Color, UX, VX);
    // Bottom Left
    RenderVertex(RenderContext, X, Y, Depth, Color, UX, VX);
    // Bottom Right
    RenderVertex(RenderContext, X + Width, Y, Depth, Color, UY, VX);
    // Top Right
    RenderVertex(RenderContext, X + Width, Y + Height, Depth, Color, UY, VY);
    render_batch *NewBatch =
        &RenderContext->AllocatedBatches[RenderContext->BatchCount];
    NewBatch->VertexCount += 6;
}

internal void
DrawRectangle(render_context *RenderContext, float X, float Y,
              float Width, float Height, u32 Color, float SortingValue)
{
    RenderMakeRoom(RenderContext, 4, true);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        float Depth = 0.f;
        render_batch *NewBatch =
            &RenderContext->AllocatedBatches[RenderContext->BatchCount];
        NewBatch->Verticies = Verticies;
        NewBatch->SortingValue = SortingValue;
        NewBatch->Program = RenderContext->LineProgram;
        NewBatch->Type = RENDER_BATCH_TYPE_RECTANGLE;
        RenderVertex(RenderContext, X, Y, Depth, Color);
        RenderVertex(RenderContext, X + Width, Y, Depth, Color);
        RenderVertex(RenderContext, X + Width, Y + Height, Depth, Color);
        RenderVertex(RenderContext, X, Y + Height, Depth, Color);
        NewBatch->VertexCount = 4;
        RenderContext->BatchCount++;
    }
}

// NOTE(zoubir): same as DrawRectangle, but filled instead of outlined
internal void
DrawFilledRectangle(render_context *RenderContext, float X, float Y,
                    float Width, float Height, u32 Color, float SortingValue)
{
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        DrawRectangle(RenderContext, X, Y, Width, Height,
                      Color, SortingValue);
        // NOTE(zoubir): read after drawing, which may have flushed
        RenderContext->AllocatedBatches[RenderContext->BatchCount - 1].Type =
            RENDER_BATCH_TYPE_FILLED_RECTANGLE;
    }
}

internal void
DrawRectangle3D(render_context *RenderContext, float X, float Y,
                float Width, float Height, float Depth,
                u32 Color, float SortingValue)
{
    RenderMakeRoom(RenderContext, 13, true);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        render_batch *NewBatch =
            &RenderContext->AllocatedBatches[RenderContext->BatchCount];
        NewBatch->Verticies = Verticies;
        NewBatch->SortingValue = SortingValue;
        NewBatch->Program = RenderContext->LineProgram;
        NewBatch->Type = RENDER_BATCH_TYPE_RECTANGLE;
        
        RenderVertex(RenderContext, X, Y, 0.f, Color);
        RenderVertex(RenderContext, X + Width, Y, 0.f, Color);
        
        RenderVertex(RenderContext, X + Width, Y + Height, 0.f, Color);
        RenderVertex(RenderContext, X, Y + Height, 0.f, Color);
        
        RenderVertex(RenderContext, X, Y - Depth, Depth, Color);
        RenderVertex(RenderContext, X + Width, Y - Depth, 0.f, Color);
        
        RenderVertex(RenderContext, X + Width, Y + Height - Depth, 0.f, Color);
        RenderVertex(RenderContext, X, Y + Height - Depth, 0.f, Color);
        
        RenderVertex(RenderContext, X, Y + Height, 0.f, Color);
        RenderVertex(RenderContext, X, Y, 0.f, Color);
        
        RenderVertex(RenderContext, X, Y - Depth, 0.f, Color);
        RenderVertex(RenderContext, X + Width, Y - Depth, 0.f, Color);
        RenderVertex(RenderContext, X + Width, Y, 0.f, Color);
        
        
        NewBatch->VertexCount = 13;
        RenderContext->BatchCount++;
    }
}

//TODO(zoubir): idea mb make this return a struct
// so that we can have multiple begin betches and
// end batches
// and also each batch will have its own
// memory;

inline void
BeginBatch(render_context *RenderContext, u32 Texture, float SortingValue,
           render_program Program)
{
    Assert(RenderContext->RendererType == RENDERER_TYPE_BATCH);
    Assert(!RenderContext->BatchOpen);
    RenderMakeRoom(RenderContext, 0, true);
    RenderContext->BatchOpen = true;

    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];
    render_batch *NewBatch =
        &RenderContext->AllocatedBatches[RenderContext->BatchCount];
    NewBatch->Verticies = Verticies;
    NewBatch->TextureID = Texture;
    NewBatch->SortingValue = SortingValue;
    NewBatch->VertexCount = 0;
    NewBatch->Program = Program;
    NewBatch->Type = RENDER_BATCH_TYPE_TEXTURE;
}

inline void
EndBatch(render_context *RenderContext)
{
    Assert(RenderContext->RendererType == RENDERER_TYPE_BATCH);
    RenderContext->BatchOpen = false;
    RenderContext->BatchCount++;
}
