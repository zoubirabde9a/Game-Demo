/* Bot dangers (server/bots/class_bots.cpp, before the class files): the
   circles near a bot that are about to hurt it, for bots that fight in
   melee and so stand where bosses slam. A monster winding up a Slam hurts
   round itself, a Blink where it lands (AbilityPoints[0]), a Mortar at
   every point it marked; a monster hazard (burning ground, magma) hurts
   where it lies. A slam with an inner radius (the Hollow King's Hollow
   Ring) hurts only the band between its radii: the ground inside is safe,
   and the way out of the band is in. BotSafeSpot moves a spot out of all
   of them, so a bot walks to the edge of a telegraph, or into a ring's
   middle, instead of standing where it lands.

   Found by the Shadowblade's bot (server/bots/shadowblade.cpp), which
   melee bots share through here. The Fire Mage's, tank's and healer's
   bots do not use it yet: the balance probe's numbers are tuned against
   them as they play now. */

// NOTE(zoubir): how far past a danger's edge a bot stands, how far off it
// looks, and how many dangers it keeps track of
#define BOT_DANGER_MARGIN 26.f
#define BOT_DANGER_SIGHT 420.f
#define BOT_MAX_DANGERS 16

struct bot_danger_circle
{
    v2 Centre;
    float Radius;
    // NOTE(zoubir): a ring's safe middle, 0 for a full circle
    float Inner;
};

// NOTE(zoubir): the danger circles near Self, into Out (BOT_MAX_DANGERS
// long); returns how many
internal u32
BotDangers(app_state *AppState, world_entity *Self, bot_danger_circle *Out)
{
    world *World = &AppState->World;
    u32 Count = 0;
    for (u32 Index = 0; Index < World->EntityCount && Count < BOT_MAX_DANGERS; ++Index)
    {
        world_entity *Other = &World->Entities[Index];
        if (!Other->IsPresent ||
            LengthSq(Other->Position.XY - Self->Position.XY) > Square(BOT_DANGER_SIGHT))
        {
            continue;
        }
        if (Other->Type == EntityType_MonsterHazard)
        {
            Out[Count++] = {Other->Position.XY, 0.5f * Other->Dimensions.X, 0.f};
            continue;
        }
        if (Other->Type != EntityType_Monster || Other->Hp <= 0.f ||
            Other->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef((monster_kind)Other->MonsterKind);
        if (Other->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Other->AbilityIndex];
        if (Ability->Kind == MonsterAbility_Slam)
        {
            Out[Count++] = {Other->Position.XY, Ability->Radius, Ability->InnerRadius};
        }
        else if (Ability->Kind == MonsterAbility_Blink && Other->AbilityPointCount)
        {
            Out[Count++] = {Other->AbilityPoints[0], Ability->Radius, 0.f};
        }
        else if (Ability->Kind == MonsterAbility_Mortar)
        {
            for (u32 Point = 0; Point < Other->AbilityPointCount && Count < BOT_MAX_DANGERS; ++Point)
            {
                Out[Count++] = {Other->AbilityPoints[Point], Ability->Radius, 0.f};
            }
        }
    }
    return Count;
}

// NOTE(zoubir): the danger circle P stands in (with the margin), 0 for none
internal bot_danger_circle *
BotDangerAt(bot_danger_circle *Dangers, u32 Count, v2 P)
{
    for (u32 Index = 0; Index < Count; ++Index)
    {
        bot_danger_circle *Danger = &Dangers[Index];
        float DistanceSq = LengthSq(P - Danger->Centre);
        bool32 InSafeMiddle = Danger->Inner > BOT_DANGER_MARGIN &&
            DistanceSq < Square(Danger->Inner - BOT_DANGER_MARGIN);
        if (DistanceSq < Square(Danger->Radius + BOT_DANGER_MARGIN) && !InSafeMiddle)
        {
            return Danger;
        }
    }
    return 0;
}

// NOTE(zoubir): P pushed out of every danger circle it is in, a few
// passes, as leaving one may enter the next; Fallback is the way out of a
// circle P is at the very centre of
internal v2
BotSafeSpot(bot_danger_circle *Dangers, u32 Count, v2 P, v2 Fallback)
{
    for (u32 Pass = 0; Pass < 4; ++Pass)
    {
        bot_danger_circle *Danger = BotDangerAt(Dangers, Count, P);
        if (!Danger)
        {
            break;
        }
        v2 Out = LengthSq(P - Danger->Centre) > 1.f ? DirectionTo(P - Danger->Centre) : Fallback;
        // NOTE(zoubir): a ring is left inward, to halfway into its middle
        float Reach = Danger->Inner > BOT_DANGER_MARGIN ? 0.5f * Danger->Inner :
            Danger->Radius + BOT_DANGER_MARGIN + 6.f;
        P = Danger->Centre + Reach * Out;
    }
    return P;
}
