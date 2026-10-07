/* Striker tests (sim/dungeon/role_kits/striker.cpp), included by
   dungeon_tests.cpp: kunai and fireball hits leave Searing stacks, a
   full mark detonates far harder than none, Detonate in burning ground
   takes every marked monster in it, and marks fade. */

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
TestSearingStacksFromKunaiAndFireball()
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
    Check(FindSearing(Run, World, Monster) && FindSearing(Run, World, Monster)->Stacks == 1);
    DungeonScaleDamage(AppState, Monster, &Fireball, 1.f);
    DungeonScaleDamage(AppState, Monster, &Kunai, 1.f);
    DungeonScaleDamage(AppState, Monster, &Kunai, 1.f);
    Check(FindSearing(Run, World, Monster)->Stacks == SEARING_MOST);
    // NOTE(zoubir): the striker's own blows (Detonate, a burn) add none
    world_entity *Other = StrikerDummy(&Crypt, V3(0.f, 120.f, 0.f));
    DungeonScaleDamage(AppState, Other, Slot->Entity, 1.f);
    Check(FindSearing(Run, World, Other) == 0);
    // NOTE(zoubir): nor does a tank's kunai
    SetPlayerRole(AppState, Slot, PlayerRole_Tank);
    DungeonScaleDamage(AppState, Other, &Kunai, 1.f);
    Check(FindSearing(Run, World, Other) == 0);
    // NOTE(zoubir): left alone, the mark fades
    UpdateSearing(Run, World, SEARING_SECONDS + 0.1f);
    Check(FindSearing(Run, World, Monster) == 0);
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
    Check(FindSearing(Run, World, Marked) == 0);
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
    Check(FindSearing(Run, World, Second) == 0);
    Check(Outside->Hp == OutsideBefore);
    Check(FindSearing(Run, World, Outside) != 0);
    DestroyCryptWorld(&Crypt);
}

internal void
RunStrikerTests()
{
    TestSearingStacksFromKunaiAndFireball();
    TestDetonateSpendsAFullMark();
    TestDetonateInFireChains();
}
