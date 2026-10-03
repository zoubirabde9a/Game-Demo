/* Loading: finding an asset in the pack, decoding textures and audio on a
   worker thread (LoadAssetWork), PrefetchAsset to start that early, and
   LoadAsset to load one now. */

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
