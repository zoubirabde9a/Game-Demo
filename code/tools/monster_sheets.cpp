/* Writes every monster's sprite sheet to build/monster_art/<kind>.png,
   scaled up 4x on a dark checkerboard, so art can be reviewed without
   starting the game. Built and run by art.bat. */

#include <direct.h>
#include "../app.cpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image/stb_image_write.h"

#define PREVIEW_SCALE 4

// NOTE(zoubir): flat colors per terrain kind, for map previews only; the
// real ground tiles are drawn in art/
global_variable u32 PreviewTerrainColors[TerrainKind_Count] =
{
    ART_RGB(86, 140, 60),   // grass
    ART_RGB(150, 116, 74),  // dirt
    ART_RGB(96, 72, 48),    // mud
    ART_RGB(80, 150, 190),  // shallow water
    ART_RGB(30, 70, 130),   // deep water
    ART_RGB(110, 108, 104), // rock
    ART_RGB(120, 116, 112), // ash
    ART_RGB(54, 50, 56),    // basalt
    ART_RGB(26, 22, 28),    // basalt wall
    ART_RGB(240, 100, 20),  // lava
    ART_RGB(230, 236, 244), // snow
    ART_RGB(170, 220, 240), // ice
    ART_RGB(150, 146, 140), // stone floor
    ART_RGB(70, 66, 70),    // stone wall
};

// NOTE(zoubir): one pixel block per tile; props as a dark dot in the middle
internal void
WriteMapPreview(map_def *Map, i32 MinX, i32 MinY, u32 TilesX, u32 TilesY,
                u32 Scale, char *Path)
{
    u32 Width = TilesX * Scale;
    u32 Height = TilesY * Scale;
    u32 *Out = (u32 *)calloc(Width * Height, sizeof(u32));
    for(u32 TileY = 0; TileY < TilesY; TileY++)
    {
        for(u32 TileX = 0; TileX < TilesX; TileX++)
        {
            i32 X = MinX + (i32)TileX;
            i32 Y = MinY + (i32)TileY;
            u32 Color = PreviewTerrainColors[TerrainAt(Map, X, Y)];
            terrain_prop Prop = PropAt(Map, X, Y);
            // NOTE(zoubir): raised ground lighter, a dark lip where it
            // drops two steps or more to the south (a cliff)
            i32 Steps = ElevationAt(Map, X, Y);
            bool32 Cliff = Steps - ElevationAt(Map, X, Y + 1) >= 2;
            u32 Lifted = 0xFF000000;
            for(u32 Shift = 0; Shift < 24; Shift += 8)
            {
                u32 Channel = (Color >> Shift) & 0xFF;
                Channel += ((255 - Channel) * (u32)Steps) / 14;
                Lifted |= Channel << Shift;
            }
            Color = Lifted;
            for(u32 PY = 0; PY < Scale; PY++)
            {
                for(u32 PX = 0; PX < Scale; PX++)
                {
                    u32 Pixel = Color;
                    bool32 Middle = PX >= Scale / 4 && PX < Scale - Scale / 4 &&
                        PY >= Scale / 4 && PY < Scale - Scale / 4;
                    if (Cliff && PY >= Scale - Scale / 4)
                    {
                        Pixel = ART_RGB(30, 26, 24);
                    }
                    if (Prop != TerrainProp_None && Middle)
                    {
                        Pixel = Prop == TerrainProp_Boulder ? ART_RGB(60, 60, 60) :
                            Prop == TerrainProp_Crate ? ART_RGB(200, 150, 60) :
                            Prop == TerrainProp_Fence ? ART_RGB(240, 220, 160) :
                            Prop == TerrainProp_Log ? ART_RGB(110, 60, 20) :
                            ART_RGB(20, 60, 20);
                    }
                    if (X == 0 && Y == 0)
                    {
                        Pixel = ART_RGB(255, 0, 255);
                    }
                    Out[(TileY * Scale + PY) * Width + TileX * Scale + PX] = Pixel;
                }
            }
        }
    }
    stbi_write_png(Path, Width, Height, 4, Out, Width * 4);
    printf("%s\n", Path);
    free(Out);
}

