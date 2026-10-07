/* Client-side drawing of world entities: tiles, sprites, health bars,
   the ground rings under players, and the animation frame the
   simulation is on.
   Reads entity state only; SimulateTick has already run this frame.

   Every entity sorts by where it stands: its Y plus the height of the
   ground under it (draw_tilemap.cpp lists the keys raised ground uses),
   and is never drawn below that ground. */

// NOTE(zoubir): sort keys of the back-to-front world pass, in world units;
// lower draws first. Flat ground is one batch under everything
#define FLAT_GROUND_SORT_KEY -1.0e7f

// NOTE(zoubir): the top of a raised tile in row TileY, Height units up
inline float
RaisedTopSortKey(i32 TileY, float TileHeight, float Height)
{
    float Result = (float)TileY * TileHeight + Height;
    return Result;
}

// NOTE(zoubir): a face whose foot is the front edge of row FootTileY, on
// ground FootHeight up: after the top of the tile it hangs over (it shades
// that tile), before anyone standing on it
inline float
CliffFaceSortKey(i32 FootTileY, float TileHeight, float FootHeight)
{
    float Result = (float)FootTileY * TileHeight + FootHeight + 0.25f;
    return Result;
}

// NOTE(zoubir): something standing at world Y on ground GroundZ up. The
// extra step keeps a sprite's feet over the next row's top of the same
// height, and still under a top one step higher in front of it
inline float
StandingSortKey(float Y, float GroundZ)
{
    float Result = Y + GroundZ + ELEVATION_STEP_HEIGHT;
    return Result;
}

// NOTE(zoubir): height of the map's raised ground under a world point
internal float
TerrainHeightAt(world *World, float X, float Y)
{
    float Result = 0.f;
    if (World->TileWidth && World->TileMap.Texture.Type == AssetType_TerrainAtlas)
    {
        map_def *Map = GetMapDef((map_id)World->MapId);
        i32 TileX = FloorDiv((i32)floorf(X), (i32)World->TileWidth);
        i32 TileY = FloorDiv((i32)floorf(Y), (i32)World->TileHeight);
        Result = (float)ElevationAt(Map, TileX, TileY) * ELEVATION_STEP_HEIGHT;
    }
    return Result;
}

// NOTE(zoubir): what an entity stands on: the simulation's GroundZ (a
// crate, a wall top) or the raised ground under it, whichever is higher.
// Replicas online carry no GroundZ, so the ground has to come from the map
inline float
EntityGroundZ(world *World, world_entity *Entity)
{
    float Result = Maximum(Entity->GroundZ,
                           TerrainHeightAt(World, Entity->Position.X,
                                           Entity->Position.Y));
    return Result;
}

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
        
    }
}

// NOTE(zoubir): player rings, shadows and the water test
#include "draw_entities/ground_contact.cpp"

