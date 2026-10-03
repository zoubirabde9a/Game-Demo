#if !defined(WORLD_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define WORLD_H
#include "entity.h"

struct tile
{
    u32 Index;
};

struct tile_map
{
    asset_id Texture;
    tile *Tiles;
};

struct world_entity_chunk
{
    u32 EntityCount;
    world_entity *Entities[16];
    world_entity_chunk *Next;
};

// NOTE(zoubir): one cell of the spatial index: the entities whose
// collision boxes touch this block of tiles. Chunks are made the first time
// something enters them and live in a hash table keyed by their signed
// coordinates, so the world can grow in every direction
struct world_chunk
{
    i32 ChunkX;
    i32 ChunkY;
    i32 ChunkZ;
    tile Tiles;
    world_entity_chunk FirstEntityChunk;
    world_chunk *NextInHash;
    world_chunk *NextInWorld;
};

// NOTE(zoubir): the chunks a box covers, inclusive
struct chunk_range
{
    i32 MinX;
    i32 MinY;
    i32 MinZ;
    i32 MaxX;
    i32 MaxY;
    i32 MaxZ;
};

#define WORLD_CHUNK_HASH_SIZE 4096

// NOTE(zoubir): size of the arena, in tiles of ARENA_TILE_SIZE
#define ARENA_TILE_SIZE 32
#define ARENA_TILES_X 80
#define ARENA_TILES_Y 40
#define ARENA_TILES_Z 12


struct world
{
    // NOTE(zoubir): the map_id this world was built from (sim/maps/);
    // set before InitSimulation, 0 is the Old Arena
    u32 MapId;
    struct entity_collision_volume_group *BoulderCollision;
    struct entity_collision_volume_group *DeadTreeCollision;
    tile_map TileMap;
    u32 NumTilesX;
    u32 NumTilesY;
    u32 NumTilesZ;
    
    u32 TileWidth;
    u32 TileHeight;
    u32 TileDepth;

    bool32 *CollisionMap;
    u32 NumCollisionX;
    u32 NumCollisionY;
    u32 CollisionWidth;
    u32 CollisionHeight;
    u32 CollisionDepth;

    u32 TilesPerChunkX;
    u32 TilesPerChunkY;
    u32 TilesPerChunkZ;
    
    float TileDepth_;
    // NOTE(zoubir): bounded maps clamp chunk lookups to the map like
    // before; unbounded ones take any X and Y, negative included
    bool32 Unbounded;
    world_chunk *ChunkHash[WORLD_CHUNK_HASH_SIZE];
    world_chunk *FirstChunk;
    u32 ChunkCount;
    world_entity Entities[4096];
    u32 EntityCount;
    // NOTE(zoubir): IDs of removed entities, reused before growing EntityCount
    u32 FreeEntityIDs[4096];
    u32 FreeEntityCount;
    world_entity_chunk *FirstFreeChunk;

    v3 MaxEntityVelocity;
};

#endif
