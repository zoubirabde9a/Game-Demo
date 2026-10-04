/* Status effects: timed conditions on any entity. Each entity keeps one
   timer per effect (StatusTimers); a new application keeps whichever lasts
   longer, so effects refresh instead of stacking. Damage over time lands
   in ticks of STATUS_TICK_SECONDS so health drops in readable steps.

   Burning   fast damage, short
   Poisoned  slow damage, long
   Slowed    movement acceleration scaled by STATUS_SLOW_SCALE
   Stunned   no moving, attacking or casting; physics still applies, so a
             stunned unit thrown into the air falls back down */

#define STATUS_TICK_SECONDS 0.5f
#define STATUS_BURN_DPS 8.f
#define STATUS_POISON_DPS 4.f
#define STATUS_SLOW_SCALE 0.45f

inline void
ApplyStatus(world_entity *Entity, status_effect Effect, float Seconds)
{
    if (Effect > StatusEffect_None && Effect < StatusEffect_Count)
    {
        Entity->StatusTimers[Effect] = Maximum(Entity->StatusTimers[Effect],
                                               Seconds);
    }
}

inline bool32
HasStatus(world_entity *Entity, status_effect Effect)
{
    bool32 Result = Entity->StatusTimers[Effect] > 0.f;
    return Result;
}

// NOTE(zoubir): multiply movement acceleration by this; covers slows and
// elite speed
inline float
GetMoveSpeedScale(world_entity *Entity)
{
    float Result = HasStatus(Entity, StatusEffect_Slowed) ?
        STATUS_SLOW_SCALE : 1.f;
    Result *= GetAffixSpeedScale(Entity);
    if (Entity->Type == EntityType_Monster && Entity->PhaseSpeedScale > 0.f)
    {
        Result *= Entity->PhaseSpeedScale;
    }
    if (Entity->GroundSpeedScale > 0.f)
    {
        Result *= Entity->GroundSpeedScale;
    }
    return Result;
}

// NOTE(zoubir): multiplies the drag that slows a unit down; under 1 on ice
inline float
GetGroundFriction(world_entity *Entity)
{
    float Result = Entity->GroundFriction > 0.f ? Entity->GroundFriction : 1.f;
    return Result;
}


inline float
StatusDamagePerSecond(world_entity *Entity)
{
    float Result = 0.f;
    if (HasStatus(Entity, StatusEffect_Burning))
    {
        Result += STATUS_BURN_DPS;
    }
    if (HasStatus(Entity, StatusEffect_Poisoned))
    {
        Result += STATUS_POISON_DPS;
    }
    return Result;
}

// NOTE(zoubir): once per tick, after every entity has moved
internal void
UpdateStatusEffects(app_state *AppState, world *World, float DeltaTime)
{
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Hp <= 0.f)
        {
            continue;
        }

        float DamagePerSecond = StatusDamagePerSecond(Entity);
        if (DamagePerSecond > 0.f)
        {
            Entity->StatusTickTimer += DeltaTime;
            while (Entity->StatusTickTimer >= STATUS_TICK_SECONDS &&
                   Entity->IsPresent)
            {
                Entity->StatusTickTimer -= STATUS_TICK_SECONDS;
                DamageEntity(AppState, World, Entity,
                             DamagePerSecond * STATUS_TICK_SECONDS, 0);
            }
        }
        else
        {
            Entity->StatusTickTimer = 0.f;
        }

        for(u32 Effect = 0; Effect < StatusEffect_Count; Effect++)
        {
            Entity->StatusTimers[Effect] =
                Maximum(0.f, Entity->StatusTimers[Effect] - DeltaTime);
        }
    }
}
