/* Terrain art: the ground tiles, drawn by code into one atlas texture at
   startup. One row per terrain kind, TERRAIN_ATLAS_COLUMNS tiles per row.
   Solid ground uses the columns as variants (picked per tile by a hash,
   so a field does not repeat); water and lava use them as animation
   frames. Every tile wraps: its left edge continues its right edge and
   its top continues its bottom, so neighbours join without seams. */

#define TERRAIN_TILE_PIXELS 32
#define TERRAIN_ATLAS_COLUMNS 4

// NOTE(zoubir): noise that repeats every TERRAIN_TILE_PIXELS, so a tile's
// edges match the next tile's. 0..255
inline u32
WrappedTileNoise(u32 Seed, i32 X, i32 Y, i32 Cell)
{
    i32 Cells = TERRAIN_TILE_PIXELS / Cell;
    i32 CX = FloorDiv(X, Cell);
    i32 CY = FloorDiv(Y, Cell);
    i32 FX = (FloorMod(X, Cell) << 8) / Cell;
    i32 FY = (FloorMod(Y, Cell) << 8) / Cell;
    i32 A = (i32)(HashLattice(Seed, FloorMod(CX, Cells), FloorMod(CY, Cells)) >> 24);
    i32 B = (i32)(HashLattice(Seed, FloorMod(CX + 1, Cells), FloorMod(CY, Cells)) >> 24);
    i32 C = (i32)(HashLattice(Seed, FloorMod(CX, Cells), FloorMod(CY + 1, Cells)) >> 24);
    i32 D = (i32)(HashLattice(Seed, FloorMod(CX + 1, Cells), FloorMod(CY + 1, Cells)) >> 24);
    i32 Top = A + (((B - A) * FX) >> 8);
    i32 Bottom = C + (((D - C) * FX) >> 8);
    u32 Result = (u32)(Top + (((Bottom - Top) * FY) >> 8));
    return Result;
}

// NOTE(zoubir): two octaves of WrappedTileNoise mapped to 0..1
inline float
TileTexture(u32 Seed, i32 X, i32 Y)
{
    u32 Coarse = WrappedTileNoise(Seed, X, Y, 8);
    u32 Fine = WrappedTileNoise(Seed + 17, X, Y, 4);
    float Result = (float)(2 * Coarse + Fine) / (3.f * 255.f);
    return Result;
}

inline u32
WrapPixel(i32 Value)
{
    u32 Result = (u32)FloorMod(Value, TERRAIN_TILE_PIXELS);
    return Result;
}

// NOTE(zoubir): a pixel that wraps around the tile, for details that may
// sit on an edge
inline void
PutTilePixel(sprite_canvas *Canvas, i32 X, i32 Y, u32 Color)
{
    PutPixel(Canvas, (i32)WrapPixel(X), (i32)WrapPixel(Y), Color);
}

// NOTE(zoubir): fills the tile from a ramp by wrapped noise, with ordered
// dithering between steps; Bias shifts the whole tile lighter or darker
internal void
FillTileGround(sprite_canvas *Canvas, color_ramp Ramp, u32 Seed, float Bias,
               float Contrast)
{
    for(i32 Y = 0; Y < TERRAIN_TILE_PIXELS; Y++)
    {
        for(i32 X = 0; X < TERRAIN_TILE_PIXELS; X++)
        {
            float Value = 0.5f + Contrast * (TileTexture(Seed, X, Y) - 0.5f) + Bias;
            PutPixel(Canvas, X, Y, ShadeRamp(&Ramp, Value, X, Y));
        }
    }
}

