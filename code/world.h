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

struct world_chunk
{
    tile Tiles;
    world_entity_chunk FirstEntityChunk;
};

#define CHUNK_MAX_X 12
#define CHUNK_MAX_Y 12
#define CHUNK_MAX_Z 4

struct world
{
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
    world_chunk Chunks[CHUNK_MAX_X][CHUNK_MAX_Y][CHUNK_MAX_Z];
    world_entity Entities[4096];
    u32 EntityCount;
    world_entity_chunk *FirstFreeChunk;

    v3 MaxEntityVelocity;
};

#endif
