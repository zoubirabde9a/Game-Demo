/* Moving an entity: MoveEntity sweeps it through the world for one tick,
   sliding along walls, applying hits and overlaps, and refreshing the
   ground height under it. Its steps are the functions above it. */

// NOTE(zoubir): how many entities a move can have nearby; crowded map
// corners hold a few dozen walls per chunk
#define MOVE_MAX_NEARBY 1024

// NOTE(zoubir): the box a move from From by Delta sweeps through, grown
// by the mover's collision volume. The destination's Z is clamped to the
// floor, as the mover never goes below it.
internal rectangle3
GetMoveBox(world_entity *Entity, v3 From, v3 Delta)
{
    v3 To = From + Delta;
    To.Z = Maximum(0.f, To.Z);
    entity_collision_volume *Total = &Entity->Collision->TotalVolume;
    v3 MinPos = Minimum3(From, To) + Total->Offset - Total->HalfDims;
    v3 MaxPos = Maximum3(From, To) + Total->Offset + Total->HalfDims;
    rectangle3 Result = RectMinMax(MinPos, MaxPos);
    return Result;
}

// NOTE(zoubir): whether Entity's sweep can hit Other at all: not itself,
// not a dead player's body, and both the type table and any pairwise rule
// allow it
inline bool32
CanSweepAgainst(app_state *AppState, world_entity *Entity, world_entity *Other)
{
    bool32 Result = (Other != Entity &&
                     !IsDeadPlayer(Other) &&
                     CanCollide(AppState, Entity, Other) &&
                     CanCollide(AppState, Entity->Type, Other->Type));
    return Result;
}

// NOTE(zoubir): how far two boxes must overlap before a sweep treats them
// as already inside each other; resting contact is closer than this
#define MOVE_OVERLAP_EPSILON 0.01f

inline bool32
IsWalkingUnit(world_entity *Entity)
{
    bool32 Result = (Entity->Type == EntityType_Player ||
                     Entity->Type == EntityType_Monster ||
                     Entity->Type == EntityType_Familiar);
    return Result;
}

// NOTE(zoubir): per axis, how far a point Rel lies inside a box of half
// size Diameter centred on 0 (negative outside)
inline v3
OverlapDepth(v3 Diameter, v3 Rel)
{
    v3 Result = V3(Diameter.X - Absolute(Rel.X),
                   Diameter.Y - Absolute(Rel.Y),
                   Diameter.Z - Absolute(Rel.Z));
    return Result;
}

// NOTE(zoubir): the way out of an overlap: the axis with least depth,
// pointing from the other box towards the mover
inline v3
ShallowestAxisNormal(v3 Depth, v3 Rel)
{
    v3 Result = {};
    if (Depth.X <= Depth.Y && Depth.X <= Depth.Z)
    {
        Result.X = (Rel.X >= 0.f) ? 1.f : -1.f;
    }
    else if (Depth.Y <= Depth.Z)
    {
        Result.Y = (Rel.Y >= 0.f) ? 1.f : -1.f;
    }
    else
    {
        Result.Z = (Rel.Z >= 0.f) ? 1.f : -1.f;
    }
    return Result;
}

