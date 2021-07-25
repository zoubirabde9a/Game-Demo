/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */

#include <stdio.h>
#include <stdlib.h>

struct v2
{
    float X, Y;
};

#include "app_defs.h"
#include "memory.h"
struct platform_file_handle;

#include "asset.h"
#include "test_asset_builder.h"



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image\stb_image.h"


struct read_file_result
{
    void *Memory;
    u32 Size;    
};

read_file_result
ReadEntireFile(char *FileName)
{
    Assert(FileName);
    read_file_result Result = {}; 

    FILE *File = 0;
    fopen_s(&File, FileName, "rb");
    if (File)
    {
        fseek(File, 0l, SEEK_END);
        u32 FileSize = ftell(File);
        fseek(File, 0l, SEEK_SET);
        void *Memory =  malloc(FileSize);
        fread(Memory, FileSize, 1, File);
        
        fclose(File);

        Result.Memory = Memory;
        Result.Size = FileSize;        
    }
    
    return Result;
}

void FreeFileMemory(void *Memory)
{
    free(Memory);
}



//TODO(zoubir) #pragma pack(push, 1)
struct zi_header
{
    u8 Channels;
    int Width;
    int Height;
};

struct load_texture_result
{
    u32 Channels;
    u32 Width;
    u32 Height;
    void *Data;
    u32 DataSizeInBytes;
};

internal load_texture_result
LoadTexture(char *FileName)
{
    read_file_result FileResult =
        ReadEntireFile(FileName);

    load_texture_result Result = {};
    
    if (FileResult.Size >= sizeof(zi_header))
    {

        zi_header *Header = (zi_header *)FileResult.Memory;
        void *Data = Header + 1;

        Result.Channels = Header->Channels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.Data = Data;
        Result.DataSizeInBytes = Result.Width * Result.Height * Result.Channels;
    }

    return Result;    
}

load_texture_result
LoadStbTexture(char *FilePath)
{
    load_texture_result Result = {};
    
    int Width, Height, Channels;
    unsigned char* ImageData = stbi_load(FilePath, &Width, &Height, &Channels, 0);
    Assert(ImageData);
    if (ImageData)
    {

        Result.Channels = Channels;
        Result.Width = Width;
        Result.Height = Height;
        Result.Data = ImageData;
        Result.DataSizeInBytes = Result.Width * Result.Height * Result.Channels;
    }

    return Result;
}

#pragma pack(push, 1)
struct wav_header {
    union
    {
        u32 FileTypeBlockID32;
        char FileTypeBlockID[4];
    };
    u32 Size;
    union
    {
        u32 FileFormatID32;
        char FileFormatID[4];
    };
    union
    {
        u32 FormatBlockID32;
        char FormatBlockID[4];
    };    
    u32 ChunkSize;
    i16 FormatType;
    i16 Channels;
    u32 SampleRate;
    u32 AvgBytesPerSec;
    i16 BytesPerSample;
    i16 BitsPerSample;
    union
    {
        u32 DataBlockID32;
        char DataBlockID[4];     
    };
    u32 DataSize;
};
#pragma pack(pop)

struct wav_file
{
    void *FileContents;
    u32 FileSize;
    wav_header *Header;
    void *Data;
    bool32 Corrupt;
};

//NOTE(zoubir): Cannot exceed 32 bit File Size
loaded_audio
LoadWav(char* FilePath) {
    loaded_audio Audio = {};
    
    read_file_result ReadResult
        = ReadEntireFile(FilePath);

    if (ReadResult.Memory == 0) {
        InvalidCodePath;
    }
    
    void *FileContents = ReadResult.Memory;
    u32 FileSize = ReadResult.Size;

    wav_header *Header = (wav_header *)ReadResult.Memory;
    if (Header->FileTypeBlockID32 != 0x46464952 ||
        Header->FileFormatID32 != 0x45564157||
        Header->FormatBlockID32 != 0x20746D66||
        Header->DataBlockID32 != 0x61746164)
    {
        InvalidCodePath;
    }
    else
    {
        i16 *Data = (i16 *)(Header + 1);
        u32 SampleCount = Header->DataSize / Header->BytesPerSample / Header->Channels;
        u32 Channels = Header->Channels;
        u32 SampleCountAlign8 = Align8(SampleCount);
        
        for(u32 ChannelIndex = 0;
            ChannelIndex < Channels;
            ChannelIndex++)
        {
            Audio.Samples[ChannelIndex] =
                (i16 *)malloc(SampleCountAlign8 * sizeof(i16));
        }
        
        for(u32 ChannelIndex = 0;
            ChannelIndex < Channels;
            ChannelIndex++)
        {
            for(u32 SampleIndex = 0;
                SampleIndex < SampleCount;
                SampleIndex++)
            {
                Audio.Samples[ChannelIndex][SampleIndex] =
                    Data[SampleIndex * Channels + ChannelIndex];
            }
        }
        
        for(u32 ChannelIndex = 0;
            ChannelIndex < Channels;
            ChannelIndex++)
        {
            for(u32 SampleIndex = SampleCount;
                SampleIndex < SampleCountAlign8;
                SampleIndex++)
            {
                Audio.Samples[ChannelIndex][SampleIndex] = 0;
            }
        }
        
        Audio.SampleCount = SampleCountAlign8;
        Audio.Channels = Channels;
    }

    FreeFileMemory(ReadResult.Memory);
    return Audio;
}

