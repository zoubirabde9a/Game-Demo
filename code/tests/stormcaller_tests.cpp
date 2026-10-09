/* Stormcaller tests (sim/dungeon/role_kits/stormcaller.cpp), included by
   dungeon_tests.cpp: the class's keys and what its bursts carry; Spark
   jumping once and building Charge, farther and harder when
   Supercharged; Chain Lightning locking its foe at the press, never
   striking a foe twice, and Conductor; Static Field shocking and slowing
   inside its circle only and making lightning arc across it, wider and
   harder with Arc Field; Thunderclap needing 20 Charge, locking its foe,
   spending all the Charge, stunning and splashing from 70; Capacitor and
   Stormbringer; the overload's nova and its cost, the cooldown floor
   while grounded, and Live Wire; Eye of the Storm holding Charge at the
   top and striking; Lightning Dash; Charge draining; Voltage; and a
   client predicting its own Stormcaller. */

struct storm_dummies
{
    world_entity *Units[8];
    v3 Spots[8];
    u32 Count;
};

// NOTE(zoubir): a still monster Offset from the Stormcaller in slot 0,
// with health to spare
internal world_entity *
StormDummy(crypt_world *Crypt, storm_dummies *Dummies, v3 Offset)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Storm = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Storm->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    Dummies->Units[Dummies->Count] = Result;
    Dummies->Spots[Dummies->Count] = Result->Position;
    Dummies->Count++;
    return Result;
}

// NOTE(zoubir): Ticks of the world with every dummy held where it stood
// and the Stormcaller kept alive
internal void
StormTick(crypt_world *Crypt, storm_dummies *Dummies, u32 Ticks)
{
    world_entity *Storm = Crypt->AppState->Players[0].Entity;
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
        Storm->Hp = Storm->MaxHp;
        TickCrypt(Crypt, 1);
    }
}

// NOTE(zoubir): a Stormcaller in slot 0 of a fresh crypt, aiming along +X
internal crypt_world
CreateStormWorld()
{
    crypt_world Crypt = CreateCryptWorld(1);
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &Crypt.AppState->Players[0];
    SetPlayerRole(Crypt.AppState, Slot, PlayerRole_Stormcaller);
    GrantClassSpells(Slot);
    Slot->Entity->Aim = V2(1.f, 0.f);
    Slot->Entity->AimReach = 0.6f;
    return Crypt;
}

// NOTE(zoubir): what a hit of Damage before the class's scale deals
inline float
StormDealt(float Damage)
{
    float Result = Damage * GetRoleDef(PlayerRole_Stormcaller)->DamageDealt;
    return Result;
}

inline bool32
StormNear(float Got, float Expected)
{
    bool32 Result = Got > 0.99f * Expected - 0.01f && Got < 1.01f * Expected + 0.01f;
    return Result;
}

// NOTE(zoubir): Charge set as a fight would leave it, not draining
inline void
SetStormCharge(player_slot *Slot, float Charge)
{
    Slot->Stormcaller.Charge = Charge;
    Slot->Stormcaller.IdleSeconds = 0.f;
}