internal void
DrawEntity(render_context *RenderContext,
           app_state *AppState,
           render_program TextureProgram,
           assets *Assets,
           world_entity *Entity, v3 CameraOffset)
{
    // NOTE(zoubir): a swing's blade and its trail are drawn by its burst
    // (fx_bursts/sword_arc.cpp); the sword entity is only its hitbox
    if (Entity->Type == EntityType_Sword)
    {
        return;
    }
    open_gl *OpenGL = AppState->OpenGL;
    world *World = &AppState->World;
    loaded_texture *Texture = 0;
    zas_texture_info *TextureInfo = 0;
    v2 EntityCameraPosition = Entity->Position.XY - CameraOffset.XY;
    float GroundZ = EntityGroundZ(World, Entity);
    // NOTE(zoubir): never drawn sunk into the ground it is over, even
    // when the simulation has not lifted it there (props on raised tiles)
    float DrawZ = Maximum(Entity->Position.Z, GroundZ);
    float SortingValue = StandingSortKey(Entity->Position.Y, GroundZ);
    // NOTE(zoubir): ground patches lie flat on the ground, under every
    // standing sprite: with the flat ground, or just over a raised top
    if (Entity->Type == EntityType_MonsterHazard)
    {
        SortingValue = FLAT_GROUND_SORT_KEY + 1.f;
        if (GroundZ > 0.f)
        {
            i32 TileY = FloorDiv((i32)floorf(Entity->Position.Y), (i32)World->TileHeight);
            SortingValue = RaisedTopSortKey(TileY, (float)World->TileHeight, GroundZ) + 0.5f;
        }
    }
    // NOTE(zoubir): squash and stretch about the sprite's origin (its
    // feet), tilt and hit flash, body_pose.cpp
    body_pose_draw Pose = GetBodyPose(AppState, Entity);
    // NOTE(zoubir): leafy trees lean a little in the wind about their roots,
    // each on its own beat, with a slower gust over the whole map
    if (Entity->Texture.Type == AssetType_Tree)
    {
        float Seconds = (float)(AppState->UpdateID % 36000) / 60.f;
        float Beat = 0.013f * Entity->Position.X + 0.021f * Entity->Position.Y;
        float Gust = 0.6f + 0.4f * Sin(0.31f * Seconds + 0.002f * Entity->Position.X);
        Pose.Angle = TREE_SWAY_ANGLE * Gust * Sin(1.3f * Seconds + Beat);
        Pose.AboutFeet = true;
    }
    
    if (Entity->Texture.Type)
    {
        TextureInfo = &GetAssetInfo(Assets, Entity->Texture)->Texture;
        Texture = GetTexture(Assets, OpenGL, AppState, Entity->Texture);
        v2 Dimensions = Entity->Dimensions * Pose.Scale;
        v2 EntityTexturePosition = EntityCameraPosition -
            TextureInfo->Origin * Dimensions;
        EntityTexturePosition.Y -= DrawZ;
        if (Pose.AboutFeet)
        {
            // NOTE(zoubir): the quad turns about its middle; move it so
            // the feet stay put
            float FeetBelowMiddle = (TextureInfo->Origin.Y - 0.5f) * Dimensions.Y;
            EntityTexturePosition.X += FeetBelowMiddle * Sin(Pose.Angle);
            EntityTexturePosition.Y += FeetBelowMiddle * (1.f - Cos(Pose.Angle));
        }
        
        if (Texture && (Entity->Type == EntityType_Player ||
                        Entity->Type == EntityType_Monster) &&
            IsStandingInWater(World, Entity, GroundZ, DrawZ))
        {
            // NOTE(zoubir): the body upside down in the water: mirrored about
            // the ground line, so a body Lift above the water shows Lift
            // below it; flipped by swapping the frame's top and bottom, faint
            // and blue, over the water and under everything standing
            float Ground = EntityCameraPosition.Y - GroundZ;
            float Lift = DrawZ - GroundZ;
            float Above = (1.f - TextureInfo->Origin.Y) * Dimensions.Y;
            v4 Flipped = V4(Entity->Uvs.X, Entity->Uvs.W, Entity->Uvs.Z, Entity->Uvs.Y);
            BeginBatch(RenderContext, Texture->ID, FLAT_GROUND_SORT_KEY + 2.f, TextureProgram);
            RenderQuadTexture(RenderContext,
                              EntityCameraPosition.X - TextureInfo->Origin.X * Dimensions.X,
                              Ground + Lift - Above, Dimensions.X, Dimensions.Y, Flipped,
                              WATER_REFLECTION_COLOR, 0.f, -Pose.Angle);
            EndBatch(RenderContext);
        }
        if (Texture && (Entity->Type == EntityType_Player ||
                        Entity->Type == EntityType_Monster))
        {
            // NOTE(zoubir): a dark outline so units read against grass and
            // stone: the sprite four times in shadow, one world unit out on
            // each side, just under the sprite itself
            BeginBatch(RenderContext, Texture->ID,
                       SortingValue - 0.001f, TextureProgram);
            v2 Sides[4] = {V2(-1.f, 0.f), V2(1.f, 0.f), V2(0.f, -1.f), V2(0.f, 1.f)};
            for(u32 Side = 0; Side < 4; Side++)
            {
                RenderQuadTexture(RenderContext,
                                  EntityTexturePosition.X + Sides[Side].X,
                                  EntityTexturePosition.Y + Sides[Side].Y,
                                  Dimensions.X, Dimensions.Y, Entity->Uvs,
                                  UNIT_OUTLINE_COLOR, DrawZ, Pose.Angle);
            }
            EndBatch(RenderContext);
        }
        if (Texture)
        {
            BeginBatch(RenderContext, Texture->ID,
                       SortingValue, TextureProgram);

            ColorRGBA8 Color;
            Color.ColorU32 = Entity->Tint ? Entity->Tint : RGBA8_WHITE;
            Color.A = 255;
            // NOTE(zoubir): a shielded player (respawn or Shield) is drawn
            // whole, inside a bubble (player_fx/shield_bubble.cpp)
            // NOTE(zoubir): a fresh hit draws the sprite red (body_pose.cpp)
            float Keep = 1.f - 0.75f * Pose.Flash;
            Color.G = (u8)(Color.G * Keep);
            Color.B = (u8)(Color.B * Keep);
            RenderQuadTexture(RenderContext,
                              EntityTexturePosition.X,
                              EntityTexturePosition.Y,
                              Dimensions.X,
                              Dimensions.Y,
                              Entity->Uvs,
                              Color.ColorU32,
                              DrawZ, Pose.Angle);
            EndBatch(RenderContext);
            if (Pose.White > 0.f)
            {
                // NOTE(zoubir): the first frames of a hit add the sprite
                // over itself as light, which washes it pale but keeps its
                // shading, so the cut drawn over it (fx_bursts.cpp) shows.
                // Twice washed it to a flat white blob that hid the cut
                BeginBatch(RenderContext, Texture->ID,
                           SortingValue + 0.001f, TextureProgram);
                RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend =
                    RenderBlend_Additive;
                u32 Light = ((u32)(255.f * Pose.White) << 24) | 0x00FFFFFF;
                RenderQuadTexture(RenderContext,
                                  EntityTexturePosition.X,
                                  EntityTexturePosition.Y,
                                  Dimensions.X,
                                  Dimensions.Y,
                                  Entity->Uvs,
                                  Light,
                                  DrawZ, Pose.Angle);
                EndBatch(RenderContext);
            }
        }
        // NOTE(zoubir): a thin health line over the sprite
        if (Entity->MaxHp > 0.f && Entity->Hp > 0.f)
        {
            DrawRectangle(RenderContext,
                          EntityTexturePosition.X + Entity->Dimensions.X * 0.25f,
                          EntityTexturePosition.Y - 10,
                          (Entity->Hp / Entity->MaxHp) * (Entity->Dimensions.X * 0.5f),
                          1, RGBA8_WHITE, SortingValue);
        }
    }

    if (Entity->Type == EntityType_Player)
    {
        u32 MarkerColor = Entity == GetLocalPlayer(AppState) ?
            SELF_MARKER_COLOR : FOE_MARKER_COLOR;
        DrawSelfMarker(RenderContext,
                       V2(EntityCameraPosition.X, EntityCameraPosition.Y - GroundZ),
                       SortingValue, MarkerColor);
    }
    DrawEntityShadow(RenderContext, AppState, TextureProgram, Assets, Entity,
                     EntityCameraPosition, GroundZ, DrawZ, SortingValue, Pose.Scale.X);
}