internal void
DrawTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Column)
{
    u32 Seed = 1000u * (u32)Kind + 31u * Column;
    // NOTE(zoubir): animated kinds keep the same pattern across frames
    // and move it instead
    float Phase = (float)Column / (float)TERRAIN_ATLAS_COLUMNS;
    switch(Kind)
    {
        case TerrainKind_Grass:
        {
            color_ramp Grass = Ramp(ART_RGB(52, 92, 40), ART_RGB(74, 122, 50),
                                    ART_RGB(96, 148, 58), ART_RGB(132, 176, 74));
            FillTileGround(Canvas, Grass, Seed, 0.f, 1.3f);
            // NOTE(zoubir): blades of grass, and on some tiles a flower
            for(u32 Blade = 0; Blade < 14; Blade++)
            {
                u32 H = HashLattice(Seed, (i32)Blade, 7);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32);
                PutTilePixel(Canvas, X, Y, Grass.C[3]);
                PutTilePixel(Canvas, X, Y + 1, Grass.C[2]);
                PutTilePixel(Canvas, X + 1, Y + 1, Grass.C[0]);
            }
            if (Column == 1 || Column == 3)
            {
                u32 H = HashLattice(Seed, 3, 3);
                i32 X = (i32)(H % 28) + 2;
                i32 Y = (i32)((H >> 8) % 28) + 2;
                u32 Petal = Column == 1 ? ART_RGB(240, 230, 120) : ART_RGB(220, 140, 200);
                PutTilePixel(Canvas, X, Y - 1, Petal);
                PutTilePixel(Canvas, X - 1, Y, Petal);
                PutTilePixel(Canvas, X + 1, Y, Petal);
                PutTilePixel(Canvas, X, Y + 1, Petal);
                PutTilePixel(Canvas, X, Y, ART_RGB(250, 250, 230));
            }
        } break;

        case TerrainKind_Dirt:
        case TerrainKind_Mud:
        {
            bool32 Wet = Kind == TerrainKind_Mud;
            color_ramp Soil = Wet ?
                Ramp(ART_RGB(48, 34, 24), ART_RGB(70, 50, 34),
                     ART_RGB(92, 68, 46), ART_RGB(120, 96, 70)) :
                Ramp(ART_RGB(102, 74, 48), ART_RGB(130, 98, 64),
                     ART_RGB(156, 120, 80), ART_RGB(186, 152, 106));
            FillTileGround(Canvas, Soil, Seed, 0.f, 1.2f);
            // NOTE(zoubir): pebbles on dirt, glints of standing water on mud
            for(u32 Speck = 0; Speck < 8; Speck++)
            {
                u32 H = HashLattice(Seed, (i32)Speck, 11);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32);
                if (Wet)
                {
                    PutTilePixel(Canvas, X, Y, ART_RGB(120, 130, 120));
                    PutTilePixel(Canvas, X + 1, Y, ART_RGB(150, 160, 150));
                }
                else
                {
                    PutTilePixel(Canvas, X, Y, Soil.C[3]);
                    PutTilePixel(Canvas, X, Y + 1, Soil.C[0]);
                }
            }
        } break;

        case TerrainKind_ShallowWater:
        case TerrainKind_DeepWater:
        {
            bool32 Deep = Kind == TerrainKind_DeepWater;
            color_ramp Water = Deep ?
                Ramp(ART_RGB(18, 40, 86), ART_RGB(26, 56, 112),
                     ART_RGB(38, 76, 140), ART_RGB(90, 140, 196)) :
                Ramp(ART_RGB(54, 112, 150), ART_RGB(70, 140, 178),
                     ART_RGB(96, 170, 204), ART_RGB(170, 220, 236));
            FillTileGround(Canvas, Water, 900u + (u32)Kind, -0.05f, 0.9f);
            // NOTE(zoubir): ripple lines drifting across with the frames
            i32 Shift = (i32)(Phase * 32.f);
            for(u32 Ripple = 0; Ripple < 4; Ripple++)
            {
                u32 H = HashLattice(77u + (u32)Kind, (i32)Ripple, 5);
                i32 X = (i32)(H % 32) + Shift;
                i32 Y = (i32)((H >> 8) % 32);
                for(i32 Step = 0; Step < 5; Step++)
                {
                    PutTilePixel(Canvas, X + Step, Y + (Step == 2 ? -1 : 0), Water.C[3]);
                }
            }
        } break;

        case TerrainKind_Rock:
        case TerrainKind_BasaltWall:
        case TerrainKind_StoneWall:
        {
            // NOTE(zoubir): blocking ground reads as raised: lit top face,
            // dark lip along the bottom
            color_ramp Stone =
                Kind == TerrainKind_Rock ?
                Ramp(ART_RGB(70, 68, 66), ART_RGB(102, 98, 94),
                     ART_RGB(134, 130, 124), ART_RGB(170, 166, 158)) :
                Kind == TerrainKind_BasaltWall ?
                Ramp(ART_RGB(16, 14, 18), ART_RGB(30, 26, 32),
                     ART_RGB(48, 42, 50), ART_RGB(74, 66, 74)) :
                Ramp(ART_RGB(54, 50, 56), ART_RGB(78, 74, 80),
                     ART_RGB(104, 100, 104), ART_RGB(136, 132, 134));
            FillTileGround(Canvas, Stone, Seed, 0.05f, 1.1f);
            if (Kind == TerrainKind_StoneWall)
            {
                // NOTE(zoubir): courses of bricks, offset every other row
                for(i32 Y = 0; Y < 32; Y++)
                {
                    for(i32 X = 0; X < 32; X++)
                    {
                        i32 Course = Y / 8;
                        i32 Offset = (Course % 2) ? 8 : 0;
                        if ((Y % 8) == 0 || ((X + Offset) % 16) == 0)
                        {
                            PutPixel(Canvas, X, Y, Stone.C[0]);
                        }
                        else if ((Y % 8) == 1)
                        {
                            PutPixel(Canvas, X, Y, Stone.C[3]);
                        }
                    }
                }
            }
            else
            {
                // NOTE(zoubir): cracks
                for(u32 Crack = 0; Crack < 2; Crack++)
                {
                    u32 H = HashLattice(Seed, (i32)Crack, 13);
                    i32 X = (i32)(H % 32);
                    i32 Y = (i32)((H >> 8) % 32);
                    for(i32 Step = 0; Step < 6; Step++)
                    {
                        PutTilePixel(Canvas, X + Step, Y + (Step / 2) * ((H >> 16) % 2 ? 1 : -1),
                                     Stone.C[0]);
                    }
                }
                if (Kind == TerrainKind_BasaltWall)
                {
                    u32 H = HashLattice(Seed, 9, 9);
                    PutTilePixel(Canvas, (i32)(H % 32), (i32)((H >> 8) % 32),
                                 ART_RGB(220, 90, 30));
                }
            }
        } break;

        case TerrainKind_Ash:
        case TerrainKind_Basalt:
        {
            bool32 IsAsh = Kind == TerrainKind_Ash;
            color_ramp Ground = IsAsh ?
                Ramp(ART_RGB(84, 80, 78), ART_RGB(108, 104, 100),
                     ART_RGB(132, 128, 122), ART_RGB(160, 156, 148)) :
                Ramp(ART_RGB(34, 30, 36), ART_RGB(50, 46, 52),
                     ART_RGB(70, 64, 70), ART_RGB(96, 88, 92));
            FillTileGround(Canvas, Ground, Seed, 0.f, 1.2f);
            for(u32 Fleck = 0; Fleck < 10; Fleck++)
            {
                u32 H = HashLattice(Seed, (i32)Fleck, 17);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32);
                PutTilePixel(Canvas, X, Y, IsAsh ? Ground.C[0] : Ground.C[3]);
            }
            if (IsAsh && Column == 2)
            {
                // NOTE(zoubir): a last ember still glowing in the ash
                u32 H = HashLattice(Seed, 2, 2);
                i32 X = (i32)(H % 30) + 1;
                i32 Y = (i32)((H >> 8) % 30) + 1;
                PutTilePixel(Canvas, X, Y, ART_RGB(250, 140, 40));
                PutTilePixel(Canvas, X + 1, Y, ART_RGB(200, 70, 20));
            }
        } break;

        case TerrainKind_Lava:
        {
            color_ramp Molten = Ramp(ART_RGB(170, 40, 10), ART_RGB(230, 100, 20),
                                     ART_RGB(255, 170, 40), ART_RGB(255, 236, 140));
            color_ramp Crust = Ramp(ART_RGB(30, 14, 12), ART_RGB(56, 24, 18),
                                    ART_RGB(80, 34, 22), ART_RGB(110, 50, 30));
            // NOTE(zoubir): crust plates over glowing melt; the glow pulses
            // through the frames
            float Pulse = 0.1f * Sin(2.f * Pi32 * Phase);
            for(i32 Y = 0; Y < 32; Y++)
            {
                for(i32 X = 0; X < 32; X++)
                {
                    float Value = TileTexture(950u, X, Y);
                    if (Value > 0.56f)
                    {
                        PutPixel(Canvas, X, Y, ShadeRamp(&Crust, (Value - 0.56f) * 3.f, X, Y));
                    }
                    else
                    {
                        float Heat = 0.4f + (0.56f - Value) * 2.f + Pulse;
                        PutPixel(Canvas, X, Y, ShadeRamp(&Molten, Heat, X, Y));
                    }
                }
            }
        } break;

        case TerrainKind_Snow:
        {
            color_ramp Snow = Ramp(ART_RGB(176, 190, 214), ART_RGB(206, 216, 234),
                                   ART_RGB(228, 234, 246), ART_RGB(248, 250, 255));
            FillTileGround(Canvas, Snow, Seed, 0.1f, 0.9f);
            for(u32 Sparkle = 0; Sparkle < 3; Sparkle++)
            {
                u32 H = HashLattice(Seed, (i32)Sparkle, 19);
                PutTilePixel(Canvas, (i32)(H % 32), (i32)((H >> 8) % 32),
                             ART_RGB(255, 255, 255));
            }
        } break;

        case TerrainKind_Ice:
        {
            color_ramp Ice = Ramp(ART_RGB(120, 170, 196), ART_RGB(150, 200, 222),
                                  ART_RGB(180, 222, 238), ART_RGB(226, 244, 252));
            FillTileGround(Canvas, Ice, Seed, 0.f, 0.7f);
            // NOTE(zoubir): glossy diagonal streaks and a crack
            for(i32 Streak = 0; Streak < 2; Streak++)
            {
                i32 Start = (i32)(HashLattice(Seed, Streak, 23) % 32);
                for(i32 Step = 0; Step < 10; Step++)
                {
                    PutTilePixel(Canvas, Start + Step, Start - Step + 16 * Streak, Ice.C[3]);
                }
            }
            u32 H = HashLattice(Seed, 4, 29);
            i32 X = (i32)(H % 32);
            i32 Y = (i32)((H >> 8) % 32);
            for(i32 Step = 0; Step < 7; Step++)
            {
                PutTilePixel(Canvas, X + Step, Y + (Step % 3) - 1, Ice.C[0]);
            }
        } break;

        case TerrainKind_StoneFloor:
        {
            color_ramp Flag = Ramp(ART_RGB(96, 92, 88), ART_RGB(122, 118, 112),
                                   ART_RGB(146, 142, 134), ART_RGB(172, 168, 160));
            FillTileGround(Canvas, Flag, Seed, 0.f, 0.8f);
            // NOTE(zoubir): four flagstones per tile with mortar between
            for(i32 Y = 0; Y < 32; Y++)
            {
                for(i32 X = 0; X < 32; X++)
                {
                    if ((X % 16) == 0 || (Y % 16) == 0)
                    {
                        PutPixel(Canvas, X, Y, Flag.C[0]);
                    }
                    else if ((X % 16) == 1 || (Y % 16) == 1)
                    {
                        PutPixel(Canvas, X, Y, Flag.C[3]);
                    }
                }
            }
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }
}

