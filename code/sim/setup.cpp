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
        CreateMonsterPopulation(MemoryArena, MONSTER_POPULATION, 1337);
    FillMonsterPopulation(AppState, &AppState->World, MemoryArena,
                          AppState->Monsters);
}

// NOTE(zoubir): throws the world away and builds it again for MapId, with
// no players and no monsters in it. The client calls it when it joins a
// server playing another map; the replicas then fill the world from the
// server's snapshots. Entity slots start again from 0, so every pairwise
// collision rule (keyed by slot) goes too
internal void
RebuildWorldForMap(app_state *AppState, memory_arena *Arena, u32 MapId)
{
    for(u32 Bucket = 0; Bucket < ArrayCount(AppState->CollisionRuleHash); Bucket++)
    {
        while (AppState->CollisionRuleHash[Bucket])
        {
            pairwise_collision_rule *Rule = AppState->CollisionRuleHash[Bucket];
            AppState->CollisionRuleHash[Bucket] = Rule->Next;
            Rule->Next = AppState->FirstFreeCollisionRule;
            AppState->FirstFreeCollisionRule = Rule;
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        AppState->Players[SlotIndex].Entity = 0;
        AppState->Players[SlotIndex].Active = false;
    }
    ZeroSize(&AppState->World, sizeof(AppState->World));
    AppState->World.MapId = MapId < MapId_Count ? MapId : MapId_Arena;
    BuildArena(AppState, Arena);
    AppState->Monsters =
        CreateMonsterPopulation(Arena, MONSTER_POPULATION, 1337);
}
