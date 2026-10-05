/* Rewind restore: puts entities back as a history frame has them. A
   bubble or a self rewind puts back only what its hold took; a world
   rewind puts back everything, bringing back what has died or burnt out
   since and taking away what came after.

   A restored entity is a whole copy of the record, so every effect on it
   goes back with it: health, speed, status effects, cooldowns, the
   animation frame. Its rewind cooldowns stay as they are now. */

// NOTE(zoubir): takes the entity out of the chunks it is filed under,
// without freeing its slot, so it can be filed again where it is next
internal void
UnlinkRewindEntity(world *World, world_entity *Entity)
{
    chunk_range Range = GetEntityChunkRange(World, Entity, Entity->Position);
    for(i32 ChunkZ = Range.MinZ; ChunkZ <= Range.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = Range.MinY; ChunkY <= Range.MaxY; ChunkY++)
        {
            for(i32 ChunkX = Range.MinX; ChunkX <= Range.MaxX; ChunkX++)
            {
                world_chunk *Chunk = FindChunk(World, ChunkX, ChunkY, ChunkZ);
                if (Chunk)
                {
                    RemoveEntity(World, Chunk, Entity);
                }
            }
        }
    }
}

internal void
LinkRewindEntity(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Entity)
{
    chunk_range Range = GetEntityChunkRange(World, Entity, Entity->Position);
    for(i32 ChunkZ = Range.MinZ; ChunkZ <= Range.MaxZ; ChunkZ++)
    {
        for(i32 ChunkY = Range.MinY; ChunkY <= Range.MaxY; ChunkY++)
        {
            for(i32 ChunkX = Range.MinX; ChunkX <= Range.MaxX; ChunkX++)
            {
                world_chunk *Chunk = GetOrCreateChunk(World, Arena, ChunkX, ChunkY, ChunkZ);
                InsertEntity(AppState, World, Arena, &Chunk->FirstEntityChunk, Entity);
            }
        }
    }
}

// NOTE(zoubir): the entity in Record's slot becomes Record. When the slot
// holds the same entity its rewind cooldowns are kept
internal void
RestoreRewindEntity(app_state *AppState, world *World, memory_arena *Arena,
                    world_entity *Record)
{
    world_entity *Entity = &World->Entities[Record->ID];
    bool32 Same = Entity->IsPresent && Entity->RewindSerial == Record->RewindSerial;
    float Cooldowns[PLAYER_REWIND_SLOTS];
    for(u32 Index = 0; Index < PLAYER_REWIND_SLOTS; Index++)
    {
        Cooldowns[Index] = Same ? Entity->RewindCooldowns[Index] :
            Record->RewindCooldowns[Index];
    }
    if (Entity->IsPresent)
    {
        UnlinkRewindEntity(World, Entity);
    }
    if (!Same)
    {
        // NOTE(zoubir): the rules were for whoever had the slot before
        ClearCollisionRulesFor(AppState, Record->ID);
    }
    *Entity = *Record;
    Entity->IsPresent = true;
    for(u32 Index = 0; Index < PLAYER_REWIND_SLOTS; Index++)
    {
        Entity->RewindCooldowns[Index] = Cooldowns[Index];
    }
    LinkRewindEntity(AppState, World, Arena, Entity);
}

// NOTE(zoubir): at the end of a playback, a unit put back where something
// has come to stand since goes to the nearest free spot around it
// (FindFreeSpotAround) instead. Left there, the separation that follows
// pushes the two apart blindly and can shove it into a rock
inline bool32
IsRewindBlocker(app_state *AppState, world_entity *Entity, world_entity *Other)
{
    bool32 Unit = (Other->Type == EntityType_Player || Other->Type == EntityType_Monster) &&
        !IsDeadPlayer(Other);
    bool32 Solid = Other->Type == EntityType_StaticObject || Other->Type == EntityType_Tiled;
    bool32 Result = Other != Entity && Other->IsPresent && Other->Collision &&
        (Unit || Solid) && CanCollide(AppState, Entity->Type, Other->Type) &&
        CanCollide(AppState, Entity, Other) && EntityOverlap(Entity, Other);
    return Result;
}

