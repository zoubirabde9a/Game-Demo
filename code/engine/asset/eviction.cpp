/* Eviction: unloading an asset to free its memory (EvictAsset). Nothing
   evicts yet: EvictAssetsAsNecessary is empty and GetLeastUsedAsset is a
   stub. Fill them in if asset memory ever runs short. */

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
}
