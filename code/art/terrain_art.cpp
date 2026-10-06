/* Terrain art: the ground tiles, drawn by code into one atlas texture at
   startup. One row per terrain kind:

   columns 0-3   full tiles. Solid ground uses them as variants (picked
                 per tile by a hash, so a field does not repeat); water
                 and lava as animation frames.
   columns 4-7   edges: the kind spilling over one side of a neighbouring
                 tile (north, east, south, west), ragged and transparent
                 elsewhere.
   columns 8-11  corners: the kind rounding into one corner of a
                 neighbour (north-west, north-east, south-east, south-west).

   column 12     cliff face: the kind's ground curling over a lip in the
                 top rows, then the face below it (TerrainCliffs). Rows
                 from TERRAIN_FACE_REPEAT_ROW down repeat every
                 TERRAIN_FACE_PERIOD, so a face taller than a tile is
                 drawn as the whole cell and then that band again.
   column 13     step: a one-step riser, TERRAIN_STEP_PIXELS rows (lit
                 nose, lighter face, dark crease), repeated down the cell.
   column 14     rim: the kind's highlight, half see-through, cropped to
                 a thin strip along a top edge above a drop.
   column 15     foot shadow, fading down from the top row; cropped to a
                 strip at the foot of a cliff.
   column 16     side shadow, fading right from the left column; mirrored
                 by its UVs for the other side.

   Every full tile wraps: its left edge continues its right edge and its
   top its bottom, so neighbours join without seams. Which kind spills
   over which is TerrainLayer: higher layers spill onto lower ones. */

#define TERRAIN_TILE_PIXELS 32
#define TERRAIN_VARIANTS 4
#define TERRAIN_EDGE_COLUMN 4
#define TERRAIN_CORNER_COLUMN 8
#define TERRAIN_CLIFF_COLUMN 12
#define TERRAIN_STEP_COLUMN 13
#define TERRAIN_RIM_COLUMN 14
#define TERRAIN_SHADOW_COLUMN 15
#define TERRAIN_SIDE_SHADOW_COLUMN 16
#define TERRAIN_ATLAS_COLUMNS 17
#define TERRAIN_FACE_REPEAT_ROW 8
#define TERRAIN_FACE_PERIOD 24
#define TERRAIN_STEP_PIXELS 8

// NOTE(zoubir): higher spills over lower at a border; liquids sit lowest
// so banks and shores cover their edges, walls highest
global_variable u32 TerrainLayer[TerrainKind_Count] =
{
    6, // grass
    4, // dirt
    3, // mud
    1, // shallow water
    0, // deep water
    8, // rock
    4, // ash
    5, // basalt
    8, // basalt wall
    1, // lava
    7, // snow
    2, // ice
    5, // stone floor
    9, // stone wall
    0, // pit: the ground around spills over its lip
    1, // spring
    6, // bramble
    3, // bog
    4, // rune
};

// NOTE(zoubir): value noise on cells of CellX x CellY pixels that repeats
// every PeriodX pixels across and PeriodY down (each a multiple of its
// cell). 0..255
inline u32
WrappedNoise(u32 Seed, i32 X, i32 Y, i32 CellX, i32 CellY, i32 PeriodX, i32 PeriodY)
{
    i32 CellsX = PeriodX / CellX;
    i32 CellsY = PeriodY / CellY;
    i32 CX = FloorDiv(X, CellX);
    i32 CY = FloorDiv(Y, CellY);
    i32 FX = (FloorMod(X, CellX) << 8) / CellX;
    i32 FY = (FloorMod(Y, CellY) << 8) / CellY;
    i32 A = (i32)(HashLattice(Seed, FloorMod(CX, CellsX), FloorMod(CY, CellsY)) >> 24);
    i32 B = (i32)(HashLattice(Seed, FloorMod(CX + 1, CellsX), FloorMod(CY, CellsY)) >> 24);
    i32 C = (i32)(HashLattice(Seed, FloorMod(CX, CellsX), FloorMod(CY + 1, CellsY)) >> 24);
    i32 D = (i32)(HashLattice(Seed, FloorMod(CX + 1, CellsX), FloorMod(CY + 1, CellsY)) >> 24);
    i32 Top = A + (((B - A) * FX) >> 8);
    i32 Bottom = C + (((D - C) * FX) >> 8);
    u32 Result = (u32)(Top + (((Bottom - Top) * FY) >> 8));
    return Result;
}

