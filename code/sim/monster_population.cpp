/* Monster population: the arena keeps a fixed number of monsters roaming
   as hazards. When one dies another appears after a short delay, on free
   ground away from every player. There are no waves or levels. */

#define MONSTER_POPULATION 10
#define MONSTER_RESPAWN_SECONDS 2.f
// NOTE(zoubir): new monsters never appear on top of a player
#define MONSTER_SPAWN_MIN_PLAYER_DISTANCE 350.f
#define MONSTER_SPAWN_TRIES 16

// NOTE(zoubir): in monster_abilities.cpp, included at the bottom
internal void
StaggerMonsterCooldowns(app_state *AppState, world_entity *Entity);

// NOTE(zoubir): every monster the game makes comes through here, so each
// gets a serial and starts with its abilities part way charged
internal world_entity *
SpawnMonster(app_state *AppState, world *World, memory_arena *Arena,
             v3 Position, monster_kind Kind)
{
    world_entity *Monster = AddMonster(AppState, World, Arena, Position, Kind);
    if (AppState->Monsters)
    {
        Monster->MonsterSerial = ++AppState->Monsters->NextMonsterSerial;
    }
    StaggerMonsterCooldowns(AppState, Monster);
    return Monster;
}

// NOTE(zoubir): the monster in Slot if it is still the one with Serial
inline world_entity *
FindMonsterBySerial(world *World, u32 Slot, u32 Serial)
{
    world_entity *Result = 0;
    if (Serial && Slot < World->EntityCount)
    {
        world_entity *Entity = &World->Entities[Slot];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterSerial == Serial)
        {
            Result = Entity;
        }
    }
    return Result;
}

inline u32
CountLiveMonsters(world *World)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster)
        {
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): true when a monster with Volume placed at Position would
// not overlap anything monsters collide with (trees, walls, units)
internal bool32
IsSpawnSpotFree(app_state *AppState, world *World, v3 Position,
                entity_collision_volume_group *Volume)
{
    world_entity Probe = {};
    Probe.Type = EntityType_Monster;
    Probe.Position = Position;
    Probe.Collision = Volume;

    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Other = &World->Entities[EntityIndex];
        if (Other->IsPresent &&
            CanCollide(AppState, EntityType_Monster, Other->Type) &&
            EntityOverlap(&Probe, Other))
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): also keeps clear of every slot's spawn point, so a player
// joining or respawning never lands inside or next to a monster
inline bool32
IsFarFromPlayers(world *World, v2 Position, float MinDistance)
{
    for(u32 SlotIndex = 0;
        SlotIndex < MAX_PLAYERS;
        SlotIndex++)
    {
        if (Length(PlayerSpawnPosition(SlotIndex).XY - Position) < MinDistance)
        {
            return false;
        }
    }

    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Player &&
            Length(Entity->Position.XY - Position) < MinDistance)
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): picks a random free spot anywhere in the arena; returns 0
// when every try was blocked
internal world_entity *
SpawnRoamingMonster(app_state *AppState, world *World, memory_arena *Arena,
                    monster_population *Population)
{
    float Margin = 64.f;
    float MapWidth = (float)(World->NumTilesX * World->TileWidth);
    float MapHeight = (float)(World->NumTilesY * World->TileHeight);
    monster_kind Kind = PickMonsterKind(&Population->Series, World);
    entity_collision_volume_group *Volume =
        GetMonsterStats(Kind)->FlyHeight > 0.f ?
        AppState->BatCollision : AppState->PlayerCollision;

    for(u32 Try = 0; Try < MONSTER_SPAWN_TRIES; Try++)
    {
        v3 Position = {};
        Position.X = RandomBetween(&Population->Series, Margin, MapWidth - Margin);
        Position.Y = RandomBetween(&Population->Series, Margin, MapHeight - Margin);
        if (IsFarFromPlayers(World, Position.XY,
                             MONSTER_SPAWN_MIN_PLAYER_DISTANCE) &&
            IsSpawnSpotFree(AppState, World, Position, Volume))
        {
            world_entity *Monster =
                SpawnMonster(AppState, World, Arena, Position, Kind);
            ApplyEliteAffix(Monster, RollEliteAffix(&Population->Series));
            return Monster;
        }
    }
    return 0;
}