// NOTE(zoubir): picks the sprite-sheet frame for the animation the
// simulation is playing; drawing only, never advances it. A body frozen
// by a hit keeps the frame it had (body_pose.cpp)
internal void
UpdateEntityUvs(world_entity *Entity, assets *Assets, app_state *AppState)
{
    if (!Entity->AnimationSet || IsBodyFrozen(AppState, Entity))
    {
        return;
    }
    loaded_texture *Texture = GetTexture(Assets, AppState->OpenGL,
                                         AppState, Entity->Texture);
    zas_texture_info *TextureInfo =
        &GetAssetInfo(Assets, Entity->Texture)->Texture;
    if (Texture)
    {
        // NOTE(zoubir): a dashing player leans into the dash, hair
        // streaming (the sheet's take-off frame), instead of running its
        // walk cycle at full tilt; a swing or cast keeps its own frames
        animation_state Shown = Entity->AnimationState;
        bool32 Dashing = Entity->Type == EntityType_Player &&
            (Entity->DashFlash > 0.f || (Entity->AbilityIndex & PLAYER_FLASH_DASH));
        if (Dashing && (Shown.CurrentType == AnimationType_Move ||
                        Shown.CurrentType == AnimationType_Stand ||
                        Shown.CurrentType == AnimationType_Stop))
        {
            Shown.CurrentType = AnimationType_JumpUp;
            Shown.SlotIndex = 0;
        }
        Entity->Uvs = GetAnimationUvs(&Shown, Entity->AnimationSet,
                                      Texture->Width, Texture->Height,
                                      TextureInfo->NumTilesX,
                                      TextureInfo->NumTilesY);
    }
}

// NOTE(zoubir): every present entity, sorted by the renderer's batch order
internal void
DrawWorldEntities(render_context *RenderContext, app_state *AppState,
                  assets *Assets, render_program TextureProgram,
                  v3 CameraOffset)
{
    world *World = &AppState->World;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || IsDeadPlayer(Entity))
        {
            continue;
        }
        if (Entity->Type == EntityType_Tiled)
        {
            DrawTileEntity(RenderContext, AppState, TextureProgram,
                           Assets, World, Entity, CameraOffset);
        }
        else
        {
            UpdateEntityUvs(Entity, Assets, AppState);
            DrawEntity(RenderContext, AppState, TextureProgram,
                       Assets, Entity, CameraOffset);
        }
    }
}
