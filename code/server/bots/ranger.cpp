/* Ranger bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Ranger bot keeps its distance like every ranged bot (BotThink holds
   it BOT_STRIKER_RANGE off its target). It keeps Hunter's Mark on the
   boss when there is one in the fight, else on what it fights; fires
   Quick Shot on cooldown; rains Volley on a pack; looses Piercing Shot
   when two foes stand in its line or its Focus is high; channels Rapid
   Fire on a big foe; and leaps away with Disengage when a foe gets close. */

// NOTE(zoubir): the living monsters within Radius of Point
internal u32
BotRangerFoesNear(app_state *AppState, v2 Point, float Radius)
{
    world *World = &AppState->World;
    u32 Result = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Monster = &World->Entities[Index];
        Result += (Monster->IsPresent && Monster->Type == EntityType_Monster && Monster->Hp > 0.f &&
                   Length(Monster->Position.XY - Point) <= Radius) ? 1 : 0;
    }
    return Result;
}

// NOTE(zoubir): the living monsters a Piercing Shot from Self along Dir
// would go through
internal u32
BotRangerFoesInLine(app_state *AppState, world_entity *Self, v2 Dir)
{
    world *World = &AppState->World;
    u32 Result = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f)
        {
            continue;
        }
        v2 Offset = Monster->Position.XY - Self->Position.XY;
        float Along = DotProduct(Offset, Dir);
        float Across = Absolute(DotProduct(Offset, V2(-Dir.Y, Dir.X)));
        Result += (Along > 0.f && Along < PIERCE_RANGE &&
                   Across < PIERCE_WIDTH + 0.5f * Monster->Dimensions.X) ? 1 : 0;
    }
    return Result;
}

internal u32
BotRangerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
              float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and W are the Ranger's: the game's fireball and
    // shockwave presses from BotThink would cast them at random
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Shockwave);
    if (!Target || Target->Type != EntityType_Monster || IsPlayerCasting(Self))
    {
        return 0;
    }
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key)
    {
        Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f && RoleSpellLearned(Slot, Key);
    }
    dungeon_run *Run = AppState->Dungeon;
    u32 Result = 0;

    // NOTE(zoubir): the mark goes on the boss of the fight, else on the target
    world_entity *Boss = Run->FightingRoom ?
        FindMonsterBySerial(&AppState->World, Run->BossSlot, Run->BossSerial) : 0;
    world_entity *MarkOn = (Boss && Boss->Hp > 0.f &&
                            Length(Boss->Position.XY - Self->Position.XY) < 0.9f * MARK_RANGE) ? Boss : Target;
    if (Ready[4] && !IsRangerMarked(AppState, Slot, MarkOn) &&
        Length(MarkOn->Position.XY - Self->Position.XY) < 0.9f * MARK_RANGE)
    {
        *Pick = (u16)(MarkOn->ID + 1);
        return NetButton_Shockwave;
    }
    // NOTE(zoubir): close in: leap away, leaving the snare in its path
    if (Ready[2] && Distance < 90.f && BotRandom(Bot) % 6 == 0)
    {
        return NetButton_Slam;
    }
    if (Ready[0] && Distance < PLAYER_AIM_REACH &&
        BotRangerFoesNear(AppState, Target->Position.XY, VOLLEY_RADIUS) >= 2 && BotRandom(Bot) % 8 == 0)
    {
        Result |= NetButton_Launch;
    }
    else if (Ready[1] && Distance < 0.9f * PIERCE_RANGE &&
             (Slot->Ranger.Focus >= 70.f || BotRangerFoesInLine(AppState, Self, Direction) >= 2) &&
             BotRandom(Bot) % 6 == 0)
    {
        Result |= NetButton_Push;
    }
    else if (Ready[3] && Distance < 0.9f * RAPID_FIRE_RANGE && Run->FightingRoom &&
             (Target == Boss || Target->Hp > 80.f) && BotRandom(Bot) % 20 == 0)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[5] && Distance < QUICK_SHOT_RANGE)
    {
        // NOTE(zoubir): Quick Shot at the marked foe when it is in reach,
        // to build Focus
        if (IsRangerMarked(AppState, Slot, MarkOn) &&
            Length(MarkOn->Position.XY - Self->Position.XY) < QUICK_SHOT_RANGE)
        {
            *Pick = (u16)(MarkOn->ID + 1);
        }
        Result |= NetButton_Fireball;
    }
    return Result;
}
