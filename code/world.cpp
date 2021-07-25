/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
#include "world.h"

inline cannonical_position
CannonicalizePosition(v3 AbsolutePosition, u32 TileWidth, u32 TileHeight,
                      u32 TileDepth)
{
    cannonical_position Result;

    Result.TileX = (u32)(AbsolutePosition.X / TileWidth);
    Result.TileRel.X = AbsolutePosition.X -
        (Result.TileX * TileWidth);
    
    Result.TileY = (u32)(AbsolutePosition.Y / TileHeight);
    Result.TileRel.Y = AbsolutePosition.Y -
        (Result.TileY * TileHeight);    
    
    Result.TileZ = (u32)(AbsolutePosition.Z / TileDepth);
    Result.TileRel.Z = AbsolutePosition.Z -
        (Result.TileZ * TileDepth);
    
    return Result;
}

inline world_chunk *
GetChunk(world *World, u32 ChunkX, u32 ChunkY, u32 ChunkZ)
{
    Assert(ChunkX < CHUNK_MAX_X);
    Assert(ChunkY < CHUNK_MAX_Y);
    Assert(ChunkZ < CHUNK_MAX_Z);
    world_chunk *Chunk = &World->Chunks[ChunkX][ChunkY][ChunkZ];
    return Chunk;
}

inline world_chunk *
GetChunk(world *World, v3 Position)
{    
    u32 ChunkX = (u32)(Position.X / (World->TilesPerChunkX * World->TileWidth));
    u32 ChunkY = (u32)(Position.Y / (World->TilesPerChunkY * World->TileHeight));
    u32 ChunkZ = (u32)(Position.Z / (World->TilesPerChunkZ * World->TileDepth));
    world_chunk *Chunk = GetChunk(World, ChunkX, ChunkY, ChunkZ);
    return Chunk;
}

internal void
CheckEntityOverlapInChunk(app_state *AppState,
                          world *World, memory_arena *Arena,
                          world_entity *Entity,
                          world_entity_chunk *FirstEntityChunk)
{
    
    for(world_entity_chunk *EntityChunk = FirstEntityChunk;
        EntityChunk;
        EntityChunk = EntityChunk->Next)
    {                    
        for(u32 EntityIndex = 0;
            EntityIndex < EntityChunk->EntityCount;
            EntityIndex++)
        {
            world_entity *TestEntity = EntityChunk->Entities[EntityIndex];
        
            if (Entity != TestEntity &&
                CanOverlap(Entity, TestEntity) &&
                EntityOverlap(Entity, TestEntity))
            {
                HandleOverlap(AppState, World, Arena,
                              Entity, TestEntity);
            }                    
        }
    }    
}

internal void
InsertEntity(app_state *AppState,
             world *World,
             memory_arena *Arena,
             world_entity_chunk *FirstEntityChunk,
             world_entity *NewEntity)
{
    FirstEntityChunk->Entities[FirstEntityChunk->EntityCount++] =
        NewEntity;
    // If the FirstChunk is Full copy it to the next
    if (FirstEntityChunk->EntityCount >= ArrayCount(FirstEntityChunk->Entities))
    {
        world_entity_chunk *OldEntityChunk;

        if (World->FirstFreeChunk)
        {
            // grap a free chunk
            OldEntityChunk = World->FirstFreeChunk;
            World->FirstFreeChunk = OldEntityChunk->Next;
        }
        else
        {
            // No free chunk was found allocate a new one            
            OldEntityChunk =
                AllocateStruct(Arena, world_entity_chunk);
        }
        *OldEntityChunk = *FirstEntityChunk;
        FirstEntityChunk->EntityCount = 0;
        FirstEntityChunk->Next = OldEntityChunk;
    }

    CheckEntityOverlapInChunk(AppState, World, Arena,
                              NewEntity, FirstEntityChunk);
}
internal bool32
RemoveEntity(world *World,
             world_chunk *Chunk,
             world_entity *Entity)
{
    bool32 Result = false;
    world_entity_chunk *FirstEntityChunk = &Chunk->FirstEntityChunk;
    world_entity_chunk *EntityChunk = FirstEntityChunk;
    do {
        for(u32 EntityIndex = 0;
            EntityIndex < EntityChunk->EntityCount;
            EntityIndex++)
        {
            world_entity *ThisEntity = EntityChunk->Entities[EntityIndex];
            if (ThisEntity == Entity)
            {
                Result = true;
                // swap with last element
                if (FirstEntityChunk->EntityCount > 0)
                {
                    u32 LastIndex = FirstEntityChunk->EntityCount - 1;
                    EntityChunk->Entities[EntityIndex] =
                        FirstEntityChunk->Entities[LastIndex];
                    FirstEntityChunk->EntityCount--;
                }
                else
                {
                    // remove first chunk
                    u32 LastIndex = EntityChunk->EntityCount - 1;
                    EntityChunk->Entities[EntityIndex] =
                        EntityChunk->Entities[LastIndex];
                    EntityChunk->EntityCount--;
                    *FirstEntityChunk = *EntityChunk;
                    // move it to the free list
                    EntityChunk->EntityCount = 0;
                    EntityChunk->Next = World->FirstFreeChunk;
                    World->FirstFreeChunk = EntityChunk;
                }
                break;
            }
        }
        EntityChunk = EntityChunk->Next;
    } while(EntityChunk);
    
    return Result;
}

