/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */

#include "render.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
internal font *
CreateFont(open_gl *OpenGL,
           memory_arena *Arena,
              float FontSize,
              int BitmapWidth, int BitmapHeight,
              char *FilePath)
{
    // TODO(zoubir): check if the memory is zeroed
    font *Font = AllocateStruct(Arena, font);

    Font->FirstGlyph = 32;
    Font->GlyphsSize = 96;
    local_persist stbtt_bakedchar c[96];
    // TODO(zoubir): IMPORTANT(zoubir): what is this +16
    Font->Glyphs = //malloc(sizeof(stbtt_bakedchar) * 98);
    AllocateArray(Arena, Font->GlyphsSize + 16, stbtt_bakedchar);
    
    Font->BitmapWidth = BitmapWidth;
    Font->BitmapHeight = BitmapHeight;
    int BitmapSize = BitmapWidth * BitmapHeight;

    //TODO(zoubir): make this arena readfile
    debug_read_file_result ReadResult =
        Platform.ReadEntireFile(FilePath);
    temporary_memory TemporaryMemory = BeginTemporaryMemory(Arena);
    void *BitmapMemory = AllocateSize(Arena, BitmapSize);
    
    stbtt_BakeFontBitmap((const unsigned char *)ReadResult.Memory, 0, FontSize,
                         (unsigned char *)BitmapMemory,
                         Font->BitmapWidth, Font->BitmapHeight,
                         Font->FirstGlyph, Font->GlyphsSize,
                         (stbtt_bakedchar *)Font->Glyphs); // no guarantee this fits!
    float MinY = 0.f;
    float MaxY = 0.f;
    for(u32 GlyphIndex = Font->FirstGlyph;
        GlyphIndex < Font->FirstGlyph + Font->GlyphsSize;
        GlyphIndex++)
    {
        int opengl_fillrule = 1;
        float d3d_bias = opengl_fillrule ? 0 : -0.5f;
        stbtt_bakedchar *b = &((stbtt_bakedchar *)Font->Glyphs)[GlyphIndex];
        int round_y = STBTT_ifloor(b->yoff + 0.5f);
        float Y0 = round_y + d3d_bias;
        float Y1 = round_y + b->y1 - b->y0 + d3d_bias;

        MinY = (Y0 < MinY) ? Y0 : MinY;
        MaxY = (Y1 > MaxY) ? Y1 : MaxY;
        
    }
    Font->UpperLimit = -MinY;
    Font->LowerLimit = MaxY;
    
    Platform.FreeFileMemory(ReadResult.Memory);
    OpenGL->glGenTextures(1, &Font->Texture);
    OpenGL->glBindTexture(GL_TEXTURE_2D, Font->Texture);
    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, Font->BitmapWidth,
                 Font->BitmapHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
                 BitmapMemory);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EndTemporaryMemory(TemporaryMemory);
    
    return Font;
}

inline v4
ClipRectangle(float X, float Y, float Width, float Height,
              float ClipX, float ClipY, float ClipWidth,
              float ClipHeight)
{
    v4 Result;
    
    if (X < ClipX)
    {
        Result.X = ClipX;
    }
    else
    {
        Result.X = X;
    }
    
    if (Y < ClipY)
    {
        Result.Y = ClipY;
    }
    else
    {
        Result.Y = Y;
    }

    if (X + Width > ClipX + ClipWidth)
    {
        Result.Z = ClipX + ClipWidth - Result.X;
    }
    else
    {
        Result.Z = Width - (Result.X - X);
    }

    if (Y + Height > ClipY + ClipHeight)
    {
        Result.W = ClipY + ClipHeight - Result.Y;
    }
    else
    {
        Result.W = Height - (Result.Y - Y);
    }

    return Result;
}

inline void
RenderProgramUse(render_context *RenderContext,
                 render_program *Program)
{
    open_gl *OpenGL = RenderContext->OpenGL;
    if (!RenderContext->AProgramIsUsed ||
        RenderContext->LastUsedProgramID != Program->ID)
    {
        OpenGL->glUseProgram(Program->ID);
        for(u32 AttribIndex = 0;
            AttribIndex < Program->NumAttrib;
            AttribIndex++)
        {
            OpenGL->glEnableVertexAttribArray(AttribIndex);
        }
        
        i32 MatrixLocation = OpenGL->glGetUniformLocation(Program->ID, "P");
        OpenGL->glUniformMatrix4fv(MatrixLocation, 1, GL_FALSE, Program->ProjectionMatrix->Data);
        
        RenderContext->AProgramIsUsed = true;
        RenderContext->LastUsedProgramID = Program->ID;
    }
        
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
                }
            }            
#endif
        }
    };

    // TODO(zoubir): have a draw call for each different
    // texture and program
}

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

internal void
DrawRectangle3D(render_context *RenderContext, float X, float Y,
                float Width, float Height, float Depth,
                u32 Color, float SortingValue)
{
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
    Assert(RenderContext->BatchCount < RenderContext->AllocatedBatchCount);
    
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
    RenderContext->BatchCount++;
}

