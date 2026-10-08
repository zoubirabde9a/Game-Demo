/* Striker and party synergy tests (sim/dungeon/role_kits/), included by
   dungeon_tests.cpp: fireball hits leave Searing stacks, a
   full mark detonates far harder than none, Detonate in burning ground
   takes every marked monster in it, and marks fade; the tank's slam
   sunders, and a sundered monster and a warded attacker both raise the
   damage dealt; and the role talents that build on them (Detonate's
   second rank, Overload, Shatter Armor), and the crowd control from the
   tree's bottom: Molten Ground's slowing fire and Cataclysm's stun. */

// NOTE(zoubir): a big monster Offset from the striker, held still
internal world_entity *
StrikerDummy(crypt_world *Crypt, v3 Offset)
{
    app_state *AppState = Crypt->AppState;
    world_entity *Striker = AppState->Players[0].Entity;
    world_entity *Result = SpawnMonster(AppState, &AppState->World, &Crypt->Arena,
                                        Striker->Position + Offset, MonsterKind_Brute);
    Result->MaxHp = Result->Hp = 2000.f;
    return Result;
}

// NOTE(zoubir): what Detonate on Monster took off it
internal float
DetonateOn(app_state *AppState, world_entity *Monster)
{
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Target = (u32)(Monster - AppState->World.Entities) + 1;
    float Before = Monster->Hp;
    Check(CastStrikerKey(AppState, Slot, Slot->Entity, 2));
    float Result = Before - Monster->Hp;
    return Result;
}

internal void
TestMarkBitsFromFireball()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    world_entity *Monster = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity Fireball = {};
    Fireball.Type = EntityType_FireBall;
    Fireball.HasOwner = true;
    Fireball.OwnerSlot = 0;
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    Check(FindFoeMark(Run, World, Monster) && FindFoeMark(Run, World, Monster)->Stacks == 1);
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    Check(FindFoeMark(Run, World, Monster)->Stacks == SEARING_MOST);
    // NOTE(zoubir): the striker's own blows (Detonate, a burn) add none
    world_entity *Other = StrikerDummy(&Crypt, V3(0.f, 120.f, 0.f));
    DungeonScaleDamage(AppState, Other, Slot->Entity, 1.f);
    Check(FindFoeMark(Run, World, Other) == 0);
    // NOTE(zoubir): nor does a tank's fireball: it sunders instead
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    DungeonScaleDamage(AppState, Other, &Fireball, 1.f);
    Check(FindFoeMark(Run, World, Other) && FindFoeMark(Run, World, Other)->Stacks == 0);
    // NOTE(zoubir): left alone, the mark fades
    UpdateFoeMarks(Run, World, SEARING_SECONDS + 0.1f);
    Check(FindFoeMark(Run, World, Monster) == 0);
    DestroyCryptWorld(&Crypt);
}

internal void
TestDetonateSpendsAFullMark()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Damage);
    world_entity *Marked = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity *Bare = StrikerDummy(&Crypt, V3(-120.f, 0.f, 0.f));
    for(u32 Stack = 0; Stack < SEARING_MOST; Stack++)
    {
        AddSearing(Run, World, Marked);
    }
    float Full = DetonateOn(AppState, Marked);
    float None = DetonateOn(AppState, Bare);
    float Expected = (DETONATE_DAMAGE + DETONATE_PER_STACK * SEARING_MOST) / DETONATE_DAMAGE;
    Check(None > 0.f && Full > 0.99f * Expected * None && Full < 1.01f * Expected * None);
    Check(FindFoeMark(Run, World, Marked) == 0);
    // NOTE(zoubir): nothing near the cursor: no cast, no cooldown spent
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Target = 0;
    Slot->Entity->Aim = V2(0.f, 1.f);
    Slot->Entity->AimReach = 0.9f;
    Check(!CastStrikerKey(AppState, Slot, Slot->Entity, 2));
    DestroyCryptWorld(&Crypt);
}

internal void
TestDetonateInFireChains()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Damage);
    world_entity *First = StrikerDummy(&Crypt, V3(150.f, 0.f, 0.f));
    world_entity *Second = StrikerDummy(&Crypt, V3(190.f, 30.f, 0.f));
    world_entity *Outside = StrikerDummy(&Crypt, V3(-200.f, 0.f, 0.f));
    AddSearing(Run, World, First);
    AddSearing(Run, World, Second);
    AddSearing(Run, World, Second);
    AddSearing(Run, World, Outside);
    inferno *Fire = &Run->Infernos[0];
    Fire->Position = First->Position;
    Fire->Radius = INFERNO_RADIUS;
    Fire->Seconds = INFERNO_BURN_SECONDS;
    float SecondBefore = Second->Hp;
    float OutsideBefore = Outside->Hp;
    DetonateOn(AppState, First);
    Check(Second->Hp < SecondBefore - DETONATE_DAMAGE - 2.f * DETONATE_PER_STACK);
    Check(FindFoeMark(Run, World, Second) == 0);
    Check(Outside->Hp == OutsideBefore);
    Check(FindFoeMark(Run, World, Outside) != 0);
    DestroyCryptWorld(&Crypt);
}

