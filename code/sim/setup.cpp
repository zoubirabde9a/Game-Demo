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
    StartDungeonRun(AppState, MemoryArena);
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
    StartDungeonRun(AppState, Arena);
}

// NOTE(zoubir): a new round: the same map (or a dungeon's next level),
// or the one a vote picked
// (sim/map_vote.cpp), built fresh with its monsters, and every player
// back at their spawn there with full health. Each slot keeps its name
// and score, and its level, experience and talents unless a vote moved
// everyone into or out of a duel, which starts them over; a vote from one
// dungeon map to another starts a new run with the levels and talents
// the party has, and a dungeon character set aside for a duel comes back
// with the next dungeon. A player that had a familiar and kept its talents gets
// it back. In a team duel each keeps its team and starts on its side;
// going into or out of teams clears the scores. Arena must hold the world and nothing else
// (RebuildWorldForMap)
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

    bool32 NewRun = AppState->NextMapVoted;
    // NOTE(zoubir): a cleared dungeon level goes on to the next one
    // (NextRunMap, sim/dungeon/levels.cpp), any other map plays again
    u32 MapId = NewRun ? AppState->NextMap : NextRunMap(World->MapId);
    // NOTE(zoubir): dungeon to dungeon keeps the characters; a change of
    // mode starts them over, since a duel's tree and a class's are apart
    bool32 FromDungeon = GetMapDef((map_id)World->MapId)->Dungeon;
    bool32 ToDungeon = MapId < MapId_Count && GetMapDef((map_id)MapId)->Dungeon;
    bool32 StartOver = NewRun && !(FromDungeon && ToDungeon);
    // NOTE(zoubir): a vote also says whether the duel is played in teams;
    // going into or out of teams is a new match, scores and all
    // (sim/teams/)
    bool32 TeamDuel = NewRun ? (AppState->NextTeamDuel && !ToDungeon) : AppState->TeamDuel;
    bool32 NewTeams = TeamDuel != AppState->TeamDuel;
    AppState->TeamDuel = TeamDuel;
    AppState->NextMapVoted = false;
    if (NewRun)
    {
        AppState->DungeonRoomsCleared = 0;
    }
    RebuildWorldForMap(AppState, Arena, MapId);
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
        if (NewTeams)
        {
            Slot->Kills = Slot->Deaths = Slot->MonsterKills = 0;
            Slot->Team = Team_None;
        }
        if (StartOver)
        {
            if (FromDungeon)
            {
                Slot->DungeonXp = Slot->Xp;
                memcpy(Slot->DungeonRanks, Slot->Ranks, sizeof(Slot->Ranks));
            }
            Slot->Xp = 0;
            Slot->XpClock = 0.f;
            Slot->Level = 1;
            ZeroArray(Slot->Ranks, TALENT_SLOTS, u8);
            Slot->WardReady = false;
            Slot->WardRecharge = 0.f;
            if (ToDungeon)
            {
                Slot->Xp = Slot->DungeonXp;
                Slot->Level = LevelForXp(Slot->Xp);
                memcpy(Slot->Ranks, Slot->DungeonRanks, sizeof(Slot->Ranks));
            }
        }
        if (NewRun)
        {
            Slot->RunStartLevel = Slot->Level;
            // NOTE(zoubir): the second tree's wild slots roll again
            // (sim/dungeon/run_tree/run_tree.cpp)
            RerollRunTree(Slot, SlotIndex, MapId * 2654435761u + Slot->Xp);
        }
        // NOTE(zoubir): after the ranks, which can raise its health
        ApplyRoleToPlayer(AppState, Slot);
        Player->SpawnShield = RespawnShieldSeconds(Slot);
        if (HadFamiliar[SlotIndex] && !StartOver)
        {
            AddFamiliar(AppState, World, Arena, Player);
        }
    }
    StartTeamRound(AppState, Arena);
}