internal void
TestStormcallerKeys()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Check(RoleHasKit(PlayerRole_Stormcaller));
    Check(RoleDropsFireball(PlayerRole_Stormcaller));
    // NOTE(zoubir): the two base spells only, before any point
    ResetRoleTalents(Slot);
    u32 Allowed = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
    Check(Allowed == (DUNGEON_SHARED_BUTTONS | PlayerButton_Shockwave | PlayerButton_Launch));
    SetClassTalentRank(Slot, StormcallerTalent_LightningDash, 1);
    SetClassTalentRank(Slot, StormcallerTalent_EyeOfTheStorm, 1);
    Allowed = RunAllowedButtons(AppState, Slot, 0);
    Check((Allowed & PlayerButton_Slam) && (Allowed & PlayerButton_Kunai));
    Check(!(Allowed & PlayerButton_Attack));
    Check(StormcallerKeyWindsUp(0) && StormcallerKeyWindsUp(4));
    Check(!StormcallerKeyWindsUp(1) && !StormcallerKeyWindsUp(5));
    // NOTE(zoubir): a burst's variant rides in its height and comes off it
    // again; a bolt's kind and length ride in the variant
    for(u32 Variant = 0; Variant < 400; Variant += 37)
    {
        for(float Z = -300.f; Z <= 1500.f; Z += 450.f)
        {
            v3 Spot = StormcallerBurstSpot(V3(10.f, 20.f, Z), Variant);
            Check(StormcallerBurstVariant(Spot) == Variant);
            Check(Absolute(StormcallerBurstPlace(Spot).Z - Z) < 0.01f);
        }
    }
    u32 Bolt = StormcallerBoltVariant(StormcallerBolt_Arc, 333.f);
    Check(StormcallerBoltKind(Bolt) == StormcallerBolt_Arc);
    Check(Absolute(StormcallerBoltLength(Bolt) - 333.f) <= 0.5f * STORMCALLER_BOLT_UNIT);
    // NOTE(zoubir): Voltage
    Check(StormcallerDealtScale(Slot, 0) == 1.f);
    SetClassTalentRank(Slot, StormcallerTalent_Voltage, 2);
    Check(Absolute(StormcallerDealtScale(Slot, 0) - (1.f + 2.f * VOLTAGE_SHARE)) < 0.001f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): X strikes the foe aimed at and jumps to the nearest other
// for SPARK_JUMP_SHARE, building SPARK_CHARGE; one past the jump radius
// is left alone. Supercharged it jumps once more and hits harder
internal void
TestSparkJumpsAndCharges()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    storm_dummies Dummies = {};
    world_entity *First = StormDummy(&Crypt, &Dummies, V3(250.f, 0.f, 0.f));
    world_entity *Second = StormDummy(&Crypt, &Dummies, V3(250.f, 120.f, 0.f));
    world_entity *Third = StormDummy(&Crypt, &Dummies, V3(250.f, 260.f, 0.f));
    PressAt(&Crypt, 0, PlayerButton_Cast, First);
    Check(Slot->RoleCooldowns[5] > 0.f);
    Check(StormNear(2000.f - First->Hp, StormDealt(SPARK_DAMAGE)));
    Check(StormNear(2000.f - Second->Hp, StormDealt(SPARK_JUMP_SHARE * SPARK_DAMAGE)));
    Check(Third->Hp == 2000.f);
    Check(Slot->Stormcaller.Charge == SPARK_CHARGE);
    StormTick(&Crypt, &Dummies, 1);
    Check(Slot->ClassMeter == (u8)SPARK_CHARGE);
    Check(!(Slot->ClassFlags & STORMCALLER_FLAG_SUPERCHARGED));

    // NOTE(zoubir): Supercharged: the third is reached from the second
    SetStormCharge(Slot, STORM_SUPERCHARGED);
    StormTick(&Crypt, &Dummies, 1);
    Check(Slot->ClassFlags & STORMCALLER_FLAG_SUPERCHARGED);
    float Before[3] = {First->Hp, Second->Hp, Third->Hp};
    Slot->RoleCooldowns[5] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Cast, First);
    float Super = SPARK_DAMAGE * (1.f + SUPERCHARGED_SHARE);
    Check(StormNear(Before[0] - First->Hp, StormDealt(Super)));
    Check(StormNear(Before[1] - Second->Hp, StormDealt(SPARK_JUMP_SHARE * Super)));
    Check(StormNear(Before[2] - Third->Hp, StormDealt(SPARK_JUMP_SHARE * SPARK_JUMP_SHARE * Super)));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Ticks for a Chain Lightning's cast
#define STORM_CHAIN_TICKS ((u32)(60.f * PlayerSpells[PlayerSpell_StormcallerA].CastTime) + 2)
#define STORM_CLAP_TICKS ((u32)(60.f * PlayerSpells[PlayerSpell_StormcallerB].CastTime) + 2)

