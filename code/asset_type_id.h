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
    AssetType_Count
};

enum asset_family
{
    AssetFamily_Texture,
    AssetFamily_Audio,
    AssetFamily_Count
};


#endif
