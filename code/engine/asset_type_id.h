#if !defined(ASSET_TYPE_ID_H)
/* Asset type ids: one entry per kind of asset. Types before
   AssetType_PackCount are stored in asset_1.zas; the ones after it are
   made by code at startup (pictures in code/art, sound effects in
   client/sounds) and never read from the pack, so a new packed type goes
   above that line. asset_family says whether an asset is a
   texture or a sound. */

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
    AssetType_MonsterHazard,
    AssetType_TerrainAtlas,
    AssetType_TerrainProp,
    // NOTE(zoubir): sound effects synthesized at startup
    // (client/sounds/sounds.cpp), one slot each; snapshots carry these
    // ids in a byte, so the whole list stays under 256
    AssetType_SfxJump,
    AssetType_SfxDash,
    AssetType_SfxBlink,
    AssetType_SfxShield,
    AssetType_SfxFireCast,
    AssetType_SfxHit,
    AssetType_SfxSword,
    AssetType_SfxKunai,
    AssetType_SfxAreaCast,
    AssetType_SfxLevelUp,
    AssetType_SfxWardBreak,
    AssetType_SfxRewind,
    AssetType_SfxTaunt,
    AssetType_SfxShieldSlam,
    AssetType_SfxHeal,
    AssetType_SfxWard,
    AssetType_SfxSanctuary,
    AssetType_SfxMeteorCast,
    AssetType_SfxExplosion,
    AssetType_SfxGiantFireball,
    AssetType_SfxCombustion,
    // NOTE(zoubir): the announcer's stings (ui/announcer/), played by the
    // client alone
    AssetType_SfxAnnounce,
    AssetType_SfxFight,
    AssetType_SfxCountdown,
    AssetType_Count
};

enum asset_family
{
    AssetFamily_Texture,
    AssetFamily_Audio,
    AssetFamily_Count
};


#endif
