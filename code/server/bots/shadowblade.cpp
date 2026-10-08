/* Shadowblade bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Shadowblade bot fights the tank's monster when the tank holds one
   (the one nearest the tank), else what it found itself, and stands at
   its back: a monster faces what it attacks, so behind it is away from
   the tank and out of its swings, and from behind a shelled monster's
   armour does not turn the cuts (sim/monster_abilities/armor.cpp). It
   Shadowsteps in when the fight is a way off, poisons what is not
   poisoned yet, Twin Strikes in reach, throws Fan of Knives into a
   pack, Eviscerates at four or five points, dances in a fight and drops
   a smoke bomb when hurt with monsters after it. */

// NOTE(zoubir): how far behind its monster the bot stands, past the
// monster's half width
#define BOT_SHADOWBLADE_GAP 26.f
// NOTE(zoubir): how close to the tank a monster is the tank's
#define BOT_SHADOWBLADE_TANK_REACH 220.f

// NOTE(zoubir): living monsters within Radius of Self
internal u32
BotShadowbladeFoesNear(app_state *AppState, world_entity *Self, float Radius)
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
BotShadowbladeTankFoe(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Self->Position.XY);
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        world_entity *Tank = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Tank || Tank == Self || RoleKindOf(AppState->Players[SlotIndex].Role) != RoleKind_Tank)
        {
            continue;
        }
        world_entity *Foe = NearestFoe(World, Tank->Position.XY, BOT_SHADOWBLADE_TANK_REACH, Room, 0, 0);
        if (Foe)
        {
            return Foe;
        }
    }
    return 0;
}

internal u32
BotShadowbladeButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
              float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and the right click are the daggers' here: the
    // game's fireball and sword presses would throw and cut at the wrong
    // moments
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Sword);
    if (!Target || Target->Type != EntityType_Monster)
    {
        return 0;
    }
    world_entity *Foe = BotShadowbladeTankFoe(AppState, Self);
    if (!Foe || Length(Foe->Position.XY - Self->Position.XY) > SHADOWSTEP_RANGE)
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
    u32 Points = Slot->ClassMeter;

    // NOTE(zoubir): to the monster's back, the way it faces being toward
    // whoever it attacks; round the side when that is the bot itself
    v2 Back = LengthSq(Foe->Direction) > 0.0001f ? -1.f * DirectionTo(Foe->Direction) :
        (Distance > 0.001f ? -1.f * DirectionTo(ToFoe) : V2(1.f, 0.f));
    float Gap = BOT_SHADOWBLADE_GAP + 0.5f * Foe->Dimensions.X;
    v2 ToWant = Foe->Position.XY + Gap * Back - Self->Position.XY;
    *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
    if (Length(ToWant) > 12.f)
    {
        *Held |= NetButtonsToward(DirectionTo(ToWant));
    }

    if (IsPlayerCasting(Self))
    {
        return 0;
    }
    u32 Result = 0;
    bool32 Fighting = AppState->Dungeon->FightingRoom != 0;
    float Reach = TWIN_STRIKE_REACH + 0.5f * Foe->Dimensions.X;
    u32 Near = BotShadowbladeFoesNear(AppState, Self, FAN_OF_KNIVES_RADIUS);
    if (Ready[2] && Slot->Aggro > 0 && Self->Hp < 0.55f * Self->MaxHp)
    {
        Result |= NetButton_Slam;
    }
    else if (Ready[3] && Fighting && Distance < 2.f * Reach && Points >= 2)
    {
        Result |= NetButton_Kunai;
    }
    else if (Ready[4] && Points >= 4 && Distance < EVISCERATE_REACH + 0.5f * Foe->Dimensions.X)
    {
        Result |= NetButton_Shockwave;
    }
    else if (Ready[0] && Distance > 150.f && Distance < 0.95f * SHADOWSTEP_RANGE)
    {
        Result |= NetButton_Launch;
    }
    else if (Ready[1] && (Near >= 2 || (Near >= 1 && Points <= 2 && BotRandom(Bot) % 4 == 0)))
    {
        Result |= NetButton_Push;
    }
    else if (Ready[5] && Distance < 0.95f * SHIV_RANGE && !HasStatus(Foe, StatusEffect_Poisoned))
    {
        Result |= NetButton_Fireball;
    }
    else if (Ready[6] && Distance < Reach + 6.f)
    {
        Result |= NetButton_Sword;
    }
    return Result;
}