enum text_justification
{
    TEXT_JUSTIFICATION_LEFT,
    TEXT_JUSTIFICATION_RIGHT,
    TEXT_JUSTIFICATION_MIDDLE
};

internal float
GetTextWidth(font *Font, char *Text)
{
    float Result = 0.f;
    
    stbtt_bakedchar *Glyphs = (stbtt_bakedchar *)Font->Glyphs;
    u32 FirstCharacter = Font->FirstGlyph;
    u32 LastCharacter = Font->FirstGlyph + Font->GlyphsSize;
    while(*Text)
    {
        u32 CurrentCode = (u32)(*Text);
        if (CurrentCode >= FirstCharacter && CurrentCode < LastCharacter) {
            Result += Glyphs[CurrentCode - FirstCharacter].xadvance;
        }
        Text++;
    }
    
    return Result;
}

internal float
GetCharacterWidth(font *Font, char Character)
{
    float Result = 0.f;
    
    stbtt_bakedchar *Glyphs = (stbtt_bakedchar *)Font->Glyphs;
    u32 FirstCharacter = Font->FirstGlyph;
    u32 LastCharacter = Font->FirstGlyph + Font->GlyphsSize;
    u32 CurrentCode = (u32)Character;
    if (CurrentCode >= FirstCharacter && CurrentCode < LastCharacter) {
        Result += Glyphs[CurrentCode - FirstCharacter].xadvance;
    }
    
    return Result;
}

internal void
RenderText(render_context *RenderContext, float X, float Y,
           font *Font, render_program Program, char *Text, u32 Color,
           float ScaleX, float ScaleY, v4 Clip, float Depth)
{
    Assert(Font);
    Assert(Text);
    render_vertex *Verticies =
        &RenderContext->AllocatedVerticies[RenderContext->VertexCount];

    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
    
        BeginBatch(RenderContext, Font->Texture, 0.f, Program);
        u32 FirstCharacter = Font->FirstGlyph;
        u32 LastCharacter = Font->FirstGlyph + Font->GlyphsSize;
        float RelativeX = 0.f;
        float RelativeY = -Font->LowerLimit * ScaleY;
        float CumulativeWidth = 0.f;
        while (*Text) {
            if ((u32)(*Text) >= FirstCharacter && (u32)(*Text) < LastCharacter) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad((stbtt_bakedchar *)Font->Glyphs,
                                   Font->BitmapWidth,
                                   Font->BitmapHeight,
                                   *Text-32, &RelativeX,
                                   &RelativeY, &q, 1);//1=opengl & d3d10+,0=d3d9
                float QuadX = q.x0 * ScaleX + X;
                float QuadY = q.y0 * ScaleY + Y;
                float QuadWidth = (q.x1 - q.x0) * ScaleX;
                float QuadHeight = (q.y1 - q.y0) * ScaleY;
                float UX = q.s0;
                float UY = 1.f - q.t0;
                float TW = q.s1;
                float TH = 1.f - q.t1;
//                CumulativeWidth += ?;
                v4 DrawRect =
                    ClipRectangle(QuadX, QuadY, QuadWidth, QuadHeight,
                                  Clip.X, Clip.Y, Clip.Z, Clip.W);
                if (QuadX >= Clip.X && QuadY >= Clip.Y &&
                    QuadX + QuadWidth < Clip.X + Clip.Z &&
                    QuadY + QuadHeight < Clip.Y + Clip.W)
                {
                    RenderGlyph(RenderContext, DrawRect.X, DrawRect.Y,
                                DrawRect.Z, DrawRect.W,
                                UX, UY, TW, TH, RGBA8_WHITE, 1.f);
                }
                #if 0
                if (DrawRect.Z > 0 && DrawRect.W > 0)
                {
                    RenderGlyph(RenderContext, DrawRect.X, DrawRect.Y,
                                DrawRect.Z, DrawRect.W,
                                UX, UY, TW, TH, RGBA8_WHITE, 1.f);
                }
                #endif
            }
            ++Text;
        }
        EndBatch(RenderContext);
    }
}

inline void
RenderText(render_context *RenderContext, float X, float Y,
           float Width, float Height, v4 Clip, font *Font, render_program Program,
           char *Text, u32 Justification, u32 Color, float Depth)
{    
    Y += Height;
    float TextWidth = GetTextWidth(Font, Text);
    float FontHeight = Font->UpperLimit + Font->LowerLimit;
    #if 0
    float ScaleX = Width / TextWidth;
    float ScaleY = Height / FontHeight;
    #else
    float ScaleX = 1.f;
    float ScaleY = 1.f;
    #endif
    
    if (ScaleX > 1.f)
    {
        ScaleX = 1.f;
    }
    if (ScaleY > 1.f)
    {
        ScaleY = 1.f;
    }

    float HalfWidth = (Width / 2.f);
    float HalfTextWidth = (TextWidth / 2.f) * ScaleX;
    
    switch(Justification)
    {
        case TEXT_JUSTIFICATION_MIDDLE:
        {
            X = X + HalfWidth - HalfTextWidth;
            break;
        }
    }
    
    RenderText(RenderContext, X, Y, Font, Program, Text, Color,
               ScaleX, ScaleY, Clip, Depth);
    
}