// NOTE(zoubir): A winds up on the foe pressed on and keeps it however it
// moves; the bolt jumps CHAIN_JUMPS times, weaker each jump, and with
// three foes in reach of each other strikes each exactly once
internal void
TestChainLightningNeverTwice()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    // NOTE(zoubir): nobody in reach: no wind-up, no cooldown
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(Slot->Entity->CastSpell == 0 && Slot->RoleCooldowns[0] == 0.f);
    storm_dummies Dummies = {};
    world_entity *A = StormDummy(&Crypt, &Dummies, V3(200.f, 0.f, 0.f));
    world_entity *B = StormDummy(&Crypt, &Dummies, V3(280.f, 60.f, 0.f));
    world_entity *C = StormDummy(&Crypt, &Dummies, V3(280.f, -60.f, 0.f));
    PressAt(&Crypt, 0, PlayerButton_Launch, B);
    Check(Slot->Entity->CastSpell == PlayerSpell_StormcallerA);
    Check(A->Hp == 2000.f && B->Hp == 2000.f);
    StormTick(&Crypt, &Dummies, STORM_CHAIN_TICKS);
    Check(Slot->Entity->CastSpell == PlayerSpell_None);
    // NOTE(zoubir): B first, then the nearest of the others, then the last
    Check(StormNear(2000.f - B->Hp, StormDealt(CHAIN_DAMAGE)));
    float Jump1 = StormDealt(CHAIN_JUMP_SHARE * CHAIN_DAMAGE);
    float Jump2 = StormDealt(CHAIN_JUMP_SHARE * CHAIN_JUMP_SHARE * CHAIN_DAMAGE);
    float DealtA = 2000.f - A->Hp;
    float DealtC = 2000.f - C->Hp;
    Check((StormNear(DealtA, Jump1) && StormNear(DealtC, Jump2)) ||
          (StormNear(DealtA, Jump2) && StormNear(DealtC, Jump1)));
    Check(Slot->Stormcaller.Charge == 3.f * CHAIN_CHARGE);
    Check(Slot->RoleCooldowns[0] > CHAIN_COOLDOWN - 1.f);

    // NOTE(zoubir): the foe pressed on is held when it backs off past the
    // cursor's pick, and nearer ones are not struck first
    float BBefore = B->Hp;
    Slot->RoleCooldowns[0] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Launch, B);
    Dummies.Spots[1].X += 200.f;
    StormTick(&Crypt, &Dummies, STORM_CHAIN_TICKS);
    Check(StormNear(BBefore - B->Hp, StormDealt(CHAIN_DAMAGE)));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Conductor: a line of seven foes 50 apart, the chain goes
// down six with 90% a jump; without it, four with 80%
internal void
TestConductorChainsFarther()
{
    for(u32 Conductor = 0; Conductor < 2; Conductor++)
    {
        crypt_world Crypt = CreateStormWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, StormcallerTalent_Conductor, (u8)Conductor);
        storm_dummies Dummies = {};
        for(u32 Index = 0; Index < 7; Index++)
        {
            StormDummy(&Crypt, &Dummies, V3(40.f + 50.f * (float)Index, 0.f, 0.f));
        }
        PressAt(&Crypt, 0, PlayerButton_Launch, Dummies.Units[0]);
        StormTick(&Crypt, &Dummies, STORM_CHAIN_TICKS);
        u32 Strikes = CHAIN_JUMPS + 1 + (Conductor ? CONDUCTOR_JUMPS : 0);
        float Share = Conductor ? CONDUCTOR_JUMP_SHARE : CHAIN_JUMP_SHARE;
        float Damage = CHAIN_DAMAGE;
        for(u32 Index = 0; Index < Dummies.Count; Index++)
        {
            float Dealt = 2000.f - Dummies.Units[Index]->Hp;
            Check(Index < Strikes ? StormNear(Dealt, StormDealt(Damage)) : Dealt == 0.f);
            Damage *= Share;
        }
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): R puts a field at the cursor: what is inside is shocked
// each tick and slowed, what is outside untouched; Arc Field widens it
// and makes it bite harder
internal void
TestStaticFieldShocks()
{
    for(u32 Arc = 0; Arc <= 4; Arc += 4)
    {
        crypt_world Crypt = CreateStormWorld();
        app_state *AppState = Crypt.AppState;
        player_slot *Slot = &AppState->Players[0];
        SetClassTalentRank(Slot, StormcallerTalent_ArcField, (u8)Arc);
        storm_dummies Dummies = {};
        v2 Point = AimPoint(Slot->Entity);
        v3 Centre = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
        Centre.Z = 0.f;
        world_entity *Inside = StormDummy(&Crypt, &Dummies, Centre + V3(10.f, 0.f, 0.f));
        world_entity *Edge = StormDummy(&Crypt, &Dummies, Centre + V3(0.f, 1.3f * STATIC_FIELD_RADIUS, 0.f));
        world_entity *Outside = StormDummy(&Crypt, &Dummies, Centre + V3(0.f, -2.5f * STATIC_FIELD_RADIUS, 0.f));
        PressOnce(&Crypt, 0, PlayerButton_Push);
        Check(Slot->RoleCooldowns[1] > 0.f);
        Check(Slot->Stormcaller.Charge == 0.f);
        StormTick(&Crypt, &Dummies, 1);
        Check(Slot->ClassFlags & STORMCALLER_FLAG_FIELD);
        StormTick(&Crypt, &Dummies, (u32)(60.f * STATIC_FIELD_SECONDS) + 10);
        Check(!(Slot->ClassFlags & STORMCALLER_FLAG_FIELD));
        float Tick = STATIC_FIELD_TICK_DAMAGE * (1.f + ARC_FIELD_DAMAGE_SHARE * (float)Arc);
        float Ticks = (2000.f - Inside->Hp) / StormDealt(Tick);
        Check(Ticks > 9.5f && Ticks < 10.5f);
        Check((Edge->Hp < 2000.f) == (Arc == 4));
        Check(Outside->Hp == 2000.f);
        Check(RoleSpellRadius(Slot, 1) > STATIC_FIELD_RADIUS * (1.f + ARC_FIELD_RADIUS_SHARE * (float)Arc) - 0.01f);
        DestroyCryptWorld(&Crypt);
    }
}

