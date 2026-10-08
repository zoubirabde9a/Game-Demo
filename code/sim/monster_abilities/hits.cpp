/* Monster hits on players: finding the nearest target, the plain bite,
   single and area hits. Each hit lands through ApplyHit (sim/hit.cpp),
   the same as players' hits. */

// NOTE(zoubir): in sim/dungeon/threat.cpp, included later: in a dungeon
// run monsters attack by threat
internal world_entity *DungeonPickTarget(app_state *AppState, world_entity *Monster,
                                         float *DistanceOut);

// NOTE(zoubir): the player Monster goes for: by threat in a dungeon run,
// else the nearest living one
internal world_entity *
FindMonsterTarget(app_state *AppState, world *World, world_entity *Monster,
                  float *DistanceOut)
{
    world_entity *Picked = DungeonPickTarget(AppState, Monster, DistanceOut);
    if (Picked)
    {
        return Picked;
    }
    v2 From = Monster->Position.XY;
    world_entity *Result = 0;
    float BestDistance = 0.f;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Player &&
            Entity->Hp > 0.f)
        {
            float Distance = Length(Entity->Position.XY - From);
            if (!Result || Distance < BestDistance)
            {
                Result = Entity;
                BestDistance = Distance;
            }
        }
    }
    if (DistanceOut)
    {
        *DistanceOut = BestDistance;
    }
    return Result;
}

// NOTE(zoubir): every monster hit on a player is an ApplyHit
// (sim/hit.cpp): damage scaled by the source's elite affix, life steal,
// the push (Push, a velocity) and Lift, and the ability's status. Source
// is the monster or shot, which earns no kill credit. Burst is drawn on
// the player, so a hit from behind or off screen still shows; Angle is
// the way it is drawn when there is no push
internal void
HitPlayerWith(app_state *AppState, world *World, world_entity *Player,
              world_entity *Source, float Damage, v2 Push, float Lift,
              status_effect Status, float StatusSeconds, sim_burst Burst)
{
    float Shove = Length(Push);
    v2 Away = Shove > 0.f ? Push * (1.f / Shove) : V2(0.f);
    if (Shove <= 0.f && Source)
    {
        v2 FromSource = Player->Position.XY - Source->Position.XY;
        float Distance = Length(FromSource);
        Away = Distance > 0.f ? (1.f / Distance) * FromSource : V2(0.f);
    }
    hit Hit = {Damage, Shove, Lift, Lift, 0.f, Burst, Status,
               StatusSeconds};
    ApplyHit(AppState, World, Player, &Hit, Away, Source, SIM_NOBODY);
}

internal void
HitPlayer(app_state *AppState, world *World, world_entity *Player,
          world_entity *Source, monster_ability *Ability, v2 Push)
{
    HitPlayerWith(AppState, World, Player, Source, Ability->Damage, Push, 0.f,
                  Ability->Status, Ability->StatusSeconds, SimBurst_MonsterHit);
}

// NOTE(zoubir): how long a monster plays its attack row after a bite
// (sim/update.cpp), so the bite reads on the monster as well as on the
// bitten
#define MONSTER_BITE_SECONDS 0.3f

// NOTE(zoubir): the plain bite every monster has, off AttackInterval. The
// shove is 0, so a bite never moves the player; Away only aims its burst
internal void
MonsterBite(app_state *AppState, world *World, world_entity *Monster,
            world_entity *Player)
{
    monster_def *Def = GetMonsterDef(Monster->MonsterKind);
    HitPlayerWith(AppState, World, Player, Monster, Def->AttackDamage,
                  V2(0.f), 0.f, StatusEffect_None, 0.f, SimBurst_MonsterBite);
}

// NOTE(zoubir): a Smite's blow (trigger.cpp): only the victim, through a
// dash or blink, never a jump's clearance
internal void
SmitePlayer(app_state *AppState, world *World, world_entity *Monster,
            world_entity *Player, monster_ability *Ability)
{
    v2 Away = Player->Position.XY - Monster->Position.XY;
    float Distance = Length(Away);
    Away = Distance > 0.f ? (1.f / Distance) * Away : V2(1.f, 0.f);
    hit Hit = {Ability->Damage, Ability->Knockback, 0.f, 0.f, 0.f,
               SimBurst_Smite, Ability->Status, Ability->StatusSeconds, true};
    ApplyHit(AppState, World, Player, &Hit, Away, Monster, SIM_NOBODY);
}

// NOTE(zoubir): an area hit (slam, mortar, blink, eruption) throws a player
// up as well as out: AREA_HIT_LIFT_SHARE of its knockback, at most
// AREA_HIT_MAX_LIFT (a brute's slam lifts 240, a hop of about 18 units)
#define AREA_HIT_LIFT_SHARE 0.4f
#define AREA_HIT_MAX_LIFT 320.f

// NOTE(zoubir): hits every player within Radius of Center and not closer
// than InnerRadius, pushing them away from it. Returns how many were hit
internal u32
HurtPlayersInRadius(app_state *AppState, world *World, world_entity *Source,
                    v2 Center, monster_ability *Ability)
{
    u32 HitCount = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < World->EntityCount;
        EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player ||
            Player->Hp <= 0.f || IsJumpingClear(Player))
        {
            continue;
        }
        v2 Away = Player->Position.XY - Center;
        float Distance = Length(Away);
        if (Distance > Ability->Radius || Distance < Ability->InnerRadius)
        {
            continue;
        }
        HitCount++;
        v2 Push = Distance > 0.f ? (Ability->Knockback / Distance) * Away : V2(0.f);
        float Lift = Minimum(AREA_HIT_MAX_LIFT,
                             AREA_HIT_LIFT_SHARE * Ability->Knockback);
        HitPlayerWith(AppState, World, Player, Source, Ability->Damage, Push,
                      Lift, Ability->Status, Ability->StatusSeconds,
                      SimBurst_MonsterHit);
    }
    return HitCount;
}
