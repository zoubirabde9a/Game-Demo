/* Cones, lanes, gazes and shares: four ability kinds for the monsters of
   the Aurora Rift and the Starless Deep (docs/deep-monsters.md), each
   asking for an answer the older kinds never did.

   Cone   a breath in front of the monster, Spread degrees wide and Radius
          long, along the aim locked when the windup starts. The answer is
          to get beside or behind it; a jump does not clear a breath.
   Lanes  Count strips Radius each side of their middle, Speed long and
          Spread apart, laid along the aim and centred on the target: an
          odd count puts a strip on it, an even one leaves it standing in
          a gap, so whoever steps aside steps onto one. Things fall on
          them from above when the windup ends, so a jump does not help:
          the answer is the gap between two strips.
   Gaze   when the windup ends every player within Radius still moving
          faster than Speed is hit, past any dodge or jump. The answer is
          to stop: let go of the keys before the eyes open.
   Share  a circle of Radius follows the player within MaxRange farthest
          from the monster (the back line) through the windup, then the
          blow lands: Damage split evenly between everyone standing in
          it, past any dodge. One player alone takes all of it; whoever
          can leave the pack to stand with them takes a share each.
          Landing on the tank would pull the whole party into the pack's
          slams. A void brand picks the same player and asks the opposite,
          so the encounters never put the two in one pack.

   Like the rift's and the deep's kinds, each is worked out from the
   ability's phase, timer, aim and points, which snapshots carry, so
   clients draw them where the server hits (client/dungeon/deep_fx.cpp)
   and nothing new goes on the wire. */

// NOTE(zoubir): the share of a Share's windup, at the end, through which
// its circle stops following the victim and stays put, so the party can
// gather on a spot that holds still
#define SHARE_LOCK_SHARE 0.35f

// NOTE(zoubir): whether P stands in a cone from Apex along Aim, Radius
// long and Degrees wide
inline bool32
IsInCone(v2 Apex, v2 Aim, float Radius, float Degrees, v2 P)
{
    v2 Out = P - Apex;
    float Distance = Length(Out);
    if (Distance > Radius)
    {
        return false;
    }
    if (Distance < 1.f)
    {
        return true;
    }
    float Cosine = DotProduct(Out, Aim) / Distance;
    bool32 Result = Cosine >= Cos(0.5f * Degrees * Pi32 / 180.f);
    return Result;
}

// NOTE(zoubir): the breath lands: every living player in the cone,
// jumping or not, pushed along the aim
internal void
BreatheCone(app_state *AppState, world *World, world_entity *Entity, monster_ability *Ability)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f ||
            !IsInCone(Entity->Position.XY, Entity->AbilityAim, Ability->Radius, Ability->Spread,
                      Player->Position.XY))
        {
            continue;
        }
        HitPlayerWith(AppState, World, Player, Entity, Ability->Damage,
                      Ability->Knockback * Entity->AbilityAim, 0.f, Ability->Status,
                      Ability->StatusSeconds, SimBurst_MonsterHit);
    }
}

// NOTE(zoubir): the strips start at the monster, Spread apart and
// centred on the line through the target
internal void
StartLanes(world_entity *Entity, monster_ability *Ability)
{
    u32 Count = Minimum(Maximum(1u, Ability->Count), (u32)MAX_ABILITY_POINTS);
    v2 Side = V2(-Entity->AbilityAim.Y, Entity->AbilityAim.X);
    for(u32 Lane = 0; Lane < Count; Lane++)
    {
        float Offset = ((float)Lane - 0.5f * (float)(Count - 1)) * Ability->Spread;
        Entity->AbilityPoints[Entity->AbilityPointCount++] = Entity->Position.XY + Offset * Side;
    }
}

