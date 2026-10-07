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

// NOTE(zoubir): a gold ring on the ground under the local player, so it
// finds itself at a glance in a crowd of look-alike players, and a fainter
// red one under every other player: in the duel rules each is a foe. Drawn
// with the ring shader on a flat quad (an ellipse), sorted just under the
// shadow, so every sprite covers it
#define SELF_MARKER_WIDTH 44.f
#define SELF_MARKER_HEIGHT 22.f
#define SELF_MARKER_COLOR UI_RGBA(240, 200, 48, 190)
#define FOE_MARKER_COLOR UI_RGBA(235, 70, 60, 120)
// NOTE(zoubir): the outline around players and monsters (DrawEntity)
#define UNIT_OUTLINE_COLOR 0x9A000000
// NOTE(zoubir): the borrowed shadow under monsters and props (DrawEntity):
// its width as a share of the sprite's, and how dark next to a player's
#define MONSTER_SHADOW_WIDTH 0.5f
#define MONSTER_SHADOW_ALPHA 0.85f
#define PROP_SHADOW_WIDTH 1.0f
#define TREE_SHADOW_WIDTH 0.7f
#define PROP_SHADOW_ALPHA 0.55f
// NOTE(zoubir): a prop's shadow falls down and right of its feet, as if lit
// from the top left, so it shows past a box-shaped prop (crate, log) that
// would otherwise cover it; a share of the shadow's width
#define PROP_SHADOW_SHIFT_X 0.14f
#define PROP_SHADOW_SHIFT_Y 0.06f

internal void
DrawSelfMarker(render_context *RenderContext, v2 Feet, float SortingValue,
               u32 Color)
{
    render_program Program = RenderContext->Programs[Shader_Ring];
    if (Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    BeginBatch(RenderContext, 0, SortingValue - 0.02f, Program);
    RenderQuadTexture(RenderContext,
                      Feet.X - 0.5f * SELF_MARKER_WIDTH,
                      Feet.Y - 0.5f * SELF_MARKER_HEIGHT,
                      SELF_MARKER_WIDTH, SELF_MARKER_HEIGHT,
                      V4(0.f, 1.f, 1.f, 0.f), Color, 0.f);
    EndBatch(RenderContext);
}

// NOTE(zoubir): the blob on the ground under an entity, just under its
// sprite. Players and shots have their own; monsters and props borrow the
// players', sized to their sprite, so nothing looks pasted on the ground.
// It spreads and narrows with the body's squash (ScaleX), lies on the
// ground under the entity, raised ground included, and fades and shrinks
// with the height above it
internal void
DrawEntityShadow(render_context *RenderContext, app_state *AppState,
                 render_program TextureProgram, assets *Assets, world_entity *Entity,
                 v2 Feet, float GroundZ, float DrawZ, float SortingValue, float ScaleX)
{
    asset_id ShadowAsset = Entity->ShadowTexture;
    float ShadowWidth = 28.f;
    float ShadowAlpha = 1.f;
    v2 ShadowShift = {};
    if (!ShadowAsset.Type && Entity->Texture.Type &&
        (Entity->Type == EntityType_Monster || Entity->Type == EntityType_StaticObject))
    {
        ShadowAsset = {AssetType_Shadow};
        ShadowWidth = Entity->Dimensions.X * MONSTER_SHADOW_WIDTH;
        ShadowAlpha = MONSTER_SHADOW_ALPHA;
        if (Entity->Type == EntityType_StaticObject)
        {
            ShadowWidth = Entity->Dimensions.X *
                (Entity->Texture.Type == AssetType_Tree ? TREE_SHADOW_WIDTH : PROP_SHADOW_WIDTH);
            ShadowAlpha = PROP_SHADOW_ALPHA;
            ShadowShift = V2(PROP_SHADOW_SHIFT_X, PROP_SHADOW_SHIFT_Y) * ShadowWidth;
        }
    }
    if (!ShadowAsset.Type)
    {
        return;
    }
    loaded_texture *ShadowTexture = GetTexture(Assets, AppState->OpenGL, AppState, ShadowAsset);
    zas_texture_info *ShadowTextureInfo = &GetAssetInfo(Assets, ShadowAsset)->Texture;
    float Height = DrawZ - GroundZ;
    if (!ShadowTexture || Height >= 200.f)
    {
        return;
    }
    v2 ShadowDims = {ShadowWidth * ScaleX, 0.5f * ShadowWidth};
    if (Height > 1.f)
    {
        ShadowDims *= (1.f - Height / 200.f);
    }
    ColorRGBA8 ShadowColor;
    ShadowColor.ColorU32 = RGBA8_WHITE;
    ShadowColor.A = (u8)((200.f - Height) * ShadowAlpha);
    v2 ShadowPosition = Feet - ShadowTextureInfo->Origin * ShadowDims + ShadowShift;
    ShadowPosition.Y -= GroundZ;
    BeginBatch(RenderContext, ShadowTexture->ID, SortingValue - 0.01f, TextureProgram);
    RenderQuadTexture(RenderContext, ShadowPosition.X, ShadowPosition.Y,
                      ShadowDims.X, ShadowDims.Y, {0.f, 0.f, 1.f, 1.f},
                      ShadowColor.ColorU32, 0.f);
    EndBatch(RenderContext);
}

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