int main()
{
    _mkdir("monster_art");
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        u32 Width = MonsterSheetWidth(Def);
        u32 Height = MonsterSheetHeight(Def);
        u32 *Sheet = (u32 *)calloc(Width * Height, sizeof(u32));
        BuildMonsterSheet((monster_kind)KindIndex, Sheet);

        u32 OutWidth = Width * PREVIEW_SCALE;
        u32 OutHeight = Height * PREVIEW_SCALE;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / PREVIEW_SCALE) * Width + X / PREVIEW_SCALE];
                bool32 FrameEdge = (X / PREVIEW_SCALE) % Def->FrameSize == 0 ||
                    (Y / PREVIEW_SCALE) % Def->FrameSize == 0;
                u32 Checker = ((X / 16 + Y / 16) % 2) ? ART_RGB(58, 66, 54) :
                    ART_RGB(66, 74, 60);
                Out[Y * OutWidth + X] = Pixel ? Pixel :
                    (FrameEdge ? ART_RGB(90, 90, 90) : Checker);
            }
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/%s.png", Def->Name);
        stbi_write_png(Path, OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("%s\n", Path);
        free(Out);
        free(Sheet);
    }
    for(u32 Style = 0; Style < ShotStyle_Count; Style++)
    {
        u32 Width = SHOT_FRAME_SIZE * SHOT_FRAMES;
        u32 Sheet[SHOT_FRAME_SIZE * SHOT_FRAMES * SHOT_FRAME_SIZE];
        BuildShotSheet((monster_shot_style)Style, Sheet);
        u32 Scale = 8;
        u32 OutWidth = Width * Scale;
        u32 OutHeight = SHOT_FRAME_SIZE * Scale;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / Scale) * Width + X / Scale];
                Out[Y * OutWidth + X] = Pixel ? Pixel : ART_RGB(58, 66, 54);
            }
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/shot_%u.png", Style);
        stbi_write_png(Path, OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("%s\n", Path);
        free(Out);
    }
    for(u32 Style = 0; Style < HazardStyle_Count; Style++)
    {
        u32 Width = HAZARD_FRAME_SIZE * HAZARD_FRAMES;
        u32 *Sheet = (u32 *)calloc(Width * HAZARD_FRAME_SIZE, sizeof(u32));
        BuildHazardSheet((monster_hazard_style)Style, Sheet);
        u32 Scale = 4;
        u32 OutWidth = Width * Scale;
        u32 OutHeight = HAZARD_FRAME_SIZE * Scale;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / Scale) * Width + X / Scale];
                Out[Y * OutWidth + X] = Pixel ? Pixel : ART_RGB(58, 66, 54);
            }
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/hazard_%u.png", Style);
        stbi_write_png(Path, OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("%s\n", Path);
        free(Out);
        free(Sheet);
    }
    {
        u32 Width = TERRAIN_ATLAS_COLUMNS * TERRAIN_TILE_PIXELS;
        u32 Height = TerrainKind_Count * TERRAIN_TILE_PIXELS;
        u32 *Atlas = (u32 *)calloc(Width * Height, sizeof(u32));
        BuildTerrainAtlas(Atlas);
        // NOTE(zoubir): each tile repeated 2 x 2 so seams would show
        u32 Scale = 3;
        u32 OutWidth = 2 * Width * Scale;
        u32 OutHeight = Height * 2 * Scale;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Tile = TERRAIN_TILE_PIXELS;
                u32 PX = X / Scale;
                u32 PY = Y / Scale;
                u32 Column = PX / (2 * Tile);
                u32 Row = PY / (2 * Tile);
                u32 SX = Column * Tile + (PX % Tile);
                u32 SY = Row * Tile + (PY % Tile);
                u32 Pixel = Atlas[SY * Width + SX];
                // NOTE(zoubir): edges and corners on a checkerboard so
                // their transparent parts show
                u32 Checker = ((PX / 4 + PY / 4) % 2) ? ART_RGB(200, 0, 200) : ART_RGB(120, 0, 120);
                Out[Y * OutWidth + X] = Pixel ? Pixel : Checker;
            }
        }
        stbi_write_png("monster_art/terrain_atlas.png", OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("monster_art/terrain_atlas.png\n");
        free(Out);

        // NOTE(zoubir): each kind as a field of 8 x 3 tiles at twice the
        // size, every tile a cell picked by hash the way the game picks
        // them, so seams and repeats show; liquids at their first frame,
        // runes as one big glyph and small ones
        u32 Tile = TERRAIN_TILE_PIXELS;
        u32 FieldTiles = 8;
        u32 FieldRows = 3;
        u32 FieldWidth = FieldTiles * Tile * 2 + Tile;
        u32 Band = FieldRows * Tile * 2 + Tile;
        u32 FieldsPerRow = 2;
        u32 FieldOutWidth = FieldsPerRow * FieldWidth;
        u32 FieldOutHeight = ((TerrainKind_Count + 1) / FieldsPerRow) * Band;
        u32 *Fields = (u32 *)calloc(FieldOutWidth * FieldOutHeight, sizeof(u32));
        for(u32 Y = 0; Y < FieldOutHeight; Y++)
        {
            for(u32 X = 0; X < FieldOutWidth; X++)
            {
                u32 Kind = (Y / Band) * FieldsPerRow + X / FieldWidth;
                u32 FX = (X % FieldWidth) / 2;
                u32 FY = (Y % Band) / 2;
                u32 Pixel = ART_RGB(30, 30, 30);
                if (Kind < TerrainKind_Count && FY < FieldRows * Tile && FX < FieldTiles * Tile)
                {
                    u32 TileX = FX / Tile;
                    u32 TileY = FY / Tile;
                    u32 Roll = HashLattice(0x7E44u, (i32)TileX, (i32)TileY) >> 8;
                    u32 Cell = Roll % TERRAIN_CELLS;
                    if (IsTerrainAnimated((terrain_kind)Kind))
                    {
                        Cell = (Roll % 4) * TERRAIN_VARIANTS;
                    }
                    else if (Kind == TerrainKind_Rune)
                    {
                        Cell = (TileX < 2 && TileY < 2) ? TileX + 2 * TileY :
                            (TileX == 2 || TileY == 2) ? 4 : 5 + Roll % 3;
                    }
                    u32 SX = TerrainCellColumn(Cell) * Tile + (FX % Tile);
                    u32 SY = Kind * Tile + (FY % Tile);
                    Pixel = Atlas[SY * Width + SX];
                }
                Fields[Y * FieldOutWidth + X] = Pixel;
            }
        }
        stbi_write_png("monster_art/terrain_fields.png", FieldOutWidth, FieldOutHeight, 4,
                       Fields, FieldOutWidth * 4);
        printf("monster_art/terrain_fields.png\n");
        free(Fields);
        free(Atlas);
    }
    {
        // NOTE(zoubir): the drawn props side by side on grass, 4 times size
        u32 Size = TERRAIN_PROP_PIXELS;
        u32 Width = Size * TerrainProp_Count;
        u32 *Sheet = (u32 *)calloc(Width * Size, sizeof(u32));
        for(u32 Prop = 0; Prop < TerrainProp_Count; Prop++)
        {
            sprite_canvas Canvas = CanvasFrame(Sheet, Width, Size, Prop, 0);
            DrawTerrainProp(&Canvas, (terrain_prop)Prop);
        }
        u32 Scale = 4;
        u32 OutWidth = Width * Scale;
        u32 OutHeight = Size * Scale;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / Scale) * Width + X / Scale];
                Out[Y * OutWidth + X] = Pixel ? Pixel : ART_RGB(74, 122, 50);
            }
        }
        stbi_write_png("monster_art/terrain_props.png", OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("monster_art/terrain_props.png\n");
        free(Out);
        free(Sheet);
    }
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/map_%s.png", Map->Name);
        if (Map->Kind == MapKind_Bounded)
        {
            WriteMapPreview(Map, -2, -2, Map->Width + 4, Map->Height + 4, 8, Path);
        }
        else
        {
            WriteMapPreview(Map, -160, -100, 320, 200, 3, Path);
            // NOTE(zoubir): a close-up of the first of each landmark found
            // near the origin, with some of its surroundings
            bool32 Shown[ArrayCount(LandmarkTable)] = {};
            for(i32 RY = -4; RY <= 4; RY++)
            {
                for(i32 RX = -4; RX <= 4; RX++)
                {
                    landmark_spot Spot = GetRegionLandmark(Map, RX, RY);
                    if (!Spot.Present || Shown[Spot.Landmark])
                    {
                        continue;
                    }
                    Shown[Spot.Landmark] = true;
                    landmark_def *Def = &LandmarkTable[Spot.Landmark];
                    char LandmarkPath[256];
                    snprintf(LandmarkPath, sizeof(LandmarkPath), "monster_art/landmark_%s.png",
                             Def->Name);
                    WriteMapPreview(Map, Spot.MinX - 6, Spot.MinY - 6, Def->Width + 12,
                                    Def->Height + 12, 12, LandmarkPath);
                }
            }
        }
    }
    return 0;
}
