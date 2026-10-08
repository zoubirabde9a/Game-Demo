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

// NOTE(zoubir): a player dashing into a wall at a slant keeps the dash's
// speed along it (wall_glide.cpp); straight at it, the wall stops them
internal void
TestPlayerGlidesAlongWall()
{
    test_world Test = CreateTestWorld();
    AddWallRow(&Test, 300.f, 300.f, 12);
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {340, 300 - 16 - 4 - 2, 0},
                                         Test.UnitVolume);
    Player->Velocity = {700.f, 700.f, 0.f};
    float MaxDistance = 10000.f;
    MoveEntity(Player, Test.World, &Test.Arena, Test.Input.DeltaTime,
               Test.AppState, {}, &MaxDistance);
    Check(Player->Velocity.X > 980.f);
    Check(Player->Velocity.Y < 0.01f);
    Check(Player->Position.Y + 4.f <= 300.f - 16.f + 0.01f);

    Player->Velocity = {0.f, 900.f, 0.f};
    MaxDistance = 10000.f;
    MoveEntity(Player, Test.World, &Test.Arena, Test.Input.DeltaTime,
               Test.AppState, {}, &MaxDistance);
    Check(Length(Player->Velocity.XY) < 1.f);
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

// NOTE(zoubir): a monster knocked back by a hit but not stunned bumps:
// it passes speed to the one it runs into, which is knocked in turn, and
// bounces off a wall, with no damage or stun either way
internal void
TestKnockedUnitBumps()
{
    test_world Test = CreateTestWorld();
    world_entity *Knocked = AddTestEntity(&Test, EntityType_Monster,
                                          {300, 300, 0}, Test.UnitVolume);
    world_entity *Struck = AddTestEntity(&Test, EntityType_Monster,
                                         {340, 300, 0}, Test.UnitVolume);
    Knocked->MaxHp = Knocked->Hp = Struck->MaxHp = Struck->Hp = 100.f;
    Knocked->HitFresh = 0.25f;
    Knocked->Velocity = V3(300.f, 0.f, 0.f);
    Walk(&Test, Knocked, {0, 0}, 4);
    Check(Struck->Velocity.X > 100.f);
    Check(Struck->HitFresh > 0.f);
    Check(!HasStatus(Struck, StatusEffect_Stunned));
    Check(Struck->Hp == 100.f && Knocked->Hp == 100.f);
    DestroyTestWorld(&Test);

    Test = CreateTestWorld();
    AddTestEntity(&Test, EntityType_StaticObject, {400, 300, 0},
                  Test.WallVolume);
    Knocked = AddTestEntity(&Test, EntityType_Monster, {360, 300, 0},
                            Test.UnitVolume);
    Knocked->MaxHp = Knocked->Hp = 100.f;
    Knocked->HitFresh = 0.25f;
    Knocked->Velocity = V3(300.f, 0.f, 0.f);
    Walk(&Test, Knocked, {0, 0}, 4);
    Check(Knocked->Velocity.X < 0.f);
    Check(Knocked->Hp == 100.f);
    Check(!HasStatus(Knocked, StatusEffect_Stunned));
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

// NOTE(zoubir): puts Entity back at Position at rest, keeping its chunk
internal void
PutBack(test_world *Test, world_entity *Entity, v3 Position)
{
    v3 Old = Entity->Position;
    Entity->Position = Position;
    Entity->Velocity = {};
    CheckAndChangeEntityChunk(Test->AppState, Test->World, &Test->Arena, Old,
                              Entity);
}

// NOTE(zoubir): the farthest Entity gets walking a third of a second in
// any of 8 directions while the Others move as they are pushed; everyone
// is put back where they were after each try
internal float
FreestDirection(test_world *Test, world_entity *Entity,
                world_entity **Others, u32 OtherCount)
{
    float Result = 0.f;
    v3 Start = Entity->Position;
    v3 Velocity = Entity->Velocity;
    v3 OtherStarts[16];
    Assert(OtherCount <= ArrayCount(OtherStarts));
    for(u32 Index = 0; Index < OtherCount; Index++)
    {
        OtherStarts[Index] = Others[Index]->Position;
    }
    for(u32 Dir = 0; Dir < 8; Dir++)
    {
        float Angle = 2.f * Pi32 * Dir / 8.f;
        for(u32 Frame = 0; Frame < 20; Frame++)
        {
            Walk(Test, Entity, V2(Cos(Angle), Sin(Angle)), 1);
            for(u32 Index = 0; Index < OtherCount; Index++)
            {
                Walk(Test, Others[Index], {0, 0}, 1);
            }
        }
        Result = Maximum(Result, Length(Entity->Position.XY - Start.XY));
        PutBack(Test, Entity, Start);
        for(u32 Index = 0; Index < OtherCount; Index++)
        {
            PutBack(Test, Others[Index], OtherStarts[Index]);
        }
    }
    Entity->Velocity = Velocity;
    return Result;
}

// NOTE(zoubir): what touches Entity, for a failure report
internal void
PrintNeighbours(world *World, world_entity *Entity)
{
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Other = &World->Entities[Index];
        v3 Rel = Other->Position - Entity->Position;
        if (!Other->IsPresent || Other == Entity || !Other->Collision ||
            Absolute(Rel.X) > 70.f || Absolute(Rel.Y) > 70.f)
        {
            continue;
        }
        v3 Half = Other->Collision->TotalVolume.HalfDims;
        printf("    type %d at (%.2f, %.2f, %.2f) half (%.1f, %.1f, %.1f)\n",
               Other->Type, Other->Position.X, Other->Position.Y,
               Other->Position.Z, Half.X, Half.Y, Half.Z);
    }
}