// NOTE(zoubir): sweeps every volume of Entity, moving from From by Delta,
// against every volume of Other (Minkowski boxes, one wall per face).
// Lowers *tMin to the earliest hit and sets *Normal; true if Other was hit
// earlier than anything tested before it.
internal bool32
SweepAgainstEntity(world_entity *Entity, v3 From, v3 Delta,
                   world_entity *Other, float *tMin, v3 *Normal)
{
    bool32 Hit = false;
    bool32 BothAreUnits = IsWalkingUnit(Entity) && IsWalkingUnit(Other);
    for(u32 VolumeIndex = 0;
        VolumeIndex < Entity->Collision->VolumesCount;
        VolumeIndex++)
    {
        entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
        for(u32 OtherIndex = 0;
            OtherIndex < Other->Collision->VolumesCount;
            OtherIndex++)
        {
            entity_collision_volume *OtherVolume =
                Other->Collision->Volumes + OtherIndex;
            v3 MinkowskiDiameter = Volume->HalfDims + OtherVolume->HalfDims;
            v3 MinCorner = -MinkowskiDiameter;
            v3 MaxCorner = MinkowskiDiameter;
            v3 Rel = (From + Volume->Offset) -
                (Other->Position + OtherVolume->Offset);

            // NOTE(zoubir): two units already overlapping (one spawned on
            // top of the other): every face is "ahead", so the walls below
            // would trap the mover. Let it move out or along; only a move
            // that goes deeper is stopped, at its start, with the normal
            // of the shallowest axis so the slide keeps the rest. Walls
            // and projectiles keep the face test, so nothing slides out
            // through a wall it started inside.
            v3 Depth = OverlapDepth(MinkowskiDiameter, Rel);
            if (BothAreUnits &&
                Depth.X > MOVE_OVERLAP_EPSILON &&
                Depth.Y > MOVE_OVERLAP_EPSILON &&
                Depth.Z > MOVE_OVERLAP_EPSILON)
            {
                float Now = Minimum(Depth.X, Minimum(Depth.Y, Depth.Z));
                v3 DepthAfter = OverlapDepth(MinkowskiDiameter, Rel + Delta);
                float After = Minimum(DepthAfter.X,
                                      Minimum(DepthAfter.Y, DepthAfter.Z));
                if (After > Now && *tMin > 0.f)
                {
                    *tMin = 0.f;
                    *Normal = ShallowestAxisNormal(Depth, Rel);
                    Hit = true;
                }
                continue;
            }

            test_wall Walls[] =
                {
                    {MinCorner.X, Rel.X, Rel.Y, Rel.Z, Delta.X, Delta.Y, Delta.Z, MinCorner.Y, MaxCorner.Y, MinCorner.Z, MaxCorner.Z, {-1, 0, 0}},
                    {MaxCorner.X, Rel.X, Rel.Y, Rel.Z, Delta.X, Delta.Y, Delta.Z, MinCorner.Y, MaxCorner.Y, MinCorner.Z, MaxCorner.Z, {1, 0, 0}},
                    {MinCorner.Y, Rel.Y, Rel.X, Rel.Z, Delta.Y, Delta.X, Delta.Z, MinCorner.X, MaxCorner.X, MinCorner.Z, MaxCorner.Z, {0, 1, 0}},
                    {MaxCorner.Y, Rel.Y, Rel.X, Rel.Z, Delta.Y, Delta.X, Delta.Z, MinCorner.X, MaxCorner.X, MinCorner.Z, MaxCorner.Z, {0, -1, 0}},
                    {MinCorner.Z, Rel.Z, Rel.Y, Rel.X, Delta.Z, Delta.Y, Delta.X, MinCorner.Y, MaxCorner.Y, MinCorner.X, MaxCorner.X, {0, 0, -1}},
                    {MaxCorner.Z, Rel.Z, Rel.Y, Rel.X, Delta.Z, Delta.Y, Delta.X, MinCorner.Y, MaxCorner.Y, MinCorner.X, MaxCorner.X, {0, 0, 1}}
                };
            for(u32 WallIndex = 0; WallIndex < ArrayCount(Walls); WallIndex++)
            {
                test_wall *Wall = &Walls[WallIndex];
                if (TestWall(Wall->X, Wall->Rel.X, Wall->Rel.Y, Wall->Rel.Z,
                             Wall->Delta.X, Wall->Delta.Y, Wall->Delta.Z,
                             tMin, Wall->MinY, Wall->MaxY,
                             Wall->MinZ, Wall->MaxZ))
                {
                    *Normal = Wall->Normal;
                    Hit = true;
                }
            }
        }
    }
    return Hit;
}

// NOTE(zoubir): overlap effects (sword hits) between Entity and each of
// Nearby. Entities removed earlier in the same move are skipped.
internal void
CheckOverlapsWith(app_state *AppState, world *World, memory_arena *Arena,
                  world_entity *Entity, world_entity **Nearby, u32 NearbyCount)
{
    for(u32 Index = 0; Index < NearbyCount; Index++)
    {
        world_entity *Other = Nearby[Index];
        if (Other != Entity &&
            Other->IsPresent &&
            !IsDeadPlayer(Other) &&
            CanOverlap(Entity, Other) &&
            CanCollide(AppState, Entity, Other) &&
            EntityOverlap(Entity, Other))
        {
            HandleOverlap(AppState, World, Arena, Entity, Other);
        }
    }
}

// NOTE(zoubir): what a hit does to the rest of the move. Blocking hits
// slide along the wall (the part of the move and velocity into it is
// removed); pass-through hits (fireball into monster) get a rule so the
// pair stops colliding, and the rest of the move carries on. Returns false
// when the hit removed Entity itself.
internal bool32
ResolveMoveHit(app_state *AppState, world *World, memory_arena *Arena,
               world_entity *Entity, world_entity *Other, v3 Normal,
               v3 AllowedDelta, v3 *Delta)
{
    bool32 StopsOnCollision = HandleCollision(AppState, World, Entity, Other);
    // NOTE(zoubir): a monster that walks into a fireball dies here, and
    // must not be put back into the chunks
    if (!Entity->IsPresent)
    {
        return false;
    }
    if (StopsOnCollision)
    {
        Entity->Velocity = Entity->Velocity -
            1.f * DotProduct(Entity->Velocity, Normal) * Normal;
        v3 DeltaLeft = *Delta - AllowedDelta;
        *Delta = DeltaLeft - 1.f * DotProduct(DeltaLeft, Normal) * Normal;
    }
    else
    {
        AddCollisionRule(AppState, Arena, Entity->ID, Other->ID, false);
        *Delta = *Delta - AllowedDelta;
    }
    return true;
}

