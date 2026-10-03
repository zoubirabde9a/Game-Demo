/* Corner slip: a walking unit that clips the edge of a wall or tree by a
   few units is pushed sideways past it instead of stopping dead. After a
   blocking hit, MoveEntity hands the blocked part of the move here; part
   of it becomes a sideways step away from the obstacle's middle. The step
   is swept like any other, so it never goes through anything. Units do
   not slip around each other: crowds push apart in separation.cpp. */

// NOTE(zoubir): the most a unit may overlap an obstacle's edge, along the
// face it hit, and still slip; capped at half the unit's own width so a
// unit square on to a wall still stops
#define CORNER_SLIP_MAX 10.f

inline bool32
IsWalkingUnit(world_entity *Entity)
{
    bool32 Result = (Entity->Type == EntityType_Player ||
                     Entity->Type == EntityType_Monster ||
                     Entity->Type == EntityType_Familiar);
    return Result;
}

// NOTE(zoubir): Blocked is what was left of the move when it hit Other,
// Normal the face it hit; *Delta is the rest of the move after sliding
internal void
SlipPastCorner(world_entity *Entity, world_entity *Other, v3 Normal,
               v3 Blocked, v3 *Delta)
{
    if (Normal.Z != 0.f || !IsWalkingUnit(Entity) || IsWalkingUnit(Other))
    {
        return;
    }
    // NOTE(zoubir): the axis along the face that was hit
    u32 Axis = (Normal.X != 0.f) ? 1 : 0;
    entity_collision_volume *Mine = &Entity->Collision->TotalVolume;
    entity_collision_volume *Theirs = &Other->Collision->TotalVolume;
    v3 Rel = (Entity->Position + Mine->Offset) -
        (Other->Position + Theirs->Offset);
    float Overlap = Mine->HalfDims.Data[Axis] + Theirs->HalfDims.Data[Axis] -
        Absolute(Rel.Data[Axis]);
    float Limit = Minimum(CORNER_SLIP_MAX, Mine->HalfDims.Data[Axis]);
    float Pushing = -DotProduct(Blocked, Normal);
    if (Overlap > 0.f && Overlap <= Limit && Pushing > 0.f)
    {
        float Side = (Rel.Data[Axis] >= 0.f) ? 1.f : -1.f;
        Delta->Data[Axis] += Side * Minimum(Pushing, Overlap + 0.05f);
    }
}
