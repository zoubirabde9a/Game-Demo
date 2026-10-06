/* Hazard tests: the status effect table (sim/status_effects.cpp) and the
   ground that applies it (sim/terrain_effects.cpp). Pits kill, and credit
   whoever threw you in; a jump clears them; springs heal and wash off
   poison; water puts out fire; a root grabs once and is then shrugged
   off; haste lifts a slow; monsters walk around pits; nothing spawns on
   one. Uses the Old Arena's layout (the test world's map), whose pits,
   springs and lava pools sit at fixed tiles. Included by sim_tests.cpp. */

// NOTE(zoubir): the middle of an Old Arena tile, in world units
inline v3
ArenaTileCenter(i32 Column, i32 Row)
{
    v3 Result = V3(((float)Column + 0.5f) * 32.f, ((float)Row + 0.5f) * 32.f, 0.f);
    return Result;
}

// NOTE(zoubir): the ground's rules and the status clocks, for Frames ticks
internal void
StepGround(test_world *Test, u32 Frames)
{
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        UpdateTerrainEffects(Test->World);
        UpdateStatusEffects(Test->AppState, Test->World, Test->Input.DeltaTime);
    }
}

internal void
TestArenaHasEveryHazard()
{
    map_def *Map = GetMapDef(MapId_Arena);
    Check(TerrainAt(Map, 69, 9) == TerrainKind_Pit);
    Check(TerrainAt(Map, 6, 17) == TerrainKind_Spring);
    Check(TerrainAt(Map, 29, 8) == TerrainKind_Lava);
    Check(TerrainAt(Map, 46, 15) == TerrainKind_Rune);
    // NOTE(zoubir): nothing in the arena but its outer wall blocks
    u32 Blocking = 0;
    for(u32 Y = 1; Y + 1 < Map->Height; Y++)
    {
        for(u32 X = 1; X + 1 < Map->Width; X++)
        {
            Blocking += GetTerrainDef(TerrainAt(Map, (i32)X, (i32)Y))->Blocks ? 1 : 0;
        }
    }
    Check(Blocking == 0);
}

internal void
TestPitDropsAndKills()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestPlayer(&Test, ArenaTileCenter(69, 9));
    Player->Velocity = V3(200.f, 0.f, 0.f);
    StepGround(&Test, 1);
    Check(HasStatus(Player, StatusEffect_Falling));
    Check(IsRooted(Player) && IsDisabled(Player));
    Check(Player->Velocity.X == 0.f);
    Check(Player->Hp == 100.f);
    StepGround(&Test, 60);
    Check(Player->Hp <= 0.f);
    Check(Test.AppState->Players[0].Deaths == 1);
    DestroyTestWorld(&Test);
}

internal void
TestJumpClearsPit()
{
    test_world Test = CreateTestWorld();
    v3 Above = ArenaTileCenter(69, 9);
    Above.Z = 30.f;
    world_entity *Player = AddTestPlayer(&Test, Above);
    StepGround(&Test, 1);
    Check(!HasStatus(Player, StatusEffect_Falling));
    DestroyTestWorld(&Test);
}

internal void
TestPitKillGoesToThrower()
{
    test_world Test = CreateTestWorld();
    world_entity *Thrower = AddTestPlayer(&Test, ArenaTileCenter(60, 9));
    Thrower->PlayerIndex = 1;
    Test.AppState->Players[1].Active = true;
    Test.AppState->Players[1].Entity = Thrower;
    world_entity *Victim = AddTestPlayer(&Test, ArenaTileCenter(69, 9));
    ApplyStatus(Victim, StatusEffect_Stunned, 1.f);
    Victim->ThrownBySlot = 2;
    StepGround(&Test, 60);
    Check(Victim->Hp <= 0.f);
    Check(Test.AppState->Players[1].Kills == 1);
    DestroyTestWorld(&Test);
}

internal void
TestSpringHealsAndWashesOffPoison()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestPlayer(&Test, ArenaTileCenter(6, 17));
    Player->Hp = 50.f;
    ApplyStatus(Player, StatusEffect_Poisoned, 5.f);
    StepGround(&Test, 60);
    Check(!HasStatus(Player, StatusEffect_Poisoned));
    Check(HasStatus(Player, StatusEffect_Regenerating));
    Check(HasStatus(Player, StatusEffect_Soaked));
    Check(Player->Hp > 50.f);
    StepGround(&Test, 60 * 20);
    Check(Player->Hp == Player->MaxHp);
    DestroyTestWorld(&Test);
}

internal void
TestLavaBurnsAndKeepsBurning()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestPlayer(&Test, ArenaTileCenter(29, 8));
    StepGround(&Test, 30);
    Check(HasStatus(Player, StatusEffect_Burning));
    Check(Player->Hp < 100.f);
    // NOTE(zoubir): stepped off onto grass: still burning a while
    Player->Position = ArenaTileCenter(25, 8);
    StepGround(&Test, 60);
    Check(HasStatus(Player, StatusEffect_Burning));
    StepGround(&Test, 120);
    Check(!HasStatus(Player, StatusEffect_Burning));
    DestroyTestWorld(&Test);
}

