/* Bot players: the server fills player slots no human is using with bots,
   so a lone player has someone to fight and online play can be tried with
   one client. A bot is a player slot whose input comes from BotThink
   instead of the network: it sends the same held buttons and aim a client
   would, through GameApplyInput, so every rule (cooldowns, damage, score)
   is the simulation's own. A human joining takes the slot over.

   The brain is small on purpose: chase the nearest living player or
   monster in sight, swing the sword up close, throw fireballs from range,
   dash now and then, and wander when nothing is near. It earns experience
   like anyone and spends each point on a random talent it may take, then
   uses Frost Nova, Shockwave and Gravity Well once it has them. In a
   dungeon run (sim/dungeon/) it fights only monsters, takes a role by
   its slot, so a lone player gets a tank, a healer and damage (and a
   bigger party more damage before a second healer or tank), and plays
   it (BotRoleButtons): the tank slams and taunts what is near and leaps
   to an ally being chased, the healer keeps out of melee and heals,
   wards and lays sanctuaries on whoever is hurt, the damage role drops
   infernos on what it fights. Included by sim_game.cpp; GameKeepBots
   runs it every tick. */

#define BOT_SIGHT 700.f
#define BOT_SWORD_RANGE 70.f
#define BOT_FIREBALL_RANGE 380.f
// NOTE(zoubir): how long a bot sits on a new talent point before spending it
#define BOT_LEARN_SECONDS 1.5f

// NOTE(zoubir): the movement keys a player would hold to go along
// Direction (8 ways)
inline u32
NetButtonsToward(v2 Direction)
{
    u32 Result = 0;
    if (Direction.X > 0.38f) Result |= NetButton_Right;
    if (Direction.X < -0.38f) Result |= NetButton_Left;
    if (Direction.Y > 0.38f) Result |= NetButton_Down;
    if (Direction.Y < -0.38f) Result |= NetButton_Up;
    return Result;
}

struct bot_brain
{
    bool32 Active;
    u32 Random;
    u32 Held;          // buttons held this tick
    float AttackWait;  // seconds until the next attack
    float WanderLeft;  // seconds until a new wander direction
    v2 Wander;
    float LearnWait;   // seconds until it spends a talent point it has
};

inline u32
BotRandom(bot_brain *Bot)
{
    u32 X = Bot->Random;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    Bot->Random = X;
    return X;
}

// NOTE(zoubir): the nearest living player (not itself) or monster within
// BOT_SIGHT, or 0
internal world_entity *
BotFindTarget(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    world_entity *Best = 0;
    float BestSq = Square(BOT_SIGHT);
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Other = &World->Entities[Index];
        if (!Other->IsPresent || Other == Self) continue;
        if (Other->Type != EntityType_Player && Other->Type != EntityType_Monster) continue;
        // NOTE(zoubir): a dungeon party fights the monsters together
        if (Other->Type == EntityType_Player && IsDungeon(AppState)) continue;
        if (IsDeadPlayer(Other)) continue;
        float DistanceSq = LengthSq(Other->Position.XY - Self->Position.XY);
        if (DistanceSq < BestSq)
        {
            BestSq = DistanceSq;
            Best = Other;
        }
    }
    return Best;
}

// NOTE(zoubir): a talent Slot may put a point into, picked at random, as
// the held buttons' talent field (net/protocol.h); 0 for none
internal u32
BotPickTalent(bot_brain *Bot, app_state *AppState, player_slot *Slot)
{
    u32 Result = 0;
    u32 Start = BotRandom(Bot) % Talent_Count;
    for (u32 Step = 0; Step < Talent_Count && !Result; ++Step)
    {
        u32 Talent = (Start + Step) % Talent_Count;
        // NOTE(zoubir): the role branch takes points only in a run
        if (IsRoleTalent(Talent) && !IsDungeon(AppState)) continue;
        if (CanLearnTalent(Slot, Talent) == TalentRefusal_None &&
            !RoleReplacesTalent(AppState, Slot, Talent))
        {
            Result = (Talent + 1) << NET_LEARN_SHIFT;
        }
    }
    return Result;
}

