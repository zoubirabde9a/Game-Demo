/* Duel rules tests (GameRules, sim/player_stats.cpp): only fireball,
   launch, blink, dash, jump, the world rewind and the shield do anything;
   a player has one health; the shield keeps every hit off for 2 s and
   comes back after 6; a fireball goes once every 6 s; the Old Arena has
   no monsters. Every test runs under the duel rules and puts the classic
   ones back. Included by sim_tests.cpp, which calls RunDuelTests. */

internal void
TestDuelRulesDropOtherAbilities()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Attack | PlayerButton_Shockwave |
        PlayerButton_Push | PlayerButton_RewindSelf | PlayerButton_RewindBubble;
    RunPlayerFrames(&Test, 0, 2);
    Check(Player->ActionCooldowns[PlayerAction_Sword] == 0.f);
    Check(Player->AreaCooldowns[PlayerArea_Shockwave] == 0.f);
    Check(Player->AreaCooldowns[PlayerArea_Push] == 0.f);
    Check(!IsPlayerCasting(Player));

    Slot->Input.Pressed = PlayerButton_Launch;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->AreaCooldowns[PlayerArea_Launch] > 0.f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

internal void
TestDuelPlayersDieInOneHit()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    Target->SpawnShield = 0.f;
    Check(Target->MaxHp == 1.f && Target->Hp == 1.f);
    world_entity *FireBall = AddFireBall(AppState, Test.World, &Test.Arena, Caster,
                                         V3(330.f, 300.f, 30.f), V3(600.f, 0.f, 0.f));
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp <= 0.f);
    Check(AppState->Players[0].Kills == 1);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): E keeps every hit off for 2 s, then comes back 6 s after
// the press; it does not cut a blink winding up
internal void
TestShieldBlocksHitsForTwoSeconds()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Target = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {360, 300, 0});
    Target->SpawnShield = 0.f;
    float Dt = Test.Input.DeltaTime;
    player_slot *Slot = &AppState->Players[1];
    Slot->Input.Aim = V2(0.f, 1.f);
    Slot->Input.Pressed = PlayerButton_Blink;
    RunPlayerFrames(&Test, 1, 1);
    Slot->Input.Pressed = PlayerButton_Shield;
    RunPlayerFrames(&Test, 1, 1);
    Check(Target->CastSpell == PlayerSpell_Blink);
    Check(Absolute(Target->MovementCooldowns[PlayerMove_Shield] - 6.f) < 0.05f);

    // NOTE(zoubir): just short of 2 s on, a fireball does nothing
    RunPlayerFrames(&Test, 1, (u32)(1.9f / Dt));
    world_entity *FireBall = AddFireBall(AppState, Test.World, &Test.Arena, Caster,
                                         V3(330.f, 300.f, 30.f), V3(600.f, 0.f, 0.f));
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp == 1.f);

    // NOTE(zoubir): a moment after 2 s it kills
    RunPlayerFrames(&Test, 1, (u32)(0.2f / Dt));
    Check(Target->SpawnShield == 0.f);
    FireBallHit(AppState, Test.World, FireBall, Target);
    Check(Target->Hp <= 0.f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

// NOTE(zoubir): one fireball, then nothing for 6 s; a click in between is
// dropped rather than firing late on its own
internal void
TestFireballWaitsSixSeconds()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    float Dt = Test.Input.DeltaTime;
    player_slot *Slot = &AppState->Players[0];
    Slot->Input.Aim = V2(1.f, 0.f);
    Slot->Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->ActionCooldowns[PlayerAction_FireBall] > 5.9f);

    RunPlayerFrames(&Test, 0, (u32)(3.f / Dt));
    Slot->Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, (u32)(2.9f / Dt));
    Check(Player->ActionCooldowns[PlayerAction_FireBall] > 0.f);
    RunPlayerFrames(&Test, 0, (u32)(0.3f / Dt));
    // NOTE(zoubir): ready again, and the early click did not fire it
    Check(Player->ActionCooldowns[PlayerAction_FireBall] == 0.f);
    Slot->Input.Pressed = PlayerButton_Cast;
    RunPlayerFrames(&Test, 0, 1);
    Check(Player->ActionCooldowns[PlayerAction_FireBall] > 5.9f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

internal void
TestOldArenaHasNoMonsters()
{
    Check(GetMapDef(MapId_Arena)->MonsterPopulation == 0);
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        if (MapIndex != MapId_Arena)
        {
            Check(GetMapDef((map_id)MapIndex)->MonsterPopulation > 0);
        }
    }
    test_world Test = CreateTestWorld();
    Test.World->MapId = MapId_Arena;
    monster_population *Population =
        CreateMonsterPopulation(&Test.Arena, MapMonsterPopulation(Test.World), 1337);
    FillMonsterPopulation(Test.AppState, Test.World, &Test.Arena, Population);
    for(u32 Tick = 0; Tick < 300; Tick++)
    {
        UpdateMonsterPopulation(Test.AppState, Test.World, &Test.Arena,
                                Population, Test.Input.DeltaTime);
    }
    Check(CountLiveMonsters(Test.World) == 0);
    DestroyTestWorld(&Test);
}

#define DUEL_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunDuelTests()
{
    DUEL_TEST(TestDuelRulesDropOtherAbilities);
    DUEL_TEST(TestDuelPlayersDieInOneHit);
    DUEL_TEST(TestShieldBlocksHitsForTwoSeconds);
    DUEL_TEST(TestFireballWaitsSixSeconds);
    DUEL_TEST(TestOldArenaHasNoMonsters);
}
