/* Waves: once every monster is dead a short countdown starts, then the
   next, larger wave spawns in a ring around the player on free ground.
   The monsters placed at startup are wave 1. */

#define WAVE_COUNTDOWN_SECONDS 3.f
#define WAVE_SPAWN_MIN_DISTANCE 380.f
#define WAVE_SPAWN_MAX_DISTANCE 480.f
#define WAVE_SPAWN_TRIES 12

struct wave_state
{
    u32 Number;
    // NOTE(zoubir): > 0 while counting down to the next wave
    float Countdown;
    random_series Series;
};

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

// NOTE(zoubir): bats get more common as waves go on
inline monster_kind
PickWaveMonsterKind(u32 WaveNumber, u32 MonsterIndex)
{
    u32 BatEvery = WaveNumber >= 3 ? 2 : 3;
    monster_kind Result = (MonsterIndex % BatEvery == BatEvery - 1) ?
        MonsterKind_Bat : MonsterKind_Brute;
    return Result;
}

internal u32
SpawnWave(app_state *AppState, world *World, memory_arena *Arena,
          wave_state *Wave, v2 Center)
{
    float MapWidth = (float)(World->NumTilesX * World->TileWidth);
    float MapHeight = (float)(World->NumTilesY * World->TileHeight);
    float Margin = 64.f;
    u32 MonsterCount = 4 + 2 * Wave->Number;
    u32 Spawned = 0;

    for(u32 MonsterIndex = 0;
        MonsterIndex < MonsterCount;
        MonsterIndex++)
    {
        monster_kind Kind = PickWaveMonsterKind(Wave->Number, MonsterIndex);
        entity_collision_volume_group *Volume =
            GetMonsterStats(Kind)->FlyHeight > 0.f ?
            AppState->BatCollision : AppState->PlayerCollision;

        for(u32 Try = 0; Try < WAVE_SPAWN_TRIES; Try++)
        {
            float Angle = RandomBetween(&Wave->Series, 0.f, 2.f * Pi32);
            float Distance = RandomBetween(&Wave->Series,
                                           WAVE_SPAWN_MIN_DISTANCE,
                                           WAVE_SPAWN_MAX_DISTANCE);
            v3 Position = {};
            Position.X = Center.X + Distance * Cos(Angle);
            Position.Y = Center.Y + Distance * Sin(Angle);
            Position.X = Minimum(MapWidth - Margin, Maximum(Margin, Position.X));
            Position.Y = Minimum(MapHeight - Margin, Maximum(Margin, Position.Y));

            if (IsSpawnSpotFree(AppState, World, Position, Volume))
            {
                AddMonster(AppState, World, Arena, Position, Kind);
                Spawned++;
                break;
            }
        }
    }
    return Spawned;
}

// NOTE(zoubir): call once per frame after entities have updated
internal void
UpdateWaves(app_state *AppState, world *World, memory_arena *Arena,
            wave_state *Wave, float DeltaTime)
{
    if (CountLiveMonsters(World) > 0)
    {
        return;
    }

    if (Wave->Countdown <= 0.f)
    {
        Wave->Countdown = WAVE_COUNTDOWN_SECONDS;
        return;
    }

    Wave->Countdown -= DeltaTime;
    if (Wave->Countdown <= 0.f)
    {
        Wave->Countdown = 0.f;
        Wave->Number++;
        world_entity *Player = AppState->Player;
        v2 Center = Player ? Player->Position.XY : V2(0.f);
        SpawnWave(AppState, World, Arena, Wave, Center);
    }
}
