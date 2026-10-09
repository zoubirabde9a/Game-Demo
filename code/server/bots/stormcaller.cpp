/* Stormcaller bots (server/bots.cpp, BotClassButtons): which of the class's
   keys a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Stormcaller bot keeps its distance like every ranged bot and waits as
   the Ranger bot does (server/bots/ranger.cpp, whose helpers it uses): it
   lets the tank start every fight and take hold of a boss, and leaves a
   boss that has turned on it alone, shooting its adds meanwhile. It
   rides the Charge: Spark is the filler; Static Field goes on a pack of
   three or more, or under the boss; Chain Lightning when two more foes
   stand near its target; Thunderclap vents from STORM_BOT_VENT Charge, and
   at once from STORM_BOT_VENT_NOW so it never caps, unless it took Live
   Wire and three foes stand inside the nova, when it lets the Charge
   overload on purpose. Lightning Dash takes it out of a telegraph it
   stands in when the way toward its target is clear; Eye of the Storm
   opens a boss fight once the tank has the boss. */

// NOTE(zoubir): seconds into a boss fight before the bot spends its big
// spells, so the tank takes hold of the boss first
#define STORM_BOT_OPENING 3.f
// NOTE(zoubir): how long it waits for its tank to pull before it goes
// in anyway
#define STORM_BOT_MOST_WAIT 20.f
// NOTE(zoubir): Thunderclap from this much Charge when the moment suits
// (a big foe, a few seconds into a fight), and from the second at once
#define STORM_BOT_VENT 75.f
#define STORM_BOT_VENT_NOW 88.f
// NOTE(zoubir): on a boss, where the Thunderclap's cooldown is the limit,
// it vents on cooldown from this much
#define STORM_BOT_VENT_BOSS 55.f
// NOTE(zoubir): how many foes make a pack for Static Field, and for a
// Live Wire overload on purpose
#define STORM_BOT_PACK 3

internal u32
BotStormcallerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                      float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and W are the Stormcaller's: the game's fireball and
    // shockwave presses from BotThink would cast them at random
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Shockwave | NetButton_Sword);
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
        Slot->Stormcaller.BotWaited = 0.f;
    }
    else if ((TankDown || (Tank && Length(Tank->Position.XY - Target->Position.XY) > Distance + 40.f)) &&
             Slot->Stormcaller.BotWaited < STORM_BOT_MOST_WAIT)
    {
        Slot->Stormcaller.BotWaited += 1.f / (float)NET_TICK_RATE;
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        return 0;
    }
    // NOTE(zoubir): step into the room being fought from past its door,
    // where no lightning of its reaches
    u32 Room = RoomAtPosition(&AppState->World, Self->Position.XY);
    if (Run->FightingRoom && Room != Run->FightingRoom && Room != 0 &&
        RoomAtPosition(&AppState->World, Target->Position.XY) == Run->FightingRoom)
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        *Held |= NetButtonsToward(Direction);
    }

    // NOTE(zoubir): standing in a telegraph: dash out when the dash toward
    // its target lands clear of it, in the same room and over no hazard
    if (Ready[2] && Run->FightingRoom)
    {
        bot_danger_circle Dangers[BOT_MAX_DANGERS];
        u32 DangerCount = BotDangers(AppState, Self, Dangers);
        v2 Landing = Self->Position.XY + LIGHTNING_DASH_LENGTH * Direction;
        if (DangerCount && BotDangerAt(Dangers, DangerCount, Self->Position.XY) &&
            !BotDangerAt(Dangers, DangerCount, Landing) &&
            RoomAtPosition(&AppState->World, Landing) == Room &&
            !IsHazardAt(&AppState->World, V3(Landing.X, Landing.Y, 0.f)))
        {
            return NetButton_Slam;
        }
    }

    world_entity *Boss = Run->FightingRoom ?
        FindMonsterBySerial(&AppState->World, Run->BossSlot, Run->BossSerial) : 0;
    if (Boss && Boss->Hp <= 0.f)
    {
        Boss = 0;
    }
    world_entity *BossAim = Boss ? FindMonsterTarget(AppState, &AppState->World, Boss, 0) : 0;
    bool32 Hold = Boss && (Run->MeterSeconds < STORM_BOT_OPENING || BossAim == Self);
    if (Hold && Target == Boss)
    {
        return 0;
    }
    float Charge = Slot->Stormcaller.Charge;
    // NOTE(zoubir): Spark and Thunderclap go at the boss when it is in
    // reach and the tank has it, as a player keeps on the boss through its
    // adds; the pack spells go at what it fights
    world_entity *Focus = Target;
    if (Boss && !Hold && Length(Boss->Position.XY - Self->Position.XY) < 0.95f * SPARK_RANGE)
    {
        Focus = Boss;
    }
    float FocusDistance = Length(Focus->Position.XY - Self->Position.XY);
    *Pick = (u16)(Focus->ID + 1);
    u32 Result = 0;
    // NOTE(zoubir): a pack round it with Live Wire: let the Charge cap
    // into the nova instead of venting it
    bool32 LiveWire = RoleRank(Slot, PlayerRole_Stormcaller, StormcallerTalent_LiveWire) > 0;
    bool32 Nova = LiveWire &&
        BotRangerFoesNear(AppState, Self->Position.XY, OVERLOAD_RADIUS * LIVE_WIRE_SCALE) >= STORM_BOT_PACK;
    bool32 Vent = !Nova && Charge >= THUNDERCLAP_MIN_CHARGE &&
        (Charge >= STORM_BOT_VENT_NOW ||
         (Charge >= STORM_BOT_VENT && (Focus->Hp > 60.f || BotRandom(Bot) % 4 == 0)) ||
         (Charge >= STORM_BOT_VENT_BOSS && Focus == Boss));
    u32 NearTarget = BotRangerFoesNear(AppState, Target->Position.XY, STORM_JUMP_RADIUS);
    if (Ready[4] && Vent && FocusDistance < 0.95f * THUNDERCLAP_RANGE)
    {
        Result |= NetButton_Shockwave;
    }
    else if (Ready[3] && Boss && !Hold && BotRandom(Bot) % 10 == 0)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[1] && !Hold && Distance < PLAYER_AIM_REACH &&
             (Target == Boss || BotRangerFoesNear(AppState, Target->Position.XY, STATIC_FIELD_RADIUS) >= STORM_BOT_PACK) &&
             BotRandom(Bot) % 6 == 0)
    {
        *Pick = (u16)(Target->ID + 1);
        Result |= NetButton_Push;
    }
    else if (Ready[6] && !Hold && Distance < BALL_LIGHTNING_SPEED * BALL_LIGHTNING_SECONDS &&
             (Target == Boss || NearTarget >= 2) && BotRandom(Bot) % 6 == 0)
    {
        // NOTE(zoubir): Ball Lightning rolled at what it fights, the aim on it
        Result |= NetButton_Sword;
    }
    else if (Ready[0] && Distance < 0.95f * CHAIN_RANGE && NearTarget >= 3 && BotRandom(Bot) % 4 == 0)
    {
        *Pick = (u16)(Target->ID + 1);
        Result |= NetButton_Launch;
    }
    else if (Ready[5] && FocusDistance < SPARK_RANGE)
    {
        Result |= NetButton_Fireball;
    }
    return Result;
}
