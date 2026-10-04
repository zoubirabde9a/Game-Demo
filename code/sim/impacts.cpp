/* Impacts: a stunned unit thrown fast into something (by Push, Launch or
   another impact) hits it. MoveEntity calls ImpactOnHit for every
   blocking hit, before the move slides along what it hit.

   - Into another unit: most of the speed passes on to it, it is stunned
     for a moment, and both take a little damage. A pushed crowd knocks
     itself apart, each body bowling into the next with less speed.
   - Into a wall, tree or rock: a slam. Damage, a longer stun, and the
     body bounces back off instead of sliding along.

   Walking, dashing and anything not stunned never cause impacts, so a
   player running into a wall is never hurt by it. A player who walks or
   dashes into another unit shoulders it instead: the unit takes on part
   of the speed the player had into it and moves out of the way on its
   own move, so a crowd can slow a player down but never pin them (a
   ring of bodies against a rock used to hold a player for good). The damage is credited
   to the player who threw the body (ThrownBySlot), and a struck unit
   counts as thrown by them too, so a kill down the chain is theirs. */

// NOTE(zoubir): speed into the surface below which nothing happens; a
// Push throws at 750, a walk is under 100
#define IMPACT_MIN_SPEED 260.f
// NOTE(zoubir): share of the speed into the other unit that it takes on
#define IMPACT_TRANSFER 0.65f
#define IMPACT_UNIT_DAMAGE 6.f
#define IMPACT_UNIT_STUN 0.5f
#define IMPACT_WALL_DAMAGE 12.f
#define IMPACT_WALL_STUN 0.9f
// NOTE(zoubir): share of the speed into a wall that bounces back
#define IMPACT_WALL_BOUNCE 0.35f
// NOTE(zoubir): share of a player's speed into a unit it walks into that
// the unit takes on
#define SHOULDER_SHARE 0.6f

// NOTE(zoubir): the player whose throw this is, or 0
inline world_entity *
GetThrower(app_state *AppState, world_entity *Entity)
{
    world_entity *Result = 0;
    u32 Slot = Entity->ThrownBySlot;
    if (Slot > 0 && Slot <= MAX_PLAYERS && AppState->Players[Slot - 1].Active)
    {
        Result = AppState->Players[Slot - 1].Entity;
    }
    return Result;
}

inline bool32
IsThrownUnit(world_entity *Entity)
{
    bool32 Result = IsWalkingUnit(Entity) &&
        HasStatus(Entity, StatusEffect_Stunned);
    return Result;
}

// NOTE(zoubir): Entity moving with its velocity hit Other through the face
// with Normal (pointing back at Entity). Returns the velocity Entity should
// keep along Normal instead of stopping dead (a bounce), usually 0.
internal float
ImpactOnHit(app_state *AppState, world *World, world_entity *Entity,
            world_entity *Other, v3 Normal)
{
    float Into = -DotProduct(Entity->Velocity.XY, Normal.XY);
    if (Normal.Z != 0.f || Into <= 0.f)
    {
        return 0.f;
    }
    if (Into < IMPACT_MIN_SPEED || !IsThrownUnit(Entity))
    {
        if (Entity->Type == EntityType_Player && IsWalkingUnit(Other) &&
            !IsDodging(Other))
        {
            Other->Velocity.XY -= (SHOULDER_SHARE * Into) * Normal.XY;
        }
        return 0.f;
    }

    // NOTE(zoubir): a small burst where they met
    v3 Contact = Entity->Position;
    Contact.XY -= 12.f * Normal.XY;
    Contact.Z += 16.f;
    EmitBurst(&AppState->Events, SimBurst_Impact, SIM_NOBODY, Contact);
    EmitSound(&AppState->Events, AssetType_Dash, Contact);

    float Bounce = 0.f;
    world_entity *Thrower = GetThrower(AppState, Entity);
    if (IsWalkingUnit(Other))
    {
        if (!IsDodging(Other))
        {
            Other->Velocity.XY -= (IMPACT_TRANSFER * Into) * Normal.XY;
            ApplyStatus(Other, StatusEffect_Stunned, IMPACT_UNIT_STUN);
            Other->ThrownBySlot = Entity->ThrownBySlot;
            DamageEntity(AppState, World, Other, IMPACT_UNIT_DAMAGE, Thrower);
        }
        DamageEntity(AppState, World, Entity, IMPACT_UNIT_DAMAGE, Thrower);
    }
    else
    {
        ApplyStatus(Entity, StatusEffect_Stunned, IMPACT_WALL_STUN);
        DamageEntity(AppState, World, Entity, IMPACT_WALL_DAMAGE, Thrower);
        Bounce = IMPACT_WALL_BOUNCE * Into;
    }
    return Bounce;
}