// NOTE(zoubir): noise that repeats every TERRAIN_TILE_PIXELS, so a tile's
// edges match the next tile's. 0..255
inline u32
WrappedTileNoise(u32 Seed, i32 X, i32 Y, i32 Cell)
{
    u32 Result = WrappedNoise(Seed, X, Y, Cell, Cell, TERRAIN_TILE_PIXELS,
                              TERRAIN_TILE_PIXELS);
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

// NOTE(zoubir): the hazard kinds' tiles (terrain_hazard_art.cpp)
internal void DrawHazardTerrainTile(sprite_canvas *Canvas, terrain_kind Kind,
                                    u32 Column, u32 Seed, float Phase);

internal void
DrawTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Column)
{
    if (Column >= TERRAIN_VARIANTS)
    {
        return;
    }
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
            DrawHazardTerrainTile(Canvas, Kind, Column, Seed, Phase);
        } break;
    }
}

// NOTE(zoubir): how deep (in pixels) the spill reaches at position Along
// on the border, 0..TERRAIN_TILE_PIXELS - 1; ragged but continuous
inline i32
EdgeReach(u32 Seed, i32 Along)
{
    i32 Result = 4 + (i32)((WrappedTileNoise(Seed, Along, 0, 8) * 6) >> 8) +
        (i32)((WrappedTileNoise(Seed + 5, Along, 3, 4) * 3) >> 8);
    return Result;
}

// NOTE(zoubir): copies the kind's first variant into the edge and corner
// columns, keeping only pixels inside the spill shape. Side 0..3 is north,
// east, south, west; corner 0..3 is north-west, north-east, south-east,
// south-west
internal void
BuildTerrainTransitions(u32 *Pixels, u32 Width, terrain_kind Kind)
{
    u32 Tile = TERRAIN_TILE_PIXELS;
    u32 *Source = Pixels + Kind * Tile * Width;
    for(u32 Shape = 0; Shape < 8; Shape++)
    {
        u32 Column = Shape < 4 ? TERRAIN_EDGE_COLUMN + Shape :
            TERRAIN_CORNER_COLUMN + (Shape - 4);
        u32 *Target = Pixels + Kind * Tile * Width + Column * Tile;
        for(i32 Y = 0; Y < (i32)Tile; Y++)
        {
            for(i32 X = 0; X < (i32)Tile; X++)
            {
                bool32 Inside = false;
                i32 Last = (i32)Tile - 1;
                if (Shape < 4)
                {
                    // NOTE(zoubir): distance in from the side, and the
                    // position along it
                    i32 Depth = Shape == 0 ? Y : Shape == 1 ? Last - X :
                        Shape == 2 ? Last - Y : X;
                    i32 Along = (Shape == 0 || Shape == 2) ? X : Y;
                    Inside = Depth < EdgeReach(311u + Kind * 7u + Shape, Along);
                }
                else
                {
                    u32 Corner = Shape - 4;
                    i32 CX = (Corner == 0 || Corner == 3) ? X : Last - X;
                    i32 CY = (Corner == 0 || Corner == 1) ? Y : Last - Y;
                    i32 Reach = EdgeReach(613u + Kind * 7u + Corner, CX + CY);
                    Inside = CX * CX + CY * CY < Reach * Reach;
                }
                if (Inside)
                {
                    // NOTE(zoubir): a dithered fringe on the last pixel so
                    // the spill feathers into the ground below
                    Target[Y * Width + X] = Source[Y * Width + X];
                }
            }
        }
    }
}

// NOTE(zoubir): what the side of raised ground shows
enum cliff_pattern
{
    // NOTE(zoubir): soil in layers with pebbles, roots under the lip
    CliffPattern_Earth,
    // NOTE(zoubir): rock split by fissures and ledges
    CliffPattern_Rock,
    // NOTE(zoubir): rock glazed with ice, icicles under the lip
    CliffPattern_Ice,
    // NOTE(zoubir): courses of cut stone
    CliffPattern_Masonry,
};

// NOTE(zoubir): specks in the cracks of rock faces
enum cliff_detail
{
    CliffDetail_None,
    CliffDetail_Embers,
    CliffDetail_Glints,
};

struct terrain_cliff
{
    cliff_pattern Pattern;
    color_ramp Face;
    // NOTE(zoubir): pebbles, glints, embers, ice or mortar highlights
    u32 Accent;
    cliff_detail Detail;
};

#include "terrain_cliff_colors.inc"

