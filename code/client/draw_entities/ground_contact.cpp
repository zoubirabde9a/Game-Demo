/* Ground contact (draw_entities.cpp): what ties a body to the ground it
   stands on. The gold or red ring under each player, the blob shadow under
   players, monsters and props (borrowed and sized to the sprite for those
   without one), the test for a body standing in water, whose reflection
   DrawEntity draws, and the numbers that tune them and a tree's sway. */

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
// NOTE(zoubir): radians a leafy tree leans each way at most in the wind
#define TREE_SWAY_ANGLE 0.018f
// NOTE(zoubir): a body's reflection in water (DrawEntity): faint, tinted
// toward the water's blue; none past this height above it
#define WATER_REFLECTION_COLOR 0x50FFD8B0
#define WATER_REFLECTION_MAX_LIFT 48.f

// NOTE(zoubir): over flat water, low enough for its reflection to show
internal bool32
IsStandingInWater(world *World, world_entity *Entity, float GroundZ, float DrawZ)
{
    if (!World->TileWidth || World->TileMap.Texture.Type != AssetType_TerrainAtlas ||
        GroundZ > 0.f || DrawZ - GroundZ > WATER_REFLECTION_MAX_LIFT)
    {
        return false;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    i32 TileX = FloorDiv((i32)floorf(Entity->Position.X), (i32)World->TileWidth);
    i32 TileY = FloorDiv((i32)floorf(Entity->Position.Y), (i32)World->TileHeight);
    terrain_kind Kind = TerrainAt(Map, TileX, TileY);
    bool32 Result = Kind == TerrainKind_ShallowWater || Kind == TerrainKind_DeepWater;
    return Result;
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
