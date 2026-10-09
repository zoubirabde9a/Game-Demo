/* Wells, brands, mirrors and eclipses: the Starless Deep's ability kinds
   (docs/dungeon-starless.md), each asking for something the levels above
   never did.

   Pull     a gravity well opens and drags every player in reach toward
            it, then collapses on whoever it held. Neither a jump nor a
            single step gets out: you run against it the whole time, or
            dash and blink clear.
   Brand    a void brand on the player farthest from the monster. When it
            bursts it hits the branded and everyone standing near them,
            so the branded runs away from the party, and the party from
            them.
   Reflect  the monster raises a mirror: hits on it do nothing, and each
            one is turned back on whoever struck. The answer is to stop
            attacking it in time. Only a dungeon run turns hits back
            (sim/dungeon/mirror_guard.cpp); the ability itself only
            stands there.
   Eclipse  the light goes out except in a few circles round the room;
            everyone outside every circle when the windup ends is hit.

   Like the rift's kinds, everything is worked out from the ability's
   timer, aim and points, which snapshots carry, so clients draw the well,
   the brand, the mirror and the lights where the server hits
   (client/dungeon/starless_fx.cpp). */

// NOTE(zoubir): a light is never placed closer than this to the monster,
// as a share of Spread, so the boss cannot stand in its own light
#define ECLIPSE_MIN_SHARE 0.35f
// NOTE(zoubir): tries to place each light before giving up on it
#define ECLIPSE_TRIES 12

// NOTE(zoubir): the well opens Spread toward the target, at most on the
// target itself; Spread 0 opens it at the monster's feet
internal void
StartPull(world_entity *Entity, monster_ability *Ability, world_entity *Target)
{
    v2 ToTarget = Target->Position.XY - Entity->Position.XY;
    float Distance = Length(ToTarget);
    float Along = Minimum(Distance, Ability->Spread);
    Entity->AbilityPoints[Entity->AbilityPointCount++] =
        Entity->Position.XY + Along * Entity->AbilityAim;
}

// NOTE(zoubir): through the Active time every living player within
// Radius of the well is dragged toward it at Speed (an acceleration, so
// a player running away still gets out, slowly), in the air or not
internal void
UpdatePull(world *World, world_entity *Entity, monster_ability *Ability, float DeltaTime)
{
    if (!Entity->AbilityPointCount)
    {
        return;
    }
    v2 Well = Entity->AbilityPoints[0];
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f)
        {
            continue;
        }
        v2 In = Well - Player->Position.XY;
        float Distance = Length(In);
        if (Distance > Ability->Radius || Distance < 4.f)
        {
            continue;
        }
        Player->Velocity.XY += (Ability->Speed * DeltaTime / Distance) * In;
    }
}

// NOTE(zoubir): the well collapses when the Active time ends: everyone on
// the ground within InnerRadius of it is hit and thrown out
internal void
CollapseWell(app_state *AppState, world *World, world_entity *Entity,
             monster_ability *Ability)
{
    if (!Entity->AbilityPointCount)
    {
        return;
    }
    v2 Well = Entity->AbilityPoints[0];
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f)
        {
            continue;
        }
        v2 Out = Player->Position.XY - Well;
        float Distance = Length(Out);
        if (Distance > Ability->InnerRadius)
        {
            continue;
        }
        Out = Distance > 0.f ? (1.f / Distance) * Out : Entity->AbilityAim;
        HitPlayerWith(AppState, World, Player, Entity, Ability->Damage,
                      Ability->Knockback * Out, 0.f, Ability->Status,
                      Ability->StatusSeconds, SimBurst_MonsterHit);
    }
}

// NOTE(zoubir): the living player within MaxRange farthest from the
// monster: the back line, so the brand pulls a healer or a caster out of
// the group rather than the tank
internal world_entity *
FindBrandVictim(world *World, world_entity *Entity, monster_ability *Ability)
{
    world_entity *Result = 0;
    float Best = -1.f;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f)
        {
            continue;
        }
        float Distance = Length(Player->Position.XY - Entity->Position.XY);
        if (Distance <= Ability->MaxRange && Distance > Best)
        {
            Best = Distance;
            Result = Player;
        }
    }
    return Result;
}

internal bool32
StartBrand(world *World, world_entity *Entity, monster_ability *Ability)
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

// NOTE(zoubir): the branded player while they live; a brand does not
// move to anyone else, and a dead player's brand fizzles
internal world_entity *
GetBrandVictim(world *World, world_entity *Entity)
{
    world_entity *Result = 0;
    if (Entity->AbilityTargetSlot < World->EntityCount)
    {
        world_entity *Victim = &World->Entities[Entity->AbilityTargetSlot];
        if (Victim->IsPresent && Victim->Type == EntityType_Player && Victim->Hp > 0.f)
        {
            Result = Victim;
        }
    }
    return Result;
}

