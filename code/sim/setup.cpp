/* InitSimulation: builds a ready-to-tick world with no players in it:
   collision shapes, animation tables, the arena and its monsters. Needs no
   window, textures or sound, so the game client and the dedicated server
   both call it. Players are added afterwards with AddPlayerToSlot. */

// NOTE(zoubir): a fingerprint of everything a client and server must agree
// on to read each other's snapshots: the entity and status enums, the
// player limit, and the monster table (kinds in order, abilities, affixes,
// see ComputeMonsterTableHash). Two builds whose content differs get
// different values, so the server turns away a mismatched client. Never 0;
// 0 in a connect request means "not a game client" (the health probe).
internal u32
SimContentId()
{
    u32 Parts[] = {EntityType_Count, MonsterKind_Count, StatusEffect_Count,
                   MAX_PLAYERS, ComputeMonsterTableHash(),
                   MapId_Count, ComputeTerrainContentHash()};
    u32 Hash = 2166136261u; // FNV-1a
    u8 *Bytes = (u8 *)Parts;
    for(u32 Index = 0; Index < sizeof(Parts); Index++)
    {
        Hash = (Hash ^ Bytes[Index]) * 16777619u;
    }
    return Hash ? Hash : 1;
}

internal void
InitSimulation(app_state *AppState, memory_arena *MemoryArena,
               memory_arena *ConstantsArena)
{
    SetupCollisionVolumes(AppState, ConstantsArena);
    SetupAnimationSets(AppState, ConstantsArena);
    BuildArena(AppState, MemoryArena);
    SetupCollisionTable(AppState);

    AppState->Monsters =
        CreateMonsterPopulation(MemoryArena, MapMonsterPopulation(&AppState->World), 1337);
    FillMonsterPopulation(AppState, &AppState->World, MemoryArena,
                          AppState->Monsters);
}

// NOTE(zoubir): throws the world away and builds it again for MapId, with
// no players and no monsters in it. StartNextRoundMap below uses it
// between rounds; the client calls it when it joins a
// server playing another map (the replicas then fill the world from the
// server's snapshots) and when the map picker starts one offline.
// Arena must hold this world and nothing else: it is emptied first, so
// switching maps any number of times uses no more memory than one map.
// Entity slots start again from 0, so every pairwise collision rule
// (keyed by slot, and living in Arena) goes too
internal void
RebuildWorldForMap(app_state *AppState, memory_arena *Arena, u32 MapId)
{
    Assert(Arena->TempCount == 0);
    Arena->Used = 0;
    for(u32 Bucket = 0; Bucket < ArrayCount(AppState->CollisionRuleHash); Bucket++)
    {
        AppState->CollisionRuleHash[Bucket] = 0;
    }
    AppState->FirstFreeCollisionRule = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->Players[SlotIndex].Entity = 0;
        AppState->Players[SlotIndex].Active = false;
    }
    ZeroSize(&AppState->World, sizeof(AppState->World));
    AppState->World.MapId = MapId < MapId_Count ? MapId : MapId_Arena;
    BuildArena(AppState, Arena);
    AppState->Monsters =
        CreateMonsterPopulation(Arena, MapMonsterPopulation(&AppState->World), 1337);
    // NOTE(zoubir): it lived in Arena; the next tick makes a new one
    AppState->Rewind = 0;
}

// NOTE(zoubir): the map after MapId in map_list.inc, round and round
inline u32
NextRoundMap(u32 MapId)
{
    u32 Result = (MapId + 1) % MapId_Count;
    return Result;
}

// NOTE(zoubir): a new round under the duel rules: the next map, built
// fresh with its monsters, and every player back at their spawn there
// with full health. Each slot keeps its name, level, experience, talents
// and score; a player that had a familiar gets it back. Arena must hold
// the world and nothing else (RebuildWorldForMap)
internal void
StartNextRoundMap(app_state *AppState, memory_arena *Arena)
{
    player_slot Kept[MAX_PLAYERS];
    bool32 HadFamiliar[MAX_PLAYERS] = {};
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Familiar &&
            Entity->FollowingEntity &&
            Entity->FollowingEntity->Type == EntityType_Player &&
            Entity->FollowingEntity->PlayerIndex < MAX_PLAYERS)
        {
            HadFamiliar[Entity->FollowingEntity->PlayerIndex] = true;
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Kept[SlotIndex] = AppState->Players[SlotIndex];
    }

    RebuildWorldForMap(AppState, Arena, NextRoundMap(World->MapId));
    FillMonsterPopulation(AppState, World, Arena, AppState->Monsters);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (!Kept[SlotIndex].Active)
        {
            continue;
        }
        world_entity *Player =
            AddPlayerToSlot(AppState, World, Arena, SlotIndex,
                            PlayerSpawnPosition(World, SlotIndex));
        player_slot *Slot = &AppState->Players[SlotIndex];
        v3 SpawnPosition = Slot->SpawnPosition;
        *Slot = Kept[SlotIndex];
        Slot->Entity = Player;
        Slot->SpawnPosition = SpawnPosition;
        Slot->RespawnTimer = 0.f;
        Slot->DelayedInputCount = 0;
        Player->SpawnShield = RespawnShieldSeconds(Slot);
        if (HadFamiliar[SlotIndex])
        {
            AddFamiliar(AppState, World, Arena, Player);
        }
    }
}
