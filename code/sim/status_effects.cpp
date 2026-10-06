/* Status effects: timed conditions on any entity, one row each in
   StatusTable. Each entity keeps one timer per effect (StatusTimers); a
   new application keeps whichever lasts longer, so effects refresh
   instead of stacking. Damage and healing over time land in ticks of
   STATUS_TICK_SECONDS so health moves in readable steps.

   A row says what the effect does while it runs: health per second (a
   negative number heals), more of it while the unit moves (a bleed), a
   movement scale, whether it roots the unit (no walking) or disables it
   (no walking, attacking or casting), whether it kills when it runs out
   (a fall), which other effect it washes off and keeps off, and how long
   the unit shrugs it off afterwards. While a unit shrugs an effect off,
   its timer runs below zero back up to zero and new applications of it
   miss; HasStatus only counts time above zero.

   Burning       fire damage; water (Soaked) puts it out
   Poisoned      slow damage, long; a spring (Regenerating) washes it out
   Slowed        movement scaled by STATUS_SLOW_SCALE; Hasted lifts it
   Stunned       no moving, attacking or casting; physics still applies,
                 so a stunned unit thrown into the air falls back down,
                 keeping most of its speed until it lands
   Bleeding      light damage standing, heavy while moving
   Regenerating  healing over time
   Hasted        moves faster
   Rooted        cannot walk but can still fight; shrugged off for a while
                 after it ends, so thorns grab once, not forever
   Soaked        a little slower; no burning while it lasts
   Falling       down a pit: no control, and dead when it runs out */

#define STATUS_TICK_SECONDS 0.5f
#define STATUS_BURN_DPS 8.f
#define STATUS_POISON_DPS 4.f
#define STATUS_SLOW_SCALE 0.45f

// NOTE(zoubir): status_def Flags
#define STATUS_ROOTS 0x1
#define STATUS_DISABLES 0x2
#define STATUS_FATAL 0x4

struct status_def
{
    char *Name;
    // NOTE(zoubir): health lost per second; below 0 heals
    float DamagePerSecond;
    // NOTE(zoubir): added to DamagePerSecond while the unit moves itself
    float MovingDamagePerSecond;
    // NOTE(zoubir): multiplies movement acceleration
    float MoveScale;
    u32 Flags;
    // NOTE(zoubir): removed when this lands, and cannot land while this runs
    status_effect Cures;
    // NOTE(zoubir): seconds the unit shrugs this effect off once it ends
    float ImmuneSeconds;
    // NOTE(zoubir): its pip over the unit's head (art/monster_render.cpp);
    // 0 for effects with a look of their own
    u32 Color;
};

global_variable status_def StatusTable[StatusEffect_Count] =
{
    //               Dps                Moving Move               Flags                                      Cures                    Immune Color
    {"",             0.f,               0.f,   1.f,               0,                                         StatusEffect_None,       0.f,   0},
    {"Burning",      STATUS_BURN_DPS,   0.f,   1.f,               0,                                         StatusEffect_None,       0.f,   0xFF2080FF},
    {"Poisoned",     STATUS_POISON_DPS, 0.f,   1.f,               0,                                         StatusEffect_None,       0.f,   0xFF30D060},
    {"Slowed",       0.f,               0.f,   STATUS_SLOW_SCALE, 0,                                         StatusEffect_None,       0.f,   0xFFFFD090},
    {"Stunned",      0.f,               0.f,   1.f,               STATUS_ROOTS|STATUS_DISABLES,              StatusEffect_None,       0.f,   0},
    {"Bleeding",     2.f,               6.f,   1.f,               0,                                         StatusEffect_None,       0.f,   0xFF2020D0},
    {"Regenerating", -8.f,              0.f,   1.f,               0,                                         StatusEffect_Poisoned,   0.f,   0xFFA0FFA0},
    {"Hasted",       0.f,               0.f,   1.5f,              0,                                         StatusEffect_Slowed,     0.f,   0xFFFFFF60},
    {"Rooted",       0.f,               0.f,   1.f,               STATUS_ROOTS,                              StatusEffect_None,       2.5f,  0xFF3070A0},
    {"Soaked",       0.f,               0.f,   0.85f,             0,                                         StatusEffect_Burning,    0.f,   0xFFE08040},
    {"Falling",      0.f,               0.f,   1.f,               STATUS_ROOTS|STATUS_DISABLES|STATUS_FATAL, StatusEffect_None,       0.f,   0},
};

