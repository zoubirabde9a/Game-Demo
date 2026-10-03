#if !defined(FILE_FORMATS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define FILE_FORMATS_H
#define ZAS_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#pragma pack(push, 1)

struct zas_header
{
#define ZAS_MAGIC_VALUE ZAS_CODE('z', 'a', 's', 'f')
    u32 MagicValue;
#define ZAS_VERSION 0
    u32 Version;

    u32 AssetCount;

    // TODO(zoubir): zas_asset_info_type -> zas_asset_type
    // zas_asset_type -> zas_asset_type_id
    u64 AssetTypesOffset; // zas_asset_type_slot[AssetCount]
    u64 AssetsInfosOffset; // zas_asset_info[AssetCount]
};

struct zas_asset_type_slot
{
    u32 FirstIndex;
    u32 OnePastLastIndex;    
};

struct zas_texture_info
{
    u32 Channels;
    u32 Width;
    u32 Height;
    
    u32 Tags;
    v2 Origin;
    u32 NumTilesX;
    u32 NumTilesY;
};

struct zas_audio_info
{
    u32 SampleCount;
    u32 Channels;
};

struct zas_asset_info
{
    asset_family Family;
    u64 DataOffset;
    union
    {
        zas_texture_info Texture;
        zas_audio_info Audio;
    };
};

#pragma pack(pop)


#endif
