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

// NOTE(zoubir): a unit whose edge clips a wall's corner by a few units
// slips past it instead of stopping dead, both across and along its width
internal void
TestUnitSlipsPastWallCorner()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    // NOTE(zoubir): Y overlap with the wall is 16 + 4 - 17 = 3 units
    world_entity *Across = AddTestEntity(&Test, EntityType_Player,
                                         {330, 283, 0}, Test.UnitVolume);
    Walk(&Test, Across, {1, 0}, 120);
    Check(Across->Position.X > 450.f);
    Check(Across->Position.Y < 281.f);

    // NOTE(zoubir): X overlap is 15 + 16 - 27 = 4 units
    world_entity *Down = AddTestEntity(&Test, EntityType_Player,
                                       {373, 230, 0}, Test.UnitVolume);
    Walk(&Test, Down, {0, 1}, 120);
    Check(Down->Position.Y > 330.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): square on to a wall, or half across it, still stops
internal void
TestUnitDoesNotSlipFromMiddleOfWall()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 292, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 120);
    Check(Player->Position.X + 15.f <= 384.01f);
    Check(Player->Position.Y == 292.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a unit in the air over a wall has its ground on the wall's
// top, and back on the floor once past it
internal void
TestGroundUnderJumpingUnit()
{
    test_world Test = CreateTestWorld();
    world_entity *Wall = AddTestEntity(&Test, EntityType_StaticObject,
                                       {400, 300, 0}, Test.WallVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {900, 300, 0},
                  Test.WallVolume);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {400, 300, 60}, Test.UnitVolume);
    entity_collision_volume *Top = &Wall->Collision->TotalVolume;
    float WallTop = Top->Offset.Z + Top->HalfDims.Z;
    float MaxDistance = 1000.f;
    MoveEntity(Player, Test.World, &Test.Arena, 1.f / 60.f, Test.AppState,
               V3(0.f, 0.f, 0.f), &MaxDistance);
    Check(Absolute(Player->GroundZ - WallTop) < 0.01f);

    world_entity *Clear = AddTestEntity(&Test, EntityType_Player,
                                        {600, 300, 60}, Test.UnitVolume);
    MoveEntity(Clear, Test.World, &Test.Arena, 1.f / 60.f, Test.AppState,
               V3(0.f, 0.f, 0.f), &MaxDistance);
    Check(Clear->GroundZ == 0.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a unit that ends up a little inside a wall (a crowd
// shoved it there, or a knockback did) walks out and along the wall; it
// used to be held by every face of the wall at once
internal void
TestUnitInsideWallEdgeWalksOut()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    // NOTE(zoubir): right edge at 386, two units past the wall's left face
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {371, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {-1, 0}, 30);
    Check(Player->Position.X < 340.f);

    world_entity *Slider = AddTestEntity(&Test, EntityType_Player,
                                         {371, 290, 0}, Test.UnitVolume);
    Walk(&Test, Slider, {0, 1}, 30);
    Check(Slider->Position.Y > 320.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a stunned monster thrown into another passes most of its
// speed on, stuns it, and both take a little damage
internal void
TestThrownUnitKnocksIntoAnother()
{
    test_world Test = CreateTestWorld();
    world_entity *Thrown = AddTestEntity(&Test, EntityType_Monster,
                                         {300, 300, 0}, Test.UnitVolume);
    world_entity *Struck = AddTestEntity(&Test, EntityType_Monster,
                                         {340, 300, 0}, Test.UnitVolume);
    Thrown->MaxHp = Thrown->Hp = Struck->MaxHp = Struck->Hp = 100.f;
    ApplyStatus(Thrown, StatusEffect_Stunned, 1.f);
    Thrown->Velocity = V3(750.f, 0.f, 0.f);
    Walk(&Test, Thrown, {0, 0}, 4);
    Check(Struck->Velocity.X > 250.f);
    Check(HasStatus(Struck, StatusEffect_Stunned));
    Check(Struck->Hp < 100.f && Thrown->Hp < 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): slammed into a wall it is hurt and bounces off; a player
// dashing into the same wall is not hurt
internal void
TestThrownUnitSlamsIntoWall()
{
    test_world Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    world_entity *Thrown = AddTestEntity(&Test, EntityType_Monster,
                                         {360, 300, 0}, Test.UnitVolume);
    Thrown->MaxHp = Thrown->Hp = 100.f;
    ApplyStatus(Thrown, StatusEffect_Stunned, 0.2f);
    Thrown->Velocity = V3(750.f, 0.f, 0.f);
    Walk(&Test, Thrown, {0, 0}, 4);
    Check(Thrown->Hp < 100.f);
    Check(Thrown->Velocity.X < 0.f);
    Check(Thrown->StatusTimers[StatusEffect_Stunned] > 0.5f);

    world_entity *Dasher = AddTestEntity(&Test, EntityType_Player,
                                         {340, 400, 0}, Test.UnitVolume);
    AddTestEntity(&Test, EntityType_StaticObject, {400, 400, 0},
                  Test.WallVolume);
    Dasher->MaxHp = Dasher->Hp = 100.f;
    Dasher->Velocity = V3(750.f, 0.f, 0.f);
    Walk(&Test, Dasher, {1, 0}, 3);
    Check(Dasher->Hp == 100.f);
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
    printf("TestUnitSlipsPastWallCorner\n");
    TestUnitSlipsPastWallCorner();
    printf("TestUnitDoesNotSlipFromMiddleOfWall\n");
    TestUnitDoesNotSlipFromMiddleOfWall();
    printf("TestGroundUnderJumpingUnit\n");
    TestGroundUnderJumpingUnit();
    printf("TestUnitInsideWallEdgeWalksOut\n");
    TestUnitInsideWallEdgeWalksOut();
    printf("TestThrownUnitKnocksIntoAnother\n");
    TestThrownUnitKnocksIntoAnother();
    printf("TestThrownUnitSlamsIntoWall\n");
    TestThrownUnitSlamsIntoWall();
}