internal void
TestWaterPutsOutFire()
{
    world_entity Entity = {};
    ApplyStatus(&Entity, StatusEffect_Burning, 3.f);
    ApplyStatus(&Entity, StatusEffect_Soaked, 2.f);
    Check(!HasStatus(&Entity, StatusEffect_Burning));
    ApplyStatus(&Entity, StatusEffect_Burning, 3.f);
    Check(!HasStatus(&Entity, StatusEffect_Burning));
}

internal void
TestRootGrabsOnceThenIsShrugged()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestPlayer(&Test, ArenaTileCenter(25, 8));
    ApplyStatus(Player, StatusEffect_Rooted, 0.5f);
    Check(IsRooted(Player) && !IsDisabled(Player));
    StepGround(&Test, 40);
    Check(!IsRooted(Player));
    ApplyStatus(Player, StatusEffect_Rooted, 0.5f);
    Check(!IsRooted(Player));
    StepGround(&Test, 60 * 3);
    ApplyStatus(Player, StatusEffect_Rooted, 0.5f);
    Check(IsRooted(Player));
    DestroyTestWorld(&Test);
}

internal void
TestHasteLiftsSlowAndSpeedsUp()
{
    world_entity Entity = {};
    ApplyStatus(&Entity, StatusEffect_Slowed, 2.f);
    ApplyStatus(&Entity, StatusEffect_Hasted, 2.f);
    Check(!HasStatus(&Entity, StatusEffect_Slowed));
    Check(GetMoveSpeedScale(&Entity) > 1.4f);
}

internal void
TestBleedHurtsMoreWhenMoving()
{
    world_entity Still = {};
    world_entity Running = {};
    ApplyStatus(&Still, StatusEffect_Bleeding, 2.f);
    ApplyStatus(&Running, StatusEffect_Bleeding, 2.f);
    Running.Velocity = V3(200.f, 0.f, 0.f);
    Check(StatusDamagePerSecond(&Running) > 2.f * StatusDamagePerSecond(&Still));
}

internal void
TestMonsterWalksAroundPit()
{
    test_world Test = CreateTestWorld();
    // NOTE(zoubir): two tiles left of the pit, wanting to walk right into it
    world_entity *Monster = AddTestMonster(&Test, MonsterKind_Brute,
                                           ArenaTileCenter(68, 9) - V3(20.f, 0.f, 0.f));
    v2 Wish = SteerAroundHazards(Test.World, Monster, V2(1.f, 0.f));
    v3 Ahead = Monster->Position;
    Ahead.XY += HAZARD_LOOK_AHEAD * Wish;
    Check(!IsHazardAt(Test.World, Ahead));
    // NOTE(zoubir): and nothing spawns on a pit
    Check(!IsSpawnSpotFree(Test.AppState, Test.World, ArenaTileCenter(69, 9),
                           Test.UnitVolume));
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a player who keeps burning dies in
// STATUS_PLAYER_BURN_SECONDS whatever its health, not in one tick at one
// point of health nor in 12 s at 100
internal void
TestPlayersBurnToDeathInFourSeconds()
{
    float Healths[3] = {1.f, 75.f, 100.f};
    for(u32 Index = 0; Index < ArrayCount(Healths); Index++)
    {
        test_world Test = CreateTestWorld();
        world_entity *Player = AddTestPlayer(&Test, ArenaTileCenter(25, 8));
        Player->MaxHp = Player->Hp = Healths[Index];
        u32 Frames = 0;
        while (Frames < (u32)(3.6f / Test.Input.DeltaTime))
        {
            ApplyStatus(Player, StatusEffect_Burning, 1.f);
            StepGround(&Test, 1);
            Frames++;
        }
        Check(Player->Hp > 0.f && Player->Hp < 0.2f * Player->MaxHp);
        while (Frames < (u32)(4.1f / Test.Input.DeltaTime))
        {
            ApplyStatus(Player, StatusEffect_Burning, 1.f);
            StepGround(&Test, 1);
            Frames++;
        }
        Check(Player->Hp <= 0.f);
        DestroyTestWorld(&Test);
    }
}

#define HAZARD_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunHazardTests()
{
    HAZARD_TEST(TestArenaHasEveryHazard);
    HAZARD_TEST(TestPitDropsAndKills);
    HAZARD_TEST(TestJumpClearsPit);
    HAZARD_TEST(TestPitKillGoesToThrower);
    HAZARD_TEST(TestSpringHealsAndWashesOffPoison);
    HAZARD_TEST(TestLavaBurnsAndKeepsBurning);
    HAZARD_TEST(TestWaterPutsOutFire);
    HAZARD_TEST(TestRootGrabsOnceThenIsShrugged);
    HAZARD_TEST(TestHasteLiftsSlowAndSpeedsUp);
    HAZARD_TEST(TestBleedHurtsMoreWhenMoving);
    HAZARD_TEST(TestMonsterWalksAroundPit);
    HAZARD_TEST(TestPlayersBurnToDeathInFourSeconds);
}
