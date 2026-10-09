/* Berserker tests (sim/dungeon/role_kits/berserker.cpp), included by
   dungeon_tests.cpp: the class's five keys, Berserk once learned, and X
   and C doing nothing; Cleave hits the
   arc in front and nothing behind, swinging each way in turn, and goes
   at a monster picked under the cursor; Rage builds
   from hits landed and taken and drains once calm; Whirlwind and Execute
   need their Rage and spend it; Execute's big hit on a foe near death and
   Massacre's refund; the Leap lands at the cursor and stuns; Bloodthirst
   makes Execute heal, more at rank 2;
   Berserk; Bladestorm makes Whirlwind hit harder by rank; Shattering
   Leap sunders what the landing strikes; and a client predicting its own Berserker winds up but leaves
   the blows and the Rage to the server. */

// NOTE(zoubir): a big brute Offset from the Berserker in slot 0, stunned
// for good so it neither walks nor hits back
internal world_entity *
BerserkerDummy(crypt_world *Crypt, v3 Offset)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Berserker = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Berserker->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    ApplyStatus(Result, StatusEffect_Stunned, 1000.f);
    return Result;
}

// NOTE(zoubir): a crypt with one Berserker facing +X
internal crypt_world
BerserkerCrypt(u32 Players = 1)
{
    crypt_world Result = CreateCryptWorld(Players);
    TickCrypt(&Result, 1);
    player_slot *Slot = &Result.AppState->Players[0];
    SetPlayerRole(Result.AppState, Slot, PlayerRole_Berserker);
    GrantClassSpells(Slot);
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Entity->Aim = V2(1.f, 0.f);
    return Result;
}

internal void
TestBerserkerKeys()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_Berserker));
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    u32 Main = PlayerButton_Push | PlayerButton_Attack;
    Check((Allowed & Main) == Main);
    Check(!(Allowed & (PlayerButton_Launch | PlayerButton_Shockwave | PlayerButton_Slam |
                       PlayerButton_Kunai | PlayerButton_Cast)));
    SetClassTalentRank(Slot, BerserkerTalent_Bloodthirst, 2);
    SetClassTalentRank(Slot, BerserkerTalent_Berserk, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check(Allowed & PlayerButton_Kunai);
    Check(!(Allowed & (PlayerButton_Slam | PlayerButton_Cast)));
    Check(BerserkerKeyWindsUp(1) && BerserkerKeyWindsUp(4) && !BerserkerKeyWindsUp(6));
    // NOTE(zoubir): the right click is Cleave, not the sword
    Check(RoleSpellOnButton(AppState, Slot->Entity, PlayerButton_Attack) == &BerserkerSpells[6]);
    Check(RoleSpellOnButton(AppState, Slot->Entity, PlayerButton_Cast) == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a foe in front and one to the side are cut, one behind is
// not; each swing goes the other way round and builds Rage
internal void
TestCleaveHitsTheArc()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Front = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    world_entity *Side = BerserkerDummy(&Crypt, V3(10.f, 64.f, 0.f));
    world_entity *Behind = BerserkerDummy(&Crypt, V3(-70.f, 0.f, 0.f));
    world_entity *Far = BerserkerDummy(&Crypt, V3(200.f, 0.f, 0.f));
    AppState->Events.Count = 0;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Front->Hp < 2000.f && Side->Hp < 2000.f);
    Check(Behind->Hp == 2000.f && Far->Hp == 2000.f);
    Check(Slot->RoleCooldowns[6] > 0.f);
    Check(BerserkerRage(Slot) > 0);
    bool32 Forward = false;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        Forward |= AppState->Events.Events[Index].Burst ==
            ClassBurst(SimBurst_BerserkerFirst, BerserkerBurst_Cleave);
    }
    Check(Forward && Slot->Berserker.CleaveBack);
    TickCrypt(&Crypt, 60);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(!Slot->Berserker.CleaveBack);
    // NOTE(zoubir): Sweeping Strikes: three foes caught hit harder each
    float Before = Front->Hp;
    StrikeAround(AppState, Slot, Slot->Entity, V2(1.f, 0.f), CLEAVE_REACH, CLEAVE_HALF_ANGLE,
                 10.f, 0.f);
    float Plain = Before - Front->Hp;
    SetClassTalentRank(Slot, BerserkerTalent_SweepingStrikes, 1);
    Before = Front->Hp;
    StrikeAround(AppState, Slot, Slot->Entity, V2(1.f, 0.f), CLEAVE_REACH, CLEAVE_HALF_ANGLE,
                 10.f, 0.f);
    Check(Before - Front->Hp > Plain * (1.f + SWEEPING_PER_FOE) - 0.01f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a swing goes at the monster under the cursor in reach,
// whatever the aim, and along the aim when it is out of reach
internal void
TestCleaveFollowsThePick()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Behind = BerserkerDummy(&Crypt, V3(-60.f, 0.f, 0.f));
    world_entity *Front = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    Slot->Input.Target = (u32)(Behind - AppState->World.Entities) + 1;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Behind->Hp < 2000.f && Front->Hp == 2000.f);
    world_entity *Far = BerserkerDummy(&Crypt, V3(-300.f, 0.f, 0.f));
    Slot->Input.Target = (u32)(Far - AppState->World.Entities) + 1;
    TickCrypt(&Crypt, 60);
    float Before = Front->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Front->Hp < Before && Far->Hp == 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Rage from a hit taken, held while fighting, drained once
