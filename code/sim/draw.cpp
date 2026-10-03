/* Drawing for world entities: tiles, sprites, health bars, debug
   collision boxes, and picking the animation frame for an entity. */

internal void
DrawTileEntity(render_context *RenderContext,
               app_state *AppState,
               render_program TextureProgram,
               assets *Assets,
               world *World,
               world_entity *Entity, v3 CameraOffset)
{
    open_gl *OpenGL = AppState->OpenGL;
    v2 TextureOrigin = V2(0.5f, 0.67f);
        
    v2 EntityCameraPosition = Entity->Position.XY - CameraOffset.XY;
    v2 EntityTexturePosition = EntityCameraPosition -
        TextureOrigin *
        V2(Entity->NumTilesX,
           Entity->NumTilesY + Entity->NumTilesZ - 1) *
        V2(World->TileWidth, World->TileHeight);
    EntityTexturePosition.Y -= Entity->Position.Z;
    
    float SortingValue =
        Entity->Position.Y - Entity->Collision->TotalVolume.HalfDims.Y;
    loaded_texture *Texture = GetTexture(Assets, OpenGL, AppState, Entity->Texture);
    if (Texture)
    {        
        ColorRGBA8 Color;
        Color.ColorU32 = RGBA8_WHITE;
        Color.A = 255;
        BeginBatch(RenderContext, Texture->ID,
                   SortingValue, TextureProgram);
        
        for(u32 TileY = 0;
            TileY < (Entity->NumTilesY + Entity->NumTilesZ - 1);
            TileY++)
        {
            for(u32 TileX = 0;
                TileX < Entity->NumTilesX;
                TileX++)
            {
                u32 TileValue =
                    Entity->TileIndices[TileX + TileY *
                                        (Entity->NumTilesX)];
                //TODO(zoubir): remove TextureTileNum X/Y
                // and replace them with the world
                // tileWidth and TileHeight
                u32 TextureTileNumX = Texture->Width /
                    World->TileWidth;
                u32 TextureTileNumY = Texture->Height /
                    World->TileHeight;
                v4 Uvs =
                    GetTextureUvsFromIndex(Texture->Width,
                                           Texture->Height,
                                           TextureTileNumX,
                                           TextureTileNumY,
                                           TileValue);

                v2 Offset = V2(TileX * World->TileWidth,
                               TileY * World->TileHeight);
                
                RenderQuadTexture(RenderContext,
                                  EntityTexturePosition.X + Offset.X,
                                  EntityTexturePosition.Y + Offset.Y,
                                  (float)World->TileWidth,
                                  (float)World->TileHeight,
                                  Uvs,
                                  Color.ColorU32, Entity->Position.Z);
            }
        }
        
        EndBatch(RenderContext);
        
#if 0
        for(u32 VolumeIndex = 0;
            VolumeIndex < Entity->Collision->VolumesCount;
            VolumeIndex++)
        {
            entity_collision_volume *Volume =
                Entity->Collision->Volumes + VolumeIndex;
            
        v2 CollisionRectPosition = EntityCameraPosition -
            Volume->HalfDims.XY;
        CollisionRectPosition.Y -= Entity->Position.Z -
            Volume->HalfDims.Z + Volume->Offset.Z;
    
        DrawRectangle3D(RenderContext,
                        CollisionRectPosition.X,
                        CollisionRectPosition.Y,
                        Volume->HalfDims.X * 2,
                        Volume->HalfDims.Y * 2,
                        Volume->HalfDims.Z * 2,
                        RGBA8_RED, SortingValue);
        }
    #endif
    }
}
internal void
DrawEntity(render_context *RenderContext,
           app_state *AppState,
           render_program TextureProgram,
           assets *Assets,
           world_entity *Entity, v3 CameraOffset)
{
    open_gl *OpenGL = AppState->OpenGL;
    loaded_texture *Texture = 0;
    zas_texture_info *TextureInfo = 0;
    v2 EntityCameraPosition = Entity->Position.XY - CameraOffset.XY;
    //TODO(zoubir): figure out if we need this check
    // inside assets
    float SortingValue =
        Entity->Position.Y;
    
    if (Entity->Texture.Type)
    {
        TextureInfo = &GetAssetInfo(Assets, Entity->Texture)->Texture;
        Texture = GetTexture(Assets, OpenGL, AppState, Entity->Texture);
        v2 EntityTexturePosition = EntityCameraPosition -
            TextureInfo->Origin * Entity->Dimensions;
        EntityTexturePosition.Y -= Entity->Position.Z;
        
        if (Texture)
        {
            BeginBatch(RenderContext, Texture->ID,
                       SortingValue, TextureProgram);

            ColorRGBA8 Color;
            Color.ColorU32 = RGBA8_WHITE;
            Color.A = 255;
            RenderQuadTexture(RenderContext,
                              EntityTexturePosition.X,
                              EntityTexturePosition.Y,
                              Entity->Dimensions.X,
                              Entity->Dimensions.Y,
                              Entity->Uvs,
                              Color.ColorU32,
                              Entity->Position.Z);
            EndBatch(RenderContext);
        }
        {
            // Hp
            if (Entity->Hp > 0.f && Entity->Hp <= 100.f)
            {
                DrawRectangle(RenderContext,
                              EntityTexturePosition.X +
                              Entity->Dimensions.X * 0.25f,
                              EntityTexturePosition.Y -
                              10,
                              (Entity->Hp / 100.f) *
                              (Entity->Dimensions.X * 0.5f),
                              1,
                              RGBA8_WHITE, SortingValue);
            }
#if 0
            DrawRectangle(RenderContext,
                          EntityTexturePosition.X,
                          EntityTexturePosition.Y,
                          Entity->Dimensions.X,
                          Entity->Dimensions.Y,
                          RGBA8_WHITE, SortingValue);
#endif

        
#if 0
            entity_collision_volume *Volume =
                &Entity->Collision->TotalVolume;
            
            v2 CollisionRectPosition = EntityCameraPosition -
                Volume->HalfDims.XY;
            CollisionRectPosition.Y -= Entity->Position.Z -
                Volume->HalfDims.Z +
                Volume->Offset.Z;
    
            DrawRectangle3D(RenderContext,
                            CollisionRectPosition.X,
                            CollisionRectPosition.Y,
                            Volume->HalfDims.X * 2,
                            Volume->HalfDims.Y * 2,
                            Volume->HalfDims.Z * 2,
                            RGBA8_RED, SortingValue);
#endif
        }

    }
    
#if 0        
    for(u32 VolumeIndex = 0;
        VolumeIndex < Entity->Collision->VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume =
            Entity->Collision->Volumes + VolumeIndex;
            
        v2 CollisionRectPosition = EntityCameraPosition -
            Volume->HalfDims.XY;
        CollisionRectPosition.Y -= Entity->Position.Z -
            Volume->HalfDims.Z +
            Volume->Offset.Z;
    
        DrawRectangle3D(RenderContext,
                        CollisionRectPosition.X,
                        CollisionRectPosition.Y,
                        Volume->HalfDims.X * 2,
                        Volume->HalfDims.Y * 2,
                        Volume->HalfDims.Z * 2,
                        RGBA8_RED, SortingValue);
    }
        
#endif
            
    loaded_texture *ShadowTexture = 0;
    zas_texture_info *ShadowTextureInfo = 0;
    if (Entity->ShadowTexture.Type)
    {
        ShadowTexture = GetTexture(Assets, OpenGL, AppState, Entity->ShadowTexture);
        ShadowTextureInfo = &GetAssetInfo(Assets, Entity->ShadowTexture)->Texture;
        
        v2 ShadowDims = {28, 14};
        ColorRGBA8 ShadowColor;
        ShadowColor.ColorU32 = RGBA8_WHITE;
        if (Entity->Position.Z < 200)
        {
            float Diff = Entity->Position.Z - Entity->GroundZ;
            ShadowColor.A = (u8)(255 - (Diff) - 55);
            if (Diff > 1.f)
            {
                ShadowDims *= (1.f - (Entity->Position.Z - Entity->GroundZ) / 200.f);
            }
        }
        else
        {
            ShadowColor.A = 0;
            ShadowDims = {};
        }
        v2 ShadowPosition = EntityCameraPosition -
            ShadowTextureInfo->Origin * ShadowDims;
        ShadowPosition.Y -= Entity->GroundZ;
    
        if (ShadowTexture)
        {
            BeginBatch(RenderContext, ShadowTexture->ID,
                       Entity->Position.Y, TextureProgram);

                            
            RenderQuadTexture(RenderContext,
                              ShadowPosition.X,
                              ShadowPosition.Y,
                              ShadowDims.X,
                              ShadowDims.Y,
                              {0.f, 0.f, 1.f, 1.f},
                              ShadowColor.ColorU32, 0.f);
            EndBatch(RenderContext);
        }
    
    }
       
    
    
}

internal void
DoEntityAnimation(world_entity *Entity, assets *Assets, app_state *AppState,
                  float DeltaTime, float AnimationSpeedRate,
                  animation_type AnimationType, animation_direction AnimationDirection)
{
    open_gl *OpenGL = AppState->OpenGL;
    loaded_texture *Texture = GetTexture(Assets, OpenGL,
                                         AppState, Entity->Texture);
    zas_texture_info *TextureInfo = &GetAssetInfo(Assets, Entity->Texture)->Texture;
    if (Texture)
    {
        Entity->Uvs = DoAnimation(&Entity->AnimationState, 
                                      Texture->Width,
                                      Texture->Height,
                                      TextureInfo->NumTilesX,
                                      TextureInfo->NumTilesY,
                                      DeltaTime,
                                      AnimationSpeedRate,
                                      Entity->AnimationSet,
                                      AnimationType,
                                      AnimationDirection);
    }
                

}
