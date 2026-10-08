/* Duelist bots (server/bots.cpp, BotRoleButtons): which of the class's keys
   a bot presses this tick, as NetButton bits; *Held may change its
   movement and *Pick becomes the unit a spell goes at.

   A Duelist bot fights the tank's monster when the tank holds one (the
   one nearest the tank, never a boss its pylons ward), else what it
   found itself, and stands at its side: a monster faces what it attacks,
   so the side is out of its swings at the tank yet close to its body,
   and it is the side nearer the bot, so it does not walk round.

   It weaves: never the same key twice running while another is ready,
   since Tempo comes only from a new key (role_kits/duelist/tempo.cpp).
   Heartseeker at four or five Tempo, or on a foe low enough for its
   bonus; Lunge to close a gap, or as a new key in the melee; Thrust in
   between; Perfect Form in a boss fight.

   Riposte is for what it cannot step out of: a slam, blink or mortar
   winding up over it whose blow lands inside the guard and whose edge is
   too far to reach first, or a frost wave about to roll over it
   (bot_rift_dangers.cpp jumps those too; a parry is better when Riposte
   is ready). A monster about to bite it in the melee is worth a guard
   too while Tempo is low. Otherwise it walks out of danger circles the
   way the other melee bots do (bot_dangers.cpp). */

// NOTE(zoubir): how far beside its monster the bot stands, past the
// monster's half width; how close to the tank a monster is the tank's
#define BOT_DUELIST_GAP 24.f
#define BOT_DUELIST_TANK_REACH 220.f
// NOTE(zoubir): a blow lands within this long of the guard going up for
// the bot to raise it, so the guard is still up when it lands (the guard
// lasts RIPOSTE_GUARD_SECONDS); and how fast the bot reckons it runs
#define BOT_DUELIST_GUARD_EARLY 0.08f
#define BOT_DUELIST_RUN_SPEED 190.f

// NOTE(zoubir): falling back: under this share of its health with this
// many monsters within this of it, the bot stands this far behind the tank
#define BOT_DUELIST_FALL_BACK 0.4f
#define BOT_DUELIST_CROWD 3
#define BOT_DUELIST_CROWD_RADIUS 110.f
#define BOT_DUELIST_FALL_BACK_GAP 70.f

// NOTE(zoubir): a living tank of the party other than Self, 0 for none
internal world_entity *
BotDuelistTank(app_state *AppState, world_entity *Self)
{
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        world_entity *Tank = LivingPlayerInSlot(AppState, SlotIndex);
        if (Tank && Tank != Self && RoleKindOf(AppState->Players[SlotIndex].Role) == RoleKind_Tank)
        {
            return Tank;
        }
    }
    return 0;
}

// NOTE(zoubir): the monster nearest a living tank of the party, if one is
// close to it and in Self's room and not a boss behind its pylons; 0 for
// none
internal world_entity *
BotDuelistTankFoe(app_state *AppState, world_entity *Self)
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
        world_entity *Foe = NearestFoe(World, Tank->Position.XY, BOT_DUELIST_TANK_REACH, Room, 0, 0);
        if (Foe && !IsWardedBoss(AppState, Foe))
        {
            return Foe;
        }
    }
    return 0;
}

