/* Terrain cache: what the client draws needs to know about each tile on
   screen (its kind, prop and height, and the two tint fields at its
   top-left corner), kept from frame to frame. On an endless map every one
   of those is the map's noise run again (TerrainAt, PropAt, ElevationAt),
   and the ground pass, the props and the lava lights asked for each tile
   several times a frame: on the Ashen Wastes that was most of the game's
   frame time. Now a tile's noise runs once, when it comes into view.

   A fixed table of TERRAIN_CACHE_SIDE x TERRAIN_CACHE_SIDE slots; tile X, Y
   lives in slot (X, Y) modulo the side, which keeps its own X and Y to
   know whether it is still that tile. The screen is narrower than the
   table, so the tiles in view never push each other out. The table empties
   when the map changes. Client only: the simulation keeps asking the map. */

#define TERRAIN_CACHE_SIDE 128

struct terrain_cache_tile
{
    i32 X;
    i32 Y;
    u8 Kind;
    u8 Prop;
    u8 Steps;
    u8 Filled;
    // NOTE(zoubir): the world tint fields at the tile's top-left corner
    // (ground_cells.cpp), 0..255
    u8 Shade;
    u8 Warmth;
};

struct terrain_cache
{
    u32 MapId;
    terrain_cache_tile Tiles[TERRAIN_CACHE_SIDE * TERRAIN_CACHE_SIDE];
};

internal terrain_cache_tile *
CachedTile(app_state *AppState, map_def *Map, i32 X, i32 Y)
{
    terrain_cache *Cache = AppState->TerrainCache;
    if (!Cache)
    {
        Cache = AppState->TerrainCache = AllocateStruct(&AppState->MemoryArena, terrain_cache);
        ZeroSize(Cache, sizeof(*Cache));
        Cache->MapId = AppState->World.MapId;
    }
    if (Cache->MapId != AppState->World.MapId)
    {
        ZeroSize(Cache, sizeof(*Cache));
        Cache->MapId = AppState->World.MapId;
    }
    u32 Slot = ((u32)Y & (TERRAIN_CACHE_SIDE - 1)) * TERRAIN_CACHE_SIDE +
        ((u32)X & (TERRAIN_CACHE_SIDE - 1));
    terrain_cache_tile *Tile = &Cache->Tiles[Slot];
    if (!Tile->Filled || Tile->X != X || Tile->Y != Y)
    {
        Tile->X = X;
        Tile->Y = Y;
        Tile->Kind = (u8)TerrainAt(Map, X, Y);
        Tile->Prop = (u8)PropAt(Map, X, Y);
        Tile->Steps = (u8)ElevationAt(Map, X, Y);
        Tile->Shade = (u8)(FractalNoise(0x5ADEu, X, Y, 6, 2) >> (NOISE_SHIFT - 8));
        Tile->Warmth = (u8)(FractalNoise(0xFA11u, X, Y, 11, 2) >> (NOISE_SHIFT - 8));
        Tile->Filled = 1;
    }
    return Tile;
}