// NOTE(zoubir): a player among rocks, walls and a crowd, walking, jumping,
// dashing and being shoved at random, is never left unable to move in
// every direction (the "sometimes we get stuck" report)
internal void
TestPlayerNeverStuck(u32 SeedValue)
{
    test_world Test = CreateTestWorld();
    SetupCollisionVolumes(Test.AppState, &Test.Arena);
    entity_collision_volume_group *Boulder =
        MakeSimpleGroundedCollisionVolume(&Test.Arena, {13.f, 8.f, 14.f});
    random_series Series = Seed(SeedValue);
    AddWallRow(&Test, 200.f, 200.f, 20);
    AddWallRow(&Test, 200.f, 840.f, 20);
    world_entity *Rocks[30];
    u32 RockCount = 0;
    for(u32 Index = 0; Index < 30; Index++)
    {
        v3 P = {RandomBetween(&Series, 240.f, 820.f),
                RandomBetween(&Series, 260.f, 780.f), 0.f};
        world_entity *Obstacle =
            AddTestEntity(&Test, EntityType_StaticObject, P,
                          (Index % 3) ? Boulder : Test.AppState->WallCollision);
        if (Index % 3)
        {
            Rocks[RockCount++] = Obstacle;
        }
    }
    world_entity *Monsters[12];
    for(u32 Index = 0; Index < ArrayCount(Monsters); Index++)
    {
        v3 P = {RandomBetween(&Series, 240.f, 820.f),
                RandomBetween(&Series, 260.f, 780.f), 0.f};
        Monsters[Index] = AddTestEntity(&Test, EntityType_Monster, P,
                                        Test.UnitVolume);
        Monsters[Index]->MaxHp = Monsters[Index]->Hp = 1.e6f;
    }
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {520, 230, 0}, Test.UnitVolume);
    Player->MaxHp = Player->Hp = 1.e6f;
    SeparateOverlappingUnits(Test.AppState, Test.World, &Test.Arena);

    u32 StuckCount = 0;
    u32 StoodOnTop = 0;
    for(u32 Step = 0; Step < 400; Step++)
    {
        float Angle = RandomBetween(&Series, 0.f, 2.f * Pi32);
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        u32 Roll = RandomChoice(&Series, 9);
        if (Roll == 0)
        {
            Player->Velocity.Z = PLAYER_JUMP_SPEED;
        }
        else if (Roll == 3)
        {
            // NOTE(zoubir): a double jump, high enough to land on a rock
            Player->Velocity.Z = PLAYER_JUMP_SPEED + PLAYER_AIR_JUMP_SPEED;
        }
        else if (Roll == 4 && Player->Position.Z > Player->GroundZ + 1.f)
        {
            // NOTE(zoubir): a slam's dive
            Player->Velocity.XY *= 0.3f;
            Player->Velocity.Z = -PlayerMovements[PlayerMove_Slam].Power;
        }
        else if (Roll == 6)
        {
            // NOTE(zoubir): lands on top of a rock, to walk off its edges
            world_entity *Rock = Rocks[RandomChoice(&Series, RockCount)];
            entity_collision_volume *Top = &Rock->Collision->TotalVolume;
            v3 OnTop = Rock->Position;
            OnTop.Z = Top->Offset.Z + Top->HalfDims.Z + 0.01f;
            PutBack(&Test, Player, OnTop);
            Walk(&Test, Player, {0, 0}, 1);
        }
        else if (Roll == 5)
        {
            // NOTE(zoubir): launched by someone, stunned and flying
            Player->Velocity.Z = PlayerAreaAbilities[PlayerArea_Launch].Hit.Lift;
            Player->Velocity.XY = 300.f * Dir;
            ApplyStatus(Player, StatusEffect_Stunned, 0.6f);
        }
        else if (Roll == 1)
        {
            Player->Velocity.XY = PlayerMovements[PlayerMove_Dash].Power * Dir;
        }
        else if (Roll == 2)
        {
            // NOTE(zoubir): the crowd shoves into the player
            for(u32 Index = 0; Index < ArrayCount(Monsters); Index++)
            {
                v2 ToPlayer = Player->Position.XY - Monsters[Index]->Position.XY;
                float Distance = Length(ToPlayer);
                if (Distance > 1.f && Distance < 120.f)
                {
                    Monsters[Index]->Velocity.XY = (600.f / Distance) * ToPlayer;
                    ApplyStatus(Monsters[Index], StatusEffect_Stunned, 0.5f);
                }
            }
        }
        u32 Frames = 5 + RandomChoice(&Series, 25);
        for(u32 Frame = 0; Frame < Frames; Frame++)
        {
            Walk(&Test, Player, Dir, 1);
            for(u32 Index = 0; Index < ArrayCount(Monsters); Index++)
            {
                Walk(&Test, Monsters[Index], {0, 0}, 1);
            }
            SeparateOverlappingUnits(Test.AppState, Test.World, &Test.Arena);
        }
        StoodOnTop += (Player->GroundZ > 0.f &&
                       Player->Position.Z <= Player->GroundZ + 0.5f);
        if (Player->Position.Z <= Player->GroundZ + 0.5f &&
            FreestDirection(&Test, Player, Monsters, ArrayCount(Monsters)) < 2.f)
        {
            if (StuckCount++ == 0)
            {
                printf("  seed %u: stuck at (%.1f, %.1f, %.1f) after step %u\n",
                       SeedValue, Player->Position.X, Player->Position.Y,
                       Player->Position.Z, Step);
                PrintNeighbours(Test.World, Player);
            }
        }
    }
    Check(StuckCount == 0);
    Check(StoodOnTop > 5);
    printf("  seed %u: stood on top of something after %u of 400 steps\n",
           SeedValue, StoodOnTop);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): raised ground, on the test-only elevation from
