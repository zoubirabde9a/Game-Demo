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

inline u32
ChunkHashSlot(i32 ChunkX, i32 ChunkY, i32 ChunkZ)
{
    u32 Hash = (u32)ChunkX * 73856093u ^ (u32)ChunkY * 19349663u ^
        (u32)ChunkZ * 83492791u;
    u32 Result = Hash & (WORLD_CHUNK_HASH_SIZE - 1);
    return Result;
}

// NOTE(zoubir): the chunk at these coordinates, or 0 if nothing has ever
// entered it
inline world_chunk *
FindChunk(world *World, i32 ChunkX, i32 ChunkY, i32 ChunkZ)
{
    world_chunk *Result = World->ChunkHash[ChunkHashSlot(ChunkX, ChunkY, ChunkZ)];
    while (Result &&
           !(Result->ChunkX == ChunkX && Result->ChunkY == ChunkY &&
             Result->ChunkZ == ChunkZ))
    {
        Result = Result->NextInHash;
    }
    return Result;
}

internal world_chunk *
GetOrCreateChunk(world *World, memory_arena *Arena,
                 i32 ChunkX, i32 ChunkY, i32 ChunkZ)
{
    world_chunk *Result = FindChunk(World, ChunkX, ChunkY, ChunkZ);
    if (!Result)
    {
        Result = AllocateStruct(Arena, world_chunk);
        ZeroSize(Result, sizeof(*Result));
        Result->ChunkX = ChunkX;
        Result->ChunkY = ChunkY;
        Result->ChunkZ = ChunkZ;
        u32 Slot = ChunkHashSlot(ChunkX, ChunkY, ChunkZ);
        Result->NextInHash = World->ChunkHash[Slot];
        World->ChunkHash[Slot] = Result;
        Result->NextInWorld = World->FirstChunk;
        World->FirstChunk = Result;
        World->ChunkCount++;
    }
    return Result;
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
    // NOTE(zoubir): inserting only records where an entity is. Overlaps
    // are checked after moves and by UpdateSword, once the spawner has
    // set the owner; checking here let a new sword hit its own caster.
}
// NOTE(zoubir): a chunk keeps its entities in FirstEntityChunk plus a
// chain of blocks that are always full; only the first block is partial.
// Removing swaps in the last entity of the first block. When the first
// block is empty it is refilled from the second block (never a later
// one: copying a later block over it used to drop the second block and
// every entity in it, so they could never be found or moved again).
internal bool32
RemoveEntity(world *World,
             world_chunk *Chunk,
             world_entity *Entity)
{
    world_entity_chunk *First = &Chunk->FirstEntityChunk;
    for(world_entity_chunk *EntityChunk = First;
        EntityChunk;
        EntityChunk = EntityChunk->Next)
    {
        for(u32 EntityIndex = 0;
            EntityIndex < EntityChunk->EntityCount;
            EntityIndex++)
        {
            if (EntityChunk->Entities[EntityIndex] == Entity)
            {
                if (First->EntityCount == 0 && First->Next)
                {
                    world_entity_chunk *Second = First->Next;
                    bool32 FoundInSecond = (EntityChunk == Second);
                    *First = *Second;
                    Second->EntityCount = 0;
                    Second->Next = World->FirstFreeChunk;
                    World->FirstFreeChunk = Second;
                    if (FoundInSecond)
                    {
                        EntityChunk = First;
                    }
                }
                Assert(First->EntityCount > 0);
                u32 LastIndex = --First->EntityCount;
                EntityChunk->Entities[EntityIndex] = First->Entities[LastIndex];
                return true;
            }
        }
    }
    return false;
}

inline i32
ChunkCoordinate(float Value, float ChunkSize)
{
    i32 Result = (i32)floorf(Value / ChunkSize);
    return Result;
}

