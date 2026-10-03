/* Abilities that move the monster or turn it: burrowing and erupting,
   charging, and slow turning toward the nearest player. */

// NOTE(zoubir): while underground the landing spot tracks the target,
// until the lock
internal void
UpdateBurrow(world *World, world_entity *Entity, monster_ability *Ability)
{
    if (Entity->AbilityTimer <= BURROW_LOCK_SHARE * Ability->Active)
    {
        return;
    }
    if (Entity->AbilityTargetSlot < World->EntityCount)
    {
        world_entity *Target = &World->Entities[Entity->AbilityTargetSlot];
        if (Target->IsPresent && Target->Type == EntityType_Player &&
            Target->Hp > 0.f)
        {
            Entity->AbilityPoints[0] = Target->Position.XY;
        }
    }
}

// NOTE(zoubir): back to the surface on the landing spot, or as close to
// it as there is room, hitting everything around
internal void
EruptFromBurrow(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Entity, monster_ability *Ability)
{
    v2 Landing = Entity->AbilityPoints[0];
    v2 Offsets[] =
        {
            V2(0.f, 0.f), V2(24.f, 0.f), V2(-24.f, 0.f), V2(0.f, 24.f),
            V2(0.f, -24.f), V2(40.f, 30.f), V2(-40.f, -30.f),
        };
    for(u32 OffsetIndex = 0; OffsetIndex < ArrayCount(Offsets); OffsetIndex++)
    {
        v2 Spot = Landing + Offsets[OffsetIndex];
        v3 Spot3 = V3(Spot.X, Spot.Y, Entity->Position.Z);
        if (IsInsideArena(World, Spot, 40.f) &&
            IsSpawnSpotFree(AppState, World, Spot3, Entity->Collision))
        {
            v3 OldPosition = Entity->Position;
            Entity->Position = Spot3;
            CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Entity);
            break;
        }
    }
    Entity->Burrowed = false;
    Entity->Velocity = {};
    HurtPlayersInRadius(AppState, World, Entity, Entity->Position.XY, Ability);
}

// NOTE(zoubir): charges move the monster and hit whoever they reach
internal void
UpdateCharge(app_state *AppState, world *World, world_entity *Entity,
             monster_ability *Ability)
{
    if (Entity->AbilityHasHit)
    {
        return;
    }
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (Player->IsPresent && Player->Type == EntityType_Player &&
            Player->Hp > 0.f &&
            Length(Player->Position.XY - Entity->Position.XY) <=
            Ability->Radius)
        {
            HitPlayer(AppState, World, Player, Entity, Ability,
                      Ability->Knockback * Entity->AbilityAim);
            Entity->AbilityHasHit = true;
        }
    }
}

// NOTE(zoubir): turns Direction toward the nearest player at the kind's
// TurnRate, so slow turners can be flanked
internal void
UpdateMonsterFacing(world *World, world_entity *Entity, monster_def *Def,
                    float DeltaTime)
{
    if (LengthSq(Entity->Direction) < 0.0001f)
    {
        Entity->Direction = V2(1.f, 0.f);
    }
    world_entity *Target = FindMonsterTarget(World, Entity->Position.XY, 0);
    if (!Target)
    {
        return;
    }
    v2 Want = Target->Position.XY - Entity->Position.XY;
    float WantLength = Length(Want);
    if (WantLength <= 0.f)
    {
        return;
    }
    Want *= 1.f / WantLength;
    if (Def->TurnRate <= 0.f)
    {
        Entity->Direction = Want;
        return;
    }
    float Current = ATan2(Entity->Direction.Y, Entity->Direction.X);
    float Delta = ATan2(Want.Y, Want.X) - Current;
    while (Delta > Pi32)
    {
        Delta -= 2.f * Pi32;
    }
    while (Delta < -Pi32)
    {
        Delta += 2.f * Pi32;
    }
    float Step = Def->TurnRate * DeltaTime;
    if (Delta > Step)
    {
        Delta = Step;
    }
    else if (Delta < -Step)
    {
        Delta = -Step;
    }
    Entity->Direction = V2(Cos(Current + Delta), Sin(Current + Delta));
}
