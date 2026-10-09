/* Striker and party synergy tests (sim/dungeon/role_kits/), included by
   dungeon_tests.cpp: fireball hits leave Searing stacks, a mark burns
   harder the more stacks it has and explodes when it runs out, splashing
   the monsters beside it, and Fireguard takes damage until it breaks or
   runs out; a class key pressed during a cast goes off when the cast
   ends; the tank's slam sunders, and a sundered monster and a warded
   attacker both raise the damage dealt; and the role talents that build
   on them (Searing Heat, Overload, Shatter Armor), and the crowd control
   from the tree's bottom: Molten Ground's slowing fire and Cataclysm's
   stun. */

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

// NOTE(zoubir): what Monster's mark exploding took off it, the mark run
// out on the next update with no burn tick in between
internal float
ExplodeOn(app_state *AppState, world_entity *Monster)
{
    dungeon_run *Run = AppState->Dungeon;
    foe_mark *Mark = FindFoeMark(Run, &AppState->World, Monster);
    Check(Mark && Mark->Stacks > 0);
    Mark->Seconds = 0.01f;
    Mark->BurnTimer = 0.f;
    float Before = Monster->Hp;
    UpdateSearing(AppState, Run, 0.02f);
    Check(Mark->Stacks == 0);
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
    GrantClassSpells(Slot);
    world_entity *Monster = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity Fireball = {};
    Fireball.Type = EntityType_FireBall;
    Fireball.HasOwner = true;
    Fireball.OwnerSlot = 0;
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    Check(FindFoeMark(Run, World, Monster) && FindFoeMark(Run, World, Monster)->Stacks == 1);
    for(u32 Shot = 0; Shot < SEARING_MOST; Shot++)
    {
        DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    }
    Check(FindFoeMark(Run, World, Monster)->Stacks == SEARING_MOST);
    // NOTE(zoubir): the striker's own blows (a burn, an explosion) add none
    world_entity *Other = StrikerDummy(&Crypt, V3(0.f, 120.f, 0.f));
    DungeonScaleDamage(AppState, Other, Slot->Entity, 1.f);
    Check(FindFoeMark(Run, World, Other) == 0);
    // NOTE(zoubir): nor does a tank's fireball: it sunders instead
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    GrantClassSpells(Slot);
    DungeonScaleDamage(AppState, Other, &Fireball, 1.f);
    Check(FindFoeMark(Run, World, Other) && FindFoeMark(Run, World, Other)->Stacks == 0);
    // NOTE(zoubir): left alone, the mark fades
    UpdateFoeMarks(Run, World, SEARING_SECONDS + 0.1f);
    Check(FindFoeMark(Run, World, Monster) == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a mark burns by its stacks every SEARING_TICK, explodes
// by its stacks when it runs out, and the explosion splashes the monster
// beside it for DETONATE_SPLASH_SHARE but not one far off; a Meteor blast
// leaves more stacks than a fireball, a Giant Fireball more again
internal void
TestSearingBurnsAndExplodes()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Damage);
    GrantClassSpells(&AppState->Players[0]);
    world_entity *Full = StrikerDummy(&Crypt, V3(160.f, 0.f, 0.f));
    world_entity *Near = StrikerDummy(&Crypt, V3(200.f, 0.f, 0.f));
    world_entity *One = StrikerDummy(&Crypt, V3(-200.f, 0.f, 0.f));
    world_entity *Far = StrikerDummy(&Crypt, V3(-200.f, 200.f, 0.f));
    AddSearing(Run, World, Full, SEARING_MOST, 0);
    AddSearing(Run, World, One, 1, 0);

    float FullBefore = Full->Hp;
    float OneBefore = One->Hp;
    UpdateSearing(AppState, Run, SEARING_TICK + 0.01f);
    float FullBurn = FullBefore - Full->Hp;
    float OneBurn = OneBefore - One->Hp;
    Check(OneBurn > 0.f);
    Check(FullBurn > 0.99f * SEARING_MOST * OneBurn && FullBurn < 1.01f * SEARING_MOST * OneBurn);
    Check(FindFoeMark(Run, World, Full)->Stacks == SEARING_MOST);

    float NearBefore = Near->Hp;
    float FarBefore = Far->Hp;
    float FullBlast = ExplodeOn(AppState, Full);
    float OneBlast = ExplodeOn(AppState, One);
    float Expected = (DETONATE_DAMAGE + DETONATE_PER_STACK * SEARING_MOST) /
        (DETONATE_DAMAGE + DETONATE_PER_STACK);
    Check(OneBlast > 0.f);
    Check(FullBlast > 0.99f * Expected * OneBlast && FullBlast < 1.01f * Expected * OneBlast);
    float Splash = NearBefore - Near->Hp;
    Check(Splash > 0.99f * DETONATE_SPLASH_SHARE * FullBlast &&
          Splash < 1.01f * DETONATE_SPLASH_SHARE * FullBlast);
    Check(Far->Hp == FarBefore);
    Check(FindFoeMark(Run, World, Full) == 0 && FindFoeMark(Run, World, Near) == 0);

    // NOTE(zoubir): the bigger the spell, the more stacks it leaves
    BurnAround(AppState, World, Far->Position, 10.f, 0, 1.f, SEARING_METEOR_STACKS);
    Check(FindFoeMark(Run, World, Far)->Stacks == SEARING_METEOR_STACKS);
    ExplodeOn(AppState, Far);
    BurnAround(AppState, World, Far->Position, 10.f, 0, 1.f, SEARING_GIANT_STACKS);
    Check(FindFoeMark(Run, World, Far)->Stacks == SEARING_GIANT_STACKS);
    Check(SEARING_GIANT_STACKS > SEARING_METEOR_STACKS && SEARING_METEOR_STACKS > 1);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Fireguard takes FIREGUARD_ABSORB, then lets the rest
// through; it shows in the ClassMeter, and goes out with its time
internal void
TestFireguardAbsorbs()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    GrantClassSpells(Slot);
    world_entity *Striker = Slot->Entity;
    Striker->SpawnShield = 0.f;
    Check(RoleSpellLearned(Slot, 2));
    Check(CastStrikerKey(AppState, Slot, Striker, 2));
    Check(Slot->FireguardAbsorb == FIREGUARD_ABSORB);
    UpdateInfernos(AppState, Run, 0.01f);
    Check(Slot->ClassMeter == (u8)FIREGUARD_ABSORB);

    float Before = Striker->Hp;
    DamageEntity(AppState, World, Striker, 0.6f * FIREGUARD_ABSORB, 0);
    Check(Striker->Hp == Before);
    DamageEntity(AppState, World, Striker, 0.6f * FIREGUARD_ABSORB, 0);
    float Through = Before - Striker->Hp;
    Check(Through > 0.19f * FIREGUARD_ABSORB && Through < 0.21f * FIREGUARD_ABSORB);
    Check(Slot->FireguardAbsorb == 0.f);

    Check(CastStrikerKey(AppState, Slot, Striker, 2));
    UpdateInfernos(AppState, Run, FIREGUARD_SECONDS + 0.1f);
    Check(Slot->FireguardAbsorb == 0.f && Slot->ClassMeter == 0);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a class key pressed while Meteor winds up is kept and
// goes off when the cast ends, instead of being lost
internal void
TestClassKeyWaitsForTheCast()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    GrantClassSpells(Slot);
    world_entity *Striker = Slot->Entity;
    PressOnce(&Crypt, 0, PlayerButton_Launch);
    Check(IsPlayerCasting(Striker));
    TickCrypt(&Crypt, 10);
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(IsPlayerCasting(Striker));
    Check(Slot->FireguardAbsorb == 0.f);
    Check(Striker->QueuedRoleKey == 3);
    for(u32 Tick = 0; Tick < 120 && IsPlayerCasting(Striker); Tick++)
    {
        TickCrypt(&Crypt, 1);
    }
    TickCrypt(&Crypt, 1);
    Check(!IsPlayerCasting(Striker));
    Check(Slot->FireguardAbsorb > 0.f);
    Check(Striker->QueuedRoleKey == 0);

    // NOTE(zoubir): a key pressed well before its cooldown ends, with no
    // cast to wait for, is dropped as before
    Slot->RoleCooldowns[2] = 5.f;
    float Absorb = Slot->FireguardAbsorb;
    PressOnce(&Crypt, 0, PlayerButton_Slam);
    Check(Striker->QueuedRoleKey == 0);
    Check(Slot->FireguardAbsorb <= Absorb);
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
    GrantClassSpells(Striker);
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    GrantClassSpells(Tank);
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

    // NOTE(zoubir): the Searing explosion spends the stacks and leaves
    // the sunder
    AddSearing(Run, World, Monster, 1, 0);
    ExplodeOn(AppState, Monster);
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
    GrantClassSpells(Striker);
    SetPlayerRole(AppState, Tank, PlayerRole_Tank);
    GrantClassSpells(Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);

    // NOTE(zoubir): Searing Heat's two ranks add to every stack of the
    // blast, and Overload makes a full mark's blast harder still
    SetClassTalentRank(Striker, StrikerTalent_SearingHeat, 2);
    SetClassTalentRank(Striker, StrikerTalent_Overload, 1);
    world_entity *Marked = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity *Bare = StrikerDummy(&Crypt, V3(-120.f, 0.f, 0.f));
    AddSearing(Run, World, Marked, SEARING_MOST, 0);
    AddSearing(Run, World, Bare, 1, 0);
    float Full = ExplodeOn(AppState, Marked);
    float One = ExplodeOn(AppState, Bare);
    float PerStack = DETONATE_PER_STACK + 2.f * SEARING_HEAT_PER_STACK;
    float Expected = (1.f + OVERLOAD_SHARE) * (DETONATE_DAMAGE + PerStack * SEARING_MOST) /
        (DETONATE_DAMAGE + PerStack);
    Check(Full > 0.99f * Expected * One && Full < 1.01f * Expected * One);

    // NOTE(zoubir): Shatter Armor: a deeper, longer sunder
    SetClassTalentRank(Tank, TankTalent_ShatterArmor, 1);
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
    GrantClassSpells(Tank);
    SetPlayerRole(AppState, Healer, PlayerRole_Healer);
    GrantClassSpells(Healer);
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
    GrantClassSpells(Striker);

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
    SetClassTalentRank(Striker, StrikerTalent_MoltenGround, 3);
    UpdateInfernos(AppState, Run, INFERNO_BURN_TICK);
    float Slow = Burning->StatusTimers[StatusEffect_Slowed];
    Check(Slow > 0.99f * 3.f * MOLTEN_GROUND_SLOW_SECONDS &&
          Slow < 1.01f * 3.f * MOLTEN_GROUND_SLOW_SECONDS);

    world_entity *Struck = StrikerDummy(&Crypt, V3(-150.f, 0.f, 0.f));
    for(u32 Taken = 0; Taken < 2; Taken++)
    {
        SetClassTalentRank(Striker, StrikerTalent_Cataclysm, (u8)Taken);
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
    TestSearingBurnsAndExplodes();
    TestFireguardAbsorbs();
    TestClassKeyWaitsForTheCast();
}
