/* Generated textures: slots for textures drawn by code (code/art) rather
   than read from the pack, and filling them. */

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
    Info->Texture.Tags = TEXTURE_NO_FILTER;
    Info->Texture.Origin = Origin;
    Info->Texture.NumTilesX = NumTilesX;
    Info->Texture.NumTilesY = NumTilesY;

    loaded_texture Texture =
        LoadOpenglTexture(Assets, OpenGL, Width, Height, GL_RGBA,
                          Width * Height * 4, Pixels, TEXTURE_NO_FILTER);
    UploadTexture(Assets, Texture, ID);
}
