#if !defined(TEST_ASSET_BUILDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define TEST_ASSET_BUILDER_H

#include "file_formats.h"

struct file_asset_info
{
    asset_family Family;

    char *FilePath;
    u32 Tags;    
    v2 Origin;
    u32 NumTilesX;
    u32 NumTilesY;
};

struct assets_builder
{
    memory_arena Arena;
    
    zas_asset_type_slot AssetTypes[AssetType_Count];

    u32 AllocatedAssetCount;
    u32 AssetCount;
    
    file_asset_info *Infos;

    u32 CurrentInfoIndex;
    asset_type_id CurrentAssetType;
};

#endif

