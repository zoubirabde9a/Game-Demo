/* Impacts: a stunned unit thrown fast into something (by Push, Launch or
   another impact) hits it. MoveEntity calls ImpactOnHit for every
   blocking hit, before the move slides along what it hit.

   - Into another unit: some of the speed passes on to it, more the
     heavier the thrown body is than the one it hits (hit.cpp
     KnockbackScale), so a bat thrown into a brute barely moves it and a
     brute thrown into bats scatters them. The struck unit is stunned for
     a moment and both take a little damage. A pushed crowd knocks itself
     apart, each body bowling into the next with less speed.
   - Into a wall, tree or rock: a slam. Damage, a longer stun, and the
     body bounces back off instead of sliding along.

   A body knocked back but not stunned (a monster hit in the last
   HIT_FRESH_SECONDS, a player still staggering from a shove) bumps
   instead, once it moves BUMP_MIN_SPEED or faster: it bounces off walls
   and passes part of its speed to a unit it runs into, which is knocked
   in turn, so a cut or a fireball sends a monster bowling into the one
   behind it. A bump does no damage and no stun.

   Walking and dashing never cause impacts, so a player running into a
   wall is never hurt by it. A player who walks or
   dashes into another unit shoulders it instead: the unit takes on part
   of the speed the player had into it and moves out of the way on its
   own move, so a crowd can slow a player down but never pin them (a
   ring of bodies against a rock used to hold a player for good). The damage is credited
   to the player who threw the body (ThrownBySlot), and a struck unit
   counts as thrown by them too, so a kill down the chain is theirs. */

// NOTE(zoubir): speed into the surface below which nothing happens; a
// Push throws at 900, a run is 260
#define IMPACT_MIN_SPEED 260.f
// NOTE(zoubir): share of the speed into the other unit that it takes on
// when both weigh the same. It scales with the thrown body's weight over
// the struck one's, up to IMPACT_MAX_TRANSFER: a bat into the lightest
// kinds 0.65, into a brute 0.43, into the warlord 0.26; a brute into a bat
// 0.99, the warlord into anything light 1
#define IMPACT_TRANSFER 0.65f
#define IMPACT_MAX_TRANSFER 1.f
#define IMPACT_UNIT_DAMAGE 6.f
#define IMPACT_UNIT_STUN 0.5f
#define IMPACT_WALL_DAMAGE 12.f
#define IMPACT_WALL_STUN 0.9f
// NOTE(zoubir): share of the speed into a wall that bounces back
#define IMPACT_WALL_BOUNCE 0.55f
// NOTE(zoubir): share of a player's speed into a unit it walks into that
// the unit takes on, counting that speed up to SHOULDER_MAX_SPEED at most:
// a blink moves at its whole distance in one tick, 19200 a second, and
// shouldered what it met across the map. The cap is half a dash's speed,
// where it was before the player got faster, so a dash shoves monsters no
// harder than it did
#define SHOULDER_SHARE 0.6f
#define SHOULDER_MAX_SPEED 650.f
// NOTE(zoubir): a knocked body bumps from this speed into the surface (a
// sword cut shoves at 280, a fireball at 200, a run is 260 but a runner
// is not knocked); share of that speed a wall gives back, and share a
// unit of the same weight takes on, scaled by weight like an impact's
#define BUMP_MIN_SPEED 160.f
#define BUMP_WALL_BOUNCE 0.5f
#define BUMP_TRANSFER 0.6f
// NOTE(zoubir): a dashing player hits what it runs into this fast or
// faster (a dash leaves at 1440, a run is 260). The dash slows below it
// about 0.09 s in, so only the dash's first stretch strikes
#define DASH_STRIKE_SPEED (300.f * PLAYER_MOVE_SCALE)

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

// NOTE(zoubir): a dashing player running into a unit
// (player_abilities/movement_abilities.cpp, included later)
internal void DashStrike(app_state *AppState, world *World, world_entity *Player,
                         world_entity *Target, v2 Away);

inline bool32
IsThrownUnit(world_entity *Entity)
{
    bool32 Result = IsWalkingUnit(Entity) &&
        HasStatus(Entity, StatusEffect_Stunned);
    return Result;
}

// NOTE(zoubir): knocked back by a hit and not yet walking again. A player
// counts by its stagger, which snapshots carry, so its own prediction
// bounces where the server does
inline bool32
IsKnockedUnit(world_entity *Entity)
{
    bool32 Result = IsWalkingUnit(Entity) &&
        (Entity->Type == EntityType_Player ? Entity->Stagger > 0.f :
         Entity->HitFresh > 0.f);
    return Result;
}

// NOTE(zoubir): Entity, knocked, ran into Other at Into: the bump.
// Returns the bounce
internal float
BumpOnHit(world_entity *Entity, world_entity *Other, v3 Normal, float Into)
{
    float Bounce = 0.f;
    if (!IsWalkingUnit(Other))
    {
        Bounce = BUMP_WALL_BOUNCE * Into;
    }
    else if (!IsDodging(Other))
    {
        float Transfer = Minimum(IMPACT_MAX_TRANSFER, BUMP_TRANSFER *
                                 KnockbackScale(Other) / KnockbackScale(Entity));
        Other->Velocity.XY -= (Transfer * Into) * Normal.XY;
        if (Other->Type == EntityType_Player)
        {
            StaggerPlayer(Other);
        }
        else
        {
            // NOTE(zoubir): clients draw its flinch from these
            Other->HitFresh = Maximum(Other->HitFresh, HIT_FRESH_SECONDS);
            Other->HitAngle = ATan2(-Normal.Y, -Normal.X);
            Other->HitThrown = false;
        }
    }
    return Bounce;
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
    // NOTE(zoubir): a client replaying its own player's moves leaves what
    // they do to anyone (damage, stuns, shoves, the burst) to the server;
    // doing it here hurt and shoved the client's copies of monsters once
    // per replayed input. Only the player's own bounce off a wall is kept,
    // or a thrown player would stop dead on its own screen
    bool32 Predicted = IsPredictedPlayer(AppState, Entity);
    bool32 Thrown = Into >= IMPACT_MIN_SPEED && IsThrownUnit(Entity);
    bool32 Knocked = !Thrown && Into >= BUMP_MIN_SPEED && IsKnockedUnit(Entity);
    if (Predicted)
    {
        float Bounce = 0.f;
        if (!IsWalkingUnit(Other))
        {
            Bounce = Thrown ? IMPACT_WALL_BOUNCE * Into :
                Knocked ? BUMP_WALL_BOUNCE * Into : 0.f;
        }
        return Bounce;
    }
    if (Knocked)
    {
        return BumpOnHit(Entity, Other, Normal, Into);
    }
    if (!Thrown)
    {
        if (Entity->Type == EntityType_Player && IsWalkingUnit(Other) &&
            !IsDodging(Other))
        {
            Other->Velocity.XY -=
                (SHOULDER_SHARE * Minimum(Into, SHOULDER_MAX_SPEED)) * Normal.XY;
            // NOTE(zoubir): a dash into it is a hit as well as a shoulder
            if (IsDodging(Entity) && Into >= DASH_STRIKE_SPEED)
            {
                DashStrike(AppState, World, Entity, Other, -Normal.XY);
            }
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
            float Transfer = Minimum(IMPACT_MAX_TRANSFER, IMPACT_TRANSFER *
                                     KnockbackScale(Other) / KnockbackScale(Entity));
            Other->Velocity.XY -= (Transfer * Into) * Normal.XY;
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
