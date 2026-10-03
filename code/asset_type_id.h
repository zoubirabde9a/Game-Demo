#if !defined(ASSET_TYPE_ID_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define ASSET_TYPE_ID_H
enum asset_type_id
{
    //NOTE(zoubir): Textures
    AssetType_Invalid,
    AssetType_Zoubir,
    AssetType_TileMap,
    AssetType_Tree,
    AssetType_Familiar,
    AssetType_Shadow,
    AssetType_FireBall,
    AssetType_Sword,
    //NOTE(zoubir): Audio
    AssetType_OpenSodaSound,
    AssetType_ZoubirAudio,
    AssetType_Dash,
    AssetType_FireCast,
    AssetType_BattleTheme,
    // NOTE(zoubir): types above are stored in asset_1.zas. Types below are
    // drawn by code at startup (code/art) and never read from the pack
    AssetType_PackCount,
    AssetType_Monster = AssetType_PackCount,
    AssetType_MonsterShot,
    AssetType_Count
};

enum asset_family
{
    AssetFamily_Texture,
    AssetFamily_Audio,
    AssetFamily_Count
};


#endif