// NOTE(zoubir): the chunks a box touches. Bounded maps clamp the box to the
// map first, as the fixed chunk grid used to; height always clamps to the
// world's depth
internal chunk_range
GetChunkRange(world *World, rectangle3 Rect)
{
    float ChunkWidth = (float)(World->TilesPerChunkX * World->TileWidth);
    float ChunkHeight = (float)(World->TilesPerChunkY * World->TileHeight);
    float ChunkDepth = (float)(World->TilesPerChunkZ * World->TileDepth);

    if (!World->Unbounded)
    {
        float TileMapWidth = (float)World->TileWidth * World->NumTilesX;
        float TileMapHeight = (float)World->TileHeight * World->NumTilesY;
        Rect.Min.X = Maximum(0.f, Rect.Min.X);
        Rect.Min.Y = Maximum(0.f, Rect.Min.Y);
        Rect.Max.X = Minimum(TileMapWidth, Rect.Max.X);
        Rect.Max.Y = Minimum(TileMapHeight, Rect.Max.Y);
    }
    float TileMapDepth = (float)World->TileDepth * World->NumTilesZ;
    Rect.Min.Z = Maximum(0.f, Rect.Min.Z);
    Rect.Max.Z = Minimum(TileMapDepth, Rect.Max.Z);

    chunk_range Result;
    Result.MinX = ChunkCoordinate(Rect.Min.X, ChunkWidth);
    Result.MinY = ChunkCoordinate(Rect.Min.Y, ChunkHeight);
    Result.MinZ = ChunkCoordinate(Rect.Min.Z, ChunkDepth);
    Result.MaxX = ChunkCoordinate(Rect.Max.X, ChunkWidth);
    Result.MaxY = ChunkCoordinate(Rect.Max.Y, ChunkHeight);
    Result.MaxZ = ChunkCoordinate(Rect.Max.Z, ChunkDepth);
    return Result;
}

inline chunk_range
GetEntityChunkRange(world *World, world_entity *Entity, v3 Position)
{
    entity_collision_volume *Total = &Entity->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Position + Total->Offset, Total->HalfDims);
    chunk_range Result = GetChunkRange(World, Box);
    return Result;
}

// NOTE(zoubir): every entity listed in the chunks Box touches, in chunk
// order (Y, then X, then Z), once per chunk it is listed in. This is the
// one place outside world bookkeeping that walks chunk storage; movement,
// collision and overlap checks work on the list it returns. Stops at
// MaxCount (asserts in debug builds).
internal u32
GatherEntitiesInBox(world *World, rectangle3 Box, world_entity **Out,
                    u32 MaxCount)
{
    u32 Count = 0;
    chunk_range Range = GetChunkRange(World, Box);
    for(i32 ChunkY = Range.MinY; ChunkY <= Range.MaxY; ChunkY++)
    {
        for(i32 ChunkX = Range.MinX; ChunkX <= Range.MaxX; ChunkX++)
        {
            for(i32 ChunkZ = Range.MinZ; ChunkZ <= Range.MaxZ; ChunkZ++)
            {
                world_chunk *Chunk = FindChunk(World, ChunkX, ChunkY, ChunkZ);
                if (!Chunk)
                {
                    continue;
                }
                for(world_entity_chunk *Block = &Chunk->FirstEntityChunk;
                    Block;
                    Block = Block->Next)
                {
                    for(u32 Index = 0; Index < Block->EntityCount; Index++)
                    {
                        Assert(Count < MaxCount);
                        if (Count < MaxCount)
                        {
                            Out[Count++] = Block->Entities[Index];
                        }
                    }
                }
            }
        }
    }
    return Count;
}