// NOTE(zoubir): whether P stands on one of the strips
inline bool32
IsOnLane(world_entity *Entity, monster_ability *Ability, v2 P)
{
    v2 Aim = Entity->AbilityAim;
    v2 Side = V2(-Aim.Y, Aim.X);
    for(u32 Lane = 0; Lane < Entity->AbilityPointCount; Lane++)
    {
        v2 Out = P - Entity->AbilityPoints[Lane];
        float Along = DotProduct(Out, Aim);
        if (Along >= -Ability->Radius && Along <= Ability->Speed &&
            Absolute(DotProduct(Out, Side)) <= Ability->Radius)
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): everything falls at once, on players in the air too
internal void
DropLanes(app_state *AppState, world *World, world_entity *Entity, monster_ability *Ability)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f ||
            !IsOnLane(Entity, Ability, Player->Position.XY))
        {
            continue;
        }
        HitPlayerWith(AppState, World, Player, Entity, Ability->Damage, V2(0.f), 0.f,
                      Ability->Status, Ability->StatusSeconds, SimBurst_MonsterHit);
    }
}

// NOTE(zoubir): whether a player counts as moving for a gaze: how fast
// they go along the ground, so a jump on the spot is still
inline bool32
IsMovingUnderGaze(world_entity *Player, monster_ability *Ability)
{
    bool32 Result = LengthSq(Player->Velocity.XY) > Square(Ability->Speed);
    return Result;
}

// NOTE(zoubir): the eyes open: whoever is in reach and still moving is
// hit, past any dodge or jump
internal void
OpenGaze(app_state *AppState, world *World, world_entity *Entity, monster_ability *Ability)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f ||
            LengthSq(Player->Position.XY - Entity->Position.XY) > Square(Ability->Radius) ||
            !IsMovingUnderGaze(Player, Ability))
        {
            continue;
        }
        hit Hit = {Ability->Damage, 0.f, 0.f, 0.f, 0.f, SimBurst_Smite, Ability->Status,
                   Ability->StatusSeconds, true};
        ApplyHit(AppState, World, Player, &Hit, V2(0.f, -1.f), Entity, SIM_NOBODY);
    }
}

// NOTE(zoubir): the circle goes on the player a brand would take
// (FindBrandVictim, wells_brands_mirrors.cpp), and follows them as a
// brand follows its victim
internal bool32
StartShare(world *World, world_entity *Entity, monster_ability *Ability)
{
    world_entity *Victim = FindBrandVictim(World, Entity, Ability);
    if (!Victim)
    {
        return false;
    }
    Entity->AbilityTargetSlot = Victim->ID;
    Entity->AbilityPoints[Entity->AbilityPointCount++] = Victim->Position.XY;
    return true;
}

// NOTE(zoubir): through the windup the circle follows its victim, until
// the last SHARE_LOCK_SHARE, when it stays put for the party to gather on
internal void
TrackShareVictim(world *World, world_entity *Entity, monster_ability *Ability)
{
    if (Entity->AbilityTimer > SHARE_LOCK_SHARE * Ability->Windup)
    {
        TrackBrandVictim(World, Entity);
    }
}

// NOTE(zoubir): how many living players stand in the circle
inline u32
CountSharers(world *World, monster_ability *Ability, v2 Centre)
{
    u32 Result = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (Player->IsPresent && Player->Type == EntityType_Player && Player->Hp > 0.f &&
            LengthSq(Player->Position.XY - Centre) <= Square(Ability->Radius))
        {
            Result++;
        }
    }
    return Result;
}

// NOTE(zoubir): the blow lands on the circle, split evenly between
// everyone in it; with nobody in it, it lands on nothing
internal void
LandShare(app_state *AppState, world *World, world_entity *Entity, monster_ability *Ability)
{
    if (!Entity->AbilityPointCount)
    {
        return;
    }
    v2 Centre = Entity->AbilityPoints[0];
    u32 Sharers = CountSharers(World, Ability, Centre);
    if (!Sharers)
    {
        return;
    }
    float Each = Ability->Damage / (float)Sharers;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f ||
            LengthSq(Player->Position.XY - Centre) > Square(Ability->Radius))
        {
            continue;
        }
        hit Hit = {Each, 0.f, 0.f, 0.f, 0.f, SimBurst_Smite, Ability->Status,
                   Ability->StatusSeconds, true};
        ApplyHit(AppState, World, Player, &Hit, V2(0.f, -1.f), Entity, SIM_NOBODY);
    }
}