internal monster_population *
CreateMonsterPopulation(memory_arena *Arena, u32 Target, u32 SeedValue)
{
    monster_population *Result = AllocateStruct(Arena, monster_population);
    *Result = {};
    Result->Target = Target;
    Result->RespawnTimer = MONSTER_RESPAWN_SECONDS;
    Result->Series = Seed(SeedValue);
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        SetupMonsterAnimationSet(&Result->AnimationSets[KindIndex], Arena,
                                 GetMonsterDef((monster_kind)KindIndex));
    }
    for(u32 Style = 0; Style < ShotStyle_Count; Style++)
    {
        // NOTE(zoubir): shots are round, every direction plays one row
        animation_set *Set = &Result->ShotAnimationSets[Style];
        for(u32 Direction = 0; Direction < AnimationDirection_Count; Direction++)
        {
            AddAnimation(Set, Arena, AnimationType_Move,
                         (animation_direction)Direction,
                         0, SHOT_FRAMES, 0.06f,
                         Direction == AnimationDirection_Left);
        }
    }
    for(u32 Style = 0; Style < HazardStyle_Count; Style++)
    {
        animation_set *Set = &Result->HazardAnimationSets[Style];
        for(u32 Direction = 0; Direction < AnimationDirection_Count; Direction++)
        {
            AddAnimation(Set, Arena, AnimationType_Stand,
                         (animation_direction)Direction,
                         0, HAZARD_FRAMES, 0.15f);
        }
    }
    return Result;
}

// NOTE(zoubir): fills the arena straight away, used when a match starts
internal void
FillMonsterPopulation(app_state *AppState, world *World, memory_arena *Arena,
                      monster_population *Population)
{
    u32 Live = CountLiveMonsters(World);
    for(u32 Count = Live; Count < Population->Target; Count++)
    {
        SpawnRoamingMonster(AppState, World, Arena, Population);
    }
}

// NOTE(zoubir): call once per frame after entities have updated; brings
// back one monster every MONSTER_RESPAWN_SECONDS while below Target
// NOTE(zoubir): children of a split spread evenly around the corpse,
// falling back to the corpse's own spot when the ring is blocked
internal void
SplitMonster(app_state *AppState, world *World, memory_arena *Arena,
             monster_death_record *Record, monster_def *Def)
{
    monster_def *ChildDef = GetMonsterDef(Def->SplitKind);
    entity_collision_volume_group *Volume = ChildDef->FlyHeight > 0.f ?
        AppState->BatCollision : AppState->PlayerCollision;
    float StartAngle = RandomBetween(&AppState->Monsters->Series, 0.f, 2.f * Pi32);
    for(u32 Child = 0; Child < Def->SplitCount; Child++)
    {
        float Angle = StartAngle + 2.f * Pi32 * (float)Child / (float)Def->SplitCount;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v3 Position = Record->Position;
        Position.XY += 20.f * Out;
        Position.Z = 0.f;
        if (!IsSpawnSpotFree(AppState, World, Position, Volume))
        {
            Position = Record->Position;
            Position.Z = 0.f;
            if (!IsSpawnSpotFree(AppState, World, Position, Volume))
            {
                continue;
            }
        }
        world_entity *Spawned = SpawnMonster(AppState, World, Arena, Position,
                                             Def->SplitKind);
        // NOTE(zoubir): an elite's children keep its affix
        ApplyEliteAffix(Spawned, Record->EliteAffix);
        // NOTE(zoubir): a little pop outward so the split reads
        Spawned->Velocity.XY = 180.f * Out;
    }
}

internal void
RunPendingMonsterDeaths(app_state *AppState, world *World, memory_arena *Arena,
                        monster_population *Population)
{
    for(u32 DeathIndex = 0;
        DeathIndex < Population->PendingDeathCount;
        DeathIndex++)
    {
        monster_death_record *Record = &Population->PendingDeaths[DeathIndex];
        monster_def *Def = GetMonsterDef(Record->Kind);
        switch(Def->DeathEffect)
        {
            case DeathEffect_Split:
            {
                SplitMonster(AppState, World, Arena, Record, Def);
            } break;

            default:
            {
            } break;
        }
    }
    Population->PendingDeathCount = 0;
}

// NOTE(zoubir): summons whose summoner is gone fall apart. They are removed
// without DamageEntity: nobody killed them, so nobody gets credit
internal void
CrumbleOrphanedSummons(world *World)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->SummonerSerial &&
            !FindMonsterBySerial(World, Entity->SummonerSlot,
                                 Entity->SummonerSerial))
        {
            RemoveEntity(World, Entity);
        }
    }
}

internal void
UpdateMonsterPopulation(app_state *AppState, world *World,
                        memory_arena *Arena,
                        monster_population *Population, float DeltaTime)
{
    RunPendingMonsterDeaths(AppState, World, Arena, Population);
    CrumbleOrphanedSummons(World);
    if (CountLiveMonsters(World) >= Population->Target)
    {
        Population->RespawnTimer = MONSTER_RESPAWN_SECONDS;
        return;
    }

    Population->RespawnTimer -= DeltaTime;
    if (Population->RespawnTimer <= 0.f)
    {
        Population->RespawnTimer = MONSTER_RESPAWN_SECONDS;
        SpawnRoamingMonster(AppState, World, Arena, Population);
    }
}

#include "monster_abilities.cpp"