internal void
GetChunksFromRectangle(world *World,
                       v2 MinPos, v2 MaxPos, v2 HalfDims,
                       u32 *MinChunkX, u32 *MinChunkY,
                       u32 *MaxChunkX, u32 *MaxChunkY)
{
    float TileMapWidth = (float)World->TileWidth * World->NumTilesX;
    float TileMapHeight = (float)World->TileHeight * World->NumTilesY;
    
    u32 ChunkWidth = World->TilesPerChunkX * World->TileWidth;
    u32 ChunkHeight = World->TilesPerChunkY * World->TileHeight;
    
    v2 Min = MinPos - HalfDims;
    Min.X = Maximum(0.f, Min.X);
    Min.Y = Maximum(0.f, Min.Y);
    v2 Max = MaxPos + HalfDims;
    Max.X = Minimum(TileMapWidth, Max.X);
    Max.Y = Minimum(TileMapHeight, Max.Y);

    *MinChunkX = (u32)(Min.X / ChunkWidth);
    *MinChunkY = (u32)(Min.Y / ChunkHeight);
    
    *MaxChunkX = (u32)(Max.X / ChunkWidth);
    *MaxChunkY = (u32)(Max.Y / ChunkHeight);
}

internal void
GetChunksFromBox(world *World,
                       rectangle3 Rect,
                       u32 *MinChunkX, u32 *MinChunkY, u32 *MinChunkZ,
                       u32 *MaxChunkX, u32 *MaxChunkY, u32 *MaxChunkZ)
{
    float TileMapWidth = (float)World->TileWidth * World->NumTilesX;
    float TileMapHeight = (float)World->TileHeight * World->NumTilesY;
    float TileMapDepth = (float)World->TileDepth * World->NumTilesZ;
    
    u32 ChunkWidth = World->TilesPerChunkX * World->TileWidth;
    u32 ChunkHeight = World->TilesPerChunkY * World->TileHeight;
    u32 ChunkDepth = World->TilesPerChunkZ * World->TileDepth;
    
    Rect.Min.X = Maximum(0.f, Rect.Min.X);
    Rect.Min.Y = Maximum(0.f, Rect.Min.Y);
    Rect.Min.Z = Maximum(0.f, Rect.Min.Z);
    
    Rect.Max.X = Minimum(TileMapWidth, Rect.Max.X);
    Rect.Max.Y = Minimum(TileMapHeight, Rect.Max.Y);
    Rect.Max.Z = Minimum(TileMapDepth, Rect.Max.Z);

    *MinChunkX = (u32)(Rect.Min.X / ChunkWidth);
    *MinChunkY = (u32)(Rect.Min.Y / ChunkHeight);
    *MinChunkZ = (u32)(Rect.Min.Z / ChunkDepth);
    
    *MaxChunkX = (u32)(Rect.Max.X / ChunkWidth);
    *MaxChunkY = (u32)(Rect.Max.Y / ChunkHeight);
    *MaxChunkZ = (u32)(Rect.Max.Z / ChunkDepth);
}


