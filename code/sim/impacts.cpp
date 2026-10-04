/* Impacts: a stunned unit thrown fast into something (by Push, Launch or
   another impact) hits it. MoveEntity calls ImpactOnHit for every
   blocking hit, before the move slides along what it hit.

   - Into another unit: most of the speed passes on to it, it is stunned
     for a moment, and both take a little damage. A pushed crowd knocks
     itself apart, each body bowling into the next with less speed.
   - Into a wall, tree or rock: a slam. Damage, a longer stun, and the
     body bounces back off instead of sliding along.

   Walking, dashing and anything not stunned never cause impacts, so a
   player running into a wall is never hurt by it. */

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
    if (Normal.Z != 0.f || Into < IMPACT_MIN_SPEED || !IsThrownUnit(Entity))
    {
        return 0.f;
    }

    // NOTE(zoubir): a small burst where they met
    v3 Contact = Entity->Position;
    Contact.XY -= 12.f * Normal.XY;
    Contact.Z += 16.f;
    EmitBurst(&AppState->Events, SimBurst_Impact, SIM_NOBODY, Contact);
    EmitSound(&AppState->Events, AssetType_Dash, Contact);

    float Bounce = 0.f;
    if (IsWalkingUnit(Other))
    {
        if (!IsDodging(Other))
        {
            Other->Velocity.XY -= (IMPACT_TRANSFER * Into) * Normal.XY;
            ApplyStatus(Other, StatusEffect_Stunned, IMPACT_UNIT_STUN);
            DamageEntity(AppState, World, Other, IMPACT_UNIT_DAMAGE, 0);
        }
        DamageEntity(AppState, World, Entity, IMPACT_UNIT_DAMAGE, 0);
    }
    else
    {
        ApplyStatus(Entity, StatusEffect_Stunned, IMPACT_WALL_STUN);
        DamageEntity(AppState, World, Entity, IMPACT_WALL_DAMAGE, 0);
        Bounce = IMPACT_WALL_BOUNCE * Into;
    }
    return Bounce;
}
