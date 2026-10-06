/* Terrain art: the ground tiles, drawn by code into one atlas texture at
   startup. One row per terrain kind:

   columns 0-3   full tiles, and columns 17-28 twelve more: sixteen cells
                 the kind's ground is painted in (TerrainCellColumn).
                 Solid ground uses them as variants, picked per tile by a
                 hash; liquids as four layouts of four animation frames;
                 runes as the quarters of a big glyph (cells 0-3), a bare
                 slab (4) and small glyphs (5-7).
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

   Every full tile wraps, and the variants of a kind share their border
   pixels' noise, so any variant joins any other without a seam. Which kind spills
   over which is TerrainLayer: higher layers spill onto lower ones.

   The full tiles are painted in terrain/ground_tiles.cpp (natural ground)
   and terrain_hazard_art.cpp (hazards); the props in
   terrain/terrain_props_art.cpp. This file builds the rest of the atlas
   from them. */

#define TERRAIN_TILE_PIXELS 32
// NOTE(zoubir): animation frames per layout; cells per kind is twice that
#define TERRAIN_VARIANTS 4
#define TERRAIN_CELLS 16
#define TERRAIN_EDGE_COLUMN 4
#define TERRAIN_CORNER_COLUMN 8
#define TERRAIN_CLIFF_COLUMN 12
#define TERRAIN_STEP_COLUMN 13
#define TERRAIN_RIM_COLUMN 14
#define TERRAIN_SHADOW_COLUMN 15
#define TERRAIN_SIDE_SHADOW_COLUMN 16
#define TERRAIN_EXTRA_COLUMN 17
#define TERRAIN_ATLAS_COLUMNS 29
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
    7, // bramble: its thorny edge spills over grass
    3, // bog
    4, // rune
};

// NOTE(zoubir): the atlas column of a kind's full-tile cell 0..15
inline u32
TerrainCellColumn(u32 Cell)
{
    u32 Result = Cell < TERRAIN_VARIANTS ? Cell :
        TERRAIN_EXTRA_COLUMN + (Cell - TERRAIN_VARIANTS);
    return Result;
}

// NOTE(zoubir): the cell edges, corners and faces copy their ground from:
// a rune's first cells carry parts of a glyph, so it uses its bare slab
inline u32
TerrainPlainColumn(u32 Kind)
{
    u32 Result = Kind == TerrainKind_Rune ? TERRAIN_EXTRA_COLUMN : 0;
    return Result;
}

#include "terrain/ground_paint.cpp"
#include "terrain/ground_tiles.cpp"
#include "terrain/liquid_tiles.cpp"

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
    u32 *Source = Pixels + Kind * Tile * Width + TerrainPlainColumn(Kind) * Tile;
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
    u32 *Ground = Pixels + (u32)Kind * Tile * Width + TerrainPlainColumn(Kind) * Tile;
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
        for(u32 Cell = 0; Cell < TERRAIN_CELLS; Cell++)
        {
            sprite_canvas Canvas = CanvasFrame(Pixels, Width, TERRAIN_TILE_PIXELS,
                                               TerrainCellColumn(Cell), Kind);
            DrawTerrainTile(&Canvas, (terrain_kind)Kind, Cell);
        }
        BuildTerrainTransitions(Pixels, Width, (terrain_kind)Kind);
        BuildTerrainFaces(Pixels, Width, (terrain_kind)Kind);
    }
}

#include "terrain/terrain_props_art.cpp"

// NOTE(zoubir): animated kinds play four frames of one of four layouts
// (cells 0-3, 4-7, ...); the rest pick one of sixteen variants per tile
inline bool32
IsTerrainAnimated(terrain_kind Kind)
{
    bool32 Result = Kind == TerrainKind_ShallowWater ||
        Kind == TerrainKind_DeepWater || Kind == TerrainKind_Lava ||
        Kind == TerrainKind_Spring;
    return Result;
}