// NOTE(zoubir): a Spark on a foe inside the field arcs to every other foe
// inside for STATIC_ARC_SHARE; one far outside, past the jump, is left
// alone. A at the field's middle, B beside it: the Spark strikes A and
// jumps to B, A's strike arcs to B and B's to A, each once
internal void
TestStaticFieldArcs()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    storm_dummies Dummies = {};
    v2 Point = AimPoint(Slot->Entity);
    v3 Centre = V3(Point.X, Point.Y, 0.f) - Slot->Entity->Position;
    Centre.Z = 0.f;
    world_entity *A = StormDummy(&Crypt, &Dummies, Centre);
    world_entity *B = StormDummy(&Crypt, &Dummies, Centre + V3(0.f, 60.f, 0.f));
    world_entity *Far = StormDummy(&Crypt, &Dummies, Centre + V3(0.f, -400.f, 0.f));
    PressOnce(&Crypt, 0, PlayerButton_Push);
    // NOTE(zoubir): between the field's ticks
    StormTick(&Crypt, &Dummies, 2);
    float BeforeA = A->Hp;
    float BeforeB = B->Hp;
    PressAt(&Crypt, 0, PlayerButton_Cast, A);
    float Spark = SPARK_DAMAGE;
    float Jump = SPARK_JUMP_SHARE * SPARK_DAMAGE;
    Check(StormNear(BeforeA - A->Hp, StormDealt(Spark + STATIC_ARC_SHARE * Jump)));
    Check(StormNear(BeforeB - B->Hp, StormDealt(Jump + STATIC_ARC_SHARE * Spark)));
    Check(Far->Hp == 2000.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): W needs THUNDERCLAP_MIN_CHARGE: short of it no cast, no
