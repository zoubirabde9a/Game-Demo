/* Bot starless dangers (server/bots.cpp, after DodgeRiftDangers): how a
   bot lives through the Starless Deep's wells, brands and eclipses
   (sim/monster_abilities/wells_brands_mirrors.cpp). Mirrors are left to
   BotFindTarget, which passes over a monster raising one.

   An eclipse beats everything: through its windup a bot out of the light
   walks to the nearest one. A brand comes next: the branded walks away
   from every ally near it, and an ally near the branded walks away from
   them. Last, a bot near a gravity well's core walks out from it. Only
   the core collapses, so a bot runs from that and not from the whole
   reach of the pull: a well the size of a boss room would otherwise send
   it out of the room, which ends the fight as a wipe. */

// NOTE(zoubir): how far past the edge of a burst a bot wants to be, and
// how far inside a light
#define BOT_STARLESS_MARGIN 30.f
// NOTE(zoubir): how far past a well's core a bot keeps, room for the pull
// to drag it a while
#define BOT_WELL_MARGIN 110.f

internal v2
BotAwayFrom(v2 Self, v2 From)
{
    v2 Away = Self - From;
    float Distance = Length(Away);
    v2 Result = Distance > 0.f ? (1.f / Distance) * Away : V2(1.f, 0.f);
    return Result;
}

// NOTE(zoubir): the way to the nearest light of an eclipse, zero when
// Self stands well inside one already
internal v2
BotEclipseShelter(world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    v2 Result = V2(0.f);
    float Best = 1.0e9f;
    for(u32 Light = 0; Light < Monster->AbilityPointCount; Light++)
    {
        v2 Way = Monster->AbilityPoints[Light] - Self->Position.XY;
        float Distance = Length(Way);
        if (Distance <= Ability->Radius - BOT_STARLESS_MARGIN)
        {
            return V2(0.f);
        }
        if (Distance < Best)
        {
            Best = Distance;
            Result = (1.f / Distance) * Way;
        }
    }
    return Result;
}

// NOTE(zoubir): the way Self walks so a brand's burst misses everyone
// but its carrier, zero when nothing needs to move
internal v2
BotBrandEscape(world *World, world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    world_entity *Victim = GetBrandVictim(World, Monster);
    if (!Victim)
    {
        return V2(0.f);
    }
    float Reach = Ability->Radius + BOT_STARLESS_MARGIN;
    if (Victim != Self)
    {
        bool32 Near = LengthSq(Self->Position.XY - Victim->Position.XY) <= Square(Reach);
        v2 Result = Near ? BotAwayFrom(Self->Position.XY, Victim->Position.XY) : V2(0.f);
        return Result;
    }
    v2 Push = V2(0.f);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Ally = &World->Entities[Index];
        if (Ally == Self || !Ally->IsPresent || Ally->Type != EntityType_Player || Ally->Hp <= 0.f)
        {
            continue;
        }
        v2 Away = Self->Position.XY - Ally->Position.XY;
        float Distance = Length(Away);
        if (Distance < Reach)
        {
            Push += (Distance > 0.f ? (1.f / Distance) : 1.f) * Away;
        }
    }
    float Size = Length(Push);
    v2 Result = Size > 0.f ? (1.f / Size) * Push : V2(0.f);
    return Result;
}

// NOTE(zoubir): straight out from the core of a well Self stands near
internal v2
BotWellEscape(world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    v2 Result = V2(0.f);
    if (Monster->AbilityPointCount)
    {
        v2 Well = Monster->AbilityPoints[0];
        if (LengthSq(Self->Position.XY - Well) <= Square(Ability->InnerRadius + BOT_WELL_MARGIN))
        {
            Result = BotAwayFrom(Self->Position.XY, Well);
        }
    }
    return Result;
}

// NOTE(zoubir): the keys of Held changed so Self gets into the light,
// away from a brand and out of a well
internal u32
DodgeStarlessDangers(app_state *AppState, world_entity *Self, u32 Held)
{
    world *World = &AppState->World;
    v2 Shelter = V2(0.f);
    v2 Brand = V2(0.f);
    v2 Well = V2(0.f);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            (Monster->AbilityPhase != AbilityPhase_Windup &&
             Monster->AbilityPhase != AbilityPhase_Active))
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        bool32 Windup = Monster->AbilityPhase == AbilityPhase_Windup;
        if (Ability->Kind == MonsterAbility_Eclipse && Windup && LengthSq(Shelter) <= 0.f)
        {
            Shelter = BotEclipseShelter(Monster, Ability, Self);
        }
        if (Ability->Kind == MonsterAbility_Brand && Windup && LengthSq(Brand) <= 0.f)
        {
            Brand = BotBrandEscape(World, Monster, Ability, Self);
        }
        if (Ability->Kind == MonsterAbility_Pull && LengthSq(Well) <= 0.f)
        {
            Well = BotWellEscape(Monster, Ability, Self);
        }
    }
    v2 Escape = LengthSq(Shelter) > 0.f ? Shelter : LengthSq(Brand) > 0.f ? Brand : Well;
    if (LengthSq(Escape) > 0.f)
    {
        Held = (Held & ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down)) |
            NetButtonsToward(Escape);
    }
    return Held;
}