inline status_def *
GetStatusDef(status_effect Effect)
{
    Assert(Effect < StatusEffect_Count);
    status_def *Result = &StatusTable[Effect];
    return Result;
}

inline bool32
HasStatus(world_entity *Entity, status_effect Effect)
{
    bool32 Result = Entity->StatusTimers[Effect] > 0.f;
    return Result;
}

inline void
ApplyStatus(world_entity *Entity, status_effect Effect, float Seconds)
{
    if (Effect <= StatusEffect_None || Effect >= StatusEffect_Count ||
        Entity->StatusTimers[Effect] < 0.f)
    {
        return;
    }
    // NOTE(zoubir): a cure works both ways: water puts out a fire, and
    // nothing catches fire while wet
    for(u32 Other = 1; Other < StatusEffect_Count; Other++)
    {
        if (StatusTable[Other].Cures == Effect &&
            HasStatus(Entity, (status_effect)Other))
        {
            return;
        }
    }
    status_effect Cures = StatusTable[Effect].Cures;
    if (Cures != StatusEffect_None && Entity->StatusTimers[Cures] > 0.f)
    {
        Entity->StatusTimers[Cures] = 0.f;
    }
    Entity->StatusTimers[Effect] = Maximum(Entity->StatusTimers[Effect], Seconds);
}

// NOTE(zoubir): every running effect's Flags together
inline u32
StatusFlags(world_entity *Entity)
{
    u32 Result = 0;
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        if (Entity->StatusTimers[Effect] > 0.f)
        {
            Result |= StatusTable[Effect].Flags;
        }
    }
    return Result;
}

// NOTE(zoubir): no walking (stunned, rooted, falling)
inline bool32
IsRooted(world_entity *Entity)
{
    bool32 Result = (StatusFlags(Entity) & STATUS_ROOTS) != 0;
    return Result;
}

// NOTE(zoubir): no walking, attacking or casting (stunned, falling)
inline bool32
IsDisabled(world_entity *Entity)
{
    bool32 Result = (StatusFlags(Entity) & STATUS_DISABLES) != 0;
    return Result;
}

// NOTE(zoubir): multiply movement acceleration by this; covers status
// effects, elite speed, enrage phases and the ground underfoot
inline float
GetMoveSpeedScale(world_entity *Entity)
{
    float Result = 1.f;
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        if (Entity->StatusTimers[Effect] > 0.f)
        {
            Result *= StatusTable[Effect].MoveScale;
        }
    }
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

// NOTE(zoubir): share of the drag a stunned monster keeps while it is in
// the air, so a throw carries until it lands and ground friction stops it
#define THROWN_AIR_FRICTION 0.3f

// NOTE(zoubir): multiplies the drag that slows a unit down; under 1 on ice
// and for a thrown monster in the air
inline float
GetGroundFriction(world_entity *Entity)
{
    float Result = Entity->GroundFriction > 0.f ? Entity->GroundFriction : 1.f;
    if (Entity->Type == EntityType_Monster &&
        HasStatus(Entity, StatusEffect_Stunned) &&
        Entity->Position.Z > Entity->GroundZ + 2.f)
    {
        Result *= THROWN_AIR_FRICTION;
    }
    return Result;
}

// NOTE(zoubir): a unit walking faster than this counts as moving, for
// MovingDamagePerSecond
#define STATUS_MOVING_SPEED 40.f