#if 1

inline void
BeginAssetType(assets_builder *Assets,
               asset_type_id Type, u32 Count)
{
    Assert(Assets->CurrentAssetType == 0);
    Assets->CurrentAssetType = Type;
    zas_asset_type_slot *AssetType = &Assets->AssetTypes[Type];

    AssetType->FirstIndex = Assets->CurrentInfoIndex;
    AssetType->OnePastLastIndex = AssetType->FirstIndex + Count;
}

inline void
AddTextureAsset(assets_builder *Assets, char *FilePath, v2 Origin,
                u32 Tags, u32 NumTilesX = 1, u32 NumTilesY = 1)
{
    Assert(Assets->AssetCount < Assets->AllocatedAssetCount);
    file_asset_info *NewInfo = &Assets->Infos[Assets->CurrentInfoIndex++];
    NewInfo->Family = AssetFamily_Texture;
    // TODO(zoubir): make a temporary memory to
    // Push Strings in it
    Assets->AssetCount++;
    
    NewInfo->FilePath = FilePath;
    NewInfo->Tags = Tags;
    NewInfo->Origin = Origin;
    NewInfo->NumTilesX = NumTilesX;
    NewInfo->NumTilesY = NumTilesY;
}

inline void
AddAudioAsset(assets_builder *Assets, char *FilePath)
{
    Assert(Assets->AssetCount < Assets->AllocatedAssetCount);
    file_asset_info *NewInfo = &Assets->Infos[Assets->CurrentInfoIndex++];
    NewInfo->Family = AssetFamily_Audio;
    Assets->AssetCount++;
    // TODO(zoubir): make a temporary memory to
    // Push Strings in it
    NewInfo->FilePath = FilePath;
}

inline void
EndAssetType(assets_builder *Assets)
{
    Assert(Assets->CurrentAssetType != 0);
    Assert(Assets->AssetTypes[Assets->CurrentAssetType].OnePastLastIndex == Assets->CurrentInfoIndex);
    Assets->CurrentAssetType = AssetType_Invalid;
}


internal void
InitializeAssets(assets_builder *Assets, memory_arena *MemoryArena)
{
    memory_arena *AssetArena = &Assets->Arena;
    SubArena(AssetArena, MemoryArena, Megabytes(32));

    Assets->AllocatedAssetCount = 100;
    Assets->Infos = AllocateArray(AssetArena, Assets->AllocatedAssetCount, file_asset_info);
}

#endif

FILE *Out = 0;

int
main(int ArgCount, char **Args)
{   
#if 1
    memory_index AssetMemorySize = Megabytes(64);
    memory_index *BaseMemory = (memory_index *)calloc(1, AssetMemorySize);
    memory_arena Arena;
    InitializeArena(&Arena, BaseMemory, AssetMemorySize);
    assets_builder *Assets = AllocateStruct(&Arena, assets_builder);
    InitializeAssets(Assets, &Arena);
    
    BeginAssetType(Assets, AssetType_Zoubir, 1);
    AddTextureAsset(Assets, "move-shizuka.png", {0.5f, 0.875f}, TEXTURE_SOFT_FILTER, 16, 16);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_Shadow, 1);
    AddTextureAsset(Assets, "player_shadow.png", {0.5f, 0.5f}, TEXTURE_SOFT_FILTER);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_TileMap, 1);
    AddTextureAsset(Assets, "forest_pallet_01.png", {}, TEXTURE_NO_FILTER, 8, 200);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_Tree, 1);
    AddTextureAsset(Assets, "tree_summer_01.png", {0.5f, 0.931f}, TEXTURE_SOFT_FILTER);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_Familiar, 1);
    AddTextureAsset(Assets, "familiar.png", {0.5f, 0.5f}, TEXTURE_SOFT_FILTER, 2, 1);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_FireBall, 1);
    AddTextureAsset(Assets, "Fireball.png", {0.5f, 0.5f}, TEXTURE_SOFT_FILTER, 4, 4);
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_Sword, 1);
    AddTextureAsset(Assets, "sword2.png", {0.5f, 0.7f}, TEXTURE_SOFT_FILTER, 3, 1);
    EndAssetType(Assets);

    // NOTE(zoubir): Audio
    BeginAssetType(Assets, AssetType_OpenSodaSound, 1);
    AddAudioAsset(Assets, "Soda.wav");
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_ZoubirAudio, 1);
    AddAudioAsset(Assets, "something.wav");
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_Dash, 1);
    AddAudioAsset(Assets, "Dash.wav");
    EndAssetType(Assets);
    BeginAssetType(Assets, AssetType_FireCast, 1);
    AddAudioAsset(Assets, "fire_cast.wav");
    EndAssetType(Assets);
