/* Collision tests: units against walls, against each other, along rows
   of wall tiles and into corners, through MoveEntity with the same
   acceleration and drag as a walking player (Walk in sim_tests.cpp).
   Included by sim_tests.cpp, which calls RunCollisionTests. */

internal void
TestWallStopsUnit()
{
    test_world Test = CreateTestWorld();
    world_entity *Wall = AddTestEntity(&Test, EntityType_StaticObject,
                                       {400, 300, 0}, Test.WallVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 120);

    float PlayerRight = Player->Position.X + 15.f;
    float WallLeft = Wall->Position.X - 16.f;
    Check(PlayerRight <= WallLeft + 0.01f);
    Check(PlayerRight > WallLeft - 2.f);
    Check(Player->Position.Y == 300.f);
    DestroyTestWorld(&Test);
}

internal void
TestUnitSlidesAlongWall()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    // NOTE(zoubir): level with the wall's left face, 9 units away
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {360, 282, 0}, Test.UnitVolume);
    // NOTE(zoubir): pressing into the face while moving down keeps the
    // downward part of the move
    Walk(&Test, Player, {0.707f, 0.707f}, 30);
    Check(Player->Position.X + 15.f <= 384.01f);
    Check(Player->Position.Y > 295.f);
    DestroyTestWorld(&Test);
}

internal void
TestUnitsDoNotPassThroughEachOther()
{
    test_world Test = CreateTestWorld();
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {400, 300, 0}, Test.UnitVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 120);
    Check(Player->Position.X + 15.f <= Monster->Position.X - 15.f + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a row of wall tiles along X, top faces at Y = 300 - 16
internal void
AddWallRow(test_world *Test, float FirstX, float Y, u32 Count)
{
    for(u32 Index = 0; Index < Count; Index++)
    {
        AddTestEntity(Test, EntityType_StaticObject,
                      {FirstX + 32.f * Index, Y, 0}, Test->WallVolume);
    }
}

// NOTE(zoubir): pressing diagonally into a row of tiles slides the whole
// way along it instead of catching where two tiles meet
internal void
TestUnitSlidesPastTileSeams()
{
    test_world Test = CreateTestWorld();
    AddWallRow(&Test, 300.f, 300.f, 12);
    // NOTE(zoubir): standing on the row's top face (unit half height 4)
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {320, 300 - 16 - 4 - 1, 0},
                                         Test.UnitVolume);
    // NOTE(zoubir): a diagonal walk moves 66 units a second along X, so
    // four seconds cross seven seams
    Walk(&Test, Player, {0.707f, 0.707f}, 240);
    Check(Player->Position.X > 540.f);
    Check(Player->Position.Y + 4.f <= 300.f - 16.f + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): driving into an inside corner stops there; it neither
// passes through nor shakes
internal void
TestUnitStopsInCorner()
{
    test_world Test = CreateTestWorld();
    AddWallRow(&Test, 300.f, 300.f, 6);
    for(u32 Index = 1; Index < 6; Index++)
    {
        AddTestEntity(&Test, EntityType_StaticObject,
                      {460.f, 300.f - 32.f * Index, 0}, Test.WallVolume);
    }
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {360, 200, 0}, Test.UnitVolume);
    Walk(&Test, Player, {0.707f, 0.707f}, 120);
    v3 Rest = Player->Position;
    Check(Rest.X + 15.f <= 460.f - 16.f + 0.01f);
    Check(Rest.Y + 4.f <= 300.f - 16.f + 0.01f);
    Walk(&Test, Player, {0.707f, 0.707f}, 30);
    Check(Player->Position.X - Rest.X < 0.05f &&
          Player->Position.X - Rest.X > -0.05f);
    Check(Player->Position.Y - Rest.Y < 0.05f &&
          Player->Position.Y - Rest.Y > -0.05f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): ten seconds of pushing into a wall never leaks through
internal void
TestLongPushStaysOutsideWall()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 600);
    Check(Player->Position.X + 15.f <= 400.f - 16.f + 0.01f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): two units that start overlapping (a spawn on top of
// another) can still walk apart
internal void
TestOverlappingUnitsCanSeparate()
{
    test_world Test = CreateTestWorld();
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {300, 300, 0}, Test.UnitVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {305, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 90);
    Check(Player->Position.X > Monster->Position.X + 30.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a dash-speed unit does not tunnel through a thin wall
internal void
TestFastUnitDoesNotTunnel()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 300, 0}, Test.UnitVolume);
    Player->Velocity = {6000.f, 0, 0};
    float MaxDistance = 10000.f;
    MoveEntity(Player, Test.World, &Test.Arena, Test.Input.DeltaTime,
               Test.AppState, {}, &MaxDistance);
    Check(Player->Position.X + 15.f <= 400.f - 16.f + 0.01f);
    DestroyTestWorld(&Test);
}

internal void
RunCollisionTests()
{
    printf("TestWallStopsUnit\n");
    TestWallStopsUnit();
    printf("TestUnitSlidesAlongWall\n");
    TestUnitSlidesAlongWall();
    printf("TestUnitsDoNotPassThroughEachOther\n");
    TestUnitsDoNotPassThroughEachOther();
    printf("TestUnitSlidesPastTileSeams\n");
    TestUnitSlidesPastTileSeams();
    printf("TestUnitStopsInCorner\n");
    TestUnitStopsInCorner();
    printf("TestLongPushStaysOutsideWall\n");
    TestLongPushStaysOutsideWall();
    printf("TestOverlappingUnitsCanSeparate\n");
    TestOverlappingUnitsCanSeparate();
    printf("TestFastUnitDoesNotTunnel\n");
    TestFastUnitDoesNotTunnel();
}