// NOTE(zoubir): the living ally (not Self) missing the largest share of
// its health within Range, or 0 when nobody is under Below of theirs
internal world_entity *
BotHurtAlly(app_state *AppState, world_entity *Self, float Range, float Below)
{
    world_entity *Result = 0;
    float Worst = Below;
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Ally || Ally->MaxHp <= 0.f ||
            Length(Ally->Position.XY - Self->Position.XY) > Range) continue;
        float Share = Ally->Hp / Ally->MaxHp;
        if (Share < Worst)
        {
            Worst = Share;
            Result = Ally;
        }
    }
    return Result;
}

// NOTE(zoubir): in a dungeon run, the role keys a bot presses this tick
// (sim/dungeon/role_abilities.cpp); *Held may lose its movement for a
// healer keeping its distance, and *Pick becomes the ally a spell goes to
internal u32
BotRoleButtons(bot_brain *Bot, app_state *AppState, world_entity *Self,
               world_entity *Target, float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    u32 Result = 0;
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key) Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f;
    if (Slot->Role == PlayerRole_Tank)
    {
        if (Target && Distance < SHIELD_SLAM_RADIUS && Ready[1] && BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_Shield;
        }
        if (Target && Distance < TAUNT_RADIUS * 0.8f && Ready[0] && BotRandom(Bot) % 30 == 0)
        {
            Result |= NetButton_Launch;
        }
        // NOTE(zoubir): an ally with monsters on them, too far to taunt off
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && Ready[2]; ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            float Gap = Ally ? Length(Ally->Position.XY - Self->Position.XY) : 0.f;
            if (Ally && Ally != Self && AppState->Players[SlotIndex].Aggro > 0 &&
                Gap > 150.f && Gap < INTERCEPT_RANGE && BotRandom(Bot) % 40 == 0)
            {
                Result |= NetButton_Kunai;
                *Pick = (u16)(Ally->ID + 1);
                break;
            }
        }
    }
    else if (Slot->Role == PlayerRole_Healer)
    {
        // NOTE(zoubir): out of the melee: back off what comes close
        if (Target && Distance < 180.f)
        {
            *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down |
                            NetButton_Sword);
            *Held |= NetButtonsToward(-Direction);
        }
        world_entity *Hurt = BotHurtAlly(AppState, Self, MENDING_BOLT_RANGE, 0.85f);
        if (Hurt && Ready[2] && BotRandom(Bot) % 6 == 0)
        {
            Result |= NetButton_Kunai;
            *Pick = (u16)(Hurt->ID + 1);
        }
        world_entity *Low = BotHurtAlly(AppState, Self, MENDING_BOLT_RANGE, 0.6f);
        if (Low && Ready[1] && !(Result & NetButton_Kunai) &&
            AppState->Players[Low->PlayerIndex].WardAbsorb <= 0.f && BotRandom(Bot) % 10 == 0)
        {
            Result |= NetButton_Shield;
            *Pick = (u16)(Low->ID + 1);
        }
        // NOTE(zoubir): nobody in danger: the ward goes on the striker for
        // its damage (role_kits/healer.cpp)
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && !Low && Ready[1] &&
             !(Result & (NetButton_Kunai | NetButton_Shield)); ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            player_slot *AllySlot = &AppState->Players[SlotIndex];
            if (Ally && AllySlot->Role == PlayerRole_Damage && AllySlot->WardAbsorb <= 0.f &&
                AppState->Dungeon->FightingRoom &&
                Length(Ally->Position.XY - Self->Position.XY) < MENDING_BOLT_RANGE &&
                BotRandom(Bot) % 10 == 0)
            {
                Result |= NetButton_Shield;
                *Pick = (u16)(Ally->ID + 1);
            }
        }
        // NOTE(zoubir): a sanctuary at its own feet when the party is hurt
        // round it
        u32 HurtNear = 0;
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            HurtNear += (Ally && Ally->Hp < 0.8f * Ally->MaxHp &&
                         Length(Ally->Position.XY - Self->Position.XY) < SANCTUARY_RADIUS) ? 1 : 0;
        }
        if (HurtNear >= 2 && Ready[0] && BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_Launch;
        }
    }
    else if (Target && Target->Type == EntityType_Monster)
    {
        // NOTE(zoubir): the striker's rotation (role_kits/striker.cpp):
        // the kunai builds Searing on what it fights, Detonate spends a
        // full mark, or one about to fade, and Inferno opens on a pack
        if (Distance > 80.f && Distance < PLAYER_AIM_REACH && Ready[0] &&
            BotRandom(Bot) % 25 == 0)
        {
            Result |= NetButton_Launch;
        }
        if (Distance < PlayerStats.KunaiRange && BotRandom(Bot) % 8 == 0)
        {
            Result |= NetButton_Kunai;
        }
        foe_mark *Mark = FindFoeMark(AppState->Dungeon, &AppState->World, Target);
        if (Mark && Mark->Stacks && Ready[1] && Distance < DETONATE_RANGE &&
            (Mark->Stacks >= SEARING_MOST || Mark->Seconds < 1.5f))
        {
            Result |= NetButton_Shield;
        }
    }
    return Result;
}

