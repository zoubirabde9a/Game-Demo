/* Meter tests (sim/dungeon/meter.cpp): the damage, healing and damage
   taken each player did in a fight, counting only health that moved, only
   while a room is fought, and starting again with the next fight.
   Included by dungeon_tests.cpp, after its crypt helpers. */

inline bool32
MeterNear(float A, float B)
{
    bool32 Result = A > B - 0.01f && A < B + 0.01f;
    return Result;
}

internal void
TestMeterCountsOneFight()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    player_slot *SlotA = &AppState->Players[0];
    player_slot *SlotB = &AppState->Players[1];
    world_entity *A = SlotA->Entity;
    world_entity *B = SlotB->Entity;
    TickCrypt(&Crypt, 1);
    // NOTE(zoubir): the empty Antechamber is a fight over at once
    u32 Fights = Run->MeterFight;
    Check(Run->FightingRoom == 0 && Run->MeterSeconds < 0.1f);

    MovePlayerTo(AppState, World, &Crypt.Arena, A, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2 && Run->MeterFight == Fights + 1 && Run->MeterRoom == 2);
    Check(SlotA->MeterDamage == 0.f && SlotB->MeterTaken == 0.f);

    // NOTE(zoubir): A's hit counts as the health it took, after the
    // role's scaling, and a killing blow only up to the monster's last point
    world_entity *Foe = &World->Entities[Run->FoeSlots[0]];
    float Before = Foe->Hp;
    DamageEntity(AppState, World, Foe, 10.f, A);
    float Took = Before - Foe->Hp;
    Check(Took > 0.f && MeterNear(SlotA->MeterDamage, Took));
    float Left = Foe->Hp;
    DamageEntity(AppState, World, Foe, 100000.f, A);
    Check(MeterNear(SlotA->MeterDamage, Took + Left));
    Check(SlotB->MeterDamage == 0.f);

    // NOTE(zoubir): a monster's bite is damage taken; the health A then
    // gives B is healing, but no more than B was missing
    world_entity *Biter = &World->Entities[Run->FoeSlots[1]];
    Before = B->Hp;
    DamageEntity(AppState, World, B, 20.f, Biter);
    float Lost = Before - B->Hp;
    Check(Lost > 0.f && MeterNear(SlotB->MeterTaken, Lost) && SlotA->MeterTaken == 0.f);
    HealPlayer(AppState, 0, B, 100000.f);
    Check(B->Hp == B->MaxHp);
    Check(MeterNear(SlotA->MeterHealing, Lost));

    TickCrypt(&Crypt, 60);
    Check(Run->MeterSeconds > 0.9f && Run->MeterSeconds < 1.1f);

    // NOTE(zoubir): once the room is cleared the numbers stay for the HUD,
    // and nothing more counts until the next fight
    KillRoomMonsters(&Crypt, 2);
    TickCrypt(&Crypt, 2);
    Check(Run->FightingRoom == 0);
    float Seconds = Run->MeterSeconds;
    float Damage = SlotA->MeterDamage;
    B->Hp = 0.5f * B->MaxHp;
    HealPlayer(AppState, 0, B, 10.f);
    TickCrypt(&Crypt, 30);
    Check(Run->MeterSeconds == Seconds && SlotA->MeterDamage == Damage);
    Check(MeterNear(SlotA->MeterHealing, Lost));

    // NOTE(zoubir): the next room starts the meter again
    MovePlayerTo(AppState, World, &Crypt.Arena, A, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 3 && Run->MeterFight == Fights + 2 && Run->MeterRoom == 3);
    Check(SlotA->MeterDamage == 0.f && SlotA->MeterHealing == 0.f && SlotB->MeterTaken == 0.f);
    Check(Run->MeterSeconds < 0.1f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the duel has no run, so nothing is counted there
internal void
TestNoMeterOutsideADungeon()
{
    crypt_world Duel = CreateDungeonWorld(MapId_Arena, 2);
    app_state *AppState = Duel.AppState;
    Check(!AppState->Dungeon);
    world_entity *B = AppState->Players[1].Entity;
    DamageEntity(AppState, &AppState->World, B, 0.25f, AppState->Players[0].Entity);
    Check(AppState->Players[0].MeterDamage == 0.f && AppState->Players[1].MeterTaken == 0.f);
    DestroyCryptWorld(&Duel);
}

internal void
RunMeterTests()
{
    TestMeterCountsOneFight();
    TestNoMeterOutsideADungeon();
}