// NOTE(zoubir): whether a blow Self cannot walk out of lands on it soon
// enough for a guard raised now to take it: a slam, blink or mortar
// winding up over it, its edge farther than the bot can run before it
// lands, or a frost wave's ring about to roll over it
internal bool32
BotDuelistMustParry(app_state *AppState, world_entity *Self, float Guard)
{
    world *World = &AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            (Monster->AbilityPhase != AbilityPhase_Windup && Monster->AbilityPhase != AbilityPhase_Active) ||
            LengthSq(Monster->Position.XY - Self->Position.XY) > Square(BOT_DANGER_SIGHT))
        {
            continue;
        }
        monster_def *Def = GetMonsterDef((monster_kind)Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        if (Ability->Kind == MonsterAbility_Wave)
        {
            float Distance = Length(Self->Position.XY - Monster->Position.XY);
            if (Distance > Ability->Radius || Ability->Speed <= 0.f)
            {
                continue;
            }
            float Elapsed = Monster->AbilityPhase == AbilityPhase_Windup ? -Monster->AbilityTimer :
                AbilityActiveElapsed(Monster, Ability);
            for (u32 Ring = 0; Ring < Maximum(1u, Ability->Count); ++Ring)
            {
                float Front = Ability->Speed * Elapsed - (float)Ring * Ability->Spread;
                float Arrives = (Distance - Front) / Ability->Speed;
                if (Arrives > BOT_DUELIST_GUARD_EARLY && Arrives < Guard - BOT_DUELIST_GUARD_EARLY)
                {
                    return true;
                }
            }
            continue;
        }
        if (Monster->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        float Lands = Monster->AbilityTimer;
        if (Lands < BOT_DUELIST_GUARD_EARLY || Lands > Guard - BOT_DUELIST_GUARD_EARLY)
        {
            continue;
        }
        v2 Centres[MAX_ABILITY_POINTS + 1];
        u32 Count = 0;
        if (Ability->Kind == MonsterAbility_Slam)
        {
            Centres[Count++] = Monster->Position.XY;
        }
        else if (Ability->Kind == MonsterAbility_Blink && Monster->AbilityPointCount)
        {
            Centres[Count++] = Monster->AbilityPoints[0];
        }
        else if (Ability->Kind == MonsterAbility_Mortar)
        {
            for (u32 Point = 0; Point < Monster->AbilityPointCount; ++Point)
            {
                Centres[Count++] = Monster->AbilityPoints[Point];
            }
        }
        for (u32 Circle = 0; Circle < Count; ++Circle)
        {
            float Distance = Length(Self->Position.XY - Centres[Circle]);
            bool32 InSafeMiddle = Ability->InnerRadius > 0.f && Distance < Ability->InnerRadius;
            float ToEdge = Ability->Radius - Distance;
            if (ToEdge > 0.f && !InSafeMiddle && ToEdge > BOT_DUELIST_RUN_SPEED * Lands)
            {
                return true;
            }
        }
    }
    return false;
}

// NOTE(zoubir): whether a monster within reach of Self is about to bite
// it: its attack is nearly back and Self is the nearest player to it
internal bool32
BotDuelistBiteComing(app_state *AppState, world_entity *Self)
{
    world *World = &AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            Monster->AttackCooldown > 0.3f ||
            LengthSq(Monster->Position.XY - Self->Position.XY) > Square(0.6f * RIPOSTE_REACH))
        {
            continue;
        }
        bool32 Nearest = true;
        float Mine = LengthSq(Monster->Position.XY - Self->Position.XY);
        for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && Nearest; ++SlotIndex)
        {
            world_entity *Other = LivingPlayerInSlot(AppState, SlotIndex);
            Nearest = !Other || Other == Self || LengthSq(Monster->Position.XY - Other->Position.XY) >= Mine;
        }
        if (Nearest)
        {
            return true;
        }
    }
    return false;
}

