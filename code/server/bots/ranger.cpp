/* Ranger bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Ranger bot keeps its distance like every ranged bot (BotThink holds
   it BOT_STRIKER_RANGE off its target). It fires Quick Shot on cooldown
   at the boss when there is one in the fight, else at what it fights,
   which keeps Hunter's Mark there; rains Volley on a pack or on the boss; looses
   Piercing Shot when two foes stand in its line or its Focus is near full; channels Rapid
   Fire on a big foe; and leaps away with Disengage when a foe gets close.
   It lets the tank start every fight, leaves a boss to the tank for the
   opening and whenever the boss has turned on it (shooting its adds
   meanwhile), and leaps only where it lands clear in the same room. */

// NOTE(zoubir): the living monsters within Radius of Point
// NOTE(zoubir): seconds into a boss fight before the bot spends its
// big shots, so the tank takes hold of the boss first
#define RANGER_BOT_OPENING 3.f
// NOTE(zoubir): how long it waits for its tank to pull before it goes
// in anyway (a tank that is down and nobody raises)
#define RANGER_BOT_MOST_WAIT 20.f

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

// NOTE(zoubir): whether Disengage, leaping back from Direction, lands
// clear: in the room it leaves from (a leap out of the fight's room
// leaves the fight) and over no hazard (lava, embers) on the way
internal bool32
BotRangerLeapIsSafe(app_state *AppState, world_entity *Self, v2 Direction)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Self->Position.XY);
    v2 Back = -1.f * Direction;
    for (u32 Step = 1; Step <= 6; ++Step)
    {
        v2 P = Self->Position.XY + (35.f * (float)Step) * Back;
        if (RoomAtPosition(World, P) != Room || IsHazardAt(World, V3(P.X, P.Y, 0.f)))
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): the tank of the party nearest Point, 0 for none; *Down
// says it is down or not back yet, which counts as far away
internal world_entity *
BotRangerTank(app_state *AppState, v2 Point, bool32 *Down)
{
    world_entity *Result = 0;
    *Down = false;
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || Slot->Role != PlayerRole_Tank)
        {
            continue;
        }
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Player)
        {
            *Down = true;
        }
        else if (!Result || Length(Player->Position.XY - Point) < Length(Result->Position.XY - Point))
        {
            Result = Player;
        }
    }
    if (Result)
    {
        *Down = false;
    }
    return Result;
}

// NOTE(zoubir): whether Kill Shot is worth its cooldown now: its marked
// foe in reach and nearly dead, or a boss it can only chip
internal bool32
BotRangerKillShot(app_state *AppState, player_slot *Slot, world_entity *Self)
{
    world_entity *Marked = RangerMarkedFoe(AppState, Slot);
    dungeon_run *Run = AppState->Dungeon;
    bool32 Result = Marked && Length(Marked->Position.XY - Self->Position.XY) < 0.95f * KILL_SHOT_RANGE &&
        Marked->MaxHp > 0.f && (Marked->Hp < KILL_SHOT_LOW * Marked->MaxHp ||
                                (Run->BossSerial && Marked->MonsterSerial == Run->BossSerial));
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
    // NOTE(zoubir): the tank pulls: out of a fight, a Ranger nearer the
    // foe it sees than its tank waits where it is, so it never starts a
    // fight (a boss's above all) alone
    bool32 TankDown = false;
    world_entity *Tank = BotRangerTank(AppState, Target->Position.XY, &TankDown);
    if (Run->FightingRoom)
    {
        Slot->Ranger.BotWaited = 0.f;
    }
    else if ((TankDown || (Tank && Length(Tank->Position.XY - Target->Position.XY) > Distance + 40.f)) &&
             Slot->Ranger.BotWaited < RANGER_BOT_MOST_WAIT)
    {
        Slot->Ranger.BotWaited += 1.f / (float)NET_TICK_RATE;
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        return 0;
    }
    // NOTE(zoubir): holding its range can leave it outside the room being
    // fought, past a doorway, where no shot of its reaches: it steps in
    u32 Room = RoomAtPosition(&AppState->World, Self->Position.XY);
    if (Run->FightingRoom && Room != Run->FightingRoom && Room != 0 &&
        RoomAtPosition(&AppState->World, Target->Position.XY) == Run->FightingRoom)
    {
        *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
        *Held |= NetButtonsToward(Direction);
    }
    bool32 CanLeap = Ready[2] && BotRangerLeapIsSafe(AppState, Self, Direction);

    // NOTE(zoubir): the mark goes on the boss of the fight, else on the target
    world_entity *Boss = Run->FightingRoom ?
        FindMonsterBySerial(&AppState->World, Run->BossSlot, Run->BossSerial) : 0;
    if (Boss && Boss->Hp <= 0.f)
    {
        Boss = 0;
    }
    // NOTE(zoubir): threat discipline, as a player plays it: the opening
    // seconds of a boss fight are the tank's to take hold of the boss, and
    // a boss that has turned on the Ranger is left alone until the tank
    // has it back; the Ranger shoots its adds meanwhile, and leaps away
    // when the boss comes close
    world_entity *BossAim = Boss ? FindMonsterTarget(AppState, &AppState->World, Boss, 0) : 0;
    bool32 Hold = Boss && (Run->MeterSeconds < RANGER_BOT_OPENING || BossAim == Self);
    if (Hold && BossAim == Self && CanLeap && Length(Boss->Position.XY - Self->Position.XY) < 160.f)
    {
        return NetButton_Slam;
    }
    world_entity *MarkOn = (Boss && Length(Boss->Position.XY - Self->Position.XY) < QUICK_SHOT_RANGE) ?
        Boss : Target;
    // NOTE(zoubir): close in: leap away, leaving the snare in its path
    if (CanLeap && Distance < 90.f && BotRandom(Bot) % 6 == 0)
    {
        return NetButton_Slam;
    }
    if (Hold && Target == Boss)
    {
        return 0;
    }
    // NOTE(zoubir): Volley on a pack, or on the boss alone, which is most
    // of a boss fight's damage race
    if (Ready[0] && !Hold && Distance < PLAYER_AIM_REACH &&
        (Target == Boss || BotRangerFoesNear(AppState, Target->Position.XY, VOLLEY_RADIUS) >= 2) &&
        BotRandom(Bot) % 8 == 0)
    {
        Result |= NetButton_Launch;
    }
    else if (Ready[1] && !Hold && Distance < 0.9f * PIERCE_RANGE &&
             (Slot->Ranger.Focus >= 85.f || BotRangerFoesInLine(AppState, Self, Direction) >= 2) &&
             BotRandom(Bot) % 6 == 0)
    {
        Result |= NetButton_Push;
    }
    else if (Ready[3] && !Hold && Distance < 0.9f * RAPID_FIRE_RANGE && Run->FightingRoom &&
             (Target == Boss || Target->Hp > 80.f) && BotRandom(Bot) % 20 == 0)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[4] && !Hold && BotRangerKillShot(AppState, Slot, Self))
    {
        Result |= NetButton_Shockwave;
    }
    else if (Ready[5] && Distance < QUICK_SHOT_RANGE)
    {
        // NOTE(zoubir): Quick Shot at the foe to keep marked when it is
        // in reach, to build Focus on it
        if (!(Hold && MarkOn == Boss) &&
            Length(MarkOn->Position.XY - Self->Position.XY) < QUICK_SHOT_RANGE)
        {
            *Pick = (u16)(MarkOn->ID + 1);
        }
        Result |= NetButton_Fireball;
    }
    return Result;
}
