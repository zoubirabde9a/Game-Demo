/* Assets: textures and sounds from the .zas pack (asset.h), loaded on
   worker threads and uploaded on the main thread. The calls the game
   makes are here: GetTexture and GetAudio (which start a load when the
   asset is not in memory yet), LoadOpenglTexturesFromQueue (each frame)
   and InitializeAssets (at startup). */
#include "asset.h"

#include "asset/memory_and_upload.cpp"
#include "asset/load.cpp"

inline loaded_texture *
GetTexture(assets *Assets, open_gl *OpenGL,
           app_state *AppState, asset_id ID)
{
    loaded_texture *Result = 0;
    asset *Asset = GetAsset(Assets, ID);
    
    
    if (Asset->State == AssetState_Loaded)
    {
        Result = &Asset->Texture;
    }
    else if (Asset->State == AssetState_Unloaded)
    {
        LoadAsset(Assets, OpenGL, AppState, ID);
    }
    
    return Result;
}
inline loaded_audio *
GetAudio(assets *Assets, app_state *AppState, asset_id ID)
{
    loaded_audio *Result = 0;
    asset *Asset = GetAsset(Assets, ID);
    zas_asset_info *Info = GetAssetInfo(Assets, ID);

    if (Asset->State == AssetState_Unloaded)
    {
        LoadAudio(Assets, ID, Info);
    }
    
    if (Asset->State == AssetState_Loaded)
    {
        Result = &Asset->Audio;
    }
    
    return Result;
}

internal void
LoadOpenglTextureFromEntry(assets *Assets, open_gl *OpenGL,
                           opengl_texture_queue_entry *Entry)
{
    loaded_texture Texture =
        LoadOpenglTexture(Assets,
                          OpenGL, Entry->Width, Entry->Height,
                          Entry->ImageFormat,
                          Entry->TextureSize,
                          Entry->TextureMemory,
                          Entry->Flags);
    
    UploadTexture(Entry->Assets, Texture, Entry->ID);
}

internal void
LoadOpenglTexturesFromQueue(assets *Assets, open_gl *OpenGL, opengl_texture_queue *Queue)
{
    while (Queue->CurrentEntryToRead != Queue->CurrentEntryToWrite)
    {
        opengl_texture_queue_entry *Entry =
            &Queue->Entries[Queue->CurrentEntryToRead];
        LoadOpenglTextureFromEntry(Assets, OpenGL, Entry);
        Entry->TextureMemory = 0;
        Queue->CurrentEntryToRead =
            (Queue->CurrentEntryToRead + 1) %
            ArrayCount(Queue->Entries);
    }
}

internal void
InitializeAssets(assets *Assets, open_gl *OpenGL, app_state *AppState,
                 memory_arena *MemoryArena)
{
    memory_arena *AssetArena = &Assets->Arena;
    SubArena(AssetArena, MemoryArena, Megabytes(32));

    Assets->TotalMemoryUsed = 0;
    
    platform_file_group *FileGroup =
        Platform.GetAllFilesOfTypeBegin("zas");

    for(u32 FileIndex = 0;
        FileIndex < FileGroup->FileCount;
        FileIndex++)
    {
        if (FileIndex == 0)
        {
            platform_file_handle *FileHandle =
                Platform.OpenNextFile(FileGroup);

            Assets->FileHandle = FileHandle;
            zas_header ZASHeader;
            Platform.ReadDataFromFile(FileHandle, 0, sizeof(zas_header), &ZASHeader);
            Assert(ZASHeader.MagicValue == ZAS_MAGIC_VALUE);
            Assert(ZASHeader.Version == ZAS_VERSION);

            u32 AssetCount = ZASHeader.AssetCount;
            u64 AssetTypesOffset = ZASHeader.AssetTypesOffset;
            u64 AssetsInfosOffset = ZASHeader.AssetsInfosOffset;
        
            // NOTE(zoubir): the pack only stores the slots below
            // AssetType_PackCount; generated types start out empty
            zas_asset_type_slot *FileAssetTypes =
                AllocateArray(AssetArena, AssetType_Count, zas_asset_type_slot);
            ZeroSize(FileAssetTypes, AssetType_Count * sizeof(zas_asset_type_slot));
        
            Platform.ReadDataFromFile(FileHandle, AssetTypesOffset,
                                      AssetType_PackCount * sizeof(zas_asset_type_slot),
                                      FileAssetTypes);
            
            Assets->AssetTypes = FileAssetTypes;
            
            zas_asset_info *FileAssetInfos =
                AllocateArray(AssetArena, AssetCount + 1 + MAX_GENERATED_ASSETS,
                              zas_asset_info);
            Platform.ReadDataFromFile(FileHandle, AssetsInfosOffset,
                                      (AssetCount + 1) * sizeof(zas_asset_info),
                                      FileAssetInfos);
            Assets->Infos = FileAssetInfos;
        
            Assets->Assets =
                AllocateArray(AssetArena, AssetCount + MAX_GENERATED_ASSETS, asset);
            ZeroSize(Assets->Assets,
                     (AssetCount + MAX_GENERATED_ASSETS) * sizeof(asset));
            Assets->AssetCount = AssetCount;
        }
    }
    Platform.GetAllFilesOfTypeEnd(FileGroup);

    #if 1
    for(u32 AssetTypeIndex = 1;
        AssetTypeIndex < AssetType_PackCount;
        AssetTypeIndex++)
    {
        if (AssetTypeIndex != AssetType_BattleTheme)
        {
            LoadAsset(Assets, OpenGL, AppState, {(asset_type_id)AssetTypeIndex});
        }
    }
    #endif

            

}

#include "asset/eviction.cpp"
#include "asset/generated.cpp"