#if 0
    BeginAssetType(Assets, AssetType_BattleTheme, 1);
    AddAudioAsset(Assets, "Battle_Theme2_Hard.wav");
    EndAssetType(Assets);
#endif
    
#endif
    
    fopen_s(&Out, "asset_1.zas", "wb+");
    if (Out)
    {
        zas_header Header = {};
        Header.MagicValue = ZAS_MAGIC_VALUE;
        Header.Version = ZAS_VERSION;
        
        u64 TypesSizeInBytes = sizeof(zas_asset_type_slot) * ArrayCount(Assets->AssetTypes);
        u64 FileInfosSizeInBytes = sizeof(zas_asset_info) * Assets->AssetCount;
        
        Header.AssetCount = Assets->AssetCount;
        Header.AssetTypesOffset = sizeof(Header);
        Header.AssetsInfosOffset = Header.AssetTypesOffset +
            TypesSizeInBytes;
        
        zas_asset_info *FileInfos =
            AllocateArray(&Arena, Assets->AssetCount, zas_asset_info);
        
        fwrite(&Header, sizeof(Header), 1, Out);
        fwrite(Assets->AssetTypes, TypesSizeInBytes, 1, Out);

        fseek(Out, (u32)FileInfosSizeInBytes, SEEK_CUR);

        
        for(u32 AssetInfoIndex = 0;
            AssetInfoIndex < Assets->AssetCount;
            AssetInfoIndex++)
        {
            u64 CurrentFileOffset = ftell(Out);
        
            file_asset_info *Info = &Assets->Infos[AssetInfoIndex];
            zas_asset_info *FileInfo = &FileInfos[AssetInfoIndex];
            FileInfo->Family = Info->Family;
            FileInfo->DataOffset = CurrentFileOffset;
            
            switch(Info->Family)
            {
                case AssetFamily_Texture:
                {
                    zas_texture_info *TextureInfo =
                        &FileInfo->Texture;
                    TextureInfo->Tags = Info->Tags;
                    TextureInfo->Origin = Info->Origin;
                    TextureInfo->NumTilesX = Info->NumTilesX;
                    TextureInfo->NumTilesY = Info->NumTilesY;
                    
                    load_texture_result Texture =
                        LoadStbTexture(Info->FilePath);
                    TextureInfo->Channels = Texture.Channels;
                    TextureInfo->Width = Texture.Width;
                    TextureInfo->Height = Texture.Height;

                    fwrite(Texture.Data, Texture.DataSizeInBytes, 1, Out);

                    //Missing a Free On Image Memory Here
                    
                    break;
                }
                case AssetFamily_Audio:
                {                    
                    zas_audio_info *AudioInfo =
                        &FileInfo->Audio;
                    loaded_audio Audio =
                        LoadWav(Info->FilePath);
                    
                    AudioInfo->SampleCount = Audio.SampleCount;
                    AudioInfo->Channels = Audio.Channels;

                    u32 ChannelSizeInBytes = Audio.SampleCount *
                        sizeof(i16);

                    for(u32 ChannelIndex = 0;
                        ChannelIndex < Audio.Channels;
                        ChannelIndex++)
                    {
                        fwrite(Audio.Samples[ChannelIndex], ChannelSizeInBytes, 1, Out);
                    }
                    
                    break;
                }
                default:
                {
                    InvalidCodePath;
                    break;
                }
            }
        }

        fseek(Out, (u32)Header.AssetsInfosOffset, SEEK_SET);        
        fwrite(FileInfos, FileInfosSizeInBytes, 1, Out);
        
        fclose(Out);
    }
}
