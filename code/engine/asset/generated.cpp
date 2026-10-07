/* Generated assets: slots for textures drawn by code (code/art) and
   sounds made by code (client/sounds) rather than read from the pack,
   and filling them. A generated asset is loaded for good: nothing reads
   it from the pack again. */

// NOTE(zoubir): textures drawn by code (code/art) instead of read from the
// pack. Reserve a type's slots once, then fill each slot; the pixels are
// RGBA rows from top to bottom, like stb_image gives them
internal void
ReserveGeneratedAssets(assets *Assets, asset_type_id Type, u32 Count)
{
    Assert(Type >= AssetType_PackCount && Type < AssetType_Count);
    zas_asset_type_slot *Slot = &Assets->AssetTypes[Type];
    Assert(Slot->FirstIndex == Slot->OnePastLastIndex);
    u32 FirstFree = Assets->AssetCount + 1;
    for(u32 TypeIndex = AssetType_PackCount;
        TypeIndex < AssetType_Count;
        TypeIndex++)
    {
        FirstFree = Maximum(FirstFree,
                            Assets->AssetTypes[TypeIndex].OnePastLastIndex);
    }
    Assert(FirstFree + Count <= Assets->AssetCount + 1 + MAX_GENERATED_ASSETS);
    Slot->FirstIndex = FirstFree;
    Slot->OnePastLastIndex = FirstFree + Count;
}

internal void
AddGeneratedTexture(assets *Assets, open_gl *OpenGL, asset_id ID,
                    u32 *Pixels, u32 Width, u32 Height,
                    u32 NumTilesX, u32 NumTilesY, v2 Origin)
{
    zas_asset_info *Info = GetAssetInfo(Assets, ID);
    *Info = {};
    Info->Family = AssetFamily_Texture;
    Info->Texture.Channels = 4;
    Info->Texture.Width = Width;
    Info->Texture.Height = Height;
    // NOTE(zoubir): sprites are filtered, which the sprite shader turns
    // into square texels with soft edges (texture_shading.frag). The
    // terrain atlas is not: its cells sit edge to edge, and filtering
    // would bleed each cell's neighbour into a seam along every tile
    u32 Filter = ID.Type == AssetType_TerrainAtlas ? TEXTURE_NO_FILTER : TEXTURE_SOFT_FILTER;
    Info->Texture.Tags = Filter;
    Info->Texture.Origin = Origin;
    Info->Texture.NumTilesX = NumTilesX;
    Info->Texture.NumTilesY = NumTilesY;

    loaded_texture Texture =
        LoadOpenglTexture(Assets, OpenGL, Width, Height, GL_RGBA,
                          Width * Height * 4, Pixels, Filter);
    UploadTexture(Assets, Texture, ID);
}

// NOTE(zoubir): one channel of SampleCount samples, at the mixer's rate;
// Samples must outlive the assets (the permanent arena)
internal void
AddGeneratedAudio(assets *Assets, asset_id ID, i16 *Samples, u32 SampleCount)
{
    zas_asset_info *Info = GetAssetInfo(Assets, ID);
    *Info = {};
    Info->Family = AssetFamily_Audio;
    Info->Audio.Channels = 1;
    Info->Audio.SampleCount = SampleCount;
    loaded_audio Audio = {};
    Audio.Data = Samples;
    Audio.Samples[0] = Samples;
    Audio.Channels = 1;
    Audio.SampleCount = SampleCount;
    Audio.Size = SampleCount * sizeof(i16);
    UploadAudio(Assets, Audio, ID);
}