// calm, held under Berserk
internal void
TestRageBuildsAndDrains()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    TickCrypt(&Crypt, 1);
    DamageEntity(AppState, &AppState->World, Player, 20.f, 0);
    TickCrypt(&Crypt, 1);
    Check(BerserkerRage(Slot) >= 5);
    AddRage(Slot, 40.f);
    u32 Held = BerserkerRage(Slot);
    TickCrypt(&Crypt, 60);
    Check(BerserkerRage(Slot) == Held);
    TickCrypt(&Crypt, (u32)(60.f * (BERSERKER_CALM_SECONDS + 2.f)));
    Check(BerserkerRage(Slot) < Held);
    // NOTE(zoubir): never past the top, and Unbridled Wrath gives more
    AddRage(Slot, 500.f);
    Check(BerserkerRage(Slot) == BERSERKER_RAGE_MAX);
    Slot->ClassMeter = 0;
    Slot->Berserker.RageCarry = 0.f;
    AddRage(Slot, 10.f);
    Check(BerserkerRage(Slot) == 10);
    SetClassTalentRank(Slot, BerserkerTalent_UnbridledWrath, 1);
    AddRage(Slot, 10.f);
    Check(BerserkerRage(Slot) == 10 + (u32)(10.f * (1.f + UNBRIDLED_WRATH_SHARE)));
    // NOTE(zoubir): Berserk: Rage holds, more dealt, less taken
    SetClassTalentRank(Slot, BerserkerTalent_Berserk, 1);
    world_entity *Foe = BerserkerDummy(&Crypt, V3(300.f, 0.f, 0.f));
    float Plain = DungeonScaleDamage(AppState, Foe, Player, 10.f);
    PressOnce(&Crypt, 0, PlayerButton_Kunai);
    Check(Slot->Berserker.BerserkSeconds > 0.f && (Slot->ClassFlags & BERSERKER_FLAG_BERSERK));
    float Raging = DungeonScaleDamage(AppState, Foe, Player, 10.f);
    Check(Raging > (1.f + BERSERK_DEALT_SHARE) * Plain - 0.01f);
    Check(BerserkerTakenScale(Slot, Player) < 1.f);
    Held = BerserkerRage(Slot);
    TickCrypt(&Crypt, (u32)(60.f * (BERSERKER_CALM_SECONDS + 2.f)));
    Check(BerserkerRage(Slot) == Held);
    TickCrypt(&Crypt, (u32)(60.f * BERSERK_SECONDS));
    Check(!(Slot->ClassFlags & BERSERKER_FLAG_BERSERK));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Whirlwind without its Rage does nothing and flashes the
// HUD; with it, it spends the Rage and hits all round, WHIRLWIND_HITS times
internal void
TestWhirlwindSpinsForRage()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Front = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    world_entity *Back = BerserkerDummy(&Crypt, V3(-60.f, 10.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Player->CastSpell == 0 && Slot->RoleCooldowns[1] == 0.f);
    Check(Slot->ClassFlags & BERSERKER_FLAG_NO_RAGE);
    AddRage(Slot, (float)WHIRLWIND_RAGE + 5.f);
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Player->CastSpell == PlayerSpell_BerserkerA);
    Check(BerserkerRage(Slot) <= 5 + 1);
    TickCrypt(&Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_BerserkerA].CastTime) + 2);
    Check(Player->CastSpell == 0);
    Check(Slot->Berserker.WhirlHits == WHIRLWIND_HITS);
    float Each = WHIRLWIND_DAMAGE * GetRoleDef(PlayerRole_Berserker)->DamageDealt;
    Check(2000.f - Front->Hp > (float)(WHIRLWIND_HITS - 1) * Each);
    Check(2000.f - Back->Hp > (float)(WHIRLWIND_HITS - 1) * Each);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Execute needs Rage, winds up, then spends all of it; the
