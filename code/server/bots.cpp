/* Bot players: the server fills player slots no human is using with bots,
   so a lone player has someone to fight and online play can be tried with
   one client. A bot is a player slot whose input comes from BotThink
   instead of the network: it sends the same held buttons and aim a client
   would, through GameApplyInput, so every rule (cooldowns, damage, score)
   is the simulation's own. A human joining takes the slot over.

   The brain is small on purpose: chase the nearest living player or
   monster in sight (in a team duel, never a teammate, sim/teams/), swing the sword up close, throw fireballs from range,
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
// NOTE(zoubir): how far from its target a striker bot holds in a dungeon
#define BOT_STRIKER_RANGE 220.f
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
    u8 HeldRole;       // and the role byte (net_input.Role)
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

// NOTE(zoubir): how much nearer an Ice Tomb looks to a bot than it is
#define BOT_TOMB_PULL 0.05f

// NOTE(zoubir): the nearest living player (not itself) or monster within
// BOT_SIGHT, or 0; a boss behind its pylons is left to the tank, and a
// monster behind a mirror to nobody
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
        // NOTE(zoubir): a dungeon party fights the monsters together, and
        // a team its foes (sim/teams/)
        if (Other->Type == EntityType_Player && IsDungeon(AppState)) continue;
        if (Other->Type == EntityType_Player &&
            AreTeammates(AppState, Self->PlayerIndex, Other->PlayerIndex)) continue;
        if (IsDeadPlayer(Other)) continue;
        // NOTE(zoubir): nor at a boss away from its fight, nor what falls
        // while it is gone (sim/dungeon/boss_departures.cpp)
        if (Other->Type == EntityType_Monster && IsOutOfReach(AppState, Other)) continue;
        if (Other->Type == EntityType_Monster && IsWardedBoss(AppState, Other) &&
            AppState->Players[Self->PlayerIndex].Role != PlayerRole_Tank) continue;
        // NOTE(zoubir): nobody hits a mirror, or one about to rise
        if (Other->Type == EntityType_Monster && IsRaisingMirror(Other)) continue;
        // NOTE(zoubir): a Frost Mark cannot be hurt; an Ice Tomb holds a
        // friend, so it counts as near as can be (sim/dungeon/frost_tombs.cpp)
        if (IsFrostMark(Other)) continue;
        float DistanceSq = LengthSq(Other->Position.XY - Self->Position.XY);
        if (Other->Type == EntityType_Monster && Other->MonsterKind == MonsterKind_IceTomb)
        {
            DistanceSq *= BOT_TOMB_PULL;
        }
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
    // NOTE(zoubir): in a run, a spell of each branch's pair first
    // (sim/dungeon/class_tree.cpp), so a bot casts its four spells as soon
    // as it can; which of the two is the bot's coin toss, so a party of
    // bots plays both builds of every class
    for (u32 Branch = 0; Branch < 2 && IsDungeon(AppState) && !Result; ++Branch)
    {
        u32 Talent = ClassTreeTalent(Branch * ROLE_TALENTS + 2 + BotRandom(Bot) % 2);
        if (!Slot->Ranks[Talent] && CanLearnTalent(Slot, Talent) == TalentRefusal_None)
        {
            Result = (Talent + 1) << NET_LEARN_SHIFT;
        }
    }
    u32 Start = BotRandom(Bot) % Talent_Count;
    for (u32 Step = 0; Step < Talent_Count && !Result; ++Step)
    {
        u32 Talent = (Start + Step) % Talent_Count;
        // NOTE(zoubir): the role branch takes points only in a run
        if (IsClassTalent(Talent) && !IsDungeon(AppState)) continue;
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

#include "bots/healer_footwork.cpp"
#include "bots/class_bots.cpp"
#include "bots/hazard_steer.cpp"

#include "bots/role_buttons.cpp"

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
    else if (Self && BotWalkToNextRoom(AppState, Self, &Direction))
    {
        Held |= NetButtonsToward(Direction);
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
    if (Slot && TalentPointsToSpend(Slot, IsDungeon(AppState)) > 0 && Bot->LearnWait <= 0.f &&
        !(Bot->Held >> NET_LEARN_SHIFT) && !Bot->HeldRole)
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
    u8 RoleAsked = 0;
    if (Slot && IsDungeon(AppState) && !AppState->Dungeon->FightingRoom && !Bot->HeldRole)
    {
        u32 Role = BotWantedRole[Self->PlayerIndex % MAX_PLAYERS];
        if (Role == PlayerRole_Damage)
        {
            Role = BotDamageClass(AppState, Self->PlayerIndex);
        }
        else if (Role == PlayerRole_Healer)
        {
            Role = BotHealerClass(AppState, Self->PlayerIndex);
        }
        if (Slot->Role != Role)
        {
            RoleAsked = (u8)(Role + 1);
        }
    }

    // NOTE(zoubir): its cursor on the unit it chases, as a player's would
    // be, or on the ally a role spell goes to
    u16 Pick = Target ? (u16)(Target->ID + 1) : 0;
    if (Self && IsDungeon(AppState) && !IsDeadPlayer(Self))
    {
        float Distance = Target ? Length(Target->Position.XY - Self->Position.XY) : 0.f;
        // NOTE(zoubir): A, R, C and V are the class's keys in a run
        // (BotRoleButtons); the game's presses on them go
        Held &= ~(u32)(NetButton_Kunai | NetButton_Launch | NetButton_Push | NetButton_Slam |
                       NetButton_FrostNova | NetButton_GravityWell);
        // NOTE(zoubir): the striker fights from range and, against a
        // monster whose shell takes hits from the front (sim/monster_
        // abilities/armor.cpp), from behind it, as the tank holds it
        player_slot *Mine = &AppState->Players[Self->PlayerIndex];
        if (RoleKindOf(Mine->Role) == RoleKind_Ranged && Target && Target->Type == EntityType_Monster)
        {
            v2 Away = (Distance > 0.001f) ? -1.f * Direction : V2(1.f, 0.f);
            v2 Side = Away;
            monster_def *Def = GetMonsterDef((monster_kind)Target->MonsterKind);
            if (Def->FrontArmor > 0.f && LengthSq(Target->Direction) > 0.0001f &&
                DotProduct(Away, Target->Direction) > -0.3f)
            {
                Side = -1.f * Target->Direction;
            }
            v2 Want = Target->Position.XY + BOT_STRIKER_RANGE * Side;
            v2 ToWant = Want - Self->Position.XY;
            Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down |
                           NetButton_Sword);
            if (Length(ToWant) > 40.f)
            {
                Held |= NetButtonsToward(DirectionTo(ToWant));
            }
        }
        Held |= BotRoleButtons(Bot, AppState, Self, Target, Distance, Direction, &Held, &Pick);
    }

    if (Self && !IsDeadPlayer(Self))
    {
        Held = DodgeDeepDangers(AppState, Self, Held);
        Held = DodgeDangers(AppState, Self, Held);
        Held = DodgeRiftDangers(AppState, Self, Held);
        Held = DodgeStarlessDangers(AppState, Self, Held);
        Held = FreezeForGazes(AppState, Self, Held);
        Held = SteerAroundHazards(AppState, Self, Held, Target);
    }
    // A press needs the button up the tick before; drop repeats.
    Held &= ~(Bot->Held & (NetButton_Sword | NetButton_Fireball | NetButton_Dash |
                           NetButton_RewindSelf | NetButton_RewindBubble |
                           NetButton_FrostNova | NetButton_Shockwave |
                           NetButton_GravityWell | NetButton_Kunai |
                           NetButton_Launch | NetButton_Shield |
                           NetButton_Push | NetButton_Slam));
    Bot->Held = Held;
    Bot->HeldRole = RoleAsked;
    Input.Buttons = Held;
    // NOTE(zoubir): marked as a bot's, for team duels (sim/teams/)
    Input.Role = (u8)(RoleAsked | NET_ROLE_BOT);
    Input.AimX = AimReach * Direction.X;
    Input.AimY = AimReach * Direction.Y;
    Input.Target = Pick;
    return Input;
}
