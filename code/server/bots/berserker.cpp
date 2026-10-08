/* Berserker bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Berserker bot fights in the melee: it walks onto the monster it
   fights, and onto its side or back when the monster's shell takes hits
   from the front (sim/monster_abilities/armor.cpp) while the tank holds
   it. It leaps in when the fight is a little way off, Cleaves on
   cooldown, throws its axe at what it cannot reach yet, spins a
   Whirlwind into a pack, Executes a foe near death or with a full bar,
   drinks with Bloodthirst when hurt, and goes Berserk in a fight. */

// NOTE(zoubir): how close the bot stands to what it fights
#define BOT_BERSERKER_GAP 46.f

// NOTE(zoubir): monsters within Radius of Self
internal u32
BotFoesAround(app_state *AppState, world_entity *Self, float Radius)
{
    world *World = &AppState->World;
    u32 Result = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Other = &World->Entities[Index];
        if (Other->IsPresent && Other->Type == EntityType_Monster && Other->Hp > 0.f &&
            LengthSq(Other->Position.XY - Self->Position.XY) < Square(Radius))
        {
            Result++;
        }
    }
    return Result;
}

internal u32
BotBerserkerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
              float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and the right click are the axe's here: the game's
    // fireball and sword presses the bot made would throw and swing it
    // at the wrong moments
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Sword);
    if (!Target || Target->Type != EntityType_Monster)
    {
        return 0;
    }
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key)
    {
        Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f && RoleSpellLearned(Slot, Key);
    }
    u32 Rage = Slot->ClassMeter;

    // NOTE(zoubir): onto the monster, behind its shell if it has one
    v2 Side = Distance > 0.001f ? -1.f * Direction : V2(1.f, 0.f);
    monster_def *Def = GetMonsterDef((monster_kind)Target->MonsterKind);
    if (Def->FrontArmor > 0.f && LengthSq(Target->Direction) > 0.0001f &&
        DotProduct(Side, Target->Direction) > -0.3f)
    {
        Side = -1.f * Target->Direction;
    }
    v2 ToWant = Target->Position.XY + BOT_BERSERKER_GAP * Side - Self->Position.XY;
    *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
    if (Length(ToWant) > 14.f)
    {
        *Held |= NetButtonsToward(DirectionTo(ToWant));
    }

    u32 Result = 0;
    if (IsPlayerCasting(Self))
    {
        return 0;
    }
    bool32 Fighting = AppState->Dungeon->FightingRoom != 0;
    bool32 Low = Target->MaxHp > 0.f && Target->Hp < EXECUTE_LOW_SHARE * Target->MaxHp;
    u32 Near = BotFoesAround(AppState, Self, WHIRLWIND_RADIUS);
    if (Ready[3] && Fighting && Distance < 160.f && Rage >= 30)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[4] && Distance < EXECUTE_REACH &&
             ((Low && Rage >= EXECUTE_MIN_RAGE) || Rage >= 70))
    {
        Result |= NetButton_Shockwave;
    }
    else if (Ready[1] && Near >= 2 && Rage >= WHIRLWIND_RAGE)
    {
        Result |= NetButton_Push;
    }
    else if (Ready[2] && Distance < BLOODTHIRST_REACH && (Self->Hp < 0.8f * Self->MaxHp ||
                                                        BotRandom(Bot) % 4 == 0))
    {
        Result |= NetButton_Slam;
    }
    else if (Ready[6] && Distance < CLEAVE_REACH + 10.f)
    {
        Result |= NetButton_Sword;
    }
    else if (Ready[0] && Distance > 150.f && Distance < 0.95f * PLAYER_AIM_REACH &&
             BotRandom(Bot) % 8 == 0)
    {
        Result |= NetButton_Launch;
    }
    else if (Ready[5] && Distance > 110.f && Distance < AXE_THROW_RANGE && BotRandom(Bot) % 10 == 0)
    {
        Result |= NetButton_Fireball;
    }
    return Result;
}
