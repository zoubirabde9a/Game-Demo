/* Ground cells: how one cell of the terrain atlas lands on screen for the
   tile pass (draw_tilemap.cpp). Holds the per-frame grid of what the pass
   reads around the visible tiles (heights, and a world tint at every tile
   corner), the atlas UVs, which cell a tile shows, and the tinted quad.

   The world tint is what keeps a field of one kind from reading as a grid
   of squares: two slow noise fields over the world, sampled at tile
   corners and blended across each quad by its vertex colours, make broad
   patches lighter and darker, warmer and cooler, that run across tiles.
   Liquids get a slow shimmer rolling over them instead, and runes a
   pulse. Client only; the noise is the terrain's integer noise, so every
   client paints the same patches. */

// NOTE(zoubir): what the ground pass reads once a frame: heights of the
// tiles drawn plus one on each side, and the tint fields at their corners
struct ground_grid
{
    i32 MinX;
    i32 MinY;
    i32 Width;
    i32 Height;
    u8 *Steps;
    // NOTE(zoubir): (Width + 1) x (Height + 1) corners, 0..255
    u8 *Shade;
    u8 *Warmth;
    float Seconds;
};

internal ground_grid
ReadGroundGrid(memory_arena *Arena, map_def *Map, i32 MinX, i32 MinY,
               i32 MaxX, i32 MaxY, float Seconds)
{
    ground_grid Grid;
    Grid.MinX = MinX - 1;
    Grid.MinY = MinY - 1;
    Grid.Width = MaxX - MinX + 3;
    Grid.Height = MaxY - MinY + 3;
    Grid.Seconds = Seconds;
    Grid.Steps = AllocateArray(Arena, (memory_index)(Grid.Width * Grid.Height), u8);
    for(i32 Y = 0; Y < Grid.Height; Y++)
    {
        for(i32 X = 0; X < Grid.Width; X++)
        {
            Grid.Steps[Y * Grid.Width + X] =
                (u8)ElevationAt(Map, Grid.MinX + X, Grid.MinY + Y);
        }
    }
    memory_index Corners = (memory_index)((Grid.Width + 1) * (Grid.Height + 1));
    Grid.Shade = AllocateArray(Arena, Corners, u8);
    Grid.Warmth = AllocateArray(Arena, Corners, u8);
    for(i32 Y = 0; Y <= Grid.Height; Y++)
    {
        for(i32 X = 0; X <= Grid.Width; X++)
        {
            i32 WorldX = Grid.MinX + X;
            i32 WorldY = Grid.MinY + Y;
            i32 Index = Y * (Grid.Width + 1) + X;
            Grid.Shade[Index] = (u8)(FractalNoise(0x5ADEu, WorldX, WorldY, 6, 2) >> (NOISE_SHIFT - 8));
            Grid.Warmth[Index] = (u8)(FractalNoise(0xFA11u, WorldX, WorldY, 11, 2) >> (NOISE_SHIFT - 8));
        }
    }
    return Grid;
}

inline i32
GridSteps(ground_grid *Grid, i32 X, i32 Y)
{
    i32 GX = X - Grid->MinX;
    i32 GY = Y - Grid->MinY;
    Assert(GX >= 0 && GY >= 0 && GX < Grid->Width && GY < Grid->Height);
    i32 Result = Grid->Steps[GY * Grid->Width + GX];
    return Result;
}

// NOTE(zoubir): UVs of one cell of the terrain atlas. The tile pass counts
// atlas rows from the bottom of the texture, unlike sprites, so the row
// index runs in reverse; each cell's own pixels are already upright
inline v4
TerrainAtlasUvs(loaded_texture *Texture, u32 Kind, u32 Column)
{
    u32 Row = TerrainKind_Count - 1 - Kind;
    v4 Result = GetTextureUvsFromIndex(Texture->Width, Texture->Height,
                                       TERRAIN_ATLAS_COLUMNS, TerrainKind_Count,
                                       Row * TERRAIN_ATLAS_COLUMNS + Column);
    // NOTE(zoubir): a hair inside the cell, so rounding at a quad's edge
    // never samples the next cell (a see-through spill cell showed as a
    // dark line down the tile)
    float InsetU = 0.05f / (float)Texture->Width;
    float InsetV = 0.05f / (float)Texture->Height;
    Result.X += InsetU;
    Result.Z -= InsetU;
    Result.Y += InsetV;
    Result.W -= InsetV;
    return Result;
}

