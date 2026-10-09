/* Hero skins (sim/dungeon/roles.cpp, hero_skin): in a dungeon run, a
   player is drawn from their class's skin's sheet instead of the
   original hero. Two skins a class: the chibi one drawn in code
   (art/heroes/), and the taller one painted from the LPC pack, loaded from
   heroes/<class>_lpc.png (misc/lpc_heroes/pack.py says how it is laid
   out; heroes/CREDITS.txt names its artists). An LPC sheet that fails to
   load falls back to the chibi one.

   Skins are drawing only. The simulation still plays the original hero's
   animation table, whose timings the attack and cast states wait on;
   each frame here is picked from how far through its animation the body
   is, so a skin with more frames plays them in the same time. A fresh
   hit plays the hurt frames, and a downed body lies in its death frames
   (DrawsDownedHero, draw_entities.cpp). */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_ASSERT(x) Assert(x)
#pragma warning(push)
#pragma warning(disable: 4244 4456 4457 4701 4702 4996)
#include "../../third_party/stb_image/stb_image.h"
#pragma warning(pop)

#define HERO_SKIN_CLASSES PlayerRole_Count
// NOTE(zoubir): asset slots: a sheet per class and skin, then the wide
// attack sheets per class
#define HERO_SKIN_ASSETS (HERO_SKIN_CLASSES * HeroSkin_Count + HERO_SKIN_CLASSES)
// NOTE(zoubir): an LPC cell is 64 pixels with the body 47 tall; drawn at
// 48 world units it stands as tall as the chibi skin's body
#define HERO_LPC_CELL 64
#define HERO_LPC_DRAW 48.f
#define HERO_LPC_ROWS 26
#define HERO_LPC_FEET 61.f

struct hero_skin_sheet
{
    bool32 Loaded;
    bool32 Lpc;
    u8 Frames[HeroAnim_Count];
    float IdleSeconds;
    bool32 WideAttack;
};

// NOTE(zoubir): must match FRAMES in misc/lpc_heroes/pack.py
global_variable u8 HeroLpcFrames[HERO_SKIN_CLASSES][HeroAnim_Count] =
{
    {2, 8, 8, 7, 3, 2, 3, 6},
    {2, 8, 6, 7, 3, 5, 3, 6},
    {2, 8, 8, 7, 3, 2, 3, 6},
    {2, 8, 6, 7, 3, 5, 3, 6},
    {2, 8, 6, 7, 3, 5, 3, 6},
    {2, 8, 6, 7, 3, 5, 3, 6},
    {2, 8, 8, 7, 3, 2, 3, 6},
    {2, 8, 6, 7, 3, 5, 3, 6},
    {2, 8, 8, 7, 3, 2, 3, 6},
    {2, 8, 8, 7, 3, 2, 3, 6},
};
global_variable char *HeroSkinFileNames[HERO_SKIN_CLASSES] =
{
    "firemage", "bulwark", "mender", "ranger", "berserker", "shadowblade", "stormcaller",
    "duelist", "frostmage", "druid",
};

global_variable hero_skin_sheet HeroSkinSheets[HERO_SKIN_CLASSES][HeroSkin_Count];
// NOTE(zoubir): world units each skin asset's cell is drawn at
global_variable float HeroSkinDrawSizes[HERO_SKIN_ASSETS];

inline asset_id
HeroSkinAsset(u32 Role, u32 Skin)
{
    asset_id Result = {AssetType_HeroSkin, Role * HeroSkin_Count + Skin};
    return Result;
}

inline asset_id
HeroWideAsset(u32 Role)
{
    asset_id Result = {AssetType_HeroSkin, HERO_SKIN_CLASSES * HeroSkin_Count + Role};
    return Result;
}

// NOTE(zoubir): heroes/<Name>, decoded to RGBA and uploaded as ID; false
// when the file is missing or is not a PNG. The upload frees the pixels
// it is given (startup.cpp's white texture tells that story), so it gets
// a copy in TempArena and stb_image's own buffer is freed here
internal bool32
LoadHeroPng(assets *Assets, open_gl *OpenGL, memory_arena *TempArena, char *Name, asset_id ID,
            u32 Columns, u32 Rows, v2 Origin)
{
    char Path[96];
    snprintf(Path, sizeof(Path), "heroes/%s", Name);
    debug_read_file_result File = Platform.ReadEntireFile(Path);
    bool32 Result = false;
    if (File.Memory)
    {
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        u8 *Pixels = stbi_load_from_memory((u8 *)File.Memory, (int)File.Size,
                                           &Width, &Height, &Channels, 4);
        if (Pixels && Width % Columns == 0 && Height % Rows == 0)
        {
            temporary_memory Temp = BeginTemporaryMemory(TempArena);
            u32 Count = (u32)(Width * Height);
            u32 *Copy = AllocateArray(TempArena, Count, u32);
            memcpy(Copy, Pixels, Count * sizeof(u32));
            AddGeneratedTexture(Assets, OpenGL, ID, Copy, (u32)Width, (u32)Height,
                                Columns, Rows, Origin);
            EndTemporaryMemory(Temp);
            Result = true;
        }
        stbi_image_free(Pixels);
        Platform.FreeFileMemory(File.Memory);
    }
    return Result;
}