inline void
GetChunksFromEntity(world *World, world_entity *Entity,
                    u32 *MinChunkX, u32 *MinChunkY, u32 *MinChunkZ,
                    u32 *MaxChunkX, u32 *MaxChunkY, u32 *MaxChunkZ)
{
    //TODO(zoubir): changing dims may break something
    // handle a change in dims ?
    entity_collision_volume *EntityCollisionTotal =
        &Entity->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Entity->Position + EntityCollisionTotal->Offset,
                                           EntityCollisionTotal->HalfDims);
    GetChunksFromBox(World, Box,
                     MinChunkX, MinChunkY, MinChunkZ,
                     MaxChunkX, MaxChunkY, MaxChunkZ);
    
}

internal world_entity *
AddEntity(app_state *AppState,
          world *World, memory_arena *Arena,
          entity_type Type,
          v3 Position,
          entity_collision_volume_group* Collision)
{
    Assert(ArrayCount(World->Entities) > World->EntityCount);
    world_entity *NewEntity = 
        World->Entities + World->EntityCount++;
    NewEntity->IsPresent = true;
    
    NewEntity->ID = World->EntityCount - 1;
    NewEntity->Type = Type;
    NewEntity->Position = Position;
    NewEntity->Collision = Collision;

    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;
    u32 MaxChunkX;
    u32 MaxChunkY;
    u32 MaxChunkZ;

    #if 0
    entity_collision_volume *EntityCollisionTotal =
        &NewEntity->Collision->TotalVolume;
    rectangle3 Box =
        RectCenterHalfDims(NewEntity->Position + EntityCollisionTotal->Offset,
                           EntityCollisionTotal->HalfDims);

    Assert(Box.Max.X - Box.Min.X > World->MaxEntityVelocity.X);
    Assert(Box.Max.Y - Box.Min.Y > World->MaxEntityVelocity.Y);
//    Assert(Box.Max.Z - Box.Min.Z > World->MaxEntityVelocity.Z);
    
    GetChunksFromBox(World, Box,
                     &MinChunkX, &MinChunkY, &MinChunkZ,
                     &MaxChunkX, &MaxChunkY, &MaxChunkZ);
    #endif
    GetChunksFromEntity(World, NewEntity,
                        &MinChunkX, &MinChunkY, &MinChunkZ,
                        &MaxChunkX, &MaxChunkY, &MaxChunkZ);
        
    for(u32 ChunkZ = MinChunkZ;
        ChunkZ <= MaxChunkZ;
        ChunkZ++)
    {
        for(u32 ChunkY = MinChunkY;
            ChunkY <= MaxChunkY;
            ChunkY++)
        {
            for(u32 ChunkX = MinChunkX;
                ChunkX <= MaxChunkX;
                ChunkX++)
            {
                world_chunk *ThisChunk =
                    GetChunk(World, ChunkX, ChunkY, ChunkZ);
                // Add Entity Pointer Into The Chunk
                world_entity_chunk *FirstEntityChunk =
                    &ThisChunk->FirstEntityChunk;    
                InsertEntity(AppState,
                             World, Arena,
                             FirstEntityChunk, NewEntity);                
            }
        }
    }
#if 0                     
    world_chunk *Chunk = GetChunk(World, Position);

    // Add Entity Pointer Into The Chunk
    world_entity_chunk *FirstEntityChunk = &Chunk->FirstEntityChunk;
    
    InsertEntity(World, Arena, FirstEntityChunk, NewEntity);
#endif    

    return NewEntity;
}