// NOTE(zoubir): one tick of a bot's thinking, as the input a client would send
internal net_input
BotThink(bot_brain *Bot, app_state *AppState, world_entity *Self, u32 Tick, float Dt)
{
    net_input Input = {};
    Input.Tick = Tick;
    // Buttons are pressed for one tick, then let go, so each one fires once.
    u32 Held = 0;
    Bot->AttackWait -= Dt;
    Bot->WanderLeft -= Dt;

    v2 Direction = {};
    // NOTE(zoubir): the aim's length, as a player's cursor gives it: the
    // cursor on the target, so the kunai picks it and Launch lands on it
    float AimReach = 1.f;
    world_entity *Target = Self ? BotFindTarget(AppState, Self) : 0;
    if (Target)
    {
        v2 Delta = Target->Position.XY - Self->Position.XY;
        float Distance = Length(Delta);
        Direction = (Distance > 0.001f) ? Delta * (1.f / Distance) : V2(1.f, 0.f);
        AimReach = Minimum(1.f, Maximum(0.05f, Distance / PLAYER_AIM_REACH));
        if (Distance > BOT_SWORD_RANGE * 0.6f) Held |= NetButtonsToward(Direction);
        if (Bot->AttackWait <= 0.f)
        {
            if (Distance < BOT_SWORD_RANGE)
            {
                Held |= NetButton_Sword;
                Bot->AttackWait = 0.4f + (BotRandom(Bot) % 40) / 100.f;
            }
            else if (Distance < BOT_FIREBALL_RANGE)
            {
                Held |= NetButton_Fireball;
                Bot->AttackWait = 0.9f + (BotRandom(Bot) % 60) / 100.f;
            }
        }
        if (BotRandom(Bot) % 400 == 0) Held |= NetButton_Dash;
        // NOTE(zoubir): a kunai now and then from mid range; it homes
        if (Distance > BOT_SWORD_RANGE && Distance < 500.f && BotRandom(Bot) % 120 == 0)
        {
            Held |= NetButton_Kunai;
        }
        // NOTE(zoubir): badly hurt, a bot may rewind itself to before the
        // hits; up close, now and then, it rewinds the fight around it.
        // Never the whole world: on a live server that is the players' call
        if (Self->Hp < 0.35f * Self->MaxHp && BotRandom(Bot) % 60 == 0)
        {
            Held |= NetButton_RewindSelf;
        }
        if (Distance < BOT_SWORD_RANGE * 2.f && BotRandom(Bot) % 900 == 0)
        {
            Held |= NetButton_RewindBubble;
        }
        // NOTE(zoubir): the talent tree's area spells, which do nothing
        // until it has them
        if (Distance < 110.f && BotRandom(Bot) % 90 == 0)
        {
            Held |= (BotRandom(Bot) & 1) ? NetButton_FrostNova : NetButton_Shockwave;
        }
        if (Distance > 120.f && Distance < 260.f && BotRandom(Bot) % 150 == 0)
        {
            Held |= NetButton_GravityWell;
        }
    }
    else
    {
        if (Bot->WanderLeft <= 0.f)
        {
            u32 Pick = BotRandom(Bot) % 5;
            v2 Ways[] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            Bot->Wander = Ways[Pick];
            Bot->WanderLeft = 1.5f + (BotRandom(Bot) % 150) / 100.f;
        }
        Direction = Bot->Wander;
        Held |= NetButtonsToward(Direction);
    }

    // A point to spend goes into the talent field for one tick; the field
    // is empty the next, so the server sees each as new.
    player_slot *Slot = Self ? &AppState->Players[Self->PlayerIndex] : 0;
    Bot->LearnWait -= Dt;
    if (Slot && TalentPointsLeft(Slot) > 0 && Bot->LearnWait <= 0.f &&
        !(Bot->Held >> NET_LEARN_SHIFT))
    {
        Held |= BotPickTalent(Bot, AppState, Slot);
        Bot->LearnWait = BOT_LEARN_SECONDS;
    }

    // A map vote gets a yes, held until the server counts it, so bots
    // never stand in the way of the players (sim/map_vote.cpp).
    if (Slot && AppState->VoteOpen &&
        AppState->Votes[Self->PlayerIndex] == MapVote_None)
    {
        Held |= (u32)MapVote_Yes << NET_VOTE_SHIFT;
    }

    // In a dungeon run each bot asks for a role by its slot until it has
    // it, one tick at a time like a talent: a tank, a healer and damage
    // first, then more damage, as a party past three needs it to keep up
    // with the boss's clock, a second healer at six and a second tank at
    // eight.
    if (Slot && IsDungeon(AppState) && !AppState->Dungeon->FightingRoom &&
        !(Bot->Held >> NET_ROLE_SHIFT))
    {
        u32 Wanted[] = {PlayerRole_Tank, PlayerRole_Healer, PlayerRole_Damage, PlayerRole_Damage,
                        PlayerRole_Damage, PlayerRole_Healer, PlayerRole_Damage, PlayerRole_Tank};
        u32 Role = Wanted[Self->PlayerIndex % ArrayCount(Wanted)];
        if (Slot->Role != Role)
        {
            Held |= (Role + 1) << NET_ROLE_SHIFT;
        }
    }

    // NOTE(zoubir): its cursor on the unit it chases, as a player's would
    // be, or on the ally a role spell goes to
    u16 Pick = Target ? (u16)(Target->ID + 1) : 0;
    if (Self && IsDungeon(AppState) && !IsDeadPlayer(Self))
    {
        float Distance = Target ? Length(Target->Position.XY - Self->Position.XY) : 0.f;
        Held &= ~(u32)NetButton_Kunai;
        Held |= BotRoleButtons(Bot, AppState, Self, Target, Distance, Direction, &Held, &Pick);
    }

    // A press needs the button up the tick before; drop repeats.
    Held &= ~(Bot->Held & (NetButton_Sword | NetButton_Fireball | NetButton_Dash |
                           NetButton_RewindSelf | NetButton_RewindBubble |
                           NetButton_FrostNova | NetButton_Shockwave |
                           NetButton_GravityWell | NetButton_Kunai |
                           NetButton_Launch | NetButton_Shield));
    Bot->Held = Held;
    Input.Buttons = Held;
    Input.AimX = AimReach * Direction.X;
    Input.AimY = AimReach * Direction.Y;
    Input.Target = Pick;
    return Input;
}
