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
            for(u32 PY = 0; PY < Scale; PY++)
            {
                for(u32 PX = 0; PX < Scale; PX++)
                {
                    u32 Pixel = Color;
                    bool32 Middle = PX >= Scale / 4 && PX < Scale - Scale / 4 &&
                        PY >= Scale / 4 && PY < Scale - Scale / 4;
                    if (Prop != TerrainProp_None && Middle)
                    {
                        Pixel = Prop == TerrainProp_Boulder ? ART_RGB(60, 60, 60) :
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
        }
    }
    return 0;
}