// terrain_tests.cpp. Tiles are 32 units, a step 8; the test units are 30
// wide and walk at about 93 units a second

// NOTE(zoubir): stairs rising one step per tile, from tile 12 up to four
// steps at tile 15 and on, are walked up without a jump
internal void
TestOneStepStairsAreWalkedUp()
{
    test_world Test = CreateTestWorld();
    test_elevation Elevation = BeginTestElevation(&Test);
    for(i32 Step = 1; Step <= 4; Step++)
    {
        SetTestSteps(&Elevation, 11 + Step, 5, Step == 4 ? 25 : 11 + Step, 14, Step);
    }
    world_entity *Walker = AddTestEntity(&Test, EntityType_Player, {330, 320, 0},
                                         Test.UnitVolume);
    Walk(&Test, Walker, {1, 0}, 240);
    Check(Walker->Position.X > 15 * 32 + 20.f);
    Check(Absolute(Walker->Position.Z - 32.f) < 0.1f);
    Check(Walker->GroundZ == 32.f);
    // NOTE(zoubir): and back down without a fall
    Walk(&Test, Walker, {-1, 0}, 240);
    Check(Walker->Position.X < 12 * 32 - 15.f);
    Check(Walker->Position.Z < 0.1f);
    EndTestElevation(&Elevation);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a ledge three steps up stops a walking unit like a wall
internal void
TestThreeStepLedgeBlocksWalking()
{
    test_world Test = CreateTestWorld();
    test_elevation Elevation = BeginTestElevation(&Test);
    SetTestSteps(&Elevation, 12, 5, 20, 14, 3);
    world_entity *Walker = AddTestEntity(&Test, EntityType_Player, {300, 320, 0},
                                         Test.UnitVolume);
    Walk(&Test, Walker, {1, 0}, 120);
    Check(Walker->Position.X + 15.f <= 12 * 32 + 0.01f);
    Check(Walker->Position.X + 15.f > 12 * 32 - 2.f);
    Check(Walker->Position.Z == 0.f);
    EndTestElevation(&Elevation);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): on top of a plateau a unit walks across it, over the seams
// between its tiles, at its height; then off its edge, and falls
internal void
TestWalkingAcrossAndOffPlateau()
{
    test_world Test = CreateTestWorld();
    test_elevation Elevation = BeginTestElevation(&Test);
    SetTestSteps(&Elevation, 8, 5, 20, 14, 3);
    world_entity *Walker = AddTestEntity(&Test, EntityType_Player, {300, 320, 24.01f},
                                         Test.UnitVolume);
    Walk(&Test, Walker, {0.707f, 0.707f}, 150);
    Check(Walker->Position.X > 380.f && Walker->Position.Y > 390.f);
    Check(Absolute(Walker->Position.Z - 24.f) < 0.1f);
    Check(Walker->GroundZ == 24.f);
    Walk(&Test, Walker, {1, 0}, 300);
    Check(Walker->Position.X > 21 * 32 + 15.f);
    Check(Walker->Position.Z == 0.f);
    Check(Walker->GroundZ == 0.f);
    EndTestElevation(&Elevation);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a monster chasing a player up on a cliff three steps high
// stays at its foot; given stairs, it climbs them
internal void
TestMonsterCannotWalkUpCliff()
{
    test_world Test = CreateTestWorld();
    test_elevation Elevation = BeginTestElevation(&Test);
    SetTestSteps(&Elevation, 12, 5, 20, 14, 3);
    world_entity *Monster = AddTestMonster(&Test, MonsterKind_Brute, {300, 320, 0});
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {580, 320, 0});
    StepMonster(&Test, Monster, 180);
    Check(Monster->Position.X + 15.f <= 12 * 32 + 0.01f);
    Check(Monster->Position.X + 15.f > 12 * 32 - 4.f);
    Check(Monster->Position.Z == 0.f);

    SetTestSteps(&Elevation, 12, 5, 12, 14, 1);
    SetTestSteps(&Elevation, 13, 5, 13, 14, 2);
    StepMonster(&Test, Monster, 180);
    Check(Monster->Position.X > 14 * 32 + 15.f);
    Check(Monster->Position.Z > 23.f);
    EndTestElevation(&Elevation);
    DestroyTestWorld(&Test);
}

#define TEST_NO_JUMP 0xFFFFFFFFu

// NOTE(zoubir): the real player tick for slot 0, holding Move and pressing
// jump on frames FirstJump and SecondJump
internal void
RunTestPlayer(test_world *Test, v2 Move, u32 Frames, u32 FirstJump,
              u32 SecondJump)
{
    app_state *AppState = Test->AppState;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        AppState->Players[0].Input.Move = Move;
        AppState->Players[0].Input.Pressed =
            (Frame == FirstJump || Frame == SecondJump) ? PlayerButton_Jump : 0;
        SimulateTick(AppState, &Test->Arena, 1.f / 60.f);
    }
}