// more Rage, the harder; a foe near death takes twice that, and Massacre
// gives Rage back for the kill
internal void
TestExecuteSpendsRage()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    u32 WindUp = (u32)(60.f * PlayerSpells[PlayerSpell_BerserkerB].CastTime) + 2;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Player->CastSpell == 0 && Slot->RoleCooldowns[4] == 0.f);
    Check(Slot->ClassFlags & BERSERKER_FLAG_NO_RAGE);
    AddRage(Slot, 30.f);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Player->CastSpell == PlayerSpell_BerserkerB);
    TickCrypt(&Crypt, WindUp);
    float Some = 2000.f - Foe->Hp;
    Check(Some > 0.f && BerserkerRage(Slot) == 0);

    // NOTE(zoubir): a full bar hits far harder
    Foe->Hp = 2000.f;
    Slot->RoleCooldowns[4] = 0.f;
    AddRage(Slot, 100.f);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    TickCrypt(&Crypt, WindUp);
    float Full = 2000.f - Foe->Hp;
    Check(Full > 1.6f * Some);

    // NOTE(zoubir): near death: twice as hard
    Foe->Hp = 0.2f * 2000.f;
    Slot->RoleCooldowns[4] = 0.f;
    AddRage(Slot, 100.f);
    float Low = Foe->Hp;
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    TickCrypt(&Crypt, WindUp);
    Check(Low - Foe->Hp > 0.95f * EXECUTE_LOW_SCALE * Full);

    // NOTE(zoubir): Massacre: a kill gives Rage back
    SetClassTalentRank(Slot, BerserkerTalent_Massacre, 1);
    world_entity *Weak = BerserkerDummy(&Crypt, V3(60.f, 30.f, 0.f));
    Weak->Hp = 30.f;
    Foe->Hp = 2000.f;
    Foe->Position.XY = Player->Position.XY + V2(0.f, -300.f);
    Slot->RoleCooldowns[4] = 0.f;
    Slot->Input.Target = (u32)(Weak - AppState->World.Entities) + 1;
    AddRage(Slot, 50.f);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    TickCrypt(&Crypt, WindUp);
    Check(Weak->Hp <= 0.f);
    Check(BerserkerRage(Slot) >= MASSACRE_REFUND);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): A jumps to the cursor, lands there and stuns what is round
// it, not what is far
internal void
TestLeapLandsAndStuns()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    v2 Start = Player->Position.XY;
    Slot->Input.Aim = V2(0.6f, 0.f);
    TickCrypt(&Crypt, 1);
    v2 Spot = AimPoint(Player);
    world_entity *There = SpawnMonster(AppState, &AppState->World, &Crypt.Arena,
                                       V3(Spot.X + 30.f, Spot.Y, Player->Position.Z),
                                       MonsterKind_Brute);
    There->MaxHp = There->Hp = 2000.f;
    world_entity *Away = BerserkerDummy(&Crypt, V3(-200.f, 0.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Slot->Berserker.Leaping && (Slot->ClassFlags & BERSERKER_FLAG_LEAPING));
    Check(Slot->RoleCooldowns[0] > 0.f);
    for(u32 Tick = 0; Tick < 120 && Slot->Berserker.Leaping; Tick++)
    {
        TickCrypt(&Crypt, 1);
    }
    Check(!Slot->Berserker.Leaping);
    Check(Length(Player->Position.XY - Spot) < 40.f);
    Check(Length(Player->Position.XY - Start) > 100.f);
    Check(There->Hp < 2000.f && HasStatus(There, StatusEffect_Stunned));
    Check(Away->Hp == 2000.f);
    TickCrypt(&Crypt, 1);
    Check(!(Slot->ClassFlags & BERSERKER_FLAG_LEAPING));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): in a Leap's flight the walk keys and the drag leave the
// speed across alone (ClassCarriesPlayer), as on a client predicting its
// own Berserker, which has the flag from the snapshot but not the leap's
// state that steers it on the server
internal void
TestLeapFlightIgnoresTheKeys()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    Slot->Input.Aim = V2(0.6f, 0.f);
    TickCrypt(&Crypt, 1);
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    TickCrypt(&Crypt, 2);
    Check(!IsOnGround(Player) && (Slot->ClassFlags & BERSERKER_FLAG_LEAPING));
    // NOTE(zoubir): one step of the player alone, as client/prediction.cpp
    // runs it: no dungeon update, so no steering and the flag stays
    v2 Across = Player->Velocity.XY;
    Slot->Input.Move = V2(0.f, 1.f);
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    Slot->Predicted = true;
    UpdatePlayer(Slot, &AppState->World, &Crypt.Arena, 1.f / 60.f, AppState,
                 &AnimationSpeed, &AnimationType, &AnimationDirection);
    Slot->Predicted = false;
    Check(LengthSq(Player->Velocity.XY - Across) < 1.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): what an Execute on full Rage healed the Berserker from 50