// NOTE(zoubir): health per second from every running effect, the losses
// and the gains apart, so poison and healing both show
inline void
StatusHealthPerSecond(world_entity *Entity, float *Damage, float *Healing)
{
    *Damage = 0.f;
    *Healing = 0.f;
    bool32 Moving = LengthSq(Entity->Velocity.XY) >
        STATUS_MOVING_SPEED * STATUS_MOVING_SPEED;
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        if (Entity->StatusTimers[Effect] <= 0.f)
        {
            continue;
        }
        status_def *Def = &StatusTable[Effect];
        float PerSecond = Def->DamagePerSecond +
            (Moving ? Def->MovingDamagePerSecond : 0.f);
        if (PerSecond > 0.f)
        {
            *Damage += PerSecond;
        }
        else
        {
            *Healing -= PerSecond;
        }
    }
}

// NOTE(zoubir): kept for the tests and the HUD: the damage half
inline float
StatusDamagePerSecond(world_entity *Entity)
{
    float Damage, Healing;
    StatusHealthPerSecond(Entity, &Damage, &Healing);
    return Damage;
}

// NOTE(zoubir): the player slot + 1 behind what happens to Entity now that
// nobody strikes directly (a fall into a pit): whoever threw it, while
// the throw lasts, or hit it a moment ago; 0 for nobody
inline u32
StatusBlameSlot(world_entity *Entity)
{
    u32 Result = 0;
    if (HasStatus(Entity, StatusEffect_Stunned))
    {
        Result = Entity->ThrownBySlot;
    }
    if (!Result && Entity->HitFresh > 0.f)
    {
        Result = Entity->HitBySlot;
    }
    return Result;
}

// NOTE(zoubir): the player entity behind a fall (ThrownBySlot, set when
// it began, sim/terrain_effects.cpp), or 0
internal world_entity *
StatusKiller(app_state *AppState, world_entity *Entity)
{
    u32 Slot = Entity->ThrownBySlot;
    world_entity *Result = 0;
    if (Slot > 0 && Slot <= MAX_PLAYERS && AppState->Players[Slot - 1].Active &&
        AppState->Players[Slot - 1].Entity != Entity)
    {
        Result = AppState->Players[Slot - 1].Entity;
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

        float Damage, Healing;
        StatusHealthPerSecond(Entity, &Damage, &Healing);
        if (Damage > 0.f || Healing > 0.f)
        {
            Entity->StatusTickTimer += DeltaTime;
            while (Entity->StatusTickTimer >= STATUS_TICK_SECONDS &&
                   Entity->IsPresent && Entity->Hp > 0.f)
            {
                Entity->StatusTickTimer -= STATUS_TICK_SECONDS;
                if (Healing > 0.f && Entity->MaxHp > 0.f)
                {
                    Entity->Hp = Minimum(Entity->MaxHp,
                                         Entity->Hp + Healing * STATUS_TICK_SECONDS);
                }
                if (Damage > 0.f)
                {
                    DamageEntity(AppState, World, Entity,
                                 Damage * STATUS_TICK_SECONDS, 0);
                }
            }
        }
        else
        {
            Entity->StatusTickTimer = 0.f;
        }

        for(u32 Effect = 1; Effect < StatusEffect_Count && Entity->IsPresent; Effect++)
        {
            float *Timer = &Entity->StatusTimers[Effect];
            if (*Timer < 0.f)
            {
                *Timer = Minimum(0.f, *Timer + DeltaTime);
            }
            else if (*Timer > 0.f)
            {
                *Timer -= DeltaTime;
                if (*Timer <= 0.f)
                {
                    status_def *Def = &StatusTable[Effect];
                    *Timer = -Def->ImmuneSeconds;
                    if ((Def->Flags & STATUS_FATAL) && Entity->Hp > 0.f)
                    {
                        KillEntity(AppState, World, Entity,
                                   StatusKiller(AppState, Entity));
                    }
                }
            }
        }
    }
}
