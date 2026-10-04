/* Terrain effects: the ground's rules applied to the units standing on it.
   Once per tick, every player and walking monster reads the terrain under
   its feet (TerrainAt for the world's map) and takes that kind's speed
   scale, friction, and standing status (lava burns). Flyers and units in
   the air are untouched; a unit on raised ground stands on its top
   (GroundHeightAt). Movement code reads the results through
   GetMoveSpeedScale and GetGroundFriction. */

// NOTE(zoubir): units higher than this off the ground skip terrain
#define TERRAIN_FEET_HEIGHT 4.f

inline terrain_kind
TerrainUnder(world *World, v3 Position)
{
    i32 TileSize = World->TileWidth ? (i32)World->TileWidth : ARENA_TILE_SIZE;
    i32 TileX = FloorDiv((i32)floorf(Position.X), TileSize);
    i32 TileY = FloorDiv((i32)floorf(Position.Y), TileSize);
    terrain_kind Result = TerrainAt(GetMapDef((map_id)World->MapId), TileX, TileY);
    return Result;
}

inline bool32
FeelsTerrain(world *World, world_entity *Entity)
{
    bool32 Result = false;
    if (Entity->IsPresent && Entity->Hp > 0.f &&
        Entity->Position.Z <= GroundHeightAt(World, Entity->Position.XY) +
        TERRAIN_FEET_HEIGHT)
    {
        if (Entity->Type == EntityType_Player)
        {
            Result = true;
        }
        else if (Entity->Type == EntityType_Monster)
        {
            Result = GetMonsterDef(Entity->MonsterKind)->FlyHeight <= 0.f &&
                !Entity->Burrowed;
        }
    }
    return Result;
}

internal void
UpdateTerrainEffects(world *World)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        Entity->GroundSpeedScale = 1.f;
        Entity->GroundFriction = 1.f;
        if (!FeelsTerrain(World, Entity))
        {
            continue;
        }
        terrain_def *Ground = GetTerrainDef(TerrainUnder(World, Entity->Position));
        Entity->GroundSpeedScale = Ground->SpeedScale;
        Entity->GroundFriction = Ground->Friction;
        ApplyStatus(Entity, Ground->StandStatus, Ground->StandStatusSeconds);
    }
}