// NOTE(zoubir): the height of the highest thing under the entity that it
// could stand on (for its shadow), or the floor
internal void
UpdateGroundZ(app_state *AppState, world *World, world_entity *Entity)
{
    float NearestDistance = 100000.f;
    entity_collision_volume *NearestVolume = 0;
    world_entity *NearestEntity = 0;
    // TODO(zoubir): Spatial partition here!
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Other = &World->Entities[EntityIndex];
        if (Other == Entity || !Other->IsPresent || IsDeadPlayer(Other) ||
            !CanCollide(AppState, Entity->Type, Other->Type) ||
            !CanCollide(AppState, Entity, Other))
        {
            continue;
        }
        for(u32 VolumeIndex = 0;
            VolumeIndex < Entity->Collision->VolumesCount;
            VolumeIndex++)
        {
            entity_collision_volume *Volume = &Entity->Collision->Volumes[VolumeIndex];
            rectangle2 EntityRect =
                RectCenterHalfDims(Entity->Position.XY + Volume->Offset.XY,
                                   Volume->HalfDims.XY);
            for(u32 OtherIndex = 0;
                OtherIndex < Other->Collision->VolumesCount;
                OtherIndex++)
            {
                entity_collision_volume *OtherVolume =
                    &Other->Collision->Volumes[OtherIndex];
                rectangle2 OtherRect =
                    RectCenterHalfDims(Other->Position.XY + OtherVolume->Offset.XY,
                                       OtherVolume->HalfDims.XY);
                if (RectanglesIntersect(EntityRect, OtherRect))
                {
                    float Distance = Entity->Position.Z -
                        (Other->Position.Z + OtherVolume->Offset.Z +
                         OtherVolume->HalfDims.Z);
                    Assert(Distance < 100000.f);
                    if (Distance > 0.f && Distance < NearestDistance)
                    {
                        NearestDistance = Distance;
                        NearestVolume = OtherVolume;
                        NearestEntity = Other;
                    }
                }
            }
        }
    }

    Entity->GroundZ = NearestVolume ?
        NearestEntity->Position.Z + NearestVolume->Offset.Z +
        NearestVolume->HalfDims.Z : 0.f;
}

// NOTE(zoubir): moves Entity under acceleration DDEntity for DeltaTime, at
// most *MaxDistance (which it uses up). Up to four sweeps: each gathers
// what is near the step, moves to the earliest hit, resolves it and
// carries the rest of the step on, then applies overlap effects. Last, the
// ground height under the entity is refreshed.
internal void
MoveEntity(world_entity *Entity, world *World,
           memory_arena *Arena,
           float DeltaTime, app_state *AppState,
           v3 DDEntity, float *MaxDistance)
{
    v3 Delta = 0.5f * DDEntity * Square(DeltaTime) + Entity->Velocity * DeltaTime;
    Entity->Velocity = DDEntity * DeltaTime + Entity->Velocity;

    world_entity *Nearby[MOVE_MAX_NEARBY];
    for(u32 Iteration = 0; Iteration < 4; Iteration++)
    {
        v3 From = Entity->Position;
        float Distance = Length(Delta);
        if (Distance <= 0.000001f)
        {
            break;
        }
        // NOTE(zoubir): the box is taken before the step is shortened to
        // MaxDistance, so it covers the full step
        rectangle3 Box = GetMoveBox(Entity, From, Delta);
        if (Distance > *MaxDistance)
        {
            Delta = (*MaxDistance / Distance) * Delta;
            *MaxDistance = 0.f;
        }
        else
        {
            *MaxDistance -= Distance;
        }

        u32 NearbyCount = GatherEntitiesInBox(World, Box, Nearby, MOVE_MAX_NEARBY);
        world_entity *Hit = 0;
        v3 Normal = {};
        float tMin = 1.f;
        for(u32 Index = 0; Index < NearbyCount; Index++)
        {
            world_entity *Other = Nearby[Index];
            if (CanSweepAgainst(AppState, Entity, Other) &&
                SweepAgainstEntity(Entity, From, Delta, Other, &tMin, &Normal))
            {
                Hit = Other;
            }
        }

        v3 AllowedDelta = tMin * Delta;
        Entity->Position = From + AllowedDelta;
        // NOTE(zoubir): cannot go lower than the ground level
        if (Entity->Position.Z < 0.f)
        {
            Entity->Position.Z = 0.f;
            Entity->Velocity.Z = 0.f;
        }
        // TODO(zoubir): once per MoveEntity call rather than every sweep
        CheckAndChangeEntityChunk(AppState, World, Arena, From, Entity);

        if (Hit && !ResolveMoveHit(AppState, World, Arena, Entity, Hit, Normal,
                                   AllowedDelta, &Delta))
        {
            return;
        }
        CheckOverlapsWith(AppState, World, Arena, Entity, Nearby, NearbyCount);
        if (!Hit)
        {
            break;
        }
    }

    UpdateGroundZ(AppState, World, Entity);
}