internal void
SettleRewoundUnit(app_state *AppState, world *World, memory_arena *Arena,
                  world_entity *Entity)
{
    if (!Entity->IsPresent || !Entity->Collision || IsDeadPlayer(Entity) ||
        (Entity->Type != EntityType_Player && Entity->Type != EntityType_Monster))
    {
        return;
    }
    bool32 Free = true;
    for(u32 Index = 0; Index < World->EntityCount && Free; Index++)
    {
        Free = !IsRewindBlocker(AppState, Entity, &World->Entities[Index]);
    }
    if (Free && CanCollide(AppState, Entity->Type, EntityType_StaticObject))
    {
        // NOTE(zoubir): raised ground, and on infinite maps walls and
        // props, are terrain stand-ins, not entities
        entity_collision_volume *Total = &Entity->Collision->TotalVolume;
        rectangle3 Box = RectCenterHalfDims(Entity->Position + Total->Offset,
                                            Total->HalfDims);
        world_entity *Terrain[64];
        u32 Count = GatherTerrainColliders(World, Box, Terrain, 0, ArrayCount(Terrain));
        for(u32 Index = 0; Index < Count && Free; Index++)
        {
            Free = !EntityOverlap(Entity, Terrain[Index]);
        }
    }
    if (Free)
    {
        return;
    }
    // NOTE(zoubir): hidden from the search, so its own body is not in the way
    Entity->IsPresent = false;
    v3 Spot = FindFreeSpotAround(AppState, World, Entity->Position, Entity->Collision);
    Entity->IsPresent = true;
    v3 OldPosition = Entity->Position;
    Entity->Position = Spot;
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
}

// NOTE(zoubir): a bubble or self rewind showing Frame: each entity its
// hold took is put back as Frame has it. One that did not exist yet then
// (a fireball cast since) is taken away, but a player never is: one that
// joined since stays where it is
internal void
RestoreRewindCast(app_state *AppState, time_rewind *Rewind, world *World,
                  memory_arena *Arena, rewind_cast *Cast, rewind_frame *Frame)
{
    for(u32 Index = 0; Index < Cast->AffectedCount; Index++)
    {
        u32 ID = Cast->AffectedID[Index];
        u32 Serial = Cast->AffectedSerial[Index];
        if (!Serial)
        {
            continue;
        }
        world_entity *Entity = &World->Entities[ID];
        if (!Entity->IsPresent || Entity->RewindSerial != Serial)
        {
            Cast->AffectedSerial[Index] = 0;
            continue;
        }
        world_entity *Record = FindRewindRecord(Rewind, Frame, ID, Serial);
        if (Record)
        {
            RestoreRewindEntity(AppState, World, Arena, Record);
        }
        else if (Entity->Type != EntityType_Player)
        {
            RemoveEntity(World, Entity);
            Rewind->LockedSerial[ID] = 0;
            Cast->AffectedSerial[Index] = 0;
        }
    }
}

// NOTE(zoubir): a world rewind showing Frame: the world becomes Frame.
// Players keep their slots: one that joined since stays as it is, and
// one that left is not brought back. Scores stay; the respawn timers,
// the monster population's timers and its random series go back too, so
// what follows is what would have followed then
internal void
RestoreRewindWorld(app_state *AppState, time_rewind *Rewind, world *World,
                   memory_arena *Arena, rewind_frame *Frame)
{
    for(u32 ID = 0; ID < World->EntityCount; ID++)
    {
        world_entity *Entity = &World->Entities[ID];
        if (!IsRewindRecorded(Entity) || Entity->Type == EntityType_Player)
        {
            continue;
        }
        if (!Entity->RewindSerial ||
            !FindRewindRecord(Rewind, Frame, ID, Entity->RewindSerial))
        {
            RemoveEntity(World, Entity);
        }
    }
    for(u32 Index = 0; Index < Frame->RecordCount; Index++)
    {
        world_entity *Record = &Rewind->Records[Frame->FirstRecord + Index];
        world_entity *Entity = &World->Entities[Record->ID];
        if (Record->Type == EntityType_Player)
        {
            player_slot *Slot = &AppState->Players[Record->PlayerIndex];
            if (!Slot->Active || Slot->Entity != Entity ||
                Entity->RewindSerial != Record->RewindSerial)
            {
                continue;
            }
            Slot->RespawnTimer = Frame->RespawnTimers[Record->PlayerIndex];
            Slot->DelayedInputCount = 0;
        }
        else if (Entity->IsPresent && Entity->RewindSerial != Record->RewindSerial)
        {
            // NOTE(zoubir): a player that joined since took the slot
            continue;
        }
        RestoreRewindEntity(AppState, World, Arena, Record);
    }
    // NOTE(zoubir): the free slots are whatever is not present now
    World->FreeEntityCount = 0;
    for(u32 Step = 0; Step < World->EntityCount; Step++)
    {
        u32 ID = World->EntityCount - 1 - Step;
        if (!World->Entities[ID].IsPresent)
        {
            World->FreeEntityIDs[World->FreeEntityCount++] = ID;
        }
    }
    if (AppState->Monsters)
    {
        memcpy(AppState->Monsters, Frame->Population, REWIND_POPULATION_PREFIX);
    }
}