//TODO(zoubir): Decrement Entity Count 
inline bool32
RemoveEntity(world *World, world_entity *Entity)
{
    bool32 Result = 0;
    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;    
    u32 MaxChunkX;    
    u32 MaxChunkY;    
    u32 MaxChunkZ;

    entity_collision_volume *EntityCollisionTotal =
        &Entity->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Entity->Position + EntityCollisionTotal->Offset,
                       EntityCollisionTotal->HalfDims);
    GetChunksFromBox(World, Box,
                     &MinChunkX, &MinChunkY, &MinChunkZ,
                     &MaxChunkX, &MaxChunkY, &MaxChunkZ);    
    

    for(u32 ChunkZ = MinChunkZ;
        ChunkZ <= MaxChunkZ;
        ChunkZ++)
    {
        for(u32 ChunkY = MinChunkY;
            ChunkY <= MaxChunkY;
            ChunkY++)
        {
            for(u32 ChunkX = MinChunkX;
                ChunkX <= MaxChunkX;
                ChunkX++)
            {
                world_chunk *Chunk =
                    GetChunk(World, ChunkX, ChunkY, ChunkZ);
                Result = RemoveEntity(World, Chunk, Entity);
                Assert(Result);
                Entity->IsPresent = false;        
            }
        }
    }
#if 0    
    world_chunk *Chunk =
        GetChunk(World, Entity->Position);
    bool32 Result = RemoveEntity(World, Chunk, Entity);
    Assert(Result);
    Entity->IsPresent = false;
    #endif
    return Result;
}
       
internal void
CheckAndChangeEntityChunk(app_state *AppState,
                          world *World, memory_arena *Arena,
                          v3 OldPosition,
                          world_entity *Entity)
{
    u32 OldMinChunkX;
    u32 OldMinChunkY;
    u32 OldMinChunkZ;    
    u32 OldMaxChunkX;    
    u32 OldMaxChunkY;    
    u32 OldMaxChunkZ;

    //TODO(zoubir): changing dims may break something
    // handle a change in dims ?
    entity_collision_volume *EntityCollisionTotal =
        &Entity->Collision->TotalVolume;
    rectangle3 OldBox = RectCenterHalfDims(OldPosition + EntityCollisionTotal->Offset,
                       EntityCollisionTotal->HalfDims);
    GetChunksFromBox(World, OldBox,
                     &OldMinChunkX, &OldMinChunkY, &OldMinChunkZ,
                     &OldMaxChunkX, &OldMaxChunkY, &OldMaxChunkZ);
    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;    
    u32 MaxChunkX;    
    u32 MaxChunkY;    
    u32 MaxChunkZ;

    rectangle3 NewBox = RectCenterHalfDims(Entity->Position + EntityCollisionTotal->Offset,
                       EntityCollisionTotal->HalfDims);
    GetChunksFromBox(World, NewBox,
                     &MinChunkX, &MinChunkY, &MinChunkZ,
                     &MaxChunkX, &MaxChunkY, &MaxChunkZ);    
    
    for(u32 ChunkZ = OldMinChunkZ;
        ChunkZ <= OldMaxChunkZ;
        ChunkZ++)
    {
        for(u32 ChunkY = OldMinChunkY;
            ChunkY <= OldMaxChunkY;
            ChunkY++)
        {
            for(u32 ChunkX = OldMinChunkX;
                ChunkX <= OldMaxChunkX;
                ChunkX++)
            {
                world_chunk *ThisChunk =
                    GetChunk(World, ChunkX, ChunkY, ChunkZ);
                bool32 Removed = RemoveEntity(World, ThisChunk, Entity);
                Assert(Removed);                
            }
        }
    }
    
    for(u32 ChunkZ = MinChunkZ;
        ChunkZ <= MaxChunkZ;
        ChunkZ++)
    {
        for(u32 ChunkY = MinChunkY;
            ChunkY <= MaxChunkY;
            ChunkY++)
        {
            for(u32 ChunkX = MinChunkX;
                ChunkX <= MaxChunkX;
                ChunkX++)
            {
                world_chunk *ThisChunk =
                    GetChunk(World, ChunkX, ChunkY, ChunkZ);
                InsertEntity(AppState,
                             World, Arena,
                             &ThisChunk->FirstEntityChunk, Entity);
            }
        }
    }
#if 0    
    world_chunk *TargetChunk =
        GetChunk(World, Entity->Position);
    world_chunk *OldChunk =
        GetChunk(World, OldPosition);
    bool32 Removed = 0;
    
    if (TargetChunk != OldChunk)
    {
        Removed = RemoveEntity(World, OldChunk, Entity);
        Assert(Removed);
        // add the entity into the target chunk
        InsertEntity(World, Arena,
                     &TargetChunk->FirstEntityChunk, Entity);
    }
    #endif
}