// NOTE(zoubir): call once at startup, after the fonts
internal void
AddHeroTextures(assets *Assets, open_gl *OpenGL, memory_arena *TempArena)
{
    ReserveGeneratedAssets(Assets, AssetType_HeroSkin, HERO_SKIN_ASSETS);
    u32 Width = HERO_SHEET_COLUMNS * HERO_FRAME_SIZE;
    u32 Height = HERO_SHEET_ROWS * HERO_FRAME_SIZE;
    for(u32 Role = 0; Role < HERO_SKIN_CLASSES; Role++)
    {
        temporary_memory Temp = BeginTemporaryMemory(TempArena);
        u32 *Pixels = AllocateArray(TempArena, Width * Height, u32);
        BuildHeroSheet(Role, Pixels);
        AddGeneratedTexture(Assets, OpenGL, HeroSkinAsset(Role, HeroSkin_Chibi), Pixels,
                            Width, Height, HERO_SHEET_COLUMNS, HERO_SHEET_ROWS,
                            V2(0.5f, HERO_FEET_Y / (float)HERO_FRAME_SIZE));
        EndTemporaryMemory(Temp);
        hero_skin_sheet *Chibi = &HeroSkinSheets[Role][HeroSkin_Chibi];
        Chibi->Loaded = true;
        Chibi->IdleSeconds = HeroAnimDefs[HeroAnim_Idle].SecondsPerFrame;
        for(u32 Anim = 0; Anim < HeroAnim_Count; Anim++)
        {
            Chibi->Frames[Anim] = (u8)HeroAnimDefs[Anim].FrameCount;
        }
        HeroSkinDrawSizes[HeroSkinAsset(Role, HeroSkin_Chibi).Index] = (float)HERO_FRAME_SIZE;

        hero_skin_sheet *Lpc = &HeroSkinSheets[Role][HeroSkin_Heroic];
        char Name[64];
        snprintf(Name, sizeof(Name), "%s_lpc.png", HeroSkinFileNames[Role]);
        Lpc->Loaded = LoadHeroPng(Assets, OpenGL, TempArena, Name, HeroSkinAsset(Role, HeroSkin_Heroic),
                                  HERO_SHEET_COLUMNS, HERO_LPC_ROWS,
                                  V2(0.5f, HERO_LPC_FEET / (float)HERO_LPC_CELL));
        Lpc->Lpc = true;
        Lpc->IdleSeconds = 0.25f;
        for(u32 Anim = 0; Anim < HeroAnim_Count; Anim++)
        {
            Lpc->Frames[Anim] = HeroLpcFrames[Role][Anim];
        }
        HeroSkinDrawSizes[HeroSkinAsset(Role, HeroSkin_Heroic).Index] = HERO_LPC_DRAW;
        // NOTE(zoubir): an attack drawn past its cell (the Bulwark's sword)
        // has a sheet of double cells, the body in the middle
        snprintf(Name, sizeof(Name), "%s_lpc_wide.png", HeroSkinFileNames[Role]);
        u32 Frames = Lpc->Frames[HeroAnim_Attack];
        Lpc->WideAttack = Lpc->Loaded &&
            LoadHeroPng(Assets, OpenGL, TempArena, Name, HeroWideAsset(Role), Frames, 4,
                        V2(0.5f, (HERO_LPC_FEET + HERO_LPC_CELL / 2) / (float)(2 * HERO_LPC_CELL)));
        HeroSkinDrawSizes[HeroWideAsset(Role).Index] = 2.f * HERO_LPC_DRAW;
    }
}

// NOTE(zoubir): the sheet a player is drawn from, 0 for the original hero
internal hero_skin_sheet *
HeroSkinSheetOf(app_state *AppState, world_entity *Entity, u32 *RoleOut, u32 *SkinOut)
{
    hero_skin_sheet *Result = 0;
    if (Entity->Type == EntityType_Player && IsDungeon(AppState) &&
        Entity->PlayerIndex < MAX_PLAYERS)
    {
        player_slot *Slot = &AppState->Players[Entity->PlayerIndex];
        u32 Role = Slot->Role;
        u32 Skin = Slot->Skin < HeroSkin_Count ? Slot->Skin : HeroSkin_Chibi;
        if (Role < HERO_SKIN_CLASSES)
        {
            if (!HeroSkinSheets[Role][Skin].Loaded)
            {
                Skin = HeroSkin_Chibi;
            }
            if (HeroSkinSheets[Role][Skin].Loaded)
            {
                Result = &HeroSkinSheets[Role][Skin];
                *RoleOut = Role;
                *SkinOut = Skin;
            }
        }
    }
    return Result;
}

#include "hero_skin_frames.cpp"