// NOTE(zoubir): a player pushing into a three-step ledge vaults onto it on
// its own; six steps need a double jump; nine are a wall
internal void
TestPlayerJumpsOntoLedges()
{
    i32 Heights[] = {3, 6, 9};
    for(u32 Index = 0; Index < ArrayCount(Heights); Index++)
    {
        i32 Steps = Heights[Index];
        float Top = Steps * ELEVATION_STEP_HEIGHT;
        test_world Test = CreateTestWorld();
        test_elevation Elevation = BeginTestElevation(&Test);
        SetTestSteps(&Elevation, 12, 5, 20, 14, Steps);
        world_entity *Player = AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena,
                                               0, {340, 320, 0});
        RunTestPlayer(&Test, {1, 0}, 90, TEST_NO_JUMP, TEST_NO_JUMP);
        if (Steps <= ELEVATION_JUMP_STEPS)
        {
            Check(Player->Position.X > 12 * 32 + 15.f);
            Check(Absolute(Player->Position.Z - Top) < 0.1f);
        }
        else
        {
            Check(Player->Position.X + 15.f <= 12 * 32 + 0.01f);
            Check(Player->Position.Z < 0.1f);
            // NOTE(zoubir): the second press near the top of the first jump
            RunTestPlayer(&Test, {1, 0}, 90, 0, 12);
            bool32 Reachable = Steps <= ELEVATION_DOUBLE_JUMP_STEPS;
            Check(Reachable == (Player->Position.X > 12 * 32 + 15.f));
            Check(Reachable == (Absolute(Player->Position.Z - Top) < 0.1f));
            if (!Reachable)
            {
                Check(Player->Position.Z < 0.1f);
            }
        }
        EndTestElevation(&Elevation);
        DestroyTestWorld(&Test);
    }
}

