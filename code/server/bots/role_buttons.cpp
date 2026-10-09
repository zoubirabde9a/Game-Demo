/* Role buttons (server/bots.cpp): what a bot of the first three classes,
   the striker (Fire Mage), the tank (Bulwark) and the healer (Mender),
   casts in a dungeon run, as the buttons a player would press. The later
   classes' bots are server/bots/<class>.cpp. */

// NOTE(zoubir): the Searing stacks Self laid on the foes within Detonate's
// reach, all added
internal u32
BotSearingNear(app_state *AppState, world_entity *Self)
{
    dungeon_run *Run = AppState->Dungeon;
    u32 Result = 0;
    for (u32 Index = 0; Index < MAX_FOE_MARKS; ++Index)
    {
        foe_mark *Mark = &Run->Marks[Index];
        world_entity *Monster = (Mark->Stacks && Mark->SearBy == Self->PlayerIndex) ?
            FindMonsterBySerial(&AppState->World, Mark->Slot, Mark->Serial) : 0;
        if (Monster && Monster->Hp > 0.f &&
            Length(Monster->Position.XY - Self->Position.XY) <= DETONATE_SPELL_REACH)
        {
            Result += Mark->Stacks;
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
    // NOTE(zoubir): a key is ready when its spell is learned and its
    // cooldown has run; A, R, C, V, W and X are NetButton_Launch, _Push,
    // _Slam, _Kunai, _Shockwave and _Fireball
    bool32 Ready[ROLE_KEYS];
    for (u32 Key = 0; Key < ROLE_KEYS; ++Key)
    {
        Ready[Key] = Slot->RoleCooldowns[Key] <= 0.f && RoleSpellLearned(Slot, Key);
    }
    bool32 Casting = IsPlayerCasting(Self);
    if (Slot->Role == PlayerRole_Tank)
    {
        if (Target && Distance < SHIELD_SLAM_RADIUS && Ready[1] && BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_Push;
        }
        if (Target && Distance < TAUNT_RADIUS * 0.8f && Ready[0] && BotRandom(Bot) % 30 == 0)
        {
            Result |= NetButton_Launch;
        }
        if (Target && Distance < 0.9f * SHIELD_THROW_RANGE && Ready[4] && BotRandom(Bot) % 15 == 0)
        {
            Result |= NetButton_Shockwave;
        }
        // NOTE(zoubir): a foe winding up a big attack: charge it to stop it
        if (Target && Target->AbilityPhase == AbilityPhase_Windup &&
            Distance < SHIELD_CHARGE_RANGE && Ready[5])
        {
            Result |= NetButton_Fireball;
            *Pick = (u16)(Target->ID + 1);
        }
        if (Self->Hp < 0.35f * Self->MaxHp && Ready[3])
        {
            Result |= NetButton_Kunai;
        }
        // NOTE(zoubir): Rallying Cry when two of the party near are hurt,
        // Demoralizing Roar with two foes on the tank
        u32 Hurt = 0;
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            Hurt += (Ally && Ally->Hp < 0.6f * Ally->MaxHp &&
                     Length(Ally->Position.XY - Self->Position.XY) < RALLYING_CRY_REACH) ? 1 : 0;
        }
        if (Ready[7] && Hurt >= 2 && AppState->Dungeon->FightingRoom)
        {
            Result |= NetButton_FrostNova;
        }
        if (Ready[8] && AppState->Dungeon->FightingRoom && Target && Distance < DEMORALIZING_ROAR_REACH &&
            BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_GravityWell;
        }
        // NOTE(zoubir): an ally with monsters on them, too far to taunt off
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && Ready[2]; ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            float Gap = Ally ? Length(Ally->Position.XY - Self->Position.XY) : 0.f;
            if (Ally && Ally != Self && AppState->Players[SlotIndex].Aggro > 0 &&
                Gap > 150.f && Gap < INTERCEPT_RANGE && BotRandom(Bot) % 40 == 0)
            {
                Result |= NetButton_Slam;
                *Pick = (u16)(Ally->ID + 1);
                break;
            }
        }
    }
    else if (Slot->Role > PlayerRole_Healer)
    {
        Result = BotClassButtons(Bot, AppState, Self, Target, Distance, Direction, Held, Pick);
    }
    else if (Slot->Role == PlayerRole_Healer)
    {
        BotHealerFootwork(AppState, Self, Target, Distance, Direction, Held);
        world_entity *Hurt = BotHurtAlly(AppState, Self, MENDING_BOLT_RANGE, 0.85f);
        if (Hurt && Ready[0] && BotRandom(Bot) % 6 == 0)
        {
            Result |= NetButton_Launch;
            *Pick = (u16)(Hurt->ID + 1);
        }
        world_entity *Low = BotHurtAlly(AppState, Self, MENDING_BOLT_RANGE, 0.6f);
        if (Low && Ready[1] && !(Result & NetButton_Launch) &&
            AppState->Players[Low->PlayerIndex].WardAbsorb <= 0.f && BotRandom(Bot) % 10 == 0)
        {
            Result |= NetButton_Push;
            *Pick = (u16)(Low->ID + 1);
        }
        // NOTE(zoubir): nobody in danger: the ward goes on the striker for
        // its damage (role_kits/healer.cpp)
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && !Low && Ready[1] &&
             !(Result & (NetButton_Launch | NetButton_Push)); ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            player_slot *AllySlot = &AppState->Players[SlotIndex];
            if (Ally && IsDamageRole(AllySlot->Role) && AllySlot->WardAbsorb <= 0.f &&
                AppState->Dungeon->FightingRoom &&
                Length(Ally->Position.XY - Self->Position.XY) < MENDING_BOLT_RANGE &&
                BotRandom(Bot) % 10 == 0)
            {
                Result |= NetButton_Push;
                *Pick = (u16)(Ally->ID + 1);
            }
        }
        // NOTE(zoubir): nobody low: Holy Fire on what it is fighting, and
        // Smite Bolts in between
        if (Target && !Low && Distance < 0.9f * HOLY_FIRE_RANGE && Ready[4] &&
            !(Result & (NetButton_Launch | NetButton_Push)) && BotRandom(Bot) % 8 == 0)
        {
            Result |= NetButton_Shockwave;
        }
        else if (Target && !Low && Distance < 0.9f * SMITE_BOLT_RANGE && Ready[6] &&
                 !(Result & (NetButton_Launch | NetButton_Push)) && BotRandom(Bot) % 4 == 0)
        {
            Result |= NetButton_Sword;
        }
        // NOTE(zoubir): the party hurt round it: Radiance, or a sanctuary
        // at its own feet
        u32 HurtNear = 0;
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
        {
            world_entity *Ally = LivingPlayerInSlot(AppState, SlotIndex);
            HurtNear += (Ally && Ally->Hp < 0.8f * Ally->MaxHp &&
                         Length(Ally->Position.XY - Self->Position.XY) < SANCTUARY_RADIUS) ? 1 : 0;
        }
        if (HurtNear >= 2 && Ready[3] && BotRandom(Bot) % 10 == 0)
        {
            Result |= NetButton_Kunai;
        }
        else if (HurtNear >= 2 && Ready[2] && BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_Slam;
        }
    }
    else if (Target && Target->Type == EntityType_Monster && !Casting)
    {
        // NOTE(zoubir): the striker's rotation (role_kits/striker.cpp):
        // fireballs build Searing on what it fights, Meteor opens on a
        // pack, the Giant Fireball goes in between, Fireguard goes up when
        // something is after it or it is hurt, and Combustion goes up in a
        // fight
        if (Distance > 80.f && Distance < PLAYER_AIM_REACH && Ready[0] &&
            BotRandom(Bot) % 25 == 0)
        {
            Result |= NetButton_Launch;
        }
        else if (Distance > 60.f && Distance < 0.8f * GIANT_FIREBALL_RANGE && Ready[1] &&
                 BotRandom(Bot) % 30 == 0)
        {
            Result |= NetButton_Push;
        }
        if (Ready[2] && AppState->Dungeon->FightingRoom &&
            (Slot->Aggro || Self->Hp < 0.7f * Self->MaxHp) && BotRandom(Bot) % 20 == 0)
        {
            Result |= NetButton_Slam;
        }
        if (Ready[3] && AppState->Dungeon->FightingRoom && BotRandom(Bot) % 60 == 0)
        {
            Result |= NetButton_Kunai;
        }
        // NOTE(zoubir): Flame Wave on what is in front, the aim on it
        if (Ready[6] && Distance < 0.9f * FLAME_WAVE_REACH && BotRandom(Bot) % 4 == 0)
        {
            Result |= NetButton_Sword;
        }
        // NOTE(zoubir): Detonate once its marks near hold enough stacks: a
        // full one, or a pack's worth
        if (Ready[4] && AppState->Dungeon->FightingRoom &&
            BotSearingNear(AppState, Self) >= 2 * SEARING_MOST - 2)
        {
            Result |= NetButton_Shockwave;
        }
    }
    return Result;
}
