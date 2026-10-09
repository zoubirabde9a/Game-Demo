/* Duel bots (server/bots.cpp): the keys a bot presses against another
   player outside a dungeon, from the whole duel kit its talents opened
   (sim/progression/talents.cpp). It shields a fireball or kunai about to
   land and a foe winding up beside it, launches a foe in reach, closes
   with a blink when far and blinks away when nearly dead, jumps and slams
   on a foe close by, and pushes, shockwaves and freezes what is in reach.
   A bot without the sword keeps to fireball range and circles there; one
   with it closes in. Each reaction lands only BOT_DUEL_NOTICE of the
   time, so a bot is beatable and the probe (tools/duel_balance.cpp)
   measures the tree, not perfect play. */

#define BOT_DUEL_NOTICE 0.6f
// NOTE(zoubir): how far ahead a bot looks along a shot's flight
#define BOT_DUEL_SHOT_LOOKAHEAD 0.35f
#define BOT_DUEL_SHOT_MISS 34.f
// NOTE(zoubir): the gap a bot without the sword keeps, and how often it
// turns the way it circles
#define BOT_DUEL_KEEP_RANGE 250.f
#define BOT_DUEL_TURN_SECONDS 2.2f

// NOTE(zoubir): whether a fireball or kunai of someone else's will reach
// Self within BOT_DUEL_SHOT_LOOKAHEAD seconds
internal bool32
BotShotIncoming(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    bool32 Result = false;
    for (u32 Index = 0; Index < World->EntityCount && !Result; ++Index)
    {
        world_entity *Shot = &World->Entities[Index];
        if (!Shot->IsPresent || (Shot->Type != EntityType_FireBall && Shot->Type != EntityType_Kunai) ||
            (Shot->HasOwner && Shot->OwnerSlot == Self->PlayerIndex))
        {
            continue;
        }
        v2 Gap = Self->Position.XY - Shot->Position.XY;
        v2 Speed = Shot->Velocity.XY;
        float SpeedSq = LengthSq(Speed);
        if (SpeedSq < 1.f)
        {
            continue;
        }
        // NOTE(zoubir): the moment of nearest approach, and how near
        float When = DotProduct(Gap, Speed) / SpeedSq;
        if (When < 0.f || When > BOT_DUEL_SHOT_LOOKAHEAD)
        {
            continue;
        }
        Result = Length(Gap - When * Speed) < BOT_DUEL_SHOT_MISS;
    }
    return Result;
}

inline bool32
BotHasAbility(player_slot *Slot, u32 Button)
{
    bool32 Result = AbilityLevel(Slot, Button) > 0;
    return Result;
}