internal void
TestSunderAndWardRaiseDamage()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Striker = &AppState->Players[0];
    player_slot *Tank = &AppState->Players[1];
    SetPlayerRole(AppState, Striker, PlayerRole_Damage);
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Tank->Entity->Position + V3(40.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Monster->MaxHp = Monster->Hp = 2000.f;
    float Plain = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);

    Check(CastTankKey(AppState, World, &Crypt.Arena, Tank, Tank->Entity, 1));
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->SunderSeconds == SUNDER_SECONDS && Mark->Stacks == 0);
    float Sundered = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);
    Check(Sundered > 1.149f * Plain && Sundered < 1.151f * Plain);

    Striker->WardAbsorb = 20.f;
    Striker->WardEmpower = WARD_EMPOWER_SHARE;
    float Warded = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);
    Check(Warded > 1.119f * Sundered && Warded < 1.121f * Sundered);
    Striker->WardAbsorb = 0.f;

    // NOTE(zoubir): Detonate spends the stacks and leaves the sunder
    AddSearing(Run, World, Monster);
    Striker->Input.Target = (u32)(Monster - World->Entities) + 1;
    Check(CastStrikerKey(AppState, Striker, Striker->Entity, 2));
    Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->Stacks == 0 && Mark->SunderSeconds > 0.f);
    UpdateFoeMarks(Run, World, SUNDER_SECONDS + 0.1f);
    Check(FindFoeMark(Run, World, Monster) == 0);
    DestroyCryptWorld(&Crypt);
}

internal void
TestRotationTalents()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Striker = &AppState->Players[0];
    player_slot *Tank = &AppState->Players[1];
    player_slot *Healer = &AppState->Players[2];
    SetPlayerRole(AppState, Striker, PlayerRole_Damage);
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);

    // NOTE(zoubir): Detonate's second rank: a full mark is 12 + 25 x 3
    Striker->Ranks[Talent_RoleFirst + StrikerTalent_Detonate] = 2;
    Striker->Ranks[Talent_RoleFirst + StrikerTalent_Overload] = 1;
    world_entity *Marked = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity *Bare = StrikerDummy(&Crypt, V3(-120.f, 0.f, 0.f));
    for(u32 Stack = 0; Stack < SEARING_MOST; Stack++)
    {
        AddSearing(Run, World, Marked);
    }
    float Full = DetonateOn(AppState, Marked);
    // NOTE(zoubir): Overload gives back part of the cooldown on a full
    // mark only
    Check(Striker->CastRefund == OVERLOAD_SECONDS);
    Striker->CastRefund = 0.f;
    float None = DetonateOn(AppState, Bare);
    Check(Striker->CastRefund == 0.f);
    float Expected = (DETONATE_DAMAGE + (DETONATE_PER_STACK + SEARING_HEAT_PER_STACK) *
                      SEARING_MOST) / DETONATE_DAMAGE;
    Check(Full > 0.99f * Expected * None && Full < 1.01f * Expected * None);

    // NOTE(zoubir): Shatter Armor: a deeper, longer sunder
    Tank->Ranks[Talent_RoleFirst + TankTalent_ShatterArmor] = 1;
    world_entity *Monster = SpawnMonster(AppState, World, &Crypt.Arena,
                                         Tank->Entity->Position + V3(40.f, 0.f, 0.f),
                                         MonsterKind_Brute);
    Check(CastTankKey(AppState, World, &Crypt.Arena, Tank, Tank->Entity, 1));
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->SunderSeconds == SUNDER_SECONDS + SHATTER_SECONDS);
    float Scale = FoeMarkDamageScale(Run, World, Monster);
    Check(Scale > 1.249f && Scale < 1.251f);

    // NOTE(zoubir): the warded one is empowered, the ally beside it only
    // shares the shield
    v3 Before = Tank->Entity->Position;
    Tank->Entity->Position = Striker->Entity->Position + V3(30.f, 0.f, 0.f);
    CheckAndChangeEntityChunk(AppState, World, &Crypt.Arena, Before, Tank->Entity);
    Healer->Input.Target = Striker->Entity->ID + 1;
    Striker->WardAbsorb = Tank->WardAbsorb = 0.f;
    Check(CastHealerKey(AppState, Healer, Healer->Entity, 1));
    Check(Tank->WardAbsorb > 0.f);
    Check(Striker->WardEmpower > 0.119f && Striker->WardEmpower < 0.121f);
    Check(Tank->WardEmpower == 0.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the tank's shots keep a sunder going, the healer's heal
internal void
TestTankAndHealerShots()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Tank = &AppState->Players[1];
    player_slot *Healer = &AppState->Players[2];
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    world_entity *Monster = StrikerDummy(&Crypt, V3(150.f, 0.f, 0.f));
    world_entity Shot = {};
    Shot.Type = EntityType_FireBall;
    Shot.HasOwner = true;
    Shot.OwnerSlot = 1;
    DungeonScaleDamage(AppState, Monster, &Shot, 10.f);
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->SunderSeconds == SUNDERING_SHOT_FRESH && Mark->Stacks == 0);
    DungeonScaleDamage(AppState, Monster, &Shot, 10.f);
    Check(Mark->SunderSeconds == SUNDERING_SHOT_FRESH + SUNDERING_SHOT_SECONDS);
    for(u32 Shots = 0; Shots < 4; Shots++)
    {
        DungeonScaleDamage(AppState, Monster, &Shot, 10.f);
    }
    Check(Mark->SunderSeconds == SUNDER_SECONDS);

    world_entity *Hurt = AppState->Players[0].Entity;
    Hurt->Hp = 0.5f * Hurt->MaxHp;
    float Before = Hurt->Hp;
    Shot.OwnerSlot = 2;
    float Dealt = DungeonScaleDamage(AppState, Monster, &Shot, 10.f);
    Check(Dealt > 0.f);
    Check(Hurt->Hp > Before + 0.99f * SMITE_SHARE * Dealt);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Molten Ground slows what burns in Meteor's ground, longer
