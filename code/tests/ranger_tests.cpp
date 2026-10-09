/* Ranger tests (sim/dungeon/role_kits/ranger.cpp), included by
   dungeon_tests.cpp: the class's keys, W doing nothing; Quick Shot
   landing when its arrow arrives; Quick Shot's Hunter's Mark raising the
   Ranger's damage and building Focus, and moving to the next foe shot;
   Piercing Shot going through a line, less for each foe further down
   it, and spending Focus, and Deadeye's crit; Volley raining and slowing inside its circle only, wider with
   Barrage; Disengage leaping back and its snare rooting (and biting at
   rank 2); Pinning Volley holding the slow on longer; Hunter's Net
   rooting every foe near the snare; Rapid Fire's stream of arrows;
   Lethal Mark jumping to the next foe; Focus draining between fights; and the burst angle's variant. */

struct ranger_dummies
{
    world_entity *Units[4];
    v3 Spots[4];
    u32 Count;
};

// NOTE(zoubir): a still monster Offset from the Ranger in slot 0, with
// health to spare
internal world_entity *
RangerDummy(crypt_world *Crypt, ranger_dummies *Dummies, v3 Offset)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Ranger = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Ranger->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    Dummies->Units[Dummies->Count] = Result;
    Dummies->Spots[Dummies->Count] = Result->Position;
    Dummies->Count++;
    return Result;
}

// NOTE(zoubir): Ticks of the world with every dummy held where it stood
// and the Ranger kept alive
internal void
RangerTick(crypt_world *Crypt, ranger_dummies *Dummies, u32 Ticks)
{
    world_entity *Ranger = Crypt->AppState->Players[0].Entity;
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        for(u32 Index = 0; Index < Dummies->Count; Index++)
        {
            v3 Old = Dummies->Units[Index]->Position;
            Dummies->Units[Index]->Position = Dummies->Spots[Index];
            Dummies->Units[Index]->Velocity = {};
            CheckAndChangeEntityChunk(Crypt->AppState, &Crypt->AppState->World, &Crypt->Arena, Old,
                                      Dummies->Units[Index]);
        }
        Ranger->Hp = Ranger->MaxHp;
        TickCrypt(Crypt, 1);
    }
}

// NOTE(zoubir): a Ranger in slot 0 of a fresh crypt, aiming along +X
internal crypt_world
CreateRangerWorld(u32 Players = 1)
{
    crypt_world Crypt = CreateCryptWorld(Players);
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &Crypt.AppState->Players[0];
    SetPlayerRole(Crypt.AppState, Slot, PlayerRole_Ranger);
    GrantClassSpells(Slot);
    Slot->Entity->Aim = V2(1.f, 0.f);
    Slot->Entity->AimReach = 0.6f;
    return Crypt;
}

internal void
TestRangerKeys()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_Ranger));
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == (DUNGEON_SHARED_BUTTONS | PlayerButton_Push | PlayerButton_Launch));
    SetClassTalentRank(Slot, RangerTalent_Disengage, 1);
    SetClassTalentRank(Slot, RangerTalent_RapidFire, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check((Allowed & PlayerButton_Slam) && (Allowed & PlayerButton_Kunai));
    Check(RangerKeyWindsUp(1) && RangerKeyWindsUp(3) && !RangerKeyWindsUp(5));
    // NOTE(zoubir): the variant rides in the burst's height, and comes off
    // it again, whatever the ground's height
    for(u32 Variant = 0; Variant < 12; Variant++)
    {
        for(float Z = -300.f; Z <= 1500.f; Z += 450.f)
        {
            v3 Spot = RangerBurstSpot(V3(10.f, 20.f, Z), Variant);
            Check(RangerBurstVariant(Spot) == Variant);
            Check(Absolute(RangerBurstPlace(Spot).Z - Z) < 0.01f);
        }
    }
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): X looses an arrow; the hit lands when it gets there, on
// the foe its mark is already on
internal void
TestQuickShotLandsOnArrival()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(Slot->RoleCooldowns[5] > 0.f);
    Check(Foe->Hp == 2000.f);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * 300.f / RANGER_ARROW_SPEED) + 2);
    float Dealt = 2000.f - Foe->Hp;
    float Expected = QUICK_SHOT_DAMAGE * GetRoleDef(PlayerRole_Ranger)->DamageDealt * (1.f + MARK_SHARE);
    Check(Dealt > 0.99f * Expected && Dealt < 1.01f * Expected);
    Check(Slot->Ranger.Focus == QUICK_SHOT_FOCUS);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Quick Shot marks its foe: it takes more from the Ranger,