// NOTE(zoubir): a textured quad with its own colour at each corner, which
// the texture is multiplied by: top-left, top-right, bottom-right,
// bottom-left
internal void
RenderTintedQuad(render_context *RenderContext, float X, float Y, float Width,
                 float Height, v4 Uvs, u32 TopLeft, u32 TopRight,
                 u32 BottomRight, u32 BottomLeft)
{
    RenderMakeRoom(RenderContext, 6, false);
    RenderVertex(RenderContext, X, Y + Height, 0.f, BottomLeft, Uvs.X, Uvs.Y);
    RenderVertex(RenderContext, X + Width, Y + Height, 0.f, BottomRight, Uvs.Z, Uvs.Y);
    RenderVertex(RenderContext, X + Width, Y, 0.f, TopRight, Uvs.Z, Uvs.W);
    RenderVertex(RenderContext, X + Width, Y, 0.f, TopRight, Uvs.Z, Uvs.W);
    RenderVertex(RenderContext, X, Y, 0.f, TopLeft, Uvs.X, Uvs.W);
    RenderVertex(RenderContext, X, Y + Height, 0.f, BottomLeft, Uvs.X, Uvs.Y);
    if (RenderContext->RendererType == RENDERER_TYPE_BATCH)
    {
        RenderContext->AllocatedBatches[RenderContext->BatchCount].VertexCount += 6;
    }
}

// NOTE(zoubir): part of one atlas cell, Left/Top/Width/Height in the
// cell's pixels from its top-left, drawn as a quad of the same size in
// world units at X, Y (camera already taken off), its corners tinted
// top-left, top-right, bottom-right, bottom-left. A negative Width takes
// the piece from the cell's right side and mirrors it
internal void
DrawTintedAtlasPiece(render_context *RenderContext, loaded_texture *Texture,
                     u32 Kind, u32 Column, float Left, float Top, float Width,
                     float Height, float X, float Y, u32 TopLeft, u32 TopRight,
                     u32 BottomRight, u32 BottomLeft)
{
    v4 Cell = TerrainAtlasUvs(Texture, Kind, Column);
    float Pixels = (float)TERRAIN_TILE_PIXELS;
    float U0 = Cell.X + (Cell.Z - Cell.X) * (Left / Pixels);
    float U1 = Cell.X + (Cell.Z - Cell.X) * ((Left + Absolute(Width)) / Pixels);
    // NOTE(zoubir): Cell.W is the cell's top row, Cell.Y its bottom
    float VTop = Cell.W - (Cell.W - Cell.Y) * (Top / Pixels);
    float VBottom = Cell.W - (Cell.W - Cell.Y) * ((Top + Height) / Pixels);
    v4 Uvs = Width < 0.f ? V4(U1, VBottom, U0, VTop) : V4(U0, VBottom, U1, VTop);
    RenderTintedQuad(RenderContext, X, Y, Absolute(Width), Height, Uvs,
                     TopLeft, TopRight, BottomRight, BottomLeft);
}

inline void
DrawAtlasPiece(render_context *RenderContext, loaded_texture *Texture,
               u32 Kind, u32 Column, float Left, float Top, float Width,
               float Height, float X, float Y)
{
    DrawTintedAtlasPiece(RenderContext, Texture, Kind, Column, Left, Top, Width,
                         Height, X, Y, RGBA8_WHITE, RGBA8_WHITE, RGBA8_WHITE,
                         RGBA8_WHITE);
}

// NOTE(zoubir): a grey of Level (0..1), opaque
inline u32
TintGrey(float Level)
{
    u32 Value = (u32)(ArtClamp01(Level) * 255.f + 0.5f);
    u32 Result = ART_RGB(Value, Value, Value);
    return Result;
}

// NOTE(zoubir): how much of the world tint each kind takes, in
// terrain_kind order. Liquids, runes and pits are lit their own way
global_variable float GroundTintStrength[TerrainKind_Count] =
{
    1.f,   // grass
    0.9f,  // dirt
    0.8f,  // mud
    0.f,   // shallow water
    0.f,   // deep water
    0.9f,  // rock
    1.f,   // ash
    0.8f,  // basalt
    0.7f,  // basalt wall
    0.f,   // lava
    0.7f,  // snow
    0.6f,  // ice
    0.6f,  // stone floor
    0.5f,  // stone wall
    0.f,   // pit
    0.f,   // spring
    0.8f,  // bramble
    0.8f,  // bog
    0.f,   // rune
};

