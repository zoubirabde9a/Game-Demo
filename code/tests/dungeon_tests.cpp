/* Dungeon tests (sim/dungeon/): roles change health and damage only in a
   dungeon run, and leave the duel alone. Included by sim_tests.cpp,
   which calls RunDungeonTests. */

internal void
TestRolesDoNothingOutsideADungeon()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    Player->SpawnShield = 0.f;
    float MaxHp = Player->MaxHp;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    Check(Player->MaxHp == MaxHp);
    DamageEntity(AppState, Test.World, Player, 10.f, 0);
    Check(Player->Hp == MaxHp - 10.f);
    DestroyTestWorld(&Test);
}

internal void
TestRolesScaleHealthAndDamage()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    world_entity *Tank = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *Healer = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    Tank->SpawnShield = Healer->SpawnShield = 0.f;
    // NOTE(zoubir): a slot joins as a damage player
    Check(Tank->MaxHp == GetRoleDef(PlayerRole_Damage)->MaxHp);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    Check(Tank->MaxHp == 180.f && Tank->Hp == 180.f);
    Check(Healer->MaxHp == 100.f);

    DamageEntity(AppState, Test.World, Tank, 10.f, 0);
    Check(Tank->Hp == 180.f - 7.f);

    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {500, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    DamageEntity(AppState, Test.World, Monster, 20.f, Healer);
    Check(Monster->Hp == 90.f);
    DamageEntity(AppState, Test.World, Monster, 20.f, Tank);
    Check(Monster->Hp == 76.f);

    AppState->Dungeon = 0;
    DestroyTestWorld(&Test);
}

internal void
TestNoDuelMapIsADungeon()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        if (Map->Dungeon)
        {
            Check(Map->MonsterPopulation == 0);
        }
    }
}

internal void
RunDungeonTests()
{
    TestRolesDoNothingOutsideADungeon();
    TestRolesScaleHealthAndDamage();
    TestNoDuelMapIsADungeon();
}
