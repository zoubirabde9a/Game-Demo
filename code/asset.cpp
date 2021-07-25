/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */

#include "asset.h"

struct load_asset_work
{
    opengl_texture_queue *Queue;
    assets *Assets;
    asset_id ID;
};

inline void *
AcquireAssetMemory(assets *Assets, memory_index Size)
{
    void *Result = Platform.AllocateMemory(Size);
    if (Result)
    {
        Assets->TotalMemoryUsed += Size;
    }
    return Result;
}

inline void
ReleaseAssetMemory(assets *Assets,
                              memory_index Size, void *Memory)
{
    Platform.DeallocateMemory(Memory);
    if (Memory)
    {
        Assets->TotalMemoryUsed -= Size;
    }
}                              
                                  

internal void
AddOpenglTextureToQueue(opengl_texture_queue *Queue,
                        assets *Assets,
                        asset_id ID,
                        u32 TextureSize,
                        void *TextureMemory, u32 Width,
                        u32 Height, i32 ImageFormat,
                        u32 Flags)
{
    opengl_texture_queue_entry NewEntry = {};
    NewEntry.ID = ID;
    NewEntry.Assets = Assets;
    NewEntry.TextureSize = TextureSize;
    NewEntry.TextureMemory = TextureMemory;
    NewEntry.Width = Width;
    NewEntry.Height = Height;
    NewEntry.ImageFormat = ImageFormat;
    NewEntry.Flags = Flags;

    bool32 DoneOperation = 0;
    while(DoneOperation == 0)
    {
        DoneOperation = 
            AtomicCompareExchangeU32(&Queue->AThreadIsAlreadyWritingAnEntry, 1, 0) == 0;
        if (DoneOperation)
        {        
            u32 NextEntryToWrite =
                (Queue->CurrentEntryToWrite + 1) %
                ArrayCount(Queue->Entries);
            opengl_texture_queue_entry *Entry =
                &Queue->Entries[Queue->CurrentEntryToWrite];
            *Entry = NewEntry;
            CompletePreviousWritesBeforeFutureWrites;
            Queue->CurrentEntryToWrite = NextEntryToWrite;
            Queue->AThreadIsAlreadyWritingAnEntry = 0;            
        }
    }
       
}



internal loaded_texture
LoadOpenglTexture(assets *Assets,
                  open_gl *OpenGL, u32 Width, u32 Height,
                  GLenum ImageFormat, memory_index MemorySize,
                  void *TextureMemory, u32 Flags)
{
    
    loaded_texture Texture;
    Texture.Width = Width;
    Texture.Height = Height;
    
    OpenGL->glGenTextures(1, &Texture.ID);
    OpenGL->glBindTexture(GL_TEXTURE_2D, Texture.ID);

    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, ImageFormat,
                 Width, Height, 0,
                 ImageFormat, GL_UNSIGNED_BYTE, TextureMemory);

    //IMPORTANT memory leak with texture memory
    ReleaseAssetMemory(Assets, MemorySize, TextureMemory);
    