// cooldown, and the flag the HUD shakes on. With it, the foe pressed on
// takes the bolt for all the Charge; from STORM_SUPERCHARGED it is stunned
// and the foes round it splashed
internal void
TestThunderclap()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    storm_dummies Dummies = {};
    world_entity *Foe = StormDummy(&Crypt, &Dummies, V3(300.f, 0.f, 0.f));
    world_entity *Near = StormDummy(&Crypt, &Dummies, V3(300.f, 60.f, 0.f));
    world_entity *Away = StormDummy(&Crypt, &Dummies, V3(300.f, 180.f, 0.f));
    world_entity *Cursor = StormDummy(&Crypt, &Dummies, V3(80.f, 0.f, 0.f));
    SetStormCharge(Slot, THUNDERCLAP_MIN_CHARGE - 1.f);
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Slot->Entity->CastSpell == 0 && Slot->RoleCooldowns[4] == 0.f);
    Check(Slot->ClassFlags & STORMCALLER_FLAG_EMPTY);
    Check(Slot->Stormcaller.Charge > THUNDERCLAP_MIN_CHARGE - 1.1f);

    // NOTE(zoubir): 50 Charge: no stun, no splash; the foe pressed on is
    // struck, not the one nearer the cursor
    SetStormCharge(Slot, 50.f);
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Slot->Entity->CastSpell == PlayerSpell_StormcallerB);
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    Check(StormNear(2000.f - Foe->Hp, StormDealt(THUNDERCLAP_DAMAGE + 50.f * THUNDERCLAP_PER_CHARGE)));
    Check(!HasStatus(Foe, StatusEffect_Stunned));
    Check(Near->Hp == 2000.f && Cursor->Hp == 2000.f);
    Check(Slot->Stormcaller.Charge == 0.f);
    Check(Slot->RoleCooldowns[4] > THUNDERCLAP_COOLDOWN - 1.f);

    // NOTE(zoubir): 80 Charge: stunned, and the one beside it splashed
    float Before = Foe->Hp;
    SetStormCharge(Slot, 80.f);
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    StormTick(&Crypt, &Dummies, STORM_CLAP_TICKS);
    float Big = StormDealt(THUNDERCLAP_DAMAGE + 80.f * THUNDERCLAP_PER_CHARGE);
    Check(StormNear(Before - Foe->Hp, Big));
    Check(HasStatus(Foe, StatusEffect_Stunned));
    Check(StormNear(2000.f - Near->Hp, THUNDERCLAP_SPLASH_SHARE * Big));
    Check(Away->Hp == 2000.f);

    // NOTE(zoubir): the foe dies in the wind-up: the Charge stays and the
    // key is ready again
    SetStormCharge(Slot, 40.f);
    Slot->RoleCooldowns[4] = 0.f;
    PressAt(&Crypt, 0, PlayerButton_Shockwave, Foe);
    Check(Slot->Entity->CastSpell == PlayerSpell_StormcallerB);
    KillEntity(AppState, &AppState->World, Foe, 0);
    Slot->Stormcaller.IdleSeconds = 0.f;
    TickCrypt(&Crypt, STORM_CLAP_TICKS);
    Check(Slot->Stormcaller.Charge > 39.f && Slot->RoleCooldowns[4] == 0.f);
    DestroyCryptWorld(&Crypt);
}

#include "stormcaller/storm_tests.cpp"

// NOTE(zoubir): Ball Lightning rolls along the aim, zaps the foe it
// passes and gives Charge for it, and leaves one off its way alone
internal void
TestBallLightning()
{
    crypt_world Crypt = CreateStormWorld();
    app_state *AppState = Crypt.AppState;
    player_slot *Slot = &AppState->Players[0];
    Slot->Entity->Aim = V2(1.f, 0.f);
    ranger_dummies Dummies = {};
    world_entity *OnWay = RangerDummy(&Crypt, &Dummies, V3(150.f, 0.f, 0.f));
    world_entity *Off = RangerDummy(&Crypt, &Dummies, V3(150.f, 300.f, 0.f));
    Slot->Stormcaller.Charge = 0.f;
    PressOnce(&Crypt, 0, PlayerButton_Attack);
    Check(Slot->RoleCooldowns[6] > 0.f);
    RangerTick(&Crypt, &Dummies, (u32)(60.f * BALL_LIGHTNING_SECONDS));
    Check(OnWay->Hp < 2000.f && Off->Hp == 2000.f);
    Check(Slot->Stormcaller.Charge > 0.f);
    DestroyCryptWorld(&Crypt);
}

internal void
RunStormcallerTests()
{
    TestBallLightning();
    TestStormcallerKeys();
    TestSparkJumpsAndCharges();
    TestChainLightningNeverTwice();
    TestConductorChainsFarther();
    TestStaticFieldShocks();
    TestStaticFieldArcs();
    TestThunderclap();
    TestCapacitorAndStormbringer();
    TestOverload();
    TestEyeOfTheStorm();
    TestLightningDash();
    TestChargeDrains();
    TestPredictedStormcaller();
}