internal world_entity *
AddEntity(app_state *AppState,
          world *World, memory_arena *Arena,
          entity_type Type,
          v3 Position,
          entity_collision_volume_group* Collision)
{
    u32 ID;
    if (World->FreeEntityCount)
    {
        ID = World->FreeEntityIDs[--World->FreeEntityCount];
        // NOTE(zoubir): rules were set up for the slot's previous owner
        ClearCollisionRulesFor(AppState, ID);
    }
    else
    {
        Assert(ArrayCount(World->Entities) > World->EntityCount);
        ID = World->EntityCount++;
    }
    world_entity *NewEntity = World->Entities + ID;
    *NewEntity = {};
    NewEntity->IsPresent = true;
    
    NewEntity->ID = ID;
    NewEntity->Type = Type;
    NewEntity->Position = Position;
    NewEntity->Collision = Collision;

    chunk_range Range = GetEntityChunkRange(World, NewEntity, NewEntity->Position);
    for(i32 ChunkZ = Range.MinZ; ChunkZ <= Range.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = Range.MinY; ChunkY <= Range.MaxY; ChunkY++)
        {
            for(i32 ChunkX = Range.MinX; ChunkX <= Range.MaxX; ChunkX++)
            {
                world_chunk *ThisChunk =
                    GetOrCreateChunk(World, Arena, ChunkX, ChunkY, ChunkZ);
                InsertEntity(AppState, World, Arena,
                             &ThisChunk->FirstEntityChunk, NewEntity);
            }
        }
    }

    return NewEntity;
}

inline bool32
RemoveEntity(world *World, world_entity *Entity)
{
    bool32 Result = 0;
    // NOTE(zoubir): a sword and a fireball can both kill the same
    // monster in one frame
    if (!Entity->IsPresent)
    {
        return Result;
    }

    chunk_range Range = GetEntityChunkRange(World, Entity, Entity->Position);
    for(i32 ChunkZ = Range.MinZ; ChunkZ <= Range.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = Range.MinY; ChunkY <= Range.MaxY; ChunkY++)
        {
            for(i32 ChunkX = Range.MinX; ChunkX <= Range.MaxX; ChunkX++)
            {
                world_chunk *Chunk = FindChunk(World, ChunkX, ChunkY, ChunkZ);
                Result = Chunk && RemoveEntity(World, Chunk, Entity);
                Assert(Result);
                Entity->IsPresent = false;
            }
        }
    }
    Assert(World->FreeEntityCount < ArrayCount(World->FreeEntityIDs));
    World->FreeEntityIDs[World->FreeEntityCount++] = Entity->ID;
    return Result;
}
       
internal void
CheckAndChangeEntityChunk(app_state *AppState,
                          world *World, memory_arena *Arena,
                          v3 OldPosition,
                          world_entity *Entity)
{
    chunk_range Old = GetEntityChunkRange(World, Entity, OldPosition);
    chunk_range New = GetEntityChunkRange(World, Entity, Entity->Position);
    if (Old.MinX == New.MinX && Old.MinY == New.MinY && Old.MinZ == New.MinZ &&
        Old.MaxX == New.MaxX && Old.MaxY == New.MaxY && Old.MaxZ == New.MaxZ)
    {
        return;
    }
    for(i32 ChunkZ = Old.MinZ; ChunkZ <= Old.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = Old.MinY; ChunkY <= Old.MaxY; ChunkY++)
        {
            for(i32 ChunkX = Old.MinX; ChunkX <= Old.MaxX; ChunkX++)
            {
                world_chunk *ThisChunk = FindChunk(World, ChunkX, ChunkY, ChunkZ);
                bool32 Removed = ThisChunk && RemoveEntity(World, ThisChunk, Entity);
                Assert(Removed);
            }
        }
    }
    for(i32 ChunkZ = New.MinZ; ChunkZ <= New.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = New.MinY; ChunkY <= New.MaxY; ChunkY++)
        {
            for(i32 ChunkX = New.MinX; ChunkX <= New.MaxX; ChunkX++)
            {
                world_chunk *ThisChunk =
                    GetOrCreateChunk(World, Arena, ChunkX, ChunkY, ChunkZ);
                InsertEntity(AppState, World, Arena,
                             &ThisChunk->FirstEntityChunk, Entity);
            }
        }
    }
}
