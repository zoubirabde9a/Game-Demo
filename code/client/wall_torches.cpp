/* Wall torches: on maps whose mood asks for them (map_moods.cpp), a torch
   burns on the face of a stone wall every few tiles where the wall looks
   out onto open ground. Placed by a hash of the tile, so every player sees
   the same torches, read from the terrain cache. Each one is a flickering
   flame drawn on the wall face (DrawWallTorches, from DrawTileMap) and a
   light in the frame's lights (world_lights.cpp), so it lights the floor
   in front of it. */

// NOTE(zoubir): about one wall tile in WALL_TORCH_ODDS that faces open
// ground carries a torch, never two side by side
#define WALL_TORCH_ODDS 5
// NOTE(zoubir): how far up the face the flame burns, as a share of the
// wall's height (or this many units on a wall drawn flat)
#define WALL_TORCH_HEIGHT 0.55f
#define WALL_TORCH_FLAT_HEIGHT 22.f

// NOTE(zoubir): whether tile X, Y carries a torch, and if so where its
// flame is: the middle of the tile's south edge, Lift units up
internal bool32
WallTorchAt(app_state *AppState, map_def *Map, i32 X, i32 Y, float *Lift)
{
    terrain_cache_tile *Tile = CachedTile(AppState, Map, X, Y);
    if (Tile->Kind != TerrainKind_StoneWall)
    {
        return false;
    }
    terrain_cache_tile *Front = CachedTile(AppState, Map, X, Y + 1);
    if (Front->Kind == TerrainKind_StoneWall || Front->Steps > Tile->Steps)
    {
        return false;
    }
    u32 Roll = HashLattice(0x70C4u, X, Y) % WALL_TORCH_ODDS;
    u32 Left = HashLattice(0x70C4u, X - 1, Y) % WALL_TORCH_ODDS;
    if (Roll != 0 || Left == 0)
    {
        return false;
    }
    float Height = (float)(Tile->Steps - Front->Steps) * ELEVATION_STEP_HEIGHT;
    *Lift = (float)Front->Steps * ELEVATION_STEP_HEIGHT +
        (Height > 0.f ? WALL_TORCH_HEIGHT * Height : WALL_TORCH_FLAT_HEIGHT);
    return true;
}

// NOTE(zoubir): every torch on screen, a small flame of three glows
// wavering on its own beat
internal void
DrawWallTorches(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                i32 MinX, i32 MinY, i32 MaxX, i32 MaxY)
{
    world *World = &AppState->World;
    render_program Program = RenderContext->Programs[Shader_Glow];
    if (MoodFor(World->MapId)->Torches <= 0.f || Program.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    float Tile = (float)World->TileWidth;
    float Seconds = (float)(AppState->UpdateID % 36000) / 60.f;
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            float Lift;
            if (!WallTorchAt(AppState, Map, X, Y, &Lift))
            {
                continue;
            }
            float Foot = (float)(Y + 1) * Tile;
            v2 Flame = V2(((float)X + 0.5f) * Tile - CameraOffset.X,
                          Foot - Lift - CameraOffset.Y);
            float Beat = (float)(HashLattice(0x70C5u, X, Y) & 0xFF) * 0.05f;
            float Waver = 0.85f + 0.15f * Sin(11.f * Seconds + Beat) * Sin(7.f * Seconds + 2.f * Beat);
            // NOTE(zoubir): over the wall's face and under anyone standing
            // in front of it
            BeginBatch(RenderContext, 0, StandingSortKey(Foot, 0.f) - 0.5f, Program);
            RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
            float Sizes[3] = {40.f, 20.f, 9.f};
            u32 Colors[3] = {0x60206AFF, 0xC040B0FF, 0xFFB0F0FF};
            for(u32 Layer = 0; Layer < 3; Layer++)
            {
                float Size = Sizes[Layer] * Waver;
                RenderQuadTexture(RenderContext, Flame.X - 0.5f * Size,
                                  Flame.Y - 0.6f * Size, Size, Size,
                                  V4(0.f, 1.f, 1.f, 0.f), Colors[Layer], 0.f);
            }
            EndBatch(RenderContext);
        }
    }
}