internal u32
BotDuelistButtons(bot_brain *Bot, app_state *AppState, world_entity *Self, world_entity *Target,
                  float Distance, v2 Direction, u32 *Held, u16 *Pick)
{
    player_slot *Slot = &AppState->Players[Self->PlayerIndex];
    // NOTE(zoubir): the right click is the rapier's here and X does
    // nothing: the game's fireball and sword presses would stab at the
    // wrong moments
    *Held &= ~(u32)(NetButton_Fireball | NetButton_Sword);
    if (!Target || Target->Type != EntityType_Monster)
    {
        return 0;
    }
    world_entity *Foe = BotDuelistTankFoe(AppState, Self);
    if (!Foe || Length(Foe->Position.XY - Self->Position.XY) > 1.5f * LUNGE_RANGE)
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
    u32 Tempo = Slot->ClassMeter;
    u32 Last = Slot->Duelist.LastKey;

    // NOTE(zoubir): to the monster's side, the way it faces being toward
    // whoever it attacks; the side nearer the bot
    v2 Facing = LengthSq(Foe->Direction) > 0.0001f ? DirectionTo(Foe->Direction) :
        (Distance > 0.001f ? -1.f * DirectionTo(ToFoe) : V2(1.f, 0.f));
    v2 Side = V2(-Facing.Y, Facing.X);
    if (DotProduct(Side, -1.f * ToFoe) < 0.f)
    {
        Side = -1.f * Side;
    }
    float Gap = BOT_DUELIST_GAP + 0.5f * Foe->Dimensions.X;
    bot_danger_circle Dangers[BOT_MAX_DANGERS];
    u32 DangerCount = BotDangers(AppState, Self, Dangers);
    v2 Spot = Foe->Position.XY + Gap * Side;
    // NOTE(zoubir): hurt in a crowd, it falls back behind the tank, whose
    // monsters are after the tank, until the healer catches up
    world_entity *Tank = BotDuelistTank(AppState, Self);
    bool32 Crowded = BotFoesAround(AppState, Self, BOT_DUELIST_CROWD_RADIUS) >= BOT_DUELIST_CROWD;
    if (Tank && Crowded && Self->Hp < BOT_DUELIST_FALL_BACK * Self->MaxHp)
    {
        v2 Away = NormalizeOr(Tank->Position.XY - Foe->Position.XY, Side);
        Spot = Tank->Position.XY + BOT_DUELIST_FALL_BACK_GAP * Away;
    }
    v2 Want = BotSafeSpot(Dangers, DangerCount, Spot, Side);
    v2 ToWant = Want - Self->Position.XY;
    *Held &= ~(u32)(NetButton_Left | NetButton_Right | NetButton_Up | NetButton_Down);
    if (Length(ToWant) > 12.f)
    {
        // NOTE(zoubir): round a danger circle on the way, as the Berserker
        // bot walks (bots/berserker.cpp)
        *Held |= NetButtonsToward(BotBerserkerWay(Dangers, DangerCount, Self->Position.XY, ToWant));
    }

    if (IsPlayerCasting(Self))
    {
        return 0;
    }
    bool32 Guarding = (Slot->ClassFlags & DUELIST_FLAG_GUARD) != 0;
    float Guard = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Bait) ? BAIT_GUARD_SECONDS :
        RIPOSTE_GUARD_SECONDS;
    // NOTE(zoubir): a blow it cannot step out of: parry it
    if (Ready[1] && !Guarding && BotDuelistMustParry(AppState, Self, Guard))
    {
        return NetButton_Push;
    }
    // NOTE(zoubir): in harm's way otherwise: out first (DodgeDangers walks it)
    if (BotDangerAt(Dangers, DangerCount, Self->Position.XY))
    {
        return 0;
    }
    dungeon_run *Run = AppState->Dungeon;
    bool32 Fighting = Run->FightingRoom != 0;
    float Reach = THRUST_REACH + 0.5f * Foe->Dimensions.X;
    float Low = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Precision) ?
        PRECISION_LOW_HEALTH : HEARTSEEKER_LOW_HEALTH;
    bool32 Hurt = Foe->MaxHp > 0.f && Foe->Hp < Low * Foe->MaxHp;
    // NOTE(zoubir): a key other than the last one, so it gains Tempo
    bool32 ThrustAgain = Last == 6 + 1;
    u32 Result = 0;
    if (Ready[3] && Fighting && Run->BossSerial && Distance < 2.f * Reach)
    {
        Result = NetButton_Kunai;
    }
    else if (Ready[4] && (Tempo >= 4 || Hurt) && Distance < HEARTSEEKER_REACH + 0.5f * Foe->Dimensions.X)
    {
        Result = NetButton_Shockwave;
    }
    else if (Ready[0] && Distance > 140.f && Distance < 0.95f * LUNGE_RANGE &&
             !BotDangerAt(Dangers, DangerCount, Foe->Position.XY - (LUNGE_GAP + 0.5f * Foe->Dimensions.X) *
                          DirectionTo(ToFoe)))
    {
        Result = NetButton_Launch;
    }
    else if (Ready[1] && !Guarding && Distance < Reach + 20.f && BotDuelistBiteComing(AppState, Self))
    {
        Result = NetButton_Push;
    }
    else if (Ready[0] && ThrustAgain && Tempo < DUELIST_MOST_TEMPO && Distance < Reach + 20.f)
    {
        Result = NetButton_Launch;
    }
    else if (Ready[6] && Distance < Reach + 6.f)
    {
        Result = NetButton_Sword;
    }
    return Result;
}
