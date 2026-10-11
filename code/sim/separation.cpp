/* Separation: the last step of SimulateTick. MoveEntity stops units from
   moving into each other, but it does nothing for two units that already
   overlap: a flyer's hover height is set directly, a summon can appear
   beside someone, a respawn can land next to a monster. Once inside each
   other they drift through freely. This step finds every player or
   monster that ends the tick overlapping something it collides with and
   pushes it out along the shortest way on the ground plane: the whole
   distance away from walls, trees and rocks, half each between two units
   (all of it on one unit when the other has a wall or another unit
   behind it, so a blink landing on a monster in a corner comes apart). On infinite maps walls
   and props are terrain, not entities, so the terrain stand-ins around
   each unit are checked too. */

#define SEPARATION_SLOP 0.01f

internal bool32
IsSeparatedUnit(world_entity *Entity)
{
    return Entity->IsPresent && Entity->Collision && !IsDeadPlayer(Entity) &&
           (Entity->Type == EntityType_Player || Entity->Type == EntityType_Monster);
}

// NOTE(zoubir): the push that moves A clear of B on the ground plane, or
// zero when they do not overlap (in all three axes, every box pair)
internal v2
SeparationPush(world_entity *A, world_entity *B)
{
    v2 Best = {};
    float BestLength = 0.f;
    for(u32 IndexA = 0; IndexA < A->Collision->VolumesCount; IndexA++)
    {
        for(u32 IndexB = 0; IndexB < B->Collision->VolumesCount; IndexB++)
        {
            entity_collision_volume *VA = &A->Collision->Volumes[IndexA];
            entity_collision_volume *VB = &B->Collision->Volumes[IndexB];
            v3 Delta = (A->Position + VA->Offset) - (B->Position + VB->Offset);
            v3 Overlap = VA->HalfDims + VB->HalfDims;
            Overlap.X -= Absolute(Delta.X);
            Overlap.Y -= Absolute(Delta.Y);
            Overlap.Z -= Absolute(Delta.Z);
            if (Overlap.X <= 0.f || Overlap.Y <= 0.f || Overlap.Z <= 0.f)
            {
                continue;
            }

            v2 Push = {};
            if (Overlap.X < Overlap.Y)
            {
                Push.X = (Delta.X >= 0.f ? 1.f : -1.f) * (Overlap.X + SEPARATION_SLOP);
            }
            else
            {
                Push.Y = (Delta.Y >= 0.f ? 1.f : -1.f) * (Overlap.Y + SEPARATION_SLOP);
            }
            float PushLength = Length(Push);
            if (PushLength > BestLength)
            {
                Best = Push;
                BestLength = PushLength;
            }
        }
    }
    return Best;
}

internal void
NudgeEntity(app_state *AppState, world *World, memory_arena *Arena,
            world_entity *Entity, v2 Push)
{
    v3 OldPosition = Entity->Position;
    Entity->Position.X += Push.X;
    Entity->Position.Y += Push.Y;
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
}

// NOTE(zoubir): whether a wall, tree or rock blocks Entity at all; a
// unit that can pass through them is never pinned against one
internal bool32
IsSolidFor(app_state *AppState, world_entity *Entity, world_entity *Other)
{
    return Other != Entity && Other->IsPresent && Other->Collision &&
           (Other->Type == EntityType_StaticObject ||
            Other->Type == EntityType_Tiled) &&
           CanCollide(AppState, Entity->Type, Other->Type) &&
           CanCollide(AppState, Entity, Other);
}

// NOTE(zoubir): whether moving Entity by Push would leave it inside a
// wall, tree or rock (or a terrain stand-in on an infinite map), or
// inside a unit other than Ignore: a unit with another unit behind it
// that has a wall behind it cannot give way either
internal bool32
PushIsBlocked(app_state *AppState, world *World, world_entity *Entity,
              v2 Push, world_entity *Ignore)
{
    v3 OldPosition = Entity->Position;
    Entity->Position.X += Push.X;
    Entity->Position.Y += Push.Y;
    entity_collision_volume *Total = &Entity->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Entity->Position + Total->Offset,
                                        Total->HalfDims);
    bool32 Blocked = false;
    world_entity *Nearby[MOVE_MAX_NEARBY];
    u32 Count = GatherEntitiesInBox(World, Box, Nearby, ArrayCount(Nearby));
    for(u32 Index = 0; Index < Count && !Blocked; Index++)
    {
        world_entity *Other = Nearby[Index];
        bool32 Unit = Other != Entity && Other != Ignore &&
            IsSeparatedUnit(Other) &&
            CanCollide(AppState, Entity->Type, Other->Type) &&
            CanCollide(AppState, Entity, Other);
        v2 Out = (Unit || IsSolidFor(AppState, Entity, Other)) ?
            SeparationPush(Entity, Other) : V2(0.f, 0.f);
        Blocked = (Out.X != 0.f || Out.Y != 0.f);
    }
    world_entity *Terrain[64];
    Count = 0;
    if (!Blocked && CanCollide(AppState, Entity->Type, EntityType_StaticObject))
    {
        Count = GatherTerrainColliders(World, Box, Terrain, 0,
                                       ArrayCount(Terrain));
    }
    for(u32 Index = 0; Index < Count && !Blocked; Index++)
    {
        v2 Out = SeparationPush(Entity, Terrain[Index]);
        Blocked = (Out.X != 0.f || Out.Y != 0.f);
    }
    Entity->Position = OldPosition;
    return Blocked;
}

