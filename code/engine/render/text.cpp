/* Fonts and text: baking a TrueType font into a texture (0 when the
   file is missing), measuring text, and drawing a line of it clipped to
   a box (left or centred). */

internal font *
CreateFont(open_gl *OpenGL,
           memory_arena *Arena,
              float FontSize,
              int BitmapWidth, int BitmapHeight,
              char *FilePath)
{
    //TODO(zoubir): make this arena readfile
    debug_read_file_result ReadResult =
        Platform.ReadEntireFile(FilePath);
    if (!ReadResult.Memory)
    {
        return 0;
    }

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

    temporary_memory TemporaryMemory = BeginTemporaryMemory(Arena);
    void *BitmapMemory = AllocateSize(Arena, BitmapSize);
    
    stbtt_BakeFontBitmap((const unsigned char *)ReadResult.Memory, 0, FontSize,
                         (unsigned char *)BitmapMemory,
                         Font->BitmapWidth, Font->BitmapHeight,
                         Font->FirstGlyph, Font->GlyphsSize,
                         (stbtt_bakedchar *)Font->Glyphs); // no guarantee this fits!
    float MinY = 0.f;
    float MaxY = 0.f;
    // NOTE(zoubir): Glyphs holds GlyphsSize entries starting at FirstGlyph
    for(u32 GlyphIndex = 0;
        GlyphIndex < Font->GlyphsSize;
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
    // NOTE(zoubir): GL_ALPHA textures read as opaque black in the core
    // shaders, so glyphs go up as white RGBA with coverage in alpha
    u8 *Coverage = (u8 *)BitmapMemory;
    // NOTE(zoubir): 1MB scratch, too big for what is left of the arena
    u32 *Pixels = (u32 *)malloc(BitmapSize * sizeof(u32));
    for(int PixelIndex = 0;
        PixelIndex < BitmapSize;
        PixelIndex++)
    {
        Pixels[PixelIndex] = ((u32)Coverage[PixelIndex] << 24) | 0x00FFFFFF;
    }
    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Font->BitmapWidth,
                 Font->BitmapHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 Pixels);
    free(Pixels);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    EndTemporaryMemory(TemporaryMemory);
    
    return Font;
}

// NOTE(zoubir): the first path that loads wins; 0 when none does
internal font *
CreateFirstFont(open_gl *OpenGL, memory_arena *Arena, float FontSize,
                char **Paths, u32 PathCount)
{
    font *Result = 0;
    for(u32 PathIndex = 0; !Result && PathIndex < PathCount; PathIndex++)
    {
        Result = CreateFont(OpenGL, Arena, FontSize, 512, 512,
                            Paths[PathIndex]);
    }
    return Result;
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
                                UX, UY, TW, TH, Color, 1.f);
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
    // NOTE(zoubir): the line is centred in the box. The glyph pass takes
    // the bottom of the line and lifts the baseline by LowerLimit, so
    // tall letters and descenders (g, y) both stay inside the clip
    float TextWidth = GetTextWidth(Font, Text);
    float FontHeight = Font->UpperLimit + Font->LowerLimit;
    Y += 0.5f * (Height + FontHeight);
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
