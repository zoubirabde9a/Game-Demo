/* Hero skins: in a dungeon run, each class's player is drawn from that
   class's sheet instead of the original hero's; outside a run (duels)
   every player wears the base class's sheet, the one a player has on
   joining (PlayerRole_Damage). A sheet is the original
   hero's own, cell for cell (16 by 16 cells of 32 pixels), recoloured for
   the class and with the hair cut short (misc/hero_skins/make_skins.py),
   loaded from heroes/<class>.png. So only the texture changes: the frames
   are picked by the original's animation table (sim/animations.cpp),
   with its timing and facings, drawn HERO_SKIN_DRAW_SCALE larger. A sheet that fails to load leaves
   that class in the original hero.

   Each class also has a newly drawn sheet, heroes_v2/<class>.png, with
   frames of its own (hero_looks.cpp); the player picks which look every
   hero is drawn in. A class whose new sheet fails to load keeps the
   classic one. Slot Role of AssetType_HeroSkin is the classic sheet,
   slot PlayerRole_Count + Role the new one. */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_ASSERT(x) Assert(x)
#pragma warning(push)
#pragma warning(disable: 4244 4456 4457 4701 4702 4996)
#include "../../third_party/stb_image/stb_image.h"
#pragma warning(pop)

// NOTE(zoubir): a class's hero is drawn this much larger than the
// original, about the feet; only the picture grows, not the body
#define HERO_SKIN_DRAW_SCALE 1.15f

// NOTE(zoubir): the original hero's sheet layout (test_asset_builder.cpp)
#define HERO_SHEET_PIXELS 512
#define HERO_SHEET_CELLS 16

global_variable char *HeroSkinFileNames[PlayerRole_Count] =
{
    "firemage", "bulwark", "mender", "ranger", "berserker", "shadowblade", "stormcaller",
    "duelist", "frostmage", "druid",
};
global_variable bool32 HeroSkinLoaded[PlayerRole_Count];
global_variable bool32 HeroLookLoaded[PlayerRole_Count];

// NOTE(zoubir): heroes/<Name>, decoded and uploaded as ID; false when the
// file is missing or is not a 512-pixel sheet. The upload frees the
// pixels it is given (startup.cpp's white texture tells that story), so
// it gets a copy in TempArena and stb_image's own buffer is freed here
internal bool32
LoadHeroSheet(assets *Assets, open_gl *OpenGL, memory_arena *TempArena, char *Folder,
              char *Name, asset_id ID, v2 Origin)
{
    char Path[96];
    snprintf(Path, sizeof(Path), "%s/%s.png", Folder, Name);
    debug_read_file_result File = Platform.ReadEntireFile(Path);
    bool32 Result = false;
    if (File.Memory)
    {
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        u8 *Pixels = stbi_load_from_memory((u8 *)File.Memory, (int)File.Size,
                                           &Width, &Height, &Channels, 4);
        if (Pixels && Width == HERO_SHEET_PIXELS && Height == HERO_SHEET_PIXELS)
        {
            temporary_memory Temp = BeginTemporaryMemory(TempArena);
            u32 Count = (u32)(Width * Height);
            u32 *Copy = AllocateArray(TempArena, Count, u32);
            memcpy(Copy, Pixels, Count * sizeof(u32));
            AddGeneratedTexture(Assets, OpenGL, ID, Copy, (u32)Width, (u32)Height,
                                HERO_SHEET_CELLS, HERO_SHEET_CELLS, Origin);
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
    ReserveGeneratedAssets(Assets, AssetType_HeroSkin, 2 * PlayerRole_Count);
    // NOTE(zoubir): placed like the original, on its texture's origin
    v2 Origin = GetAssetInfo(Assets, {AssetType_Zoubir})->Texture.Origin;
    for(u32 Role = 0; Role < PlayerRole_Count; Role++)
    {
        HeroSkinLoaded[Role] = LoadHeroSheet(Assets, OpenGL, TempArena, (char *)"heroes",
                                             HeroSkinFileNames[Role],
                                             {AssetType_HeroSkin, Role}, Origin);
        HeroLookLoaded[Role] = LoadHeroSheet(Assets, OpenGL, TempArena, (char *)"heroes_v2",
                                             HeroSkinFileNames[Role],
                                             {AssetType_HeroSkin, PlayerRole_Count + Role},
                                             Origin);
    }
}

// NOTE(zoubir): the size an entity's sprite is drawn at
inline v2
SpriteDrawDimensions(world_entity *Entity)
{
    v2 Result = Entity->Dimensions;
    if (Entity->Texture.Type == AssetType_HeroSkin)
    {
        Result *= HERO_SKIN_DRAW_SCALE;
    }
    return Result;
}

// NOTE(zoubir): the texture is a class's newly drawn sheet, whose frames
// hero_looks.cpp picks
inline bool32
IsHeroLookTexture(asset_id Texture)
{
    bool32 Result = Texture.Type == AssetType_HeroSkin && Texture.Index >= PlayerRole_Count;
    return Result;
}

// NOTE(zoubir): from UpdateEntityUvs: a player in a run takes its class's
// sheet, a player in a duel the base class's, in the look that player
// picked: this machine's own setting (GlobalHeroLook) for the local
// player, written to its entity for the offline simulation, and the one
// the server sent (HeroLookPick) for everyone else. The classic sheet
// when the new one failed to load, the original hero's when that one did
// too
internal void
PickHeroSkin(app_state *AppState, world_entity *Entity)
{
    if (Entity->Type != EntityType_Player)
    {
        return;
    }
    asset_id Texture = {AssetType_Zoubir};
    if (Entity->PlayerIndex < MAX_PLAYERS)
    {
        u32 Role = IsDungeon(AppState) ? AppState->Players[Entity->PlayerIndex].Role :
            (u32)PlayerRole_Damage;
        if (Entity == GetLocalPlayer(AppState))
        {
            Entity->HeroLookPick = (GlobalHeroLook == HeroLook_Classic) ? 1 : 0;
        }
        bool32 New = Entity->HeroLookPick == 0;
        if (Role < PlayerRole_Count && New && HeroLookLoaded[Role])
        {
            Texture = {AssetType_HeroSkin, PlayerRole_Count + Role};
        }
        else if (Role < PlayerRole_Count && HeroSkinLoaded[Role])
        {
            Texture = {AssetType_HeroSkin, Role};
        }
    }
    Entity->Texture = Texture;
}
