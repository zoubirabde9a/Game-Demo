/* Ground contact (draw_entities.cpp): what ties a body to the ground it
   stands on. The gold or red ring under each player, the blob shadow under
   players, monsters and props (borrowed and sized to the sprite for those
   without one), the test for a body standing in water, whose reflection
   DrawEntity draws, and the numbers that tune them and a tree's sway. */

// NOTE(zoubir): a gold ring on the ground under the local player, so it
// finds itself at a glance in a crowd of look-alike players, and a fainter
// red one under every other player: in the duel rules each is a foe. In a
// team duel the ring is the player's team colour (TeamMarkerColor). Drawn
// with the ring shader on a flat quad (an ellipse), sorted just under the
// shadow, so every sprite covers it
#define SELF_MARKER_WIDTH 44.f
#define SELF_MARKER_HEIGHT 22.f
#define SELF_MARKER_COLOR UI_RGBA(240, 200, 48, 190)
#define FOE_MARKER_COLOR UI_RGBA(235, 70, 60, 120)
// NOTE(zoubir): the outline around players and monsters (DrawEntity)
#define UNIT_OUTLINE_COLOR 0x9A000000
// NOTE(zoubir): the health bar over a unit (DrawEntity), in world units
#define HEALTH_BAR_HEIGHT 2.f
// NOTE(zoubir): radians a leafy tree leans each way at most in the wind
#define TREE_SWAY_ANGLE 0.018f
// NOTE(zoubir): a body's reflection in water or ice (DrawEntity): faint,
// tinted toward their blue; none past this height above them
#define WATER_REFLECTION_COLOR 0x50FFD8B0
#define ICE_REFLECTION_COLOR 0x38FFF0E0
#define WATER_REFLECTION_MAX_LIFT 48.f

// NOTE(zoubir): the colour a reflection is drawn in over flat tile X, Y:
// faint and blue over water, fainter and paler over ice, 0 elsewhere
internal u32
ReflectionColorAt(world *World, i32 TileX, i32 TileY)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    u32 Result = 0;
    if (ElevationAt(Map, TileX, TileY) == 0)
    {
        terrain_kind Kind = TerrainAt(Map, TileX, TileY);
        if (Kind == TerrainKind_ShallowWater || Kind == TerrainKind_DeepWater)
        {
            Result = WATER_REFLECTION_COLOR;
        }
        else if (Kind == TerrainKind_Ice)
        {
            Result = ICE_REFLECTION_COLOR;
        }
    }
    return Result;
}

// NOTE(zoubir): a body's reflection: the sprite upside down, mirrored
// about the ground line (a body Lift above it shows Lift below it), cut
// into one slice per tile row and drawn only where that row, under the
// body's middle, is water or ice, so it stops at the shore. Feet is the
// body's foot on screen, Quad the sprite's size, Origin its texture origin
internal void
DrawReflection(render_context *RenderContext, render_program TextureProgram,
               world *World, world_entity *Entity, u32 Texture, v2 Feet, float Lift,
               v2 Quad, v2 Origin, v3 CameraOffset)
{
    if (!World->TileWidth || World->TileMap.Texture.Type != AssetType_TerrainAtlas ||
        Lift > WATER_REFLECTION_MAX_LIFT)
    {
        return;
    }
    float Tile = (float)World->TileHeight;
    i32 TileX = FloorDiv((i32)floorf(Entity->Position.X), (i32)World->TileWidth);
    // NOTE(zoubir): only a body standing over water or ice is reflected;
    // one on the bank above a river is not, though its reflection would
    // fall on the river's rows
    i32 FootY = FloorDiv((i32)floorf(Entity->Position.Y), (i32)World->TileHeight);
    if (!ReflectionColorAt(World, TileX, FootY))
    {
        return;
    }
    float Top = Feet.Y + Lift - (1.f - Origin.Y) * Quad.Y;
    float Left = Feet.X - Origin.X * Quad.X;
    v4 Uvs = Entity->Uvs;
    i32 FirstRow = FloorDiv((i32)floorf(Top + CameraOffset.Y), (i32)Tile);
    i32 LastRow = FloorDiv((i32)floorf(Top + Quad.Y + CameraOffset.Y), (i32)Tile);
    for(i32 Row = FirstRow; Row <= LastRow; Row++)
    {
        u32 Color = ReflectionColorAt(World, TileX, Row);
        if (!Color)
        {
            continue;
        }
        float SliceTop = Maximum(Top, (float)Row * Tile - CameraOffset.Y);
        float SliceBottom = Minimum(Top + Quad.Y, (float)(Row + 1) * Tile - CameraOffset.Y);
        if (SliceBottom <= SliceTop)
        {
            continue;
        }
        // NOTE(zoubir): flipped: the top of the slice shows the frame's
        // bottom rows
        float T0 = (SliceTop - Top) / Quad.Y;
        float T1 = (SliceBottom - Top) / Quad.Y;
        v4 Slice = V4(Uvs.X, Uvs.Y + (Uvs.W - Uvs.Y) * T1, Uvs.Z, Uvs.Y + (Uvs.W - Uvs.Y) * T0);
        BeginBatch(RenderContext, Texture, FLAT_GROUND_SORT_KEY + 2.f, TextureProgram);
        RenderQuadTexture(RenderContext, Left, SliceTop, Quad.X, SliceBottom - SliceTop,
                          Slice, Color, 0.f);
        EndBatch(RenderContext);
    }
}
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
    // NOTE(zoubir): a low sun at dusk and dawn draws shadows out long to
    // the east; at night only the moon throws them, faintly
    float Low = AppState->SunShadow.X;
    float Dark = AppState->SunShadow.Y;
    v2 ShadowDims = {ShadowWidth * ScaleX * (1.f + 0.9f * Low), 0.5f * ShadowWidth};
    ShadowShift.X += 0.35f * Low * ShadowWidth;
    ShadowAlpha *= 1.f - 0.45f * Dark;
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