//    DEBUGPlatformFreeFileMemory(Entry->MemoryToFree);
    //Set some texture parameters
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if ((Flags & TEXTURE_SOFT_FILTER) == TEXTURE_SOFT_FILTER) {
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else {
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    
    OpenGL->glGenerateMipmap(GL_TEXTURE_2D);
    OpenGL->glBindTexture(GL_TEXTURE_2D, 0);

    return Texture;
}


inline zas_asset_info *
GetAssetInfo(assets *Assets, asset_id ID)
{    
    Assert(ID.Type);
    Assert(ID.Type < AssetType_Count);
    zas_asset_type_slot *AssetType = &Assets->AssetTypes[ID.Type];
    Assert(ID.Index < AssetType->OnePastLastIndex - AssetType->FirstIndex);
    u32 Index = AssetType->FirstIndex + ID.Index;
    zas_asset_info *Result = &Assets->Infos[Index];
    
    return Result;    
}

inline asset *
GetAsset(assets *Assets, asset_id ID)
{
    asset *Asset = 0;
    
    Assert(ID.Type);
    Assert(ID.Type < AssetType_Count);    
    zas_asset_type_slot *AssetType = &Assets->AssetTypes[ID.Type];
    Assert(ID.Index < AssetType->OnePastLastIndex - AssetType->FirstIndex);
    u32 Index = AssetType->FirstIndex + ID.Index;
    Asset = &Assets->Assets[Index];
    
    return Asset;
}

inline void
UploadTexture(assets *Assets, loaded_texture Texture,
              asset_id ID)
{
    asset *Asset = GetAsset(Assets, ID);
    Asset->Texture = Texture;
    
    CompletePreviousWritesBeforeFutureWrites;
    Asset->State = AssetState_Loaded;
}

internal void
LoadTextureDeferred(opengl_texture_queue *Queue,
            assets *Assets,
            asset_id ID, zas_asset_info *AssetInfo)
{
    zas_texture_info *TextureInfo = &AssetInfo->Texture;
    Assert(TextureInfo->Channels == 4 || TextureInfo->Channels == 3);    
    int ImageFormat = (TextureInfo->Channels == 4) ? GL_RGBA : GL_RGB;
    
    u32 TextureSize = TextureInfo->Channels * TextureInfo->Width * TextureInfo->Height;
    //IMPORTANT memory leak
    // void *Data = AllocateSize(&Assets->Arena, TextureSize);
    void *Data = AcquireAssetMemory(Assets, TextureSize);
    
    Platform.ReadDataFromFile(Assets->FileHandle,
                              AssetInfo->DataOffset,
                              TextureSize, Data);
    
    if (PlatformNoFileErrors(Assets->FileHandle))
    {
        AddOpenglTextureToQueue(Queue,
                                Assets,
                                ID,
                                TextureSize,
                                Data,
                                TextureInfo->Width,
                                TextureInfo->Height,
                                ImageFormat,
                                TextureInfo->Tags);   
    }
        

}

internal void
LoadTexture(assets *Assets,
            open_gl *OpenGL,
            asset_id ID, zas_asset_info *AssetInfo)
{
#if 1
    zas_texture_info *TextureInfo = &AssetInfo->Texture;
    Assert(TextureInfo->Channels == 4 || TextureInfo->Channels == 3);    
    int ImageFormat = (TextureInfo->Channels == 4) ? GL_RGBA : GL_RGB;
    
    u32 TextureSize = TextureInfo->Channels * TextureInfo->Width * TextureInfo->Height;
    //IMPORTANT memory leak
    void *Data = AllocateSize(&Assets->Arena, TextureSize);
    
    Platform.ReadDataFromFile(Assets->FileHandle,
                              AssetInfo->DataOffset,
                              TextureSize, Data);
   loaded_texture Texture =
       LoadOpenglTexture(Assets, OpenGL, TextureInfo->Width,
                          TextureInfo->Height,
                         ImageFormat, TextureSize, Data,
                          TextureInfo->Tags);
    
    UploadTexture(Assets, Texture, ID);
    #endif

}

inline void
UploadAudio(assets *Assets, loaded_audio Audio,
              asset_id ID)
{
    asset *Asset = GetAsset(Assets, ID);
//    asset *Asset = &Assets->Assets[ID.Type].Array[ID.Index];
    Asset->Audio = Audio;

    CompletePreviousWritesBeforeFutureWrites;
    Asset->State = AssetState_Loaded;
}

internal void
LoadAudio(assets *Assets,
            asset_id ID,
            zas_asset_info *Info)
{
    loaded_audio Audio;
    zas_audio_info *AudioInfo = &Info->Audio;
    u32 AudioSize = AudioInfo->Channels * AudioInfo->SampleCount * sizeof(i16);
    //void *Data = AllocateSize(&Assets->Arena, AudioSize);
    void *Data = AcquireAssetMemory(Assets, AudioSize);
    
    Platform.ReadDataFromFile(Assets->FileHandle,
                              Info->DataOffset,
                              AudioSize, Data);

    Audio.Data = Data;
    Audio.Channels = AudioInfo->Channels;
    Audio.SampleCount = AudioInfo->SampleCount;
    Audio.Size = AudioSize;
    
    for(u32 ChannelIndex = 0;
        ChannelIndex < Audio.Channels;
        ChannelIndex++)
    {
       Audio.Samples[ChannelIndex] =
           &((i16 *)Data)[ChannelIndex * AudioInfo->SampleCount];
    }
    
    
    UploadAudio(Assets, Audio, ID);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
    load_asset_work *Work = (load_asset_work *)Data;

    zas_asset_info *Info = GetAssetInfo(Work->Assets, Work->ID);
        
    switch(Info->Family)
    {
        case AssetFamily_Texture:
        {
            LoadTextureDeferred(Work->Queue,
                        Work->Assets,
                        Work->ID,
                        Info);
            break;
        }
        case AssetFamily_Audio:
        {
            LoadAudio(Work->Assets, Work->ID, Info);
            break;
        }
        default:
        {
            InvalidCodePath;
        }
    };
}

internal void
PrefetchAsset(assets *Assets, app_state *AppState, asset_id ID)
{
    Assert(ID.Type);
    asset *Asset = GetAsset(Assets, ID);
    bool32 Result = 0;
    if (AtomicCompareExchangeU32((u32 volatile *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded)

    {
        //IMPORTANT(zoubir): memory leak
        //TODO(zoubir): this load_asset_work is never getting
        // freed
        load_asset_work *Work = AllocateStruct(&Assets->Arena, load_asset_work);
        
        Work->Queue = &AppState->OpenglTextureQueue;
        Work->Assets = Assets;
        Work->ID = ID;
    
        Platform.AddWorkEntry(AppState->WorkQueue,
                              LoadAssetWork, Work);
    }    
}

internal void
LoadAsset(assets *Assets, open_gl *OpenGL,
          app_state *AppState, asset_id ID)
{
    zas_asset_info *Info = GetAssetInfo(Assets, ID);
        
    switch(Info->Family)
    {
        case AssetFamily_Texture:
        {
            LoadTexture(Assets, OpenGL, ID, Info);
            break;
        }
        case AssetFamily_Audio:
        {
            LoadAudio(Assets, ID, Info);
            break;
        }
        default:
        {
            InvalidCodePath;
        }
    };
}

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
        
            zas_asset_type_slot *FileAssetTypes =
                AllocateArray(AssetArena, AssetType_Count, zas_asset_type_slot);
        
            Platform.ReadDataFromFile(FileHandle, AssetTypesOffset,
                                      AssetType_Count * sizeof(zas_asset_type_slot),
                                      FileAssetTypes);
            
            Assets->AssetTypes = FileAssetTypes;
            
            zas_asset_info *FileAssetInfos =
                AllocateArray(AssetArena, AssetCount + 1, zas_asset_info);
            Platform.ReadDataFromFile(FileHandle, AssetsInfosOffset,
                                      (AssetCount + 1) * sizeof(zas_asset_info),
                                      FileAssetInfos);
            Assets->Infos = FileAssetInfos;
        
            Assets->Assets =
                AllocateArray(AssetArena, AssetCount, asset);
            Assets->AssetCount = AssetCount;
        }
    }
    Platform.GetAllFilesOfTypeEnd(FileGroup);

    #if 1
    for(u32 AssetTypeIndex = 1;
        AssetTypeIndex < AssetType_Count;
        AssetTypeIndex++)
    {
        if (AssetTypeIndex != AssetType_BattleTheme)
        {
            LoadAsset(Assets, OpenGL, AppState, {(asset_type_id)AssetTypeIndex});
        }
    }
    #endif

            

}