// health, and what it dealt
internal float
ExecuteHealed(crypt_world *Crypt, world_entity *Foe, float *Dealt)
{
    player_slot *Slot = &Crypt->AppState->Players[0];
    world_entity *Player = Slot->Entity;
    Player->Hp = 50.f;
    Foe->Hp = 2000.f;
    Slot->RoleCooldowns[4] = 0.f;
    AddRage(Slot, 100.f);
    PressOnce(Crypt, 0, PlayerButton_Shockwave);
    TickCrypt(Crypt, (u32)(60.f * PlayerSpells[PlayerSpell_BerserkerB].CastTime) + 2);
    *Dealt = 2000.f - Foe->Hp;
    return Player->Hp - 50.f;
}

// NOTE(zoubir): Bloodthirst makes Execute heal for a share of what it
// deals, more at rank 2; without it Execute heals nothing (past the rest
// between fights, the same every time)
internal void
TestBloodthirstHeals()
{
    crypt_world Crypt = BerserkerCrypt();
    player_slot *Slot = &Crypt.AppState->Players[0];
    world_entity *Foe = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    float Dealt = 0.f;
    float Rest = ExecuteHealed(&Crypt, Foe, &Dealt);
    Check(Dealt > 0.f && Rest < 0.1f * Dealt);
    SetClassTalentRank(Slot, BerserkerTalent_Bloodthirst, 1);
    float Healed = ExecuteHealed(&Crypt, Foe, &Dealt) - Rest;
    Check(Healed > 0.99f * BLOODTHIRST_HEAL_SHARE * Dealt && Healed < 1.01f * BLOODTHIRST_HEAL_SHARE * Dealt);
    SetClassTalentRank(Slot, BerserkerTalent_Bloodthirst, 2);
    float More = ExecuteHealed(&Crypt, Foe, &Dealt) - Rest;
    Check(More > 0.99f * BLOODTHIRST_RANK2_HEAL * Dealt && More < 1.01f * BLOODTHIRST_RANK2_HEAL * Dealt);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): online, the client predicting its own Berserker starts the