// NOTE(zoubir): the buttons for a duel against Target, Distance away along
// Direction; may turn the aim (*Aim, *AimReach) for a blink away
internal u32
BotDuelButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
               float Distance, v2 Direction, v2 *Aim, float *AimReach, float Dt)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    u32 Held = 0;
    float Health = Self->MaxHp > 0.f ? Self->Hp / Self->MaxHp : 1.f;
    u32 Roll = BotRandom(Bot) % 1000;
    bool32 Notices = (float)Roll < 1000.f * BOT_DUEL_NOTICE;
    Bot->DuelThink -= Dt;
    Bot->SlamWait = Maximum(0.f, Bot->SlamWait - Dt);
    bool32 Thinks = Bot->DuelThink <= 0.f;
    if (Thinks)
    {
        // NOTE(zoubir): a reaction time, so a bot is not perfect
        Bot->DuelThink = 0.12f + (BotRandom(Bot) % 10) / 100.f;
    }

    // NOTE(zoubir): the shield, against a shot about to land or a foe
    // winding up a spell beside it
    if (Thinks && Notices && BotHasAbility(Slot, PlayerButton_Shield) &&
        (BotShotIncoming(AppState, Self) ||
         (Target->Type == EntityType_Player && IsPlayerCasting(Target) && Distance < 150.f)))
    {
        Held |= NetButton_Shield;
    }

    // NOTE(zoubir): circling at fireball range, or closing in with a sword
    // or a close spell ready
    Bot->DuelTurn -= Dt;
    if (Bot->DuelTurn <= 0.f)
    {
        Bot->DuelTurn = BOT_DUEL_TURN_SECONDS + (BotRandom(Bot) % 150) / 100.f;
        Bot->DuelSide = (BotRandom(Bot) & 1) ? 1.f : -1.f;
    }
    bool32 Sword = BotHasAbility(Slot, PlayerButton_Attack);
    bool32 Wave = BotHasAbility(Slot, PlayerButton_Shockwave) &&
        Self->AreaCooldowns[PlayerArea_Shockwave] <= 0.f;
    bool32 Nova = BotHasAbility(Slot, PlayerButton_FrostNova) &&
        Self->AreaCooldowns[PlayerArea_FrostNova] <= 0.f;
    bool32 Shove = BotHasAbility(Slot, PlayerButton_Push) &&
        Self->AreaCooldowns[PlayerArea_Push] <= 0.f;
    bool32 Slam = BotHasAbility(Slot, PlayerButton_Slam) &&
        Self->MovementCooldowns[PlayerMove_Slam] <= 0.f;
    bool32 Closes = Sword || Wave || Nova || Slam;
    v2 Side = Bot->DuelSide * V2(-Direction.Y, Direction.X);
    v2 Walk = Closes ? Direction + 0.4f * Side : Side;
    if (!Closes && Distance < BOT_DUEL_KEEP_RANGE - 60.f)
    {
        Walk = Side - Direction;
    }
    else if (!Closes && Distance > BOT_DUEL_KEEP_RANGE + 60.f)
    {
        Walk = Side + 2.f * Direction;
    }
    // NOTE(zoubir): the aim leads a moving foe by the fireball's flight
    float Flight = Distance / Maximum(1.f, PlayerStats.FireballSpeed * FireballSpeedScale(AppState, Self));
    v2 Lead = Target->Position.XY + Minimum(Flight, 0.8f) * Target->Velocity.XY - Self->Position.XY;
    float LeadLength = Length(Lead);
    if (LeadLength > 1.f)
    {
        *Aim = Lead * (1.f / LeadLength);
        *AimReach = Minimum(1.f, Maximum(0.05f, LeadLength / PLAYER_AIM_REACH));
    }
    Held |= NetButtonsToward(NormalizeOr(Walk, Direction));

    if (!Thinks)
    {
        return Held;
    }
    // NOTE(zoubir): a jump now, the slam once in the air
    if (Bot->SlamWait > 0.f)
    {
        if (!IsOnGround(Self))
        {
            Held |= NetButton_Slam;
            Bot->SlamWait = 0.f;
        }
    }
    else if (Distance < 100.f && Notices && Slam && IsOnGround(Self))
    {
        Held |= NetButton_Jump;
        Bot->SlamWait = 0.5f;
    }
    // NOTE(zoubir): blink away when nearly dead and pressed, in when far
    if (BotHasAbility(Slot, PlayerButton_Blink) && Self->MovementCooldowns[PlayerMove_Blink] <= 0.f)
    {
        if (Health < 0.4f && Distance < 160.f && Notices)
        {
            Held |= NetButton_Blink;
            *Aim = -1.f * Direction;
            *AimReach = 1.f;
        }
        else if (Distance > (Sword ? 180.f : 420.f) && Roll < 300)
        {
            Held |= NetButton_Blink;
        }
    }
    if (Distance < 290.f && Roll < 500 && BotHasAbility(Slot, PlayerButton_Launch))
    {
        Held |= NetButton_Launch;
    }
    // NOTE(zoubir): up close: freeze first, then the shockwave; a push
    // when hurt, to get away
    if (Distance < 115.f && Notices)
    {
        Held |= Nova ? NetButton_FrostNova : Wave ? NetButton_Shockwave :
            (Shove && (Health < 0.5f || !Sword)) ? NetButton_Push : 0;
    }
    if (Distance > 110.f && Distance < 270.f && Roll < 250)
    {
        Held |= NetButton_GravityWell;
    }
    if (Distance > BOT_SWORD_RANGE && Distance < 500.f && Roll < 300)
    {
        Held |= NetButton_Kunai;
    }
    return Held;
}
