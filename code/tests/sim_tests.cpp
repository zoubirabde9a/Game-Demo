/* Simulation tests: builds the real game code (app.cpp) into a console
   program, sets up a small world with no window or assets, and checks
   movement, collision and damage rules. Run test.bat from the repo root;
   exit code 0 means every check passed. */

#include "../app.cpp"

global_variable int TestFailures;
global_variable int TestChecks;

#define Check(Expression) CheckImpl((Expression) != 0, #Expression, __FILE__, __LINE__)

internal void
CheckImpl(bool Passed, char *Expression, char *File, int Line)
{
    TestChecks++;
    if (!Passed)
    {
        TestFailures++;
        printf("  FAILED %s(%d): %s\n", File, Line, Expression);
    }
}

struct test_world
{
    app_state *AppState;
    world *World;
    memory_arena Arena;
    app_input Input;
    entity_collision_volume_group *UnitVolume;
    entity_collision_volume_group *WallVolume;
    entity_collision_volume_group *FireBallVolume;
};

// NOTE(zoubir): 64x64 tiles of 32 units, every rule from the real game
internal test_world
CreateTestWorld()
{
    test_world Result = {};
    Result.AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index ArenaSize = Megabytes(16);
    InitializeArena(&Result.Arena, (memory_index *)calloc(1, ArenaSize),
                    ArenaSize);

    world *World = &Result.AppState->World;
    World->TileWidth = World->TileHeight = World->TileDepth = 32;
    World->CollisionWidth = World->CollisionHeight = World->CollisionDepth = 32;
    World->TilesPerChunkX = 16;
    World->TilesPerChunkY = 16;
    World->TilesPerChunkZ = 4;
    World->NumTilesX = 64;
    World->NumTilesY = 64;
    World->NumTilesZ = 1;
    Result.World = World;

    SetupCollisionTable(Result.AppState);
    Result.UnitVolume =
        MakeSimpleGroundedCollisionVolume(&Result.Arena, {15, 4, 19.f});
    Result.WallVolume =
        MakeSimpleGroundedCollisionVolume(&Result.Arena, {16, 16, 16.f});
    Result.FireBallVolume =
        MakeSimpleGroundedCollisionVolume(&Result.Arena, {9, 9, 0.f});
    Result.Input.DeltaTime = 1.f / 60.f;
    return Result;
}

internal void
DestroyTestWorld(test_world *Test)
{
    free(Test->Arena.Base);
    free(Test->AppState);
}

internal world_entity *
AddTestEntity(test_world *Test, entity_type Type, v3 Position,
              entity_collision_volume_group *Volume)
{
    world_entity *Result = AddEntity(Test->AppState, Test->World,
                                     &Test->Arena, Type, Position, Volume);
    return Result;
}

// NOTE(zoubir): same acceleration, drag and gravity as UpdatePlayer
internal void
Walk(test_world *Test, world_entity *Entity, v2 Direction, u32 Frames)
{
    for(u32 Frame = 0; Frame < Frames && Entity->IsPresent; Frame++)
    {
        v3 DDEntity = {};
        DDEntity.XY = Direction;
        DDEntity *= 56000.f * Test->Input.DeltaTime;
        DDEntity -= 10.f * Entity->Velocity;
        DDEntity.Z = -1000.f;
        float MaxDistance = 10000.f;
        MoveEntity(Entity, Test->World, &Test->Arena, &Test->Input,
                   Test->AppState, DDEntity, &MaxDistance);
    }
}

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

internal void
TestFireBallKillsMonsterOnce()
{
    test_world Test = CreateTestWorld();
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {400, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = FIREBALL_DAMAGE;
    world_entity *FireBall = AddTestEntity(&Test, EntityType_FireBall,
                                           {300, 300, 0},
                                           Test.FireBallVolume);
    FireBall->Velocity = {450, 0, 0};
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        float MaxDistance = 10000.f;
        MoveEntity(FireBall, Test.World, &Test.Arena, &Test.Input,
                   Test.AppState, {}, &MaxDistance);
    }
    Check(!Monster->IsPresent);
    Check(Test.AppState->KillCount == 1);
    // NOTE(zoubir): fireballs pierce, so it kept flying past
    Check(FireBall->Position.X > 420.f);
    DestroyTestWorld(&Test);
}

internal void
TestMonsterDyingMidMoveLeavesNoGhost()
{
    test_world Test = CreateTestWorld();
    world_entity *FireBall = AddTestEntity(&Test, EntityType_FireBall,
                                           {400, 300, 0},
                                           Test.FireBallVolume);
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {300, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = FIREBALL_DAMAGE;
    Walk(&Test, Monster, {1, 0}, 60);
    Check(!Monster->IsPresent);
    Check(FireBall->IsPresent);

    // NOTE(zoubir): the spot where the monster died must be walkable
    // (players pass through fireballs)
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {250, 300, 0}, Test.UnitVolume);
    Walk(&Test, Player, {1, 0}, 240);
    Check(Player->Position.X > 450.f);
    DestroyTestWorld(&Test);
}

internal void
TestRemovedSlotIsReused()
{
    test_world Test = CreateTestWorld();
    world_entity *First = AddTestEntity(&Test, EntityType_Monster,
                                        {300, 300, 0}, Test.UnitVolume);
    u32 FirstID = First->ID;
    u32 CountBefore = Test.World->EntityCount;
    RemoveEntity(Test.World, First);
    world_entity *Second = AddTestEntity(&Test, EntityType_Monster,
                                         {300, 300, 0}, Test.UnitVolume);
    Check(Second->ID == FirstID);
    Check(Test.World->EntityCount == CountBefore);
    Check(Second->IsPresent);
    DestroyTestWorld(&Test);
}

internal void
TestCopyString()
{
    char Buffer[4];
    CopyString(Buffer, sizeof(Buffer), "ab");
    Check(strcmp(Buffer, "ab") == 0);
    CopyString(Buffer, sizeof(Buffer), "abcdef");
    Check(strcmp(Buffer, "abc") == 0);
}

internal void
TestEmptyAnimationSlotDoesNotCrash()
{
    animation_state State = {};
    animation_slot Empty = {};
    v4 Uvs = DoAnimation(&State, 64, 64, 4, 4, 1.f / 60.f, 1.f, Empty);
    Check(Uvs.X == 0.f && Uvs.Y == 0.f && Uvs.Z == 0.f && Uvs.W == 0.f);
}

#define RUN(Test) printf("%s\n", #Test); Test()

int
main()
{
    // NOTE(zoubir): a failed Assert crashes, so print as we go to show
    // which test it was
    setvbuf(stdout, 0, _IONBF, 0);

    RUN(TestWallStopsUnit);
    RUN(TestUnitSlidesAlongWall);
    RUN(TestUnitsDoNotPassThroughEachOther);
    RUN(TestFireBallKillsMonsterOnce);
    RUN(TestMonsterDyingMidMoveLeavesNoGhost);
    RUN(TestRemovedSlotIsReused);
    RUN(TestCopyString);
    RUN(TestEmptyAnimationSlotDoesNotCrash);

    printf("%d of %d checks passed\n", TestChecks - TestFailures, TestChecks);
    return TestFailures ? 1 : 0;
}