// NOTE(zoubir): the colour a kind's cell is multiplied by at world corner
// X, Y (a tile's top-left corner has the tile's coordinates)
internal u32
GroundCornerTint(ground_grid *Grid, u32 Kind, i32 X, i32 Y)
{
    i32 Index = (Y - Grid->MinY) * (Grid->Width + 1) + (X - Grid->MinX);
    float Shade = (float)Grid->Shade[Index] / 255.f;
    float Warmth = (float)Grid->Warmth[Index] / 255.f;
    float Wave = 0.5f + 0.5f * Sin(1.6f * Grid->Seconds + 0.8f * (float)X + 0.55f * (float)Y);
    float R = 1.f;
    float G = 1.f;
    float B = 1.f;
    switch(Kind)
    {
        case TerrainKind_ShallowWater:
        case TerrainKind_DeepWater:
        case TerrainKind_Spring:
        {
            R = G = B = 0.9f + 0.1f * Wave - 0.06f * Shade;
        } break;

        case TerrainKind_Lava:
        {
            G = 0.78f + 0.22f * Wave;
            B = 0.7f + 0.3f * Wave;
        } break;

        case TerrainKind_Rune:
        {
            float Pulse = 0.5f + 0.5f * Sin(2.4f * Grid->Seconds);
            R = G = 0.8f + 0.2f * Pulse;
            B = 0.9f + 0.1f * Pulse;
        } break;

        default:
        {
            float Strength = GroundTintStrength[Kind];
            float Light = 1.f - Strength * (0.02f + 0.24f * Shade);
            // NOTE(zoubir): warm patches lose some blue and a little
            // green, cool ones some red
            R = Light * (1.f - Strength * 0.08f * (1.f - Warmth));
            G = Light * (1.f - Strength * 0.04f * Warmth);
            B = Light * (1.f - Strength * 0.12f * Warmth);
        } break;
    }
    u32 Result = ART_RGB((u32)(ArtClamp01(R) * 255.f), (u32)(ArtClamp01(G) * 255.f),
                         (u32)(ArtClamp01(B) * 255.f));
    return Result;
}

inline bool32
IsRuneAt(map_def *Map, i32 X, i32 Y)
{
    bool32 Result = TerrainAt(Map, X, Y) == TerrainKind_Rune;
    return Result;
}

// NOTE(zoubir): a rune in a 2 x 2 patch shows its quarter of the big
// glyph; any other rune one of the small glyphs
internal u32
PickRuneColumn(map_def *Map, i32 TileX, i32 TileY, u32 Roll)
{
    bool32 West = IsRuneAt(Map, TileX - 1, TileY);
    bool32 East = IsRuneAt(Map, TileX + 1, TileY);
    bool32 North = IsRuneAt(Map, TileX, TileY - 1);
    bool32 South = IsRuneAt(Map, TileX, TileY + 1);
    i32 QuarterX = (East && !West) ? 0 : (West && !East) ? 1 : -1;
    i32 QuarterY = (South && !North) ? 0 : (North && !South) ? 1 : -1;
    if (QuarterX >= 0 && QuarterY >= 0 &&
        IsRuneAt(Map, TileX + 1 - 2 * QuarterX, TileY + 1 - 2 * QuarterY))
    {
        return TerrainCellColumn((u32)(QuarterX + 2 * QuarterY));
    }
    u32 Result = TerrainCellColumn(5 + Roll % 3);
    return Result;
}

// NOTE(zoubir): the atlas column a tile shows: animated kinds play the
// shared frame of one of four layouts, runes join into glyphs, the rest
// pick one of sixteen variants
internal u32
PickTerrainColumn(map_def *Map, u32 Kind, i32 TileX, i32 TileY, u32 Frame)
{
    u32 Roll = HashLattice(0x7E44u, TileX, TileY) >> 8;
    if (IsTerrainAnimated((terrain_kind)Kind))
    {
        return TerrainCellColumn((Roll % 4) * TERRAIN_VARIANTS + Frame);
    }
    if (Kind == TerrainKind_Rune)
    {
        return PickRuneColumn(Map, TileX, TileY, Roll);
    }
    u32 Result = TerrainCellColumn(Roll % TERRAIN_CELLS);
    return Result;
}

// NOTE(zoubir): a whole cell over one tile, Lift up the screen, tinted at
// its corners for the kind it shows
internal void
DrawGroundCell(render_context *RenderContext, world *World, ground_grid *Grid,
               loaded_texture *Texture, i32 TileX, i32 TileY, u32 Kind,
               u32 Column, float Lift, v3 CameraOffset)
{
    RenderTintedQuad(RenderContext,
                     (float)TileX * World->TileWidth - CameraOffset.X,
                     (float)TileY * World->TileHeight - CameraOffset.Y - Lift,
                     (float)World->TileWidth, (float)World->TileHeight,
                     TerrainAtlasUvs(Texture, Kind, Column),
                     GroundCornerTint(Grid, Kind, TileX, TileY),
                     GroundCornerTint(Grid, Kind, TileX + 1, TileY),
                     GroundCornerTint(Grid, Kind, TileX + 1, TileY + 1),
                     GroundCornerTint(Grid, Kind, TileX, TileY + 1));
}
