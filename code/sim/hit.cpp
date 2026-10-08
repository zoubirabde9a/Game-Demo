/* Hits: what one attack does to one target, as data, and ApplyHit, the one
   place any hit lands. Players' swords, dashes and area abilities
   (player_abilities/) and monsters' bites, abilities and shots
   (monster_abilities/hits.cpp) all go through it, on players and monsters
   alike.

   A hit deals damage (scaled by an elite attacker's affix, which may also
   steal life or add a status), shoves the target away, lifts it (more
   when it is already in the air: a juggle), may stun it and give it a
   status, and draws a burst. Heavy monsters are shoved and lifted less
   (KnockbackScale); a shoved player staggers for a moment, the keys
   off, so the shove carries (Stagger, player_fields.inc). A solid hit
   freezes the monsters in it for a moment, a hit-pause, so the hit
   reads (HitStop). The target keeps the hit's direction, whether it
   was lifted and who hit it for a moment (HitFresh), which clients draw
   its flinch and tumble from. */

struct hit
{
    float Damage;
    // NOTE(zoubir): horizontal speed given to the target, away from the
    // attacker; Lift is the vertical one
    float Shove;
    float Lift;
    // NOTE(zoubir): the lift instead when the target is already in the air
    // (thrown, launched, jumping), so hits keep it up: a juggle
    float AirLift;
    // NOTE(zoubir): a stunned target flying into something is an impact
    // (impacts.cpp), credited to the attacker
    float StunSeconds;
    // NOTE(zoubir): drawn on each target hit; SimBurst_Count for none
    sim_burst Burst;
    status_effect Status;
    float StatusSeconds;
    // NOTE(zoubir): lands on a player mid-dash or blink too (a Smite,
    // sim/monster_abilities/trigger.cpp)
    bool32 Unavoidable;
};

// NOTE(zoubir): a monster's knockback is scaled by
// sqrt(KNOCKBACK_REFERENCE_HP / its kind's MaxHp), at most 1: kinds up to
// that health (bats, imps) fly as far as the hit says, a brute (140) about
// two thirds as far, the warlord (420) KNOCKBACK_SCALE_MIN. Players are 1
#define KNOCKBACK_REFERENCE_HP 60.f
#define KNOCKBACK_SCALE_MIN 0.4f

// NOTE(zoubir): hit-pause: a hit of HITSTOP_MIN_DAMAGE or more freezes the
// monsters in it (target, and a monster attacker) for HITSTOP_PER_DAMAGE
// seconds per point of damage, at most HITSTOP_MAX. After one ends the
// unit runs for HITSTOP_GRACE before another can start, so a crowd of
// attackers slows a monster down but can never hold it still.
// Players never pause: the client predicts its own player with
// UpdatePlayer alone, and a pause only the server ran would pull it back
#define HITSTOP_MIN_DAMAGE 10.f
#define HITSTOP_PER_DAMAGE 0.003f
#define HITSTOP_MAX 0.1f
#define HITSTOP_GRACE 0.15f
// NOTE(zoubir): how long a target keeps its last hit for clients; longer
// than a snapshot's gap (3 ticks) so every hit reaches them
#define HIT_FRESH_SECONDS 0.25f

inline float
KnockbackScale(world_entity *Entity)
{
    float Result = 1.f;
    if (Entity->Type == EntityType_Monster)
    {
        float MaxHp = Maximum(1.f, GetMonsterDef(Entity->MonsterKind)->MaxHp);
        Result = SquareRoot(KNOCKBACK_REFERENCE_HP / MaxHp);
        Result = Maximum(KNOCKBACK_SCALE_MIN, Minimum(1.f, Result));
    }
    return Result;
}

// NOTE(zoubir): HitStop is the seconds left frozen while above 0; below 0
// it counts the grace back up to 0, when a new pause may start
inline void
StartHitStop(world_entity *Entity, float Damage)
{
    if (Entity && Entity->IsPresent && Entity->Type == EntityType_Monster &&
        Entity->HitStop == 0.f && Damage >= HITSTOP_MIN_DAMAGE)
    {
        Entity->HitStop = Minimum(HITSTOP_MAX, HITSTOP_PER_DAMAGE * Damage);
    }
}

