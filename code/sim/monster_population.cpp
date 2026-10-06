/* Monster population: the map keeps a fixed number of monsters roaming
   as hazards (map_def.MonsterPopulation, none on the Old Arena). When one
   dies another appears after a short delay, on free ground away from
   every player. There are no waves or levels. */

#define MONSTER_RESPAWN_SECONDS 2.f
// NOTE(zoubir): new monsters never appear on top of a player
#define MONSTER_SPAWN_MIN_PLAYER_DISTANCE 350.f
#define MONSTER_SPAWN_TRIES 16

// NOTE(zoubir): how many monsters World's map keeps roaming
inline u32
MapMonsterPopulation(world *World)
{
    u32 Result = GetMapDef((map_id)World->MapId)->MonsterPopulation;
    return Result;
}

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

// NOTE(zoubir): true when a monster with Volume at Position would overlap
// nothing monsters collide with (trees, walls, units) nor stand on a hazard
internal bool32
IsSpawnSpotFree(app_state *AppState, world *World, v3 Position,
                entity_collision_volume_group *Volume)
{
    if (IsHazardAt(World, Position)) return false;
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
    {
        // NOTE(zoubir): infinite maps keep walls and props as terrain, not
        // entities, and every map keeps raised ground that way; ask for
        // stand-ins around the probe
        entity_collision_volume *Total = &Volume->TotalVolume;
        rectangle3 Box = RectCenterHalfDims(Position + Total->Offset, Total->HalfDims);
        world_entity *Nearby[64];
        u32 Count = GatherTerrainColliders(World, Box, Nearby, 0, ArrayCount(Nearby));
        for(u32 Index = 0; Index < Count; Index++)
        {
            if (EntityOverlap(&Probe, Nearby[Index]))
            {
                return false;
            }
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
        if (Length(PlayerSpawnPosition(World, SlotIndex).XY - Position) < MinDistance)
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

// NOTE(zoubir): on infinite maps monsters appear between these distances
// from a player, and leave once they are further than DESPAWN from all
#define MONSTER_RING_MIN 450.f
#define MONSTER_RING_MAX 900.f
#define MONSTER_DESPAWN_DISTANCE 1600.f

internal world_entity *
PickRandomPlayer(world *World, random_series *Series)
{
    world_entity *Players[64];
    u32 Count = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount && Count < ArrayCount(Players);
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Player && Entity->Hp > 0.f)
        {
            Players[Count++] = Entity;
        }
    }
    world_entity *Result = Count ? Players[RandomChoice(Series, Count)] : 0;
    return Result;
}

// NOTE(zoubir): on an infinite map the population follows the players:
// monsters left far behind everyone are taken away (no death, no credit)
// so the refill can put them where someone is
internal void
DespawnFarMonsters(world *World)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster)
        {
            continue;
        }
        bool32 Near = false;
        for(u32 Other = 0; Other < World->EntityCount && !Near; Other++)
        {
            world_entity *Player = &World->Entities[Other];
            Near = Player->IsPresent && Player->Type == EntityType_Player &&
                LengthSq(Player->Position.XY - Monster->Position.XY) <
                MONSTER_DESPAWN_DISTANCE * MONSTER_DESPAWN_DISTANCE;
        }
        if (!Near)
        {
            RemoveEntity(World, Monster);
        }
    }
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
    // NOTE(zoubir): infinite maps have no edges to pick inside; monsters
    // appear in a ring around a random player instead
    world_entity *Around = 0;
    if (World->Unbounded)
    {
        Around = PickRandomPlayer(World, &Population->Series);
        if (!Around)
        {
            return 0;
        }
    }
    monster_kind Kind = PickMonsterKind(&Population->Series, World);
    entity_collision_volume_group *Volume =
        GetMonsterStats(Kind)->FlyHeight > 0.f ?
        AppState->BatCollision : AppState->PlayerCollision;

    for(u32 Try = 0; Try < MONSTER_SPAWN_TRIES; Try++)
    {
        v3 Position = {};
        if (Around)
        {
            float Angle = RandomBetween(&Population->Series, 0.f, 2.f * Pi32);
            float Distance = RandomBetween(&Population->Series,
                                           MONSTER_RING_MIN, MONSTER_RING_MAX);
            Position.XY = Around->Position.XY + Distance * V2(Cos(Angle), Sin(Angle));
        }
        else
        {
            Position.X = RandomBetween(&Population->Series, Margin, MapWidth - Margin);
            Position.Y = RandomBetween(&Population->Series, Margin, MapHeight - Margin);
        }
        // NOTE(zoubir): on top of raised ground, never inside it
        Position = OnGround(World, Position);
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
        Position = OnGround(World, Position);
        if (!IsSpawnSpotFree(AppState, World, Position, Volume))
        {
            Position = OnGround(World, Record->Position);
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

// NOTE(zoubir): a landmark's guards appear the first time a player comes
// this close to its middle
#define LANDMARK_WAKE_DISTANCE 420.f

inline bool32
IsLandmarkAwake(monster_population *Population, i32 RegionX, i32 RegionY)
{
    for(u32 Index = 0; Index < Population->AwakenedCount; Index++)
    {
        if (Population->AwakenedRegionX[Index] == RegionX &&
            Population->AwakenedRegionY[Index] == RegionY)
        {
            return true;
        }
    }
    return false;
}

inline void
MarkLandmarkAwake(monster_population *Population, i32 RegionX, i32 RegionY)
{
    u32 Slot = Population->AwakenedNext;
    Population->AwakenedRegionX[Slot] = RegionX;
    Population->AwakenedRegionY[Slot] = RegionY;
    Population->AwakenedNext = (Slot + 1) % ArrayCount(Population->AwakenedRegionX);
    if (Population->AwakenedCount < ArrayCount(Population->AwakenedRegionX))
    {
        Population->AwakenedCount++;
    }
}

internal void
AwakenNearbyLandmarks(app_state *AppState, world *World, memory_arena *Arena,
                      monster_population *Population)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    i32 Tile = (i32)World->TileWidth;
    i32 RegionSize = LANDMARK_REGION_TILES * Tile;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f)
        {
            continue;
        }
        i32 PlayerRegionX = FloorDiv((i32)floorf(Player->Position.X), RegionSize);
        i32 PlayerRegionY = FloorDiv((i32)floorf(Player->Position.Y), RegionSize);
        for(i32 DY = -1; DY <= 1; DY++)
        {
            for(i32 DX = -1; DX <= 1; DX++)
            {
                landmark_spot Spot = GetRegionLandmark(Map, PlayerRegionX + DX,
                                                       PlayerRegionY + DY);
                if (!Spot.Present ||
                    IsLandmarkAwake(Population, Spot.RegionX, Spot.RegionY) ||
                    Length(GetLandmarkCenter(&Spot, Tile).XY - Player->Position.XY) >
                    LANDMARK_WAKE_DISTANCE)
                {
                    continue;
                }
                MarkLandmarkAwake(Population, Spot.RegionX, Spot.RegionY);
                landmark_def *Def = &LandmarkTable[Spot.Landmark];
                v3 Spots[MAX_LANDMARK_GUARDS];
                u32 SpotCount = GetLandmarkGuardSpots(&Spot, Tile, Spots, ArrayCount(Spots));
                for(u32 Guard = 0; Guard < Def->GuardCount && Guard < SpotCount; Guard++)
                {
                    monster_kind Kind = Def->Guards[Guard];
                    monster_def *KindDef = GetMonsterDef(Kind);
                    entity_collision_volume_group *Volume = KindDef->FlyHeight > 0.f ?
                        AppState->BatCollision : AppState->PlayerCollision;
                    v3 GuardSpot = OnGround(World, Spots[Guard]);
                    if (!IsSpawnSpotFree(AppState, World, GuardSpot, Volume))
                    {
                        continue;
                    }
                    world_entity *Monster = SpawnMonster(AppState, World, Arena,
                                                         GuardSpot, Kind);
                    // NOTE(zoubir): the first guard leads, always an elite
                    if (Guard == 0)
                    {
                        ApplyEliteAffix(Monster, 1 + RandomChoice(&Population->Series,
                                                                  MonsterAffix_Count - 1));
                    }
                }
            }
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
    if (World->Unbounded)
    {
        DespawnFarMonsters(World);
        AwakenNearbyLandmarks(AppState, World, Arena, Population);
    }
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
