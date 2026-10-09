/* Berserker bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Berserker bot fights in the melee on the tank's monster when the tank
   holds one (the one nearest the tank), else on what it found itself,
   and stands at its back: a monster faces what it attacks, so behind it
   is away from the tank and out of its swings, and a shelled monster's
   armour does not turn the axe there (sim/monster_abilities/armor.cpp).
   Its swings go at that monster (*Pick, which Cleave follows).

   It keeps out of what monsters telegraph and of burning ground
   (bot_dangers.cpp): its spot is pushed out of every danger circle, and
   while it stands in one it walks straight out pressing nothing.
   It leaps in when its fight is a way off and the landing is clear,
   Cleaves on cooldown, spins a Whirlwind into a pack, Executes a foe
   near death or with a full bar, or sooner when hurt and Bloodthirst
   heals it, and goes Berserk in a fight. */

// NOTE(zoubir): how far behind its monster the bot stands, past the
// monster's half width; how close to the tank a monster is the tank's
#define BOT_BERSERKER_GAP 22.f
#define BOT_BERSERKER_TANK_REACH 220.f

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

// NOTE(zoubir): the monster nearest a living tank of the party, if one is
// close to it and in Self's room; 0 for none
internal world_entity *
BotBerserkerTankFoe(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Self->Position.XY);
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        world_entity *Tank = LivingPlayerInSlot(AppState, SlotIndex);
        if (Tank && Tank != Self && RoleKindOf(AppState->Players[SlotIndex].Role) == RoleKind_Tank)
        {
            world_entity *Foe = NearestFoe(World, Tank->Position.XY, BOT_BERSERKER_TANK_REACH, Room, 0, 0);
            if (Foe)
            {
                return Foe;
            }
        }
    }
    return 0;
}

// NOTE(zoubir): the way to walk toward a spot ToWant away without
// crossing a danger circle (burning ground sets a player alight, which
// kills whatever its health): when the straight way enters one it is not
// already in, round its edge on the side nearer the spot
internal v2
BotBerserkerWay(bot_danger_circle *Dangers, u32 Count, v2 From, v2 ToWant)
{
    v2 Way = DirectionTo(ToWant);
    float Ahead = Minimum(Length(ToWant), 40.f);
    bot_danger_circle *Danger = BotDangerAt(Dangers, Count, From + Ahead * Way);
    if (Danger && !BotDangerAt(Dangers, Count, From))
    {
        v2 Out = NormalizeOr(From - Danger->Centre, -1.f * Way);
        v2 Round = V2(-Out.Y, Out.X);
        if (DotProduct(Round, Way) < 0.f)
        {
            Round = -1.f * Round;
        }
        Way = NormalizeOr(Round + 0.35f * Out, Round);
    }
    return Way;
}

internal u32
BotBerserkerButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
              float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): the right click is the axe's here and X does nothing:
    // the game's fireball and sword presses the bot made would swing it
    // at the wrong moments
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Sword);
    if (!Target || Target->Type != EntityType_Monster)
    {
        return 0;
    }
    world_entity *Foe = BotBerserkerTankFoe(AppState, Self);
    if (!Foe || Length(Foe->Position.XY - Self->Position.XY) > PLAYER_AIM_REACH)
    {
        Foe = Target;
    }
    v2 ToFoe = Foe->Position.XY - Self->Position.XY;
    Distance = Length(ToFoe);
    *Pick = (u16)(Foe->ID + 1);
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key)
    {
        Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f && RoleSpellLearned(Slot, Key);
    }
    u32 Rage = Slot->ClassMeter;

    // NOTE(zoubir): to the monster's back, out of every danger circle
    v2 Back = LengthSq(Foe->Direction) > 0.0001f ? -1.f * DirectionTo(Foe->Direction) :
        (Distance > 0.001f ? -1.f * DirectionTo(ToFoe) : V2(1.f, 0.f));
    float HalfWidth = 0.5f * Foe->Dimensions.X;
    bot_danger_circle Dangers[BOT_MAX_DANGERS];
    u32 DangerCount = BotDangers(AppState, Self, Dangers);
    v2 Want = BotSafeSpot(Dangers, DangerCount, Foe->Position.XY + (BOT_BERSERKER_GAP + HalfWidth) * Back,
                          Back);
    v2 ToWant = Want - Self->Position.XY;
    *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
    if (Length(ToWant) > 12.f)
    {
        *Held |= NetButtonsToward(BotBerserkerWay(Dangers, DangerCount, Self->Position.XY, ToWant));
    }

    if (IsPlayerCasting(Self))
    {
        return 0;
    }
    u32 Result = 0;
    float Reach = CLEAVE_REACH + HalfWidth;
    // NOTE(zoubir): in harm's way: out first
    if (BotDangerAt(Dangers, DangerCount, Self->Position.XY))
    {
        return Result;
    }
    bool32 Fighting = AppState->Dungeon->FightingRoom != 0;
    bool32 Low = Foe->MaxHp > 0.f && Foe->Hp < EXECUTE_LOW_SHARE * Foe->MaxHp;
    u32 Near = BotFoesAround(AppState, Self, WHIRLWIND_RADIUS);
    bool32 Thirsty = RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_Bloodthirst) > 0;
    if (Ready[2] && Fighting && Distance < 2.f * Reach && Rage < 30)
    {
        // NOTE(zoubir): Battle Shout: Rage to full and the party near
        // hits harder, so in the thick of it with the Rage spent
        Result |= NetButton_Slam;
    }
    else if (Ready[3] && Fighting && Distance < 2.f * Reach && Rage >= 30)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[4] && Distance < EXECUTE_REACH + HalfWidth &&
             ((Low && Rage >= EXECUTE_MIN_RAGE) || Rage >= 70 ||
              (Thirsty && Self->Hp < 0.7f * Self->MaxHp && Rage >= 40)))
    {
        Result |= NetButton_Shockwave;
    }
    else if (Ready[1] && Near >= 2 && Rage >= WHIRLWIND_RAGE)
    {
        Result |= NetButton_Push;
    }
    else if (Ready[6] && Distance < Reach)
    {
        Result |= NetButton_Sword;
    }
    else if (Ready[0] && Foe == Target && Distance > 150.f && Distance < 0.95f * PLAYER_AIM_REACH &&
             !BotDangerAt(Dangers, DangerCount, Foe->Position.XY) && BotRandom(Bot) % 8 == 0)
    {
        // NOTE(zoubir): the aim is on Target (BotThink), so the leap lands there
        Result |= NetButton_Launch;
    }
    return Result;
}