// NOTE(zoubir): Pixels holds (TERRAIN_ATLAS_COLUMNS * TERRAIN_TILE_PIXELS) x
// (TerrainKind_Count * TERRAIN_TILE_PIXELS) u32s
internal void
BuildTerrainAtlas(u32 *Pixels)
{
    u32 Width = TERRAIN_ATLAS_COLUMNS * TERRAIN_TILE_PIXELS;
    u32 Height = TerrainKind_Count * TERRAIN_TILE_PIXELS;
    ZeroSize(Pixels, Width * Height * sizeof(u32));
    for(u32 Kind = 0; Kind < TerrainKind_Count; Kind++)
    {
        for(u32 Column = 0; Column < TERRAIN_ATLAS_COLUMNS; Column++)
        {
            sprite_canvas Canvas = CanvasFrame(Pixels, Width, TERRAIN_TILE_PIXELS,
                                               Column, Kind);
            DrawTerrainTile(&Canvas, (terrain_kind)Kind, Column);
        }
    }
}

// NOTE(zoubir): props that are not the packed tree: one 48-pixel frame
// each, feet on the 7/8 line like monsters
internal void
DrawTerrainProp(sprite_canvas *Canvas, terrain_prop Prop)
{
    switch(Prop)
    {
        case TerrainProp_Boulder:
        {
            color_ramp Stone = Ramp(ART_RGB(70, 68, 66), ART_RGB(108, 104, 98),
                                    ART_RGB(146, 140, 130), ART_RGB(188, 182, 170));
            color_ramp Moss = Ramp(ART_RGB(40, 70, 30), ART_RGB(60, 100, 40),
                                   ART_RGB(84, 130, 50), ART_RGB(120, 160, 70));
            FillBlob(Canvas, 24.f, 34.f, 13.f, 9.f, Stone);
            FillBlob(Canvas, 18.f, 37.f, 7.f, 5.f, Stone, -0.1f);
            FillBlob(Canvas, 31.f, 36.f, 6.f, 5.f, Stone, 0.05f);
            FillBlob(Canvas, 21.f, 27.f, 6.f, 2.5f, Moss, 0.1f);
            FillLimb(Canvas, V2(26.f, 30.f), V2(30.f, 36.f), 0.5f, 0.4f,
                     Ramp(Stone.C[0], Stone.C[0], Stone.C[0], Stone.C[0]));
            OutlineFrame(Canvas, ART_RGB(26, 24, 22));
        } break;

        case TerrainProp_DeadTree:
        {
            color_ramp Bark = Ramp(ART_RGB(40, 32, 28), ART_RGB(66, 54, 46),
                                   ART_RGB(94, 80, 68), ART_RGB(126, 110, 94));
            v2 Root = V2(24.f, 42.f);
            v2 Fork = V2(23.f, 22.f);
            FillLimb(Canvas, Root, Fork, 3.f, 2.f, Bark);
            FillLimb(Canvas, Root, Root + V2(-6.f, 1.f), 1.5f, 0.6f, Bark, -0.1f);
            FillLimb(Canvas, Root, Root + V2(6.f, 1.f), 1.5f, 0.6f, Bark, -0.1f);
            FillLimb(Canvas, Fork, V2(14.f, 10.f), 1.8f, 0.6f, Bark);
            FillLimb(Canvas, Fork, V2(32.f, 8.f), 1.8f, 0.6f, Bark);
            FillLimb(Canvas, V2(27.f, 15.f), V2(35.f, 16.f), 1.f, 0.4f, Bark);
            FillLimb(Canvas, V2(18.f, 15.f), V2(12.f, 18.f), 1.f, 0.4f, Bark);
            FillLimb(Canvas, V2(23.f, 22.f), V2(24.f, 6.f), 1.4f, 0.5f, Bark, 0.05f);
            OutlineFrame(Canvas, ART_RGB(16, 12, 10));
        } break;

        default:
        {
        } break;
    }
}

// NOTE(zoubir): animated kinds play their row as frames; the rest pick a
// variant per tile
inline bool32
IsTerrainAnimated(terrain_kind Kind)
{
    bool32 Result = Kind == TerrainKind_ShallowWater ||
        Kind == TerrainKind_DeepWater || Kind == TerrainKind_Lava;
    return Result;
}