// shots on it build Focus, which clients see, and the mark moves to the
// next foe shot
internal void
TestQuickShotMarks()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
    world_entity *Other = RangerDummy(&Crypt, &Dummies, V3(150.f, 110.f, 0.f));
    Slot->Input.Target = (u32)(Foe - AppState->World.Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(IsRangerMarked(AppState, Slot, Foe) && !IsRangerMarked(AppState, Slot, Other));
    Check(RangerDealtScale(Slot, Foe) > 1.f + MARK_SHARE - 0.001f);
    Check(RangerDealtScale(Slot, Other) == 1.f);
    RangerTick(&Crypt, &Dummies, 30);
    Check(Slot->ClassFlags & RANGER_FLAG_MARK);
    Check(Slot->ClassMeter == (u8)QUICK_SHOT_FOCUS);
    // NOTE(zoubir): shooting it again keeps the mark there, fresh
    Slot->Ranger.MarkSeconds = 1.f;
    Slot->RoleCooldowns[5] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(IsRangerMarked(AppState, Slot, Foe) && Slot->Ranger.MarkSeconds > MARK_SECONDS - 0.1f);
    RangerTick(&Crypt, &Dummies, 20);
    Check(Slot->Ranger.Focus == 2.f * QUICK_SHOT_FOCUS);
    // NOTE(zoubir): a shot on another foe takes the mark there
    Slot->Input.Target = (u32)(Other - AppState->World.Entities) + 1;
    Slot->RoleCooldowns[5] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    Check(IsRangerMarked(AppState, Slot, Other) && !IsRangerMarked(AppState, Slot, Foe));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Ticks for a Piercing Shot's cast and its flight
#define RANGER_PIERCE_TICKS ((u32)(60.f * (1.f + PIERCE_RANGE / PIERCE_SPEED)) + 4)

// NOTE(zoubir): R draws, then hits everything in its line, Focus spent
internal void
TestPiercingShotThroughALine()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Near = RangerDummy(&Crypt, &Dummies, V3(120.f, 0.f, 0.f));
    world_entity *Far = RangerDummy(&Crypt, &Dummies, V3(320.f, 10.f, 0.f));
    world_entity *Aside = RangerDummy(&Crypt, &Dummies, V3(200.f, 160.f, 0.f));
    world_entity *Behind = RangerDummy(&Crypt, &Dummies, V3(-150.f, 0.f, 0.f));
    Slot->Ranger.Focus = 50.f;
    Slot->Ranger.FocusHold = 10.f;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Slot->Entity->CastSpell == PlayerSpell_RangerA);
    RangerTick(&Crypt, &Dummies, RANGER_PIERCE_TICKS);
    Check(Slot->Entity->CastSpell == PlayerSpell_None);
    float Expected = (PIERCE_DAMAGE + 50.f * PIERCE_PER_FOCUS) * GetRoleDef(PlayerRole_Ranger)->DamageDealt;
    float NearDealt = 2000.f - Near->Hp;
    Check(NearDealt > 0.99f * Expected && NearDealt < 1.01f * Expected);
    // NOTE(zoubir): the one further down the line takes PIERCE_FALLOFF of it
    float FarDealt = 2000.f - Far->Hp;
    Check(FarDealt > 0.99f * PIERCE_FALLOFF * NearDealt && FarDealt < 1.01f * PIERCE_FALLOFF * NearDealt);
    Check(Aside->Hp == 2000.f && Behind->Hp == 2000.f);
    Check(Slot->Ranger.Focus == 0.f);

    // NOTE(zoubir): Deadeye: a full Focus crits
    SetClassTalentRank(Slot, RangerTalent_Deadeye, 1);
    Slot->Ranger.Focus = RANGER_FOCUS_MOST;
    Slot->Ranger.FocusHold = 10.f;
    Slot->RoleCooldowns[1] = 0.f;
    float Before = Near->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    RangerTick(&Crypt, &Dummies, RANGER_PIERCE_TICKS);
    float Crit = (PIERCE_DAMAGE + RANGER_FOCUS_MOST * PIERCE_PER_FOCUS) * DEADEYE_SCALE *
        GetRoleDef(PlayerRole_Ranger)->DamageDealt;
    Check(Before - Near->Hp > 0.99f * Crit && Before - Near->Hp < 1.01f * Crit);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): A rains on the circle at the cursor: what is inside is