// NOTE(zoubir): pushes Entity out of every wall, tree and rock it is in;
// returns how many pushes it made
internal u32
PushOutOfSolids(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Entity)
{
    u32 Pushes = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Other = &World->Entities[Index];
        if (!IsSolidFor(AppState, Entity, Other))
        {
            continue;
        }
        v2 Push = SeparationPush(Entity, Other);
        if (Push.X != 0.f || Push.Y != 0.f)
        {
            NudgeEntity(AppState, World, Arena, Entity, Push);
            Pushes++;
        }
    }

    if (CanCollide(AppState, Entity->Type, EntityType_StaticObject))
    {
        entity_collision_volume *Total = &Entity->Collision->TotalVolume;
        rectangle3 Box = RectCenterHalfDims(Entity->Position + Total->Offset,
                                            Total->HalfDims);
        world_entity *Terrain[64];
        u32 Count = GatherTerrainColliders(World, Box, Terrain, 0,
                                           ArrayCount(Terrain));
        for(u32 Index = 0; Index < Count; Index++)
        {
            v2 Push = SeparationPush(Entity, Terrain[Index]);
            if (Push.X != 0.f || Push.Y != 0.f)
            {
                NudgeEntity(AppState, World, Arena, Entity, Push);
                Pushes++;
            }
        }
    }
    return Pushes;
}

// NOTE(zoubir): one pass over every unit; returns how many pushes it made.
// Each unit leaves walls first, then the units after it. Two units share
// a push half each, unless one has a wall, tree, rock or third unit right
// behind it: then the other takes all of it. Split evenly, a blink
// landing on a monster against a wall pushed the monster into the wall,
// the wall pushed it back into the player, and four passes ended with one
// of them still inside the other or the wall.
internal u32
SeparationPass(app_state *AppState, world *World, memory_arena *Arena)
{
    u32 Pushes = 0;
    for(u32 IndexA = 0; IndexA < World->EntityCount; IndexA++)
    {
        world_entity *A = &World->Entities[IndexA];
        if (!IsSeparatedUnit(A))
        {
            continue;
        }
        Pushes += PushOutOfSolids(AppState, World, Arena, A);

        // NOTE(zoubir): each unit pair once, from its lower index
        for(u32 IndexB = IndexA + 1; IndexB < World->EntityCount; IndexB++)
        {
            world_entity *B = &World->Entities[IndexB];
            if (!IsSeparatedUnit(B) ||
                !CanCollide(AppState, A->Type, B->Type) ||
                !CanCollide(AppState, A, B))
            {
                continue;
            }

            v2 Push = SeparationPush(A, B);
            if (Push.X == 0.f && Push.Y == 0.f)
            {
                continue;
            }
            v2 PushA = 0.5f * Push;
            v2 PushB = -0.5f * Push;
            if (PushIsBlocked(AppState, World, A, PushA, B) &&
                !PushIsBlocked(AppState, World, B, -Push, A))
            {
                PushA = V2(0.f, 0.f);
                PushB = -Push;
            }
            else if (PushIsBlocked(AppState, World, B, PushB, A) &&
                     !PushIsBlocked(AppState, World, A, Push, B))
            {
                PushA = Push;
                PushB = V2(0.f, 0.f);
            }
            NudgeEntity(AppState, World, Arena, A, PushA);
            NudgeEntity(AppState, World, Arena, B, PushB);
            Pushes++;
        }
    }
    return Pushes;
}

// NOTE(zoubir): in a crowd, pushing one unit out of a second can push it
// into a third, so repeat until a pass moves nothing (a few at most)
internal void
SeparateOverlappingUnits(app_state *AppState, world *World,
                         memory_arena *Arena)
{
    for(u32 Pass = 0; Pass < 4; Pass++)
    {
        if (SeparationPass(AppState, World, Arena) == 0)
        {
            break;
        }
    }
}