// NOTE(zoubir): through the windup the brand's point follows the
// branded, so every client draws it on them
internal void
TrackBrandVictim(world *World, world_entity *Entity)
{
    world_entity *Victim = GetBrandVictim(World, Entity);
    if (Victim && Entity->AbilityPointCount)
    {
        Entity->AbilityPoints[0] = Victim->Position.XY;
    }
}

// NOTE(zoubir): the brand bursts: the branded and every other player
// within Radius of them, past any dodge for the branded, who carried it
internal void
BurstBrand(app_state *AppState, world *World, world_entity *Entity,
           monster_ability *Ability)
{
    world_entity *Victim = GetBrandVictim(World, Entity);
    if (!Victim)
    {
        return;
    }
    v2 Centre = Victim->Position.XY;
    Entity->AbilityPoints[0] = Centre;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player ||
            Player->Hp <= 0.f || Player == Victim)
        {
            continue;
        }
        v2 Out = Player->Position.XY - Centre;
        float Distance = Length(Out);
        if (Distance > Ability->Radius)
        {
            continue;
        }
        Out = Distance > 0.f ? (1.f / Distance) * Out : V2(1.f, 0.f);
        HitPlayerWith(AppState, World, Player, Entity, Ability->Damage,
                      Ability->Knockback * Out, 0.f, Ability->Status,
                      Ability->StatusSeconds, SimBurst_MonsterHit);
    }
    hit Hit = {Ability->Damage, 0.f, 0.f, 0.f, 0.f, SimBurst_Smite, Ability->Status,
               Ability->StatusSeconds, true};
    ApplyHit(AppState, World, Victim, &Hit, V2(0.f, -1.f), Entity, SIM_NOBODY);
}

// NOTE(zoubir): whether the straight way from From to To crosses no tile
// that blocks, so a light is never placed beyond a wall
internal bool32
IsClearLine(world *World, v2 From, v2 To)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float Tile = (float)World->TileWidth;
    if (!Map || Tile <= 0.f)
    {
        return true;
    }
    float Distance = Length(To - From);
    v2 Way = Distance > 0.f ? (1.f / Distance) * (To - From) : V2(0.f);
    for(float Along = 0.f; Along <= Distance; Along += BEAM_STEP)
    {
        v2 Point = From + Along * Way;
        i32 X = (i32)floorf(Point.X / Tile);
        i32 Y = (i32)floorf(Point.Y / Tile);
        if (GetTerrainDef(TerrainAt(Map, X, Y))->Blocks)
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): up to Count lights on open ground in sight of the
// monster, between ECLIPSE_MIN_SHARE and all of Spread from it, spread
// round it so they are not all on one side
internal bool32
StartEclipse(app_state *AppState, world *World, world_entity *Entity,
             monster_ability *Ability)
{
    u32 Count = Minimum(Maximum(1u, Ability->Count), (u32)MAX_ABILITY_POINTS);
    float Offset = MonsterRandomBetween(AppState, 0.f, 2.f * Pi32);
    for(u32 Light = 0; Light < Count; Light++)
    {
        for(u32 Try = 0; Try < ECLIPSE_TRIES; Try++)
        {
            float Angle = Offset + 2.f * Pi32 * (float)Light / (float)Count +
                MonsterRandomBetween(AppState, -0.4f, 0.4f);
            float Reach = MonsterRandomBetween(AppState, ECLIPSE_MIN_SHARE, 1.f) * Ability->Spread;
            v2 Point = Entity->Position.XY + Reach * V2(Cos(Angle), Sin(Angle));
            if (IsInsideArena(World, Point, Ability->Radius) &&
                IsClearLine(World, Entity->Position.XY, Point))
            {
                Entity->AbilityPoints[Entity->AbilityPointCount++] = Point;
                break;
            }
        }
    }
    bool32 Result = Entity->AbilityPointCount > 0;
    return Result;
}

// NOTE(zoubir): whether P stands inside one of the eclipse's lights
inline bool32
IsInEclipseLight(world_entity *Entity, monster_ability *Ability, v2 P)
{
    for(u32 Light = 0; Light < Entity->AbilityPointCount; Light++)
    {
        if (LengthSq(P - Entity->AbilityPoints[Light]) <= Square(Ability->Radius))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): the dark falls: every player out of the light is hit,
// jumping or not, past any dodge
internal void
FallDark(app_state *AppState, world *World, world_entity *Entity, monster_ability *Ability)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f ||
            IsInEclipseLight(Entity, Ability, Player->Position.XY))
        {
            continue;
        }
        hit Hit = {Ability->Damage, 0.f, 0.f, 0.f, 0.f, SimBurst_Smite, Ability->Status,
                   Ability->StatusSeconds, true};
        ApplyHit(AppState, World, Player, &Hit, V2(0.f, -1.f), Entity, SIM_NOBODY);
    }
}
