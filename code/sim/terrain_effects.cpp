/* Terrain effects: the ground's rules applied to the units standing on it.
   Once per tick, every player and walking monster reads the terrain under
   its feet (TerrainAt for the world's map) and takes that kind's speed
   scale, friction, and standing statuses (lava burns, a spring heals, a
   pit drops you). Flyers and units in the air are untouched, so a jump or
   a dash carries a player over a pit; a unit on raised ground stands on
   its top (GroundHeightAt). Movement code reads the results through
   GetMoveSpeedScale and GetGroundFriction. Walking monsters steer around
   hazards (SteerAroundHazards), unless something throws them in. */

// NOTE(zoubir): units higher than this off the ground skip terrain
#define TERRAIN_FEET_HEIGHT 4.f

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

// NOTE(zoubir): how far ahead a walking monster looks for a hazard
#define HAZARD_LOOK_AHEAD 26.f

// NOTE(zoubir): a walking monster's wish to move along Wish, turned aside
// (a quarter turn either way) or stopped when it would carry it onto a
// pit or into lava. Where it already stands does not count, so one shoved
// onto a hazard still walks off it
internal v2
SteerAroundHazards(world *World, world_entity *Entity, v2 Wish)
{
    float WishLength = Length(Wish);
    if (WishLength <= 0.f)
    {
        return Wish;
    }
    v2 Along = Wish * (1.f / WishLength);
    v2 Side = V2(-Along.Y, Along.X);
    v2 Tries[3] = {Along, Side, -Side};
    for(u32 Try = 0; Try < ArrayCount(Tries); Try++)
    {
        v3 Ahead = Entity->Position;
        Ahead.XY += HAZARD_LOOK_AHEAD * Tries[Try];
        if (!IsHazardAt(World, Ahead))
        {
            return WishLength * Tries[Try];
        }
    }
    return V2(0.f, 0.f);
}

// NOTE(zoubir): the ground's grip on one unit: its speed scale and
// friction, or plain ground when it is in the air or flies. Returns the
// ground it stands on, or 0. Online prediction (client/prediction.cpp)
// calls it for the local player each predicted step, since prediction
// never runs UpdateTerrainEffects
internal terrain_def *
SetGroundUnderfoot(world *World, world_entity *Entity)
{
    Entity->GroundSpeedScale = 1.f;
    Entity->GroundFriction = 1.f;
    terrain_def *Result = 0;
    if (FeelsTerrain(World, Entity))
    {
        Result = GetTerrainDef(TerrainUnder(World, Entity->Position));
        Entity->GroundSpeedScale = Result->SpeedScale;
        Entity->GroundFriction = Result->Friction;
    }
    return Result;
}

// NOTE(zoubir): what the ground Entity stands on does to it this tick: its
// statuses, held while it stands there. Online prediction runs it for the
// local player too, so a rune's haste starts on the same tick there
internal void
ApplyGroundStatuses(world_entity *Entity, terrain_def *Ground)
{
    for(u32 Index = 0; Index < ArrayCount(Ground->Stand); Index++)
    {
        terrain_stand_status *Stand = &Ground->Stand[Index];
        if (Stand->Effect == StatusEffect_None)
        {
            continue;
        }
        // NOTE(zoubir): most effects are held while you stand there; one
        // that is shrugged off after (a root) or that ends in death (a
        // fall) runs its own course from the step that started it
        status_def *Def = GetStatusDef(Stand->Effect);
        if ((Def->ImmuneSeconds > 0.f || (Def->Flags & STATUS_FATAL)) &&
            HasStatus(Entity, Stand->Effect))
        {
            continue;
        }
        // NOTE(zoubir): the moment a fall begins: the body stops where
        // it went over the edge, and the kill is pinned on whoever
        // threw or hit it in
        if (Stand->Effect == StatusEffect_Falling)
        {
            Entity->ThrownBySlot = StatusBlameSlot(Entity);
            Entity->Velocity = V3(0.f, 0.f, Minimum(Entity->Velocity.Z, 0.f));
        }
        ApplyStatus(Entity, Stand->Effect, Stand->Seconds);
    }
}

internal void
UpdateTerrainEffects(world *World)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        terrain_def *Ground = SetGroundUnderfoot(World, Entity);
        if (!Ground)
        {
            continue;
        }
        ApplyGroundStatuses(Entity, Ground);
    }
}
