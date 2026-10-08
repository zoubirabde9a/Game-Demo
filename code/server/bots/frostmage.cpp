/* Frost Mage bots (server/bots.cpp, BotClassButtons): which of the class's
   keys a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Frost Mage bot keeps its distance like every ranged bot and plays as
   the Ranger bot does (server/bots/ranger.cpp, whose helpers it uses): it
   lets the tank start every fight and take hold of a boss, and leaves a
   boss that has turned on it alone. It casts Frostbolt on cooldown, a
   Glacial Spike on five Icicles or on a frozen foe, Blizzard on a pack or
   the boss, Frozen Orb at a pack, Frost Nova when foes reach it (then
   steps back from them while they are held), and Ice Barrier when it is
   hurt or the boss turns on it. */

// NOTE(zoubir): seconds into a boss fight before the bot spends its big
// spells, so the tank takes hold of the boss first
#define FROSTMAGE_BOT_OPENING 3.f
// NOTE(zoubir): how long it waits for its tank to pull before it goes
// in anyway
#define FROSTMAGE_BOT_MOST_WAIT 20.f

internal u32
BotFrostMageButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                    float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and W are the Frost Mage's: the game's fireball and
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
    // NOTE(zoubir): the tank pulls (as the Ranger bot waits)
    bool32 TankDown = false;
    world_entity *Tank = BotRangerTank(AppState, Target->Position.XY, &TankDown);
    if (Run->FightingRoom)
    {
        Slot->FrostMage.BotWaited = 0.f;
    }
    else if ((TankDown || (Tank && Length(Tank->Position.XY - Target->Position.XY) > Distance + 40.f)) &&
             Slot->FrostMage.BotWaited < FROSTMAGE_BOT_MOST_WAIT)
    {
        Slot->FrostMage.BotWaited += 1.f / (float)NET_TICK_RATE;
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        return 0;
    }
    // NOTE(zoubir): step into the room being fought from past its door
    u32 Room = RoomAtPosition(&AppState->World, Self->Position.XY);
    if (Run->FightingRoom && Room != Run->FightingRoom && Room != 0 &&
        RoomAtPosition(&AppState->World, Target->Position.XY) == Run->FightingRoom)
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        *Held |= NetButtonsToward(Direction);
    }

    world_entity *Boss = Run->FightingRoom ?
        FindMonsterBySerial(&AppState->World, Run->BossSlot, Run->BossSerial) : 0;
    if (Boss && Boss->Hp <= 0.f)
    {
        Boss = 0;
    }
    world_entity *BossAim = Boss ? FindMonsterTarget(AppState, &AppState->World, Boss, 0) : 0;
    bool32 Hold = Boss && (Run->MeterSeconds < FROSTMAGE_BOT_OPENING || BossAim == Self);

    // NOTE(zoubir): hurt, or the boss on it: the barrier first
    if (Ready[2] && Slot->FireguardAbsorb <= 0.f && Run->FightingRoom &&
        (Self->Hp < 0.6f * Self->MaxHp || BossAim == Self))
    {
        return NetButton_Slam;
    }
    // NOTE(zoubir): foes at its robes: freeze them, then back off while
    // they are held
    u32 Close = BotRangerFoesNear(AppState, Self->Position.XY, 0.8f * FROST_NOVA_RADIUS);
    if (Ready[4] && Close >= 1 && Run->FightingRoom)
    {
        return NetButton_Shockwave;
    }
    if (Distance < 110.f && IsFrozenFoe(Target))
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down |
                        NetButton_Sword);
        *Held |= NetButtonsToward(-Direction);
    }
    if (Hold && Target == Boss)
    {
        return 0;
    }
    u32 Result = 0;
    bool32 Pack = BotRangerFoesNear(AppState, Target->Position.XY, BLIZZARD_RADIUS) >= 2;
    if (Ready[1] && !Hold && Distance < 0.9f * GLACIAL_SPIKE_RANGE &&
        (Slot->FrostMage.Icicles >= FROSTMAGE_ICICLES_MOST ||
         (Slot->FrostMage.Icicles >= 3 && IsFrozenFoe(Target))))
    {
        Result |= NetButton_Push;
        *Pick = (u16)(Target->ID + 1);
    }
    else if (Ready[0] && !Hold && Distance < PLAYER_AIM_REACH && (Target == Boss || Pack) &&
             BotRandom(Bot) % 8 == 0)
    {
        Result |= NetButton_Launch;
    }
    else if (Ready[3] && !Hold && Distance < FROZEN_ORB_SPEED * FROZEN_ORB_SECONDS && Pack &&
             BotRandom(Bot) % 10 == 0)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[5] && Distance < FROSTBOLT_RANGE)
    {
        *Pick = (u16)(Target->ID + 1);
        Result |= NetButton_Fireball;
    }
    return Result;
}
