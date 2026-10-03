/* Separation: the last step of SimulateTick. MoveEntity stops units from
   moving into each other, but it does nothing for two units that already
   overlap: a flyer's hover height is set directly, a summon can appear
   beside someone, a respawn can land next to a monster. Once inside each
   other they drift through freely. This step finds every player or
   monster that ends the tick overlapping something it collides with and
   pushes it out along the shortest way on the ground plane: the whole
   distance away from walls, trees and rocks, half each between two units. */

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

// NOTE(zoubir): one pass over every unit; returns how many pushes it made
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

        for(u32 IndexB = 0; IndexB < World->EntityCount; IndexB++)
        {
            world_entity *B = &World->Entities[IndexB];
            if (B == A || !B->IsPresent || !B->Collision || IsDeadPlayer(B))
            {
                continue;
            }
            bool32 Solid = (B->Type == EntityType_StaticObject ||
                            B->Type == EntityType_Tiled);
            // NOTE(zoubir): each unit pair once, from its lower index
            bool32 OtherUnit = IsSeparatedUnit(B) && IndexB > IndexA;
            if (!Solid && !OtherUnit)
            {
                continue;
            }
            if (!CanCollide(AppState, A->Type, B->Type) ||
                !CanCollide(AppState, A, B))
            {
                continue;
            }

            v2 Push = SeparationPush(A, B);
            if (Push.X == 0.f && Push.Y == 0.f)
            {
                continue;
            }
            if (Solid)
            {
                NudgeEntity(AppState, World, Arena, A, Push);
            }
            else
            {
                NudgeEntity(AppState, World, Arena, A, 0.5f * Push);
                NudgeEntity(AppState, World, Arena, B, -0.5f * Push);
            }
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