internal void EvictAsset(open_gl *OpenGL, assets *Assets, u32 SlotIndex)
{
    asset *Asset = &Assets->Assets[SlotIndex];
    zas_asset_info *Info = &Assets->Infos[SlotIndex];
    Assert(Asset->State == AssetState_Loaded);

    if (Info->Family == AssetFamily_Texture)
    {
        loaded_texture *Texture = &Asset->Texture;
        OpenGL->glDeleteTextures(1, &Texture->ID);
    }
    else if (Info->Family == AssetFamily_Audio)
    {
        loaded_audio *Audio = &Asset->Audio;
        ReleaseAssetMemory(Assets, Audio->Size, Audio->Data);
    }    
    
    Asset->State = AssetState_Unloaded;    
}

inline u32 GetLeastUsedAsset(assets* Assets)
{
    return 0;
}

internal void EvictAssetsAsNecessary(open_gl *OpenGL, assets *Assets)
{
    #if 0
    while(Assets->TargetMemoryUsed < Assets->TotalMemoryUsed)
    {
        u32 SlotIndex = GetLeastUsedAsset(Assets);
        if (SlotIndex)
        {
            EvictAsset(OpenGL, Assets, SlotIndex);
        }
        else
        {
            InvalidCodePath;
            break;
        }
    }
    #endif
}