// Whirlwind's spin and Execute's wind-up, but deals nothing and leaves
// its Rage alone, and casts none of the rest
internal void
TestPredictedBerserkerWindsUp()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Player = Slot->Entity;
    world_entity *Foe = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    AddRage(Slot, 60.f);
    Slot->Predicted = true;
    PressOnce(&Crypt, 0, PlayerButton_Push);
    Check(Player->CastSpell == PlayerSpell_BerserkerA);
    Check(BerserkerRage(Slot) == 60);
    TickCrypt(&Crypt, 120);
    Check(Player->CastSpell == 0);
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(!Slot->Berserker.Leaping);
    PressOnce(&Crypt, 0, PlayerButton_Shockwave);
    Check(Player->CastSpell == PlayerSpell_BerserkerB);
    TickCrypt(&Crypt, 60);
    Check(Foe->Hp == 2000.f);
    Slot->Predicted = false;
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Bladestorm: each rank makes a turn of the Whirlwind hit
// BLADESTORM_SHARE harder, +60% at rank 4
internal void
TestBladestormWhirlsHarder()
{
    crypt_world Crypt = BerserkerCrypt();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    world_entity *Foe = BerserkerDummy(&Crypt, V3(60.f, 0.f, 0.f));
    WhirlHit(AppState, Slot, Slot->Entity);
    float Plain = 2000.f - Foe->Hp;
    Check(Plain > 0.f);
    SetClassTalentRank(Slot, BerserkerTalent_Bladestorm, 4);
    Foe->Hp = 2000.f;
    WhirlHit(AppState, Slot, Slot->Entity);
    float Storm = 2000.f - Foe->Hp;
    float Want = (1.f + 4.f * BLADESTORM_SHARE) * Plain;
    Check(Storm > 0.99f * Want && Storm < 1.01f * Want);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Shattering Leap: a foe the landing strikes is sundered for
// SHATTERING_LEAP_SECONDS, taking SHATTERING_LEAP_SHARE more from
// everyone; one out of reach is not, and without the talent nothing is
internal void
TestShatteringLeapSunders()
{
    for(u32 Learned = 0; Learned < 2; Learned++)
    {
        crypt_world Crypt = BerserkerCrypt();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        world_entity *Player = Slot->Entity;
        world *World = &AppState->World;
        SetClassTalentRank(Slot, BerserkerTalent_ShatteringLeap, (u8)Learned);
        world_entity *Near = BerserkerDummy(&Crypt, V3(40.f, 0.f, 0.f));
        world_entity *Away = BerserkerDummy(&Crypt, V3(-250.f, 0.f, 0.f));
        LeapSlam(AppState, Slot, Player);
        Check(Near->Hp < 2000.f && Away->Hp == 2000.f);
        foe_mark *Mark = FindFoeMark(AppState->Dungeon, World, Near);
        Check(!FindFoeMark(AppState->Dungeon, World, Away));
        if (Learned)
        {
            Check(Mark && Mark->SunderSeconds == SHATTERING_LEAP_SECONDS);
            float Scale = FoeMarkDamageScale(AppState->Dungeon, World, Near);
            Check(Scale > 0.999f * (1.f + SHATTERING_LEAP_SHARE) &&
                  Scale < 1.001f * (1.f + SHATTERING_LEAP_SHARE));
            UpdateFoeMarks(AppState->Dungeon, World, SHATTERING_LEAP_SECONDS + 0.1f);
            Check(FoeMarkDamageScale(AppState->Dungeon, World, Near) == 1.f);
        }
        else
        {
            Check(!Mark);
        }
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): Battle Shout fills the Rage, raises the damage of the
// Berserker and an ally near, not one far off, runs out with its time,
// and with Shattering Leap sunders the foes round it
internal void
TestBattleShout()
{
    crypt_world Crypt = BerserkerCrypt(3);
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    player_slot *Near = &AppState->Players[1];
    player_slot *Far = &AppState->Players[2];
    world_entity *Player = Slot->Entity;
    v3 Old = Near->Entity->Position;
    Near->Entity->Position = Player->Position + V3(100.f, 0.f, 0.f);
    CheckAndChangeEntityChunk(AppState, &AppState->World, &Crypt.Arena, Old, Near->Entity);
    Old = Far->Entity->Position;
    Far->Entity->Position = Player->Position + V3(0.f, BATTLE_SHOUT_REACH + 200.f, 0.f);
    CheckAndChangeEntityChunk(AppState, &AppState->World, &Crypt.Arena, Old, Far->Entity);
    // NOTE(zoubir): the far ally is checked by distance, wherever the walls
    // put it
    float FarOff = Length(Far->Entity->Position.XY - Player->Position.XY);
    world_entity *Foe = BerserkerDummy(&Crypt, V3(80.f, 40.f, 0.f));
    SetClassTalentRank(Slot, BerserkerTalent_ShatteringLeap, 1);
    GrantClassSpells(Slot);
    Check(BerserkerRage(Slot) == 0);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    TickCrypt(&Crypt, 1);
    Check(BerserkerRage(Slot) == BERSERKER_RAGE_MAX);
    Check(Slot->RoleCooldowns[2] > 0.f && (Slot->ClassFlags & BERSERKER_FLAG_SHOUT));
    Check(Slot->RunAuraDamage >= BATTLE_SHOUT_SHARE - 0.001f);
    Check(Near->RunAuraDamage >= BATTLE_SHOUT_SHARE - 0.001f);
    FarOff = Length(Far->Entity->Position.XY - Player->Position.XY);
    Check(FarOff <= BATTLE_SHOUT_REACH || Far->RunAuraDamage < 0.001f);
    Check(FindFoeMark(AppState->Dungeon, &AppState->World, Foe) != 0);
    TickCrypt(&Crypt, (u32)(60.f * BATTLE_SHOUT_SECONDS) + 2);
    Check(!(Slot->ClassFlags & BERSERKER_FLAG_SHOUT) && Near->RunAuraDamage < 0.001f);
    DestroyCryptWorld(&Crypt);
}

internal void
RunBerserkerTests()
{
    TestBerserkerKeys();
    TestBattleShout();
    TestCleaveHitsTheArc();
    TestCleaveFollowsThePick();
    TestRageBuildsAndDrains();
    TestWhirlwindSpinsForRage();
    TestExecuteSpendsRage();
    TestLeapLandsAndStuns();
    TestLeapFlightIgnoresTheKeys();
    TestBloodthirstHeals();
    TestPredictedBerserkerWindsUp();
    TestBladestormWhirlsHarder();
    TestShatteringLeapSunders();
}