// NOTE(zoubir): one pixel of a kind's cliff face, for any row: the face
// repeats every TERRAIN_FACE_PERIOD rows and across every tile. Bias
// lightens it (steps are lighter than cliffs)
internal u32
CliffFacePixel(terrain_kind Kind, i32 X, i32 Y, float Bias)
{
    terrain_cliff *Cliff = &TerrainCliffs[Kind];
    i32 Period = TERRAIN_FACE_PERIOD;
    i32 Tile = TERRAIN_TILE_PIXELS;
    i32 Row = FloorMod(Y, Period);
    u32 Seed = 5000u + 37u * (u32)Kind;
    float Grain = (float)WrappedNoise(Seed + 3, X, Row, 4, 4, Tile, Period) / 255.f;
    float Value = 0.f;
    switch(Cliff->Pattern)
    {
        case CliffPattern_Earth:
        {
            // NOTE(zoubir): layers: noise stretched sideways, and pebbles
            // sitting in them, lit on top
            float Layer = (float)WrappedNoise(Seed, X, Row, 16, 4, Tile, Period) / 255.f;
            Value = 0.15f + 0.6f * Layer + 0.25f * Grain;
            // NOTE(zoubir): at most one pebble per 8 x 6 cell, placed by
            // the cell's roll so they do not line up
            i32 Column = FloorMod(X, Tile);
            u32 Pebble = HashLattice(Seed + 9, Column / 8, Row / 6);
            i32 PebbleX = (Column / 8) * 8 + 1 + (i32)((Pebble >> 8) % 5);
            i32 PebbleY = (Row / 6) * 6 + (i32)((Pebble >> 16) % 4);
            if ((Pebble % 3) == 0 && Column >= PebbleX && Column < PebbleX + 2)
            {
                if (Row == PebbleY)
                {
                    return Cliff->Accent;
                }
                if (Row == PebbleY + 1)
                {
                    return Cliff->Face.C[0];
                }
            }
        } break;

        case CliffPattern_Rock:
        case CliffPattern_Ice:
        {
            float Block = (float)WrappedNoise(Seed, X, Row, 8, 8, Tile, Period) / 255.f;
            Value = 0.2f + 0.55f * Block + 0.25f * Grain;
            // NOTE(zoubir): a fissure down each 8-pixel column band, where
            // the band rolls it; dark, with a lit pixel on its left
            i32 Band = FloorMod(X, Tile) / 8;
            u32 Roll = HashLattice(Seed + 21, Band, 0);
            i32 Wobble = (i32)(WrappedNoise(Seed + 5, Band * 8, Row, 8, 6, Tile, Period) >> 6);
            i32 Fissure = Band * 8 + (i32)(Roll % 4) + Wobble;
            bool32 Open = WrappedNoise(Seed + 11, Band * 8, Row, 8, 8, Tile, Period) > 90;
            i32 Column = FloorMod(X, Tile);
            if ((Roll & 1) && Open)
            {
                if (Column == Fissure)
                {
                    u32 Glow = HashLattice(Seed + 31, Band, Row);
                    bool32 Ember = Cliff->Detail == CliffDetail_Embers && (Glow % 5) == 0;
                    return Ember ? Cliff->Accent : Cliff->Face.C[0];
                }
                if (Column == Fissure - 1)
                {
                    return Cliff->Face.C[3];
                }
            }
            // NOTE(zoubir): one ledge per period: lit edge over a shadow
            i32 Ledge = (i32)(HashLattice(Seed + 41, Band, 1) % (u32)Period);
            if (Row == Ledge && (Roll & 2))
            {
                return Cliff->Face.C[3];
            }
            if (Row == (Ledge + 1) % Period && (Roll & 2))
            {
                return Cliff->Face.C[0];
            }
            if (Cliff->Pattern == CliffPattern_Ice)
            {
                // NOTE(zoubir): ice glazing runs down in streaks
                u32 Streak = WrappedNoise(Seed + 51, X, Row, 2, 12, Tile, Period);
                if (Streak > 200)
                {
                    return Cliff->Accent;
                }
                if (Streak > 170)
                {
                    Value += 0.35f;
                }
            }
            else if (Cliff->Detail == CliffDetail_Glints)
            {
                if ((HashLattice(Seed + 61, X, Row) % 41) == 0)
                {
                    return Cliff->Accent;
                }
            }
        } break;

        case CliffPattern_Masonry:
        {
            // NOTE(zoubir): courses 6 rows high, 16-pixel stones offset
            // every other course; each stone its own shade
            i32 Course = Row / 6;
            i32 Offset = (Course & 1) ? 8 : 0;
            i32 Column = FloorMod(X + Offset, Tile);
            if ((Row % 6) == 0 || (Column % 16) == 0)
            {
                return Cliff->Face.C[0];
            }
            if ((Row % 6) == 1 || (Column % 16) == 1)
            {
                Value += 0.25f;
            }
            float Stone = (float)(HashLattice(Seed + 71, Column / 16, Course) >> 24) / 255.f;
            Value += 0.25f + 0.35f * Stone + 0.25f * Grain;
        } break;
    }
    u32 Result = ShadeRamp(&Cliff->Face, Value + Bias, X, Y);
    return Result;
}