// struck each tick of the rain and slowed, what is outside untouched;
// Barrage widens it
internal void
TestVolleyRainsOnTheCircle()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    v2 Point = AimPoint(Slot->Entity);
    v3 Centre = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
    Centre.Z = 0.f;
    world_entity *Inside = RangerDummy(&Crypt, &Dummies, Centre + V3(20.f, 0.f, 0.f));
    world_entity *Edge = RangerDummy(&Crypt, &Dummies, Centre + V3(0.f, 1.2f * VOLLEY_RADIUS, 0.f));
    world_entity *Outside = RangerDummy(&Crypt, &Dummies, Centre + V3(0.f, -2.5f * VOLLEY_RADIUS, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Slot->RoleCooldowns[0] > 0.f);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * VOLLEY_DRAW) - 2);
    Check(Inside->Hp == 2000.f);
    RangerTick(&Crypt, &Dummies, 30);
    Check(Inside->Hp < 2000.f && HasStatus(Inside, StatusEffect_Slowed));
    RangerTick(&Crypt, &Dummies, (u32)(60.f * VOLLEY_SECONDS) + 10);
    float Dealt = 2000.f - Inside->Hp;
    float Strikes = Dealt / (VOLLEY_TICK_DAMAGE * GetRoleDef(PlayerRole_Ranger)->DamageDealt);
    Check(Strikes > 4.5f && Strikes < 5.5f);
    Check(Outside->Hp == 2000.f);

    // NOTE(zoubir): Barrage: the edge one is inside now, and it rains longer
    SetClassTalentRank(Slot, RangerTalent_Barrage, 1);
    Check(RoleSpellRadius(Slot, 0) > VOLLEY_RADIUS * 1.29f);
    Slot->RoleCooldowns[0] = 0.f;
    float EdgeBefore = Edge->Hp;
    float InsideBefore = Inside->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * (VOLLEY_DRAW + VOLLEY_SECONDS + BARRAGE_SECONDS)) + 20);
    Check(Edge->Hp < EdgeBefore);
    Check((InsideBefore - Inside->Hp) / (VOLLEY_TICK_DAMAGE * GetRoleDef(PlayerRole_Ranger)->DamageDealt) > 7.5f);
    Check(Outside->Hp == 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): C leaps back from the aim and leaves a snare that roots
// the first foe on it; rank 2 roots longer and bites
internal void
TestDisengageLeavesASnare()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Ranger = Slot->Entity;
    ranger_run *Run = &AppState->Dungeon->Ranger;
    SetClassTalentRank(Slot, RangerTalent_Disengage, 1);
    // NOTE(zoubir): aiming back toward the wall the run starts by, so the
    // leap goes into the room
    Ranger->Aim = V2(-1.f, 0.f);
    v3 Start = Ranger->Position;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Slot->RoleCooldowns[2] > 0.f);
    Check(RangerTrapDown(Run, 0));
    TickCrypt(&Crypt, 40);
    Check(Ranger->Position.X > Start.X + 120.f && Ranger->Position.X < Start.X + 260.f);
    Check(Slot->ClassFlags & RANGER_FLAG_TRAP);

    // NOTE(zoubir): a foe walks onto it
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, Start - Ranger->Position);
    RangerTick(&Crypt, &Dummies, 2);
    Check(!RangerTrapDown(Run, 0));
    Check(HasStatus(Foe, StatusEffect_Rooted));
    float Bite = 2000.f - Foe->Hp;
    Check(Bite > 0.f && Bite < 1.01f * TRAP_DAMAGE * GetRoleDef(PlayerRole_Ranger)->DamageDealt);

    // NOTE(zoubir): rank 2 bites harder; a new snare takes the old one's place
    SetClassTalentRank(Slot, RangerTalent_Disengage, 2);
    Foe->StatusTimers[StatusEffect_Rooted] = 0.f;
    Slot->RoleCooldowns[2] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Slot->RoleCooldowns[2] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    u32 Down = 0;
    for(u32 Index = 0; Index < RANGER_MAX_TRAPS; Index++)
    {
        Down += Run->Traps[Index].Seconds > 0.f ? 1 : 0;
    }
    Check(Down == 1);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Pinning Volley: a foe Volley struck stays slowed