// NOTE(zoubir): a log, a fence and a crate stop a walking unit, and a
// player pushing into one vaults it; a crate holds whoever lands on it
internal void
TestPropsAreJumpable()
{
    terrain_prop Props[] = {TerrainProp_Log, TerrainProp_Fence, TerrainProp_Crate};
    for(u32 Index = 0; Index < ArrayCount(Props); Index++)
    {
        test_world Test = CreateTestWorld();
        test_elevation Elevation = BeginTestElevation(&Test);
        v3 PropAt = {400, 320, 0};
        world_entity *Prop = AddTerrainProp(Test.AppState, Test.World, &Test.Arena,
                                            PropAt, Props[Index]);
        Check(Prop->Texture.Type == AssetType_TerrainProp);
        Check(Prop->Texture.Index == (u32)Props[Index]);
        float HalfX = PropTable[Props[Index]].HalfDims.X;
        float Height = 2.f * PropTable[Props[Index]].HalfDims.Z;

        world_entity *Walker = AddTestEntity(&Test, EntityType_Monster, {300, 320, 0},
                                             Test.UnitVolume);
        Walk(&Test, Walker, {1, 0}, 120);
        Check(Walker->Position.X + 15.f <= PropAt.X - HalfX + 0.01f);
        RemoveEntity(Test.World, Walker);

        world_entity *Player = AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena,
                                               0, {340, 320, 0});
        RunTestPlayer(&Test, {1, 0}, 120, TEST_NO_JUMP, TEST_NO_JUMP);
        Check(Player->Position.X - 15.f > PropAt.X + HalfX);
        Check(Player->Position.Z < 0.1f);

        // NOTE(zoubir): dropped onto it, a player stands on a crate
        if (Props[Index] == TerrainProp_Crate)
        {
            v3 Old = Player->Position;
            Player->Position = PropAt + V3(0, 0, Height + 6.f);
            Player->Velocity = {};
            CheckAndChangeEntityChunk(Test.AppState, Test.World, &Test.Arena, Old, Player);
            RunTestPlayer(&Test, {0, 0}, 30, TEST_NO_JUMP, TEST_NO_JUMP);
            Check(Player->GroundZ == Height);
            Check(Absolute(Player->Position.Z - Height) < 0.1f);
        }
        EndTestElevation(&Elevation);
        DestroyTestWorld(&Test);
    }
}

internal void
RunCollisionTests()
{
    GROUP(RUN(TestOneStepStairsAreWalkedUp));
    GROUP(RUN(TestThreeStepLedgeBlocksWalking));
    GROUP(RUN(TestWalkingAcrossAndOffPlateau));
    GROUP(RUN(TestMonsterCannotWalkUpCliff));
    GROUP(RUN(TestPlayerJumpsOntoLedges));
    GROUP(RUN(TestPropsAreJumpable));
    GROUP(RUN(TestWallStopsUnit));
    GROUP(RUN(TestUnitSlidesAlongWall));
    GROUP(RUN(TestUnitsDoNotPassThroughEachOther));
    GROUP(RUN(TestUnitSlidesPastTileSeams));
    GROUP(RUN(TestPlayerGlidesAlongWall));
    GROUP(RUN(TestUnitStopsInCorner));
    GROUP(RUN(TestLongPushStaysOutsideWall));
    GROUP(RUN(TestOverlappingUnitsCanSeparate));
    GROUP(RUN(TestFastUnitDoesNotTunnel));
    GROUP(RUN(TestUnitSlipsPastWallCorner));
    GROUP(RUN(TestUnitDoesNotSlipFromMiddleOfWall));
    GROUP(RUN(TestGroundUnderJumpingUnit));
    GROUP(RUN(TestUnitInsideWallEdgeWalksOut));
    GROUP(RUN(TestThrownUnitKnocksIntoAnother));
    GROUP(RUN(TestKnockedUnitBumps));
    GROUP(RUN(TestThrownUnitSlamsIntoWall));
    for(u32 SeedValue = 1; SeedValue <= 6; SeedValue++)
    {
        GROUP(printf("TestPlayerNeverStuck %u\n", SeedValue * 977); TestPlayerNeverStuck(SeedValue * 977));
    }
}
