/* Druid bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit or ally a spell goes at.

   A Druid bot moves as the Mender's does (healer_footwork.cpp): it backs
   off what comes close and stands over a downed ally until it is up. It
   heals first: Regrowth on an ally under three quarters of its health
   (best with Bloom banked, so while it waits for Regrowth it casts the
   instant Wrath), Rejuvenation on one a little hurt that has none of its own
   yet and on the tank all through a fight, Tranquility when several round it are hurt. With nobody to heal it
   fights: Moonfire on what it fights when that does not burn yet,
   Entangling Roots on a pack that comes at it, Starfire, and Wrath in
   between, which banks Bloom for the next heal. */

// NOTE(zoubir): the living monsters within Radius of Point
internal u32
BotDruidFoesNear(app_state *AppState, v2 Point, float Radius)
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

// NOTE(zoubir): whether a Rejuvenation of the Druid in slot By heals Ally
internal bool32
BotDruidRejuvenates(app_state *AppState, u32 By, world_entity *Ally)
{
    druid_run *Run = &AppState->Dungeon->Druid;
    bool32 Result = false;
    for (u32 Index = 0; Index < DRUID_MAX_REJUVENATIONS; ++Index)
    {
        druid_rejuvenation *Rejuv = &Run->Rejuvenations[Index];
        Result |= Rejuv->Seconds > 1.f && Rejuv->By == By && Rejuv->Ally == Ally->PlayerIndex;
    }
    return Result;
}

// NOTE(zoubir): the living ally, Self too, missing the largest share of
// its health within Range, or 0 when nobody is under Below of theirs
internal world_entity *
BotDruidHurt(app_state *AppState, world_entity *Self, float Range, float Below)
{
    world_entity *Result = BotHurtAlly(AppState, Self, Range, Below);
    float Worst = Result ? Result->Hp / Result->MaxHp : Below;
    if (Self->MaxHp > 0.f && Self->Hp / Self->MaxHp < Worst)
    {
        Result = Self;
    }
    return Result;
}

internal u32
BotDruidButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): X and W are the Druid's: the game's fireball and
    // shockwave presses from BotThink would cast them at random
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Shockwave);
    BotHealerFootwork(AppState, Self, Target, Distance, Direction, Held);
    if (IsPlayerCasting(Self))
    {
        return 0;
    }
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key)
    {
        Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f && RoleSpellLearned(Slot, Key);
    }
    dungeon_run *Run = AppState->Dungeon;

    // NOTE(zoubir): the heals
    world_entity *Low = BotDruidHurt(AppState, Self, REGROWTH_RANGE, 0.75f);
    if (Low && Ready[4] && BotRandom(Bot) % 3 == 0)
    {
        *Pick = (u16)(Low->ID + 1);
        return NetButton_Shockwave;
    }
    world_entity *Hurt = BotDruidHurt(AppState, Self, REJUVENATION_RANGE, 0.85f);
    if (Hurt && Ready[0] && !BotDruidRejuvenates(AppState, Self->PlayerIndex, Hurt) &&
        BotRandom(Bot) % 5 == 0)
    {
        *Pick = (u16)(Hurt->ID + 1);
        return NetButton_Launch;
    }
    // NOTE(zoubir): in a fight the tank always has one going, as it takes
    // the most
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && Ready[0] && Run->FightingRoom; ++SlotIndex)
    {
        world_entity *Tank = LivingPlayerInSlot(AppState, SlotIndex);
        if (Tank && RoleKindOf(AppState->Players[SlotIndex].Role) == RoleKind_Tank &&
            Length(Tank->Position.XY - Self->Position.XY) < REJUVENATION_RANGE &&
            !BotDruidRejuvenates(AppState, Self->PlayerIndex, Tank) && BotRandom(Bot) % 5 == 0)
        {
            *Pick = (u16)(Tank->ID + 1);
            return NetButton_Launch;
        }
    }
    u32 HurtNear = 0;
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
        HurtNear += (Ally && Ally->Hp < 0.7f * Ally->MaxHp &&
                     Length(Ally->Position.XY - Self->Position.XY) < TRANQUILITY_RADIUS) ? 1 : 0;
    }
    // NOTE(zoubir): a Druid that took Tranquility over Regrowth leans on it
    // for one hurt ally too
    u32 Want = RoleSpellLearned(Slot, 4) ? 2 : 1;
    if (HurtNear >= Want && Ready[3] && Run->FightingRoom && BotRandom(Bot) % 10 == 0)
    {
        return NetButton_Kunai;
    }
    if (Low)
    {
        // NOTE(zoubir): someone is low and Regrowth is not back: no cast
        // that holds the Druid still, but Wrath is instant and banks
        // Bloom, so the next Regrowth heals more
        if (Target && Target->Type == EntityType_Monster && Ready[6] && Distance < WRATH_RANGE)
        {
            *Pick = (u16)(Target->ID + 1);
            return NetButton_Sword;
        }
        return 0;
    }

    // NOTE(zoubir): nobody to heal: the damage half
    if (!Target || Target->Type != EntityType_Monster || !Run->FightingRoom)
    {
        return 0;
    }
    if (Ready[2] && Distance < 220.f && BotDruidFoesNear(AppState, Target->Position.XY, ROOTS_RADIUS) >= 2 &&
        BotRandom(Bot) % 8 == 0)
    {
        return NetButton_Slam;
    }
    if (Ready[5] && Distance < 0.9f * MOONFIRE_RANGE && !IsDruidMoonfired(AppState, Self->PlayerIndex, Target))
    {
        *Pick = (u16)(Target->ID + 1);
        return NetButton_Fireball;
    }
    if (Ready[1] && Distance < 0.9f * STARFIRE_RANGE && BotRandom(Bot) % 6 == 0)
    {
        *Pick = (u16)(Target->ID + 1);
        return NetButton_Push;
    }
    if (Ready[6] && Distance < WRATH_RANGE)
    {
        *Pick = (u16)(Target->ID + 1);
        return NetButton_Sword;
    }
    return 0;
}