// PINNING_VOLLEY_SECONDS longer per rank, so a second after the rain stops
// it is still slowed at rank 4 and free at rank 0
internal void
TestPinningVolleyHoldsTheSlow()
{
    for(u32 Rank = 0; Rank <= 4; Rank += 4)
    {
        crypt_world Crypt = CreateRangerWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, RangerTalent_PinningVolley, (u8)Rank);
        ranger_dummies Dummies = {};
        v2 Point = AimPoint(Slot->Entity);
        v3 Centre = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
        Centre.Z = 0.f;
        world_entity *Inside = RangerDummy(&Crypt, &Dummies, Centre);
        PressOnce(&Crypt, 0, PlayerButton_Launch);
        RangerTick(&Crypt, &Dummies, (u32)(60.f * (VOLLEY_DRAW + VOLLEY_SECONDS)) + 2);
        Check(Inside->Hp < 2000.f);
        float Left = Inside->StatusTimers[StatusEffect_Slowed];
        float Expected = VOLLEY_SLOW_SECONDS + PINNING_VOLLEY_SECONDS * (float)Rank - 0.5f * VOLLEY_TICK;
        Check(Left > Expected - 0.1f && Left < Expected + 0.1f);
        RangerTick(&Crypt, &Dummies, 60);
        Check(HasStatus(Inside, StatusEffect_Slowed) == (Rank == 4));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Hunter's Net: the snare springs on the foe that steps on
// it, and with the capstone roots every foe within HUNTERS_NET_RADIUS of
// it too, but none further; without it, only the first
internal void
TestHuntersNetRootsThePack()
{
    for(u32 Net = 0; Net < 2; Net++)
    {
        crypt_world Crypt = CreateRangerWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        world_entity *Ranger = Slot->Entity;
        SetClassTalentRank(Slot, RangerTalent_Disengage, 1);
        SetClassTalentRank(Slot, RangerTalent_HuntersNet, (u8)Net);
        Ranger->Aim = V2(-1.f, 0.f);
        v3 Start = Ranger->Position;
        PressOnce(&Crypt, 0, PlayerButton_Slam);
        TickCrypt(&Crypt, 40);
        Check(RangerTrapDown(&AppState->Dungeon->Ranger, 0));

        ranger_dummies Dummies = {};
        v3 Trap = Start - Ranger->Position;
        world_entity *First = RangerDummy(&Crypt, &Dummies, Trap);
        world_entity *Near = RangerDummy(&Crypt, &Dummies, Trap + V3(70.f, 80.f, 0.f));
        world_entity *Far = RangerDummy(&Crypt, &Dummies, Trap + V3(320.f, 0.f, 0.f));
        Check(Length(Near->Position.XY - Start.XY) < HUNTERS_NET_RADIUS);
        Check(Length(Near->Position.XY - Start.XY) > TRAP_RADIUS + 0.4f * Near->Dimensions.X);
        RangerTick(&Crypt, &Dummies, 2);
        Check(!RangerTrapDown(&AppState->Dungeon->Ranger, 0));
        Check(HasStatus(First, StatusEffect_Rooted));
        Check(HasStatus(Near, StatusEffect_Rooted) == (Net != 0));
        Check((Near->Hp < 2000.f) == (Net != 0));
        Check(!HasStatus(Far, StatusEffect_Rooted));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): V channels, arrows at the foe through the cast
internal void
TestRapidFireStreams()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, RangerTalent_RapidFire, 1);
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(250.f, 0.f, 0.f));
    Slot->Input.Target = (u32)(Foe - AppState->World.Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    Check(Slot->Entity->CastSpell == PlayerSpell_RangerB);
    RangerTick(&Crypt, &Dummies, 60);
    Check(Slot->ClassFlags & RANGER_FLAG_RAPID);
    Check(Foe->Hp < 2000.f);
    RangerTick(&Crypt, &Dummies, 80);
    Check(Slot->Entity->CastSpell == PlayerSpell_None);
    float Arrows = (2000.f - Foe->Hp) / (RAPID_FIRE_DAMAGE * GetRoleDef(PlayerRole_Ranger)->DamageDealt);
    Check(Arrows > RAPID_FIRE_ARROWS - 0.5f && Arrows < RAPID_FIRE_ARROWS + 0.5f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): with Lethal Mark the mark jumps from a foe that dies to the
// nearest one; without it, it goes
internal void
TestLethalMarkJumps()
{
    for(u32 Lethal = 0; Lethal < 2; Lethal++)
    {
        crypt_world Crypt = CreateRangerWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, RangerTalent_LethalMark, (u8)Lethal);
        ranger_dummies Dummies = {};
        world_entity *First = RangerDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
        world_entity *Next = RangerDummy(&Crypt, &Dummies, V3(260.f, 80.f, 0.f));
        Slot->Input.Target = (u32)(First - AppState->World.Entities) + 1;
        PressOnce(&Crypt, 0, PlayerButton_Cast);
        Check(IsRangerMarked(AppState, Slot, First));
        if (Lethal)
        {
            Check(RangerDealtScale(Slot, First) > 1.f + MARK_SHARE + LETHAL_MARK_SHARE - 0.001f);
        }
        DamageEntity(AppState, &AppState->World, First, 5000.f, Slot->Entity);
        RangerTick(&Crypt, &Dummies, 2);
        Check(IsRangerMarked(AppState, Slot, Next) == (Lethal != 0));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): between fights Focus drains away, and a player who leaves
// the class loses it
internal void
TestFocusDrains()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(!AppState->Dungeon->FightingRoom);
    Slot->Ranger.Focus = 60.f;
    Slot->Ranger.FocusHold = 0.f;
    TickCrypt(&Crypt, 60);
    Check(Slot->Ranger.Focus < 60.f - 0.9f * RANGER_FOCUS_DRAIN);
    Check(Slot->ClassMeter == (u8)(Slot->Ranger.Focus + 0.5f));
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    GrantClassSpells(Slot);
    TickCrypt(&Crypt, 1);
    Check(Slot->Ranger.Focus == 0.f && Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Kill Shot goes only at the Ranger's marked foe; twice as
// hard on one under KILL_SHOT_LOW; a kill brings it back at once
internal void
TestKillShot()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    ranger_dummies Dummies = {};
    world_entity *Foe = RangerDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    // NOTE(zoubir): no mark, no shot and no cooldown
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->RoleCooldowns[4] == 0.f);
    Slot->Input.Target = (u32)(Foe - AppState->World.Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_Cast);
    RangerTick(&Crypt, &Dummies, 30);
    float Before = Foe->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->RoleCooldowns[4] > 0.f);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * 300.f / RANGER_ARROW_SPEED) + 2);
    float Dealt = Before - Foe->Hp;
    float Scale = RangerDealtScale(Slot, Foe) * GetRoleDef(PlayerRole_Ranger)->DamageDealt;
    Check(Dealt > 0.99f * KILL_SHOT_DAMAGE * Scale && Dealt < 1.01f * KILL_SHOT_DAMAGE * Scale);
    // NOTE(zoubir): nearly dead, it lands twice as hard
    Foe->Hp = 0.2f * Foe->MaxHp;
    Before = Foe->Hp;
    Slot->RoleCooldowns[4] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * 300.f / RANGER_ARROW_SPEED) + 2);
    Dealt = Before - Foe->Hp;
    Check(Dealt > 0.99f * KILL_SHOT_LOW_SCALE * KILL_SHOT_DAMAGE * Scale);
    // NOTE(zoubir): a kill brings it back
    Foe->Hp = 5.f;
    Slot->RoleCooldowns[4] = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Slot->RoleCooldowns[4] > 0.f);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * 300.f / RANGER_ARROW_SPEED) + 2);
    Check(Foe->Hp <= 0.f || !Foe->IsPresent);
    Check(Slot->RoleCooldowns[4] == 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Survival's talents pay off either spell: with Hunter's
// Net a Volley's first arrows root what they catch; with Barrage the snare
// holds longer, and Pinning Volley keeps what it held slow after
internal void
TestSurvivalTalentsFitBothSpells()
{
    {
        crypt_world Crypt = CreateRangerWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, RangerTalent_HuntersNet, 1);
        ranger_dummies Dummies = {};
        v2 Point = AimPoint(Slot->Entity);
        v3 Centre = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
        Centre.Z = 0.f;
        world_entity *Inside = RangerDummy(&Crypt, &Dummies, Centre);
        PressOnce(&Crypt, 0, PlayerButton_Launch);
        RangerTick(&Crypt, &Dummies, (u32)(60.f * (VOLLEY_DRAW + 0.5f * VOLLEY_TICK)) + 2);
        Check(HasStatus(Inside, StatusEffect_Rooted));
        DestroyCryptWorld(&Crypt);
    }
    for(u32 Talents = 0; Talents < 2; Talents++)
    {
        crypt_world Crypt = CreateRangerWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        world_entity *Ranger = Slot->Entity;
        SetClassTalentRank(Slot, RangerTalent_Barrage, Talents);
        SetClassTalentRank(Slot, RangerTalent_PinningVolley, 2 * Talents);
        GrantClassSpells(Slot);
        Ranger->Aim = V2(-1.f, 0.f);
        v3 Start = Ranger->Position;
        PressOnce(&Crypt, 0, PlayerButton_Slam);
        TickCrypt(&Crypt, 40);
        ranger_dummies Dummies = {};
        world_entity *Foe = RangerDummy(&Crypt, &Dummies, Start - Ranger->Position);
        RangerTick(&Crypt, &Dummies, 2);
        float Root = Foe->StatusTimers[StatusEffect_Rooted];
        float Want = TRAP_ROOT_SECONDS + (Talents ? BARRAGE_ROOT_SECONDS : 0.f);
        Check(Root > Want - 0.1f && Root <= Want);
        Check(HasStatus(Foe, StatusEffect_Slowed) == (Talents != 0));
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Explosive Trap lands at the cursor and, when a foe steps
// on it, blows on every foe near it, rooting them with Hunter's Net; one
// far off is untouched
internal void
TestExplosiveTrap()
{
    crypt_world Crypt = CreateRangerWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    SetClassTalentRank(Slot, RangerTalent_HuntersNet, 1);
    GrantClassSpells(Slot);
    v2 Point = AimPoint(Slot->Entity);
    v3 Spot = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
    Spot.Z = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->RoleCooldowns[6] > 0.f);
    Check(RangerTrapDown(&AppState->Dungeon->Ranger, 0));
    ranger_dummies Dummies = {};
    world_entity *On = RangerDummy(&Crypt, &Dummies, Spot);
    world_entity *Near = RangerDummy(&Crypt, &Dummies, Spot + V3(60.f, 30.f, 0.f));
    world_entity *Far = RangerDummy(&Crypt, &Dummies, Spot + V3(0.f, 3.f * EXPLOSIVE_TRAP_RADIUS, 0.f));
    RangerTick(&Crypt, &Dummies, 2);
    Check(!RangerTrapDown(&AppState->Dungeon->Ranger, 0));
    Check(On->Hp < 2000.f && Near->Hp < 2000.f && Far->Hp == 2000.f);
    Check(HasStatus(Near, StatusEffect_Rooted) && !HasStatus(Far, StatusEffect_Rooted));
    DestroyCryptWorld(&Crypt);
}

internal void
RunRangerTests()
{
    TestRangerKeys();
    TestQuickShotLandsOnArrival();
    TestQuickShotMarks();
    TestKillShot();
    TestExplosiveTrap();
    TestPiercingShotThroughALine();
    TestVolleyRainsOnTheCircle();
    TestDisengageLeavesASnare();
    TestPinningVolleyHoldsTheSlow();
    TestHuntersNetRootsThePack();
    TestSurvivalTalentsFitBothSpells();
    TestRapidFireStreams();
    TestLethalMarkJumps();
    TestFocusDrains();
}