// a rank; Cataclysm makes a Giant Fireball's blast stun. Neither does
// anything untaken
internal void
TestStrikerCrowdControl()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Striker = &AppState->Players[0];
    SetPlayerRole(AppState, Striker, PlayerRole_Damage);

    world_entity *Burning = StrikerDummy(&Crypt, V3(150.f, 0.f, 0.f));
    inferno *Fire = &Run->Infernos[0];
    Fire->Position = Burning->Position;
    Fire->Radius = INFERNO_RADIUS;
    Fire->Seconds = INFERNO_BURN_SECONDS;
    Fire->Delay = 0.f;
    Fire->TickTimer = 0.f;
    Fire->By = 0;
    float Before = Burning->Hp;
    UpdateInfernos(AppState, Run, INFERNO_BURN_TICK + 0.01f);
    Check(Burning->Hp < Before);
    Check(!HasStatus(Burning, StatusEffect_Slowed));
    Striker->Ranks[Talent_RoleFirst + StrikerTalent_MoltenGround] = 3;
    UpdateInfernos(AppState, Run, INFERNO_BURN_TICK);
    float Slow = Burning->StatusTimers[StatusEffect_Slowed];
    Check(Slow > 0.99f * 3.f * MOLTEN_GROUND_SLOW_SECONDS &&
          Slow < 1.01f * 3.f * MOLTEN_GROUND_SLOW_SECONDS);

    world_entity *Struck = StrikerDummy(&Crypt, V3(-150.f, 0.f, 0.f));
    for(u32 Taken = 0; Taken < 2; Taken++)
    {
        Striker->Ranks[Talent_RoleFirst + StrikerTalent_Cataclysm] = (u8)Taken;
        Struck->StatusTimers[StatusEffect_Stunned] = 0.f;
        giant_fireball *Ball = &Run->GiantFireballs[0];
        Ball->Position = Struck->Position + V3(10.f, 0.f, 0.f);
        Ball->Velocity = V2(-GIANT_FIREBALL_SPEED, 0.f);
        Ball->Distance = GIANT_FIREBALL_RANGE;
        Ball->Room = RoomAtPosition(World, Ball->Position.XY);
        Ball->By = 0;
        UpdateGiantFireballs(AppState, Run, 0.01f);
        Check(Ball->Distance <= 0.f);
        float Stun = Struck->StatusTimers[StatusEffect_Stunned];
        Check(Taken ? (Stun > 0.99f * CATACLYSM_STUN_SECONDS &&
                       Stun < 1.01f * CATACLYSM_STUN_SECONDS) : Stun <= 0.f);
    }
    DestroyCryptWorld(&Crypt);
}

internal void
RunStrikerTests()
{
    TestStrikerCrowdControl();
    TestTankAndHealerShots();
    TestRotationTalents();
    TestSunderAndWardRaiseDamage();
    TestMarkBitsFromFireball();
    TestDetonateSpendsAFullMark();
    TestDetonateInFireChains();
}