// NOTE(zoubir): a colour with its alpha replaced
inline u32
ArtWithAlpha(u32 Color, u32 Alpha)
{
    u32 Result = (Color & 0x00FFFFFF) | (Alpha << 24);
    return Result;
}

// NOTE(zoubir): the kind's raised-ground cells (columns 12-16, see the
// top of this file), built from its first variant and TerrainCliffs
internal void
BuildTerrainFaces(u32 *Pixels, u32 Width, terrain_kind Kind)
{
    i32 Tile = TERRAIN_TILE_PIXELS;
    u32 *Ground = Pixels + (u32)Kind * Tile * Width;
    terrain_cliff *Cliff = &TerrainCliffs[Kind];
    sprite_canvas Face = CanvasFrame(Pixels, Width, Tile, TERRAIN_CLIFF_COLUMN, Kind);
    sprite_canvas Step = CanvasFrame(Pixels, Width, Tile, TERRAIN_STEP_COLUMN, Kind);
    sprite_canvas Rim = CanvasFrame(Pixels, Width, Tile, TERRAIN_RIM_COLUMN, Kind);
    sprite_canvas Shadow = CanvasFrame(Pixels, Width, Tile, TERRAIN_SHADOW_COLUMN, Kind);
    sprite_canvas Side = CanvasFrame(Pixels, Width, Tile, TERRAIN_SIDE_SHADOW_COLUMN, Kind);

    // NOTE(zoubir): the rim is the ground's average lifted toward white
    u32 Sum[3] = {};
    for(i32 Index = 0; Index < Tile * Tile; Index++)
    {
        u32 Pixel = Ground[(Index / Tile) * Width + (Index % Tile)];
        Sum[0] += Pixel & 0xFF;
        Sum[1] += (Pixel >> 8) & 0xFF;
        Sum[2] += (Pixel >> 16) & 0xFF;
    }
    u32 Light[3];
    for(u32 Channel = 0; Channel < 3; Channel++)
    {
        u32 Average = Sum[Channel] / (u32)(Tile * Tile);
        Light[Channel] = Average + (255 - Average) / 2;
    }
    u32 RimColor = ArtWithAlpha(ART_RGB(Light[0], Light[1], Light[2]), 150);

    // NOTE(zoubir): shadows fade out over 8 pixels, in 2-pixel bands
    u32 ShadowAlpha[8] = {150, 150, 104, 104, 60, 60, 24, 24};
    u32 ShadowColor = ART_RGB(10, 8, 18);

    for(i32 X = 0; X < Tile; X++)
    {
        // NOTE(zoubir): the top's ground curls over a ragged lip, 2 to 4
        // rows, then a dark row of shade under it
        i32 Lip = 2 + (i32)((WrappedTileNoise(777u + (u32)Kind, X, 0, 4) * 3) >> 8);
        u32 Icicle = HashLattice(888u + (u32)Kind, X, 0);
        i32 IcicleLength = (Cliff->Pattern == CliffPattern_Ice && (Icicle % 3) == 0) ?
            1 + (i32)((Icicle >> 8) % 3) : 0;
        i32 Root = (Cliff->Pattern == CliffPattern_Earth && (Icicle % 9) == 0) ?
            2 + (i32)((Icicle >> 8) % 2) : 0;
        for(i32 Y = 0; Y < Tile; Y++)
        {
            u32 Color;
            if (Y < Lip)
            {
                Color = Ground[Y * Width + X];
            }
            else if (Y == Lip)
            {
                Color = Cliff->Face.C[0];
            }
            else if (Y <= Lip + IcicleLength)
            {
                Color = Y == Lip + IcicleLength ? Cliff->Face.C[3] : Cliff->Accent;
            }
            else if (Y <= Lip + Root)
            {
                Color = Cliff->Face.C[0];
            }
            else
            {
                // NOTE(zoubir): a little lighter just under the lip
                Color = CliffFacePixel(Kind, X, Y, Y < Lip + 4 ? 0.12f : 0.f);
            }
            PutPixel(&Face, X, Y, Color);

            // NOTE(zoubir): a step: the top's ground on the nose, a lit
            // edge, a lighter riser and a dark crease at its foot
            i32 StepRow = Y % TERRAIN_STEP_PIXELS;
            u32 StepColor =
                StepRow == 0 ? Ground[X] :
                StepRow == 1 ? Cliff->Face.C[3] :
                StepRow == TERRAIN_STEP_PIXELS - 1 ? Cliff->Face.C[0] :
                CliffFacePixel(Kind, X, StepRow + TERRAIN_FACE_REPEAT_ROW, 0.3f);
            PutPixel(&Step, X, Y, StepColor);

            PutPixel(&Rim, X, Y, RimColor);
            PutPixel(&Shadow, X, Y, Y < 8 ? ArtWithAlpha(ShadowColor, ShadowAlpha[Y]) : ART_CLEAR);
            PutPixel(&Side, X, Y, X < 8 ? ArtWithAlpha(ShadowColor, ShadowAlpha[X]) : ART_CLEAR);
        }
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
        for(u32 Column = 0; Column < TERRAIN_VARIANTS; Column++)
        {
            sprite_canvas Canvas = CanvasFrame(Pixels, Width, TERRAIN_TILE_PIXELS,
                                               Column, Kind);
            DrawTerrainTile(&Canvas, (terrain_kind)Kind, Column);
        }
        BuildTerrainTransitions(Pixels, Width, (terrain_kind)Kind);
        BuildTerrainFaces(Pixels, Width, (terrain_kind)Kind);
    }
}

