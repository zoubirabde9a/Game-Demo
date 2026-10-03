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
    monster_kind Kind = PickMonsterKind(&Population->Series);
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
                AddMonster(AppState, World, Arena, Position, Kind);
            StaggerMonsterCooldowns(AppState, Monster);
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
internal void
UpdateMonsterPopulation(app_state *AppState, world *World,
                        memory_arena *Arena,
                        monster_population *Population, float DeltaTime)
{
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
