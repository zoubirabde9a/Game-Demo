/* Bot deep dangers (server/bots.cpp): how a bot lives through the cones,
   lanes, gazes and shares of the rift's and the deep's newer monsters
   (sim/monster_abilities/cones_lanes_gazes_shares.cpp).

   DodgeDeepDangers runs before the other dodges, so a well, a brand, an
   eclipse or a slam still takes a bot where it has to go. A bot in front
   of a monster about to breathe walks out of the cone sideways, the
   short way out of the fan; a bot on a strip walks off it toward the
   nearer edge. Bots do not gather on a share's circle: crossing a fight
   to reach it cost them more over the balance probe's runs than taking
   the blow alone, so the probe measures the share on a lone victim.

   FreezeForGazes runs after every other dodge, as standing still is the
   only answer to a gaze: inside its reach, for the last BOT_GAZE_FREEZE
   seconds of its windup, a bot lets go of every key that moves it. */

// NOTE(zoubir): how long before a gaze opens a bot stands still, enough
// for friction to stop it on stone; on ice it slides and may be caught
#define BOT_GAZE_FREEZE 0.5f
// NOTE(zoubir): how far past a cone's or a lane's edge a bot wants to be
#define BOT_DEEP_MARGIN 24.f

#define BOT_MOVE_KEYS (NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down)

// NOTE(zoubir): sideways out of a cone, the way Self already leans
internal v2
BotConeEscape(world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    v2 Result = V2(0.f);
    if (IsInCone(Monster->Position.XY, Monster->AbilityAim, Ability->Radius + BOT_DEEP_MARGIN,
                 Ability->Spread + 20.f, Self->Position.XY))
    {
        v2 Side = V2(-Monster->AbilityAim.Y, Monster->AbilityAim.X);
        float Lean = DotProduct(Self->Position.XY - Monster->Position.XY, Side);
        Result = Lean >= 0.f ? Side : -Side;
    }
    return Result;
}

// NOTE(zoubir): off the strip Self stands on, toward its nearer edge
internal v2
BotLaneEscape(world_entity *Monster, monster_ability *Ability, world_entity *Self)
{
    v2 Aim = Monster->AbilityAim;
    v2 Side = V2(-Aim.Y, Aim.X);
    for(u32 Lane = 0; Lane < Monster->AbilityPointCount; Lane++)
    {
        v2 Out = Self->Position.XY - Monster->AbilityPoints[Lane];
        float Along = DotProduct(Out, Aim);
        float Across = DotProduct(Out, Side);
        if (Along >= -Ability->Radius && Along <= Ability->Speed + BOT_DEEP_MARGIN &&
            Absolute(Across) <= Ability->Radius + BOT_DEEP_MARGIN)
        {
            // NOTE(zoubir): the outer strips are left outward, so a bot
            // does not walk from one strip onto the next
            float Middle = 0.5f * (float)(Monster->AbilityPointCount - 1);
            float Way = (float)Lane < Middle ? -1.f : (float)Lane > Middle ? 1.f :
                (Across >= 0.f ? 1.f : -1.f);
            return Way * Side;
        }
    }
    return V2(0.f);
}

// NOTE(zoubir): the keys of Held changed so Self leaves a cone or a lane
internal u32
DodgeDeepDangers(app_state *AppState, world_entity *Self, u32 Held)
{
    world *World = &AppState->World;
    v2 Cone = V2(0.f);
    v2 Lane = V2(0.f);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Monster->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        if (Ability->Kind == MonsterAbility_Cone && LengthSq(Cone) <= 0.f)
        {
            Cone = BotConeEscape(Monster, Ability, Self);
        }
        if (Ability->Kind == MonsterAbility_Lanes && LengthSq(Lane) <= 0.f)
        {
            Lane = BotLaneEscape(Monster, Ability, Self);
        }
    }
    v2 Escape = LengthSq(Cone) > 0.f ? Cone : Lane;
    if (LengthSq(Escape) > 0.f)
    {
        Held = (Held & ~(u32)BOT_MOVE_KEYS) | NetButtonsToward(Escape);
    }
    return Held;
}

// NOTE(zoubir): Held without the keys that move Self while a gaze it
// stands in is about to open
internal u32
FreezeForGazes(app_state *AppState, world_entity *Self, u32 Held)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Monster->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        if (Ability->Kind == MonsterAbility_Gaze && Monster->AbilityTimer <= BOT_GAZE_FREEZE &&
            LengthSq(Self->Position.XY - Monster->Position.XY) <=
            Square(Ability->Radius + BOT_DEEP_MARGIN))
        {
            Held &= ~(u32)(BOT_MOVE_KEYS | NetButton_Dash);
            break;
        }
    }
    return Held;
}