// NOTE(zoubir): props that are not the packed tree: one 48-pixel frame
// each, feet on the 7/8 line like monsters. Logs, fences and crates are
// the jumpables: low and wide, so a unit standing on one reads as on top
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

        case TerrainProp_Log:
        {
            // NOTE(zoubir): a fallen trunk lying across the tile, its cut
            // end toward the viewer's right showing the rings
            color_ramp Bark = Ramp(ART_RGB(48, 34, 24), ART_RGB(78, 56, 38),
                                   ART_RGB(106, 78, 52), ART_RGB(136, 104, 72));
            color_ramp Wood = Ramp(ART_RGB(150, 112, 70), ART_RGB(184, 142, 92),
                                   ART_RGB(210, 172, 118), ART_RGB(232, 200, 150));
            color_ramp Moss = Ramp(ART_RGB(40, 70, 30), ART_RGB(60, 100, 40),
                                   ART_RGB(84, 130, 50), ART_RGB(120, 160, 70));
            FillLimb(Canvas, V2(8.f, 35.f), V2(38.f, 35.f), 6.5f, 6.f, Bark);
            FillLimb(Canvas, V2(19.f, 30.f), V2(15.f, 24.f), 1.6f, 1.f, Bark, 0.05f);
            // NOTE(zoubir): bark ridges along the trunk
            u32 Ridge = Bark.C[0];
            FillLimb(Canvas, V2(10.f, 33.f), V2(26.f, 33.f), 0.4f, 0.4f,
                     Ramp(Ridge, Ridge, Ridge, Ridge));
            FillLimb(Canvas, V2(14.f, 37.f), V2(34.f, 37.f), 0.4f, 0.4f,
                     Ramp(Ridge, Ridge, Ridge, Ridge));
            FillBlob(Canvas, 24.f, 29.5f, 6.f, 1.8f, Moss, 0.1f);
            FillBlob(Canvas, 39.f, 35.f, 4.f, 6.f, Wood, 0.1f);
            FillFlatEllipse(Canvas, 39.f, 35.f, 2.4f, 3.8f, Wood.C[1]);
            FillFlatEllipse(Canvas, 39.f, 35.f, 1.2f, 2.f, Wood.C[3]);
            FillDot(Canvas, 39.f, 35.f, 0.6f, Bark.C[1]);
            OutlineFrame(Canvas, ART_RGB(24, 16, 10));
        } break;

        case TerrainProp_Fence:
        {
            // NOTE(zoubir): two posts with pointed tops and two rails
            color_ramp Plank = Ramp(ART_RGB(70, 50, 32), ART_RGB(104, 76, 48),
                                    ART_RGB(136, 102, 66), ART_RGB(170, 132, 88));
            FillLimb(Canvas, V2(6.f, 26.f), V2(42.f, 26.f), 1.5f, 1.5f, Plank, 0.05f);
            FillLimb(Canvas, V2(6.f, 34.f), V2(42.f, 34.f), 1.5f, 1.5f, Plank, -0.05f);
            for(u32 Post = 0; Post < 2; Post++)
            {
                float X = Post ? 35.f : 13.f;
                FillLimb(Canvas, V2(X, 41.f), V2(X, 22.f), 2.2f, 2.f, Plank);
                FillTriangle(Canvas, V2(X - 2.f, 21.f), V2(X + 2.5f, 21.f),
                             V2(X, 17.f), Plank, 0.8f, 0.5f);
                FillDot(Canvas, X, 26.f, 0.6f, ART_RGB(60, 60, 64));
                FillDot(Canvas, X, 34.f, 0.6f, ART_RGB(60, 60, 64));
            }
            OutlineFrame(Canvas, ART_RGB(26, 18, 10));
        } break;

        case TerrainProp_Crate:
        {
            // NOTE(zoubir): a box seen from the front and above: a lit top,
            // a plank front with a cross brace, iron on the corners
            color_ramp Plank = Ramp(ART_RGB(92, 62, 34), ART_RGB(128, 90, 50),
                                    ART_RGB(162, 120, 70), ART_RGB(196, 154, 98));
            color_ramp Iron = Ramp(ART_RGB(40, 40, 46), ART_RGB(70, 70, 78),
                                   ART_RGB(104, 104, 112), ART_RGB(150, 150, 160));
            i32 Left = 11;
            i32 Right = 37;
            i32 Lid = 15;
            i32 Front = 23;
            i32 Bottom = 42;
            for(i32 Y = Lid; Y < Bottom; Y++)
            {
                for(i32 X = Left; X < Right; X++)
                {
                    bool32 Top = Y < Front;
                    // NOTE(zoubir): top planks run front to back, front
                    // planks across; a dark seam between planks
                    bool32 Seam = Top ? ((X - Left) % 6) == 5 : ((Y - Front) % 5) == 4;
                    float Light = Top ? 0.85f : 0.45f;
                    Light += 0.1f * (float)((HashLattice(91u, X / 6, Y / 5) >> 24) & 1);
                    u32 Color = Seam ? Plank.C[0] : ShadeRamp(&Plank, Light, X, Y);
                    PutPixel(Canvas, X, Y, Color);
                }
            }
            FillLimb(Canvas, V2((float)Left + 2.f, (float)Bottom - 2.f),
                     V2((float)Right - 2.f, (float)Front + 2.f), 1.3f, 1.3f, Plank, 0.15f);
            // NOTE(zoubir): the lid's front edge catches the light
            for(i32 X = Left; X < Right; X++)
            {
                PutPixel(Canvas, X, Front, Plank.C[3]);
            }
            // NOTE(zoubir): an iron bracket bent round each corner of the
            // front and of the lid, 6 pixels along each edge, with a rivet
            i32 CornerX[2] = {Left, Right - 1};
            i32 CornerY[3] = {Lid, Front, Bottom - 1};
            for(u32 CY = 0; CY < 3; CY++)
            {
                for(u32 CX = 0; CX < 2; CX++)
                {
                    i32 StepX = CX ? -1 : 1;
                    i32 StepY = CY == 2 ? -1 : 1;
                    for(i32 Along = 0; Along < 6; Along++)
                    {
                        for(i32 Across = 0; Across < 2; Across++)
                        {
                            i32 AX = CornerX[CX] + StepX * Along;
                            i32 AY = CornerY[CY] + StepY * Across;
                            i32 BX = CornerX[CX] + StepX * Across;
                            i32 BY = CornerY[CY] + StepY * Along;
                            float Light = Across == 0 ? 0.85f : 0.4f;
                            PutPixel(Canvas, AX, AY, ShadeRamp(&Iron, Light, AX, AY));
                            if (CY != 0 || Along < 3)
                            {
                                PutPixel(Canvas, BX, BY, ShadeRamp(&Iron, Light, BX, BY));
                            }
                        }
                    }
                    PutPixel(Canvas, CornerX[CX] + 2 * StepX, CornerY[CY] + 2 * StepY,
                             Iron.C[3]);
                }
            }
            OutlineFrame(Canvas, ART_RGB(24, 16, 10));
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
        Kind == TerrainKind_DeepWater || Kind == TerrainKind_Lava ||
        Kind == TerrainKind_Spring || Kind == TerrainKind_Rune;
    return Result;
}