// NOTE(zoubir): once per tick per entity, before it updates. Returns
// whether it is frozen this tick (it neither moves, thinks nor animates)
inline bool32
TickHitStop(world_entity *Entity, float DeltaTime)
{
    Entity->HitFresh = Maximum(0.f, Entity->HitFresh - DeltaTime);
    bool32 Frozen = Entity->HitStop > 0.f;
    if (Frozen)
    {
        Entity->HitStop -= DeltaTime;
        if (Entity->HitStop <= 0.f)
        {
            Entity->HitStop = -HITSTOP_GRACE;
        }
    }
    else if (Entity->HitStop < 0.f)
    {
        Entity->HitStop = Minimum(0.f, Entity->HitStop + DeltaTime);
    }
    return Frozen;
}

// NOTE(zoubir): player_stats.cpp, included after this
internal void StaggerPlayer(world_entity *Player);

// NOTE(zoubir): Hit on Target, thrown along Away (a unit vector), by
// Source (the attacker, or its sword or shot), on behalf of the player in
// slot BySlot (SIM_NOBODY for monsters). Returns whether the target
// survived to be thrown.
internal bool32
ApplyHit(app_state *AppState, world *World, world_entity *Target,
         hit *Hit, v2 Away, world_entity *Source, u32 BySlot)
{
    // NOTE(zoubir): nor is a unit a rewind froze thrown (sim/time_rewind/)
    if (!Target->IsPresent || Target->Hp <= 0.f || IsRewindInvulnerable(AppState, Target) ||
        IsFriendlyFire(AppState, Target, Source))
    {
        return false;
    }
    bool32 Dodges = Hit->Unavoidable ? Target->SpawnShield > 0.f : IsDodging(Target);
    if (Hit->Burst != SimBurst_Count && !Dodges)
    {
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, Hit->Burst, (u8)BySlot, Chest,
                  ATan2(Away.Y, Away.X));
    }

    monster_affix_def *Affix = GetAffix(Source ? Source->EliteAffix : 0);
    float Damage = Hit->Damage * Affix->DamageScale;
    // NOTE(zoubir): a ward (sim/progression/talents.cpp) takes the whole
    // hit, the shove and the stun with it
    if (!Dodges && WardTakesHit(AppState, Target, Damage))
    {
        return false;
    }
    float HpBefore = Target->Hp;
    bool32 Killed = DealDamage(AppState, World, Target, Damage, Source,
                               Hit->Unavoidable);
    float Dealt = HpBefore - Maximum(0.f, Target->Hp);
    if (Dealt > 0.f)
    {
        EmitSound(&AppState->Events, AssetType_SfxHit, Target->Position);
    }
    if (Source && Source->Type == EntityType_Monster && Source->IsPresent &&
        Affix->LifeSteal > 0.f && Dealt > 0.f)
    {
        Source->Hp = Minimum(Source->MaxHp, Source->Hp + Affix->LifeSteal * Dealt);
    }
    if (Killed || !Target->IsPresent || Target->Hp <= 0.f || Dodges)
    {
        return false;
    }

    float Scale = KnockbackScale(Target);
    Target->Velocity.XY += (Scale * Hit->Shove) * Away;
    if (Target->Type == EntityType_Player && Hit->Shove > 0.f)
    {
        StaggerPlayer(Target);
    }
    bool32 Airborne = Target->Position.Z > Target->GroundZ + 2.f;
    float Lift = Airborne ? Maximum(Hit->Lift, Hit->AirLift) : Hit->Lift;
    if (Lift > 0.f)
    {
        Target->Velocity.Z = Maximum(Target->Velocity.Z, Scale * Lift);
    }
    Target->HitAngle = ATan2(Away.Y, Away.X);
    Target->HitThrown = Lift > 0.f;
    Target->HitBySlot = BySlot != SIM_NOBODY ? BySlot + 1 : 0;
    Target->HitFresh = HIT_FRESH_SECONDS;
    ApplyStatus(Target, Hit->Status, Hit->StatusSeconds);
    ApplyStatus(Target, Affix->OnHitStatus, Affix->OnHitStatusSeconds);
    if (Hit->StunSeconds > 0.f)
    {
        ApplyStatus(Target, StatusEffect_Stunned, Hit->StunSeconds);
        if (BySlot != SIM_NOBODY)
        {
            Target->ThrownBySlot = BySlot + 1;
        }
    }
    if (Dealt > 0.f)
    {
        StartHitStop(Target, Dealt);
        StartHitStop(Source, Dealt);
    }
    return true;
}
