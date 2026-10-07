/* Striker and party synergy tests (sim/dungeon/role_kits/), included by
   dungeon_tests.cpp: kunai and fireball hits leave Searing stacks, a
   full mark detonates far harder than none, Detonate in burning ground
   takes every marked monster in it, and marks fade; the tank's slam
   sunders, and a sundered monster and a warded attacker both raise the
   damage dealt. */

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
    Check(CastStrikerKey(AppState, Slot, Slot->Entity, 1));
    float Result = Before - Monster->Hp;
    return Result;
}

internal void
TestMarkBitsFromKunaiAndFireball()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    player_slot *Slot = &AppState->Players[0];
    SetPlayerRole(AppState, Slot, PlayerRole_Damage);
    world_entity *Monster = StrikerDummy(&Crypt, V3(120.f, 0.f, 0.f));
    world_entity Kunai = {};
    Kunai.Type = EntityType_Kunai;
    Kunai.HasOwner = true;
    Kunai.OwnerSlot = 0;
    world_entity Fireball = Kunai;
    Fireball.Type = EntityType_FireBall;
    DungeonScaleDamage(AppState, Monster, &Kunai, 1.f);
    Check(FindFoeMark(Run, World, Monster) && FindFoeMark(Run, World, Monster)->Stacks == 1);
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    DungeonScaleDamage(AppState, Monster, &Kunai, 1.f);
    DungeonScaleDamage(AppState, Monster, &Kunai, 1.f);
    Check(FindFoeMark(Run, World, Monster)->Stacks == SEARING_MOST);
    // NOTE(zoubir): the striker's own blows (Detonate, a burn) add none
    world_entity *Other = StrikerDummy(&Crypt, V3(0.f, 120.f, 0.f));
    DungeonScaleDamage(AppState, Other, Slot->Entity, 1.f);
    Check(FindFoeMark(Run, World, Other) == 0);
    // NOTE(zoubir): nor does a tank's kunai
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    DungeonScaleDamage(AppState, Other, &Kunai, 1.f);
    Check(FindFoeMark(Run, World, Other) == 0);
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
    Check(!CastStrikerKey(AppState, Slot, Slot->Entity, 1));
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
    float Warded = DungeonScaleDamage(AppState, Monster, Striker->Entity, 10.f);
    Check(Warded > 1.119f * Sundered && Warded < 1.121f * Sundered);
    Striker->WardAbsorb = 0.f;

    // NOTE(zoubir): Detonate spends the stacks and leaves the sunder
    AddSearing(Run, World, Monster);
    Striker->Input.Target = (u32)(Monster - World->Entities) + 1;
    Check(CastStrikerKey(AppState, Striker, Striker->Entity, 1));
    Mark = FindFoeMark(Run, World, Monster);
    Check(Mark && Mark->Stacks == 0 && Mark->SunderSeconds > 0.f);
    UpdateFoeMarks(Run, World, SUNDER_SECONDS + 0.1f);
    Check(FindFoeMark(Run, World, Monster) == 0);
    DestroyCryptWorld(&Crypt);
}

internal void
RunStrikerTests()
{
    TestSunderAndWardRaiseDamage();
    TestMarkBitsFromKunaiAndFireball();
    TestDetonateSpendsAFullMark();
    TestDetonateInFireChains();
}
