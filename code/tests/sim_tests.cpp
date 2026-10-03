/* Simulation tests: builds the real game code (app.cpp) into a console
   program, sets up a small world with no window or assets, and checks
   movement, collision and damage rules. Run test.bat from the repo root;
   exit code 0 means every check passed. */

#include "../app.cpp"

// Sets an environment variable for the online-session tests. "" counts as
// unset to the game, so Windows deleting it and POSIX keeping it empty agree.
#if defined(_WIN32)
#define TestSetEnv(Name, Value) _putenv_s(Name, Value)
#else
#define TestSetEnv(Name, Value) setenv(Name, Value, 1)
#endif

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

    app_state *AppState = Result.AppState;
    AppState->PlayerCollision = Result.UnitVolume;
    AppState->BatCollision = Result.UnitVolume;
    AppState->FireBallCollision = Result.FireBallVolume;
    AppState->FamiliarCollision =
        MakeSimpleGroundedCollisionVolume(&Result.Arena, {11, 6, 3.f});
    AppState->SwordCollision =
        MakeSimpleGroundedCollisionVolume(&Result.Arena, {31, 31, 31.f});
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
        MoveEntity(Entity, Test->World, &Test->Arena, Test->Input.DeltaTime,
                   Test->AppState, DDEntity, &MaxDistance);
    }
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
    FireBall->HasOwner = true;
    FireBall->OwnerSlot = 2;
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        float MaxDistance = 10000.f;
        MoveEntity(FireBall, Test.World, &Test.Arena, Test.Input.DeltaTime,
                   Test.AppState, {}, &MaxDistance);
    }
    Check(!Monster->IsPresent);
    Check(Test.AppState->Players[2].MonsterKills == 1);
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
TestShockwaveHitsOnlyNearbyMonsters()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {300, 300, 0}, Test.UnitVolume);
    world_entity *Weak = AddTestEntity(&Test, EntityType_Monster,
                                       {350, 300, 0}, Test.UnitVolume);
    Weak->MaxHp = Weak->Hp = SHOCKWAVE_DAMAGE;
    world_entity *Tough = AddTestEntity(&Test, EntityType_Monster,
                                        {300, 360, 0}, Test.UnitVolume);
    Tough->MaxHp = Tough->Hp = 100.f;
    world_entity *Far = AddTestEntity(&Test, EntityType_Monster,
                                      {300 + SHOCKWAVE_RADIUS + 20.f, 300, 0},
                                      Test.UnitVolume);
    Far->MaxHp = Far->Hp = 100.f;

    u32 Hits = TriggerShockwave(Test.AppState, Test.World, Player);
    Check(Hits == 2);
    Check(!Weak->IsPresent);
    Check(Test.AppState->Players[Player->PlayerIndex].MonsterKills == 1);
    Check(Tough->Hp == 100.f - SHOCKWAVE_DAMAGE);
    // NOTE(zoubir): thrown away from the player, which is above it
    Check(Tough->Velocity.Y > 0.f);
    Check(Far->Hp == 100.f);
    Check(Player->ShockwaveFlash > 0.f);
    DestroyTestWorld(&Test);
}

internal void
TestMonsterPopulationRefillsAwayFromPlayers()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AppState->BatCollision = Test.UnitVolume;
    world_entity *Player = AddTestEntity(&Test, EntityType_Player,
                                         {1000, 1000, 0}, Test.UnitVolume);
    // NOTE(zoubir): scatter walls so some random spots are blocked
    for(u32 WallIndex = 0; WallIndex < 40; WallIndex++)
    {
        AddTestEntity(&Test, EntityType_StaticObject,
                      {100.f + 48.f * WallIndex, 200.f + 37.f * (WallIndex % 7), 0},
                      Test.WallVolume);
    }

    monster_population *Population =
        CreateMonsterPopulation(&Test.Arena, 6, 7);
    AppState->Monsters = Population;
    FillMonsterPopulation(AppState, Test.World, &Test.Arena, Population);
    Check(CountLiveMonsters(Test.World) == 6);

    // NOTE(zoubir): kill two, they come back one per respawn delay. Kinds
    // that split on death are skipped, their children would add to the count
    u32 Killed = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < Test.World->EntityCount && Killed < 2;
        EntityIndex++)
    {
        world_entity *Entity = &Test.World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            GetMonsterDef(Entity->MonsterKind)->DeathEffect == DeathEffect_None)
        {
            DamageEntity(AppState, Test.World, Entity, 1000.f, 0);
            Killed++;
        }
    }
    Check(CountLiveMonsters(Test.World) == 4);

    float DeltaTime = 1.f / 60.f;
    u32 RespawnFrames = (u32)(MONSTER_RESPAWN_SECONDS * 60.f) + 2;
    for(u32 Frame = 0; Frame < RespawnFrames; Frame++)
    {
        UpdateMonsterPopulation(AppState, Test.World, &Test.Arena,
                                Population, DeltaTime);
    }
    Check(CountLiveMonsters(Test.World) == 5);
    for(u32 Frame = 0; Frame < 10 * RespawnFrames; Frame++)
    {
        UpdateMonsterPopulation(AppState, Test.World, &Test.Arena,
                                Population, DeltaTime);
    }
    Check(CountLiveMonsters(Test.World) == 6);

    // NOTE(zoubir): nobody starts inside a wall, a unit, or next to a player
    for(u32 EntityIndex = 0;
        EntityIndex < Test.World->EntityCount;
        EntityIndex++)
    {
        world_entity *Monster = &Test.World->Entities[EntityIndex];
        if (Monster->IsPresent && Monster->Type == EntityType_Monster)
        {
            Check(Length(Monster->Position.XY - Player->Position.XY) >=
                  MONSTER_SPAWN_MIN_PLAYER_DISTANCE);
            for(u32 OtherIndex = 0;
                OtherIndex < Test.World->EntityCount;
                OtherIndex++)
            {
                world_entity *Other = &Test.World->Entities[OtherIndex];
                if (Other != Monster && Other->IsPresent &&
                    CanCollide(AppState, Monster->Type, Other->Type))
                {
                    Check(!EntityOverlap(Monster, Other));
                }
            }
        }
    }
    DestroyTestWorld(&Test);
}

internal void
TestIdleMonsterWanders()
{
    test_world Test = CreateTestWorld();
    Test.AppState->Monsters = CreateMonsterPopulation(&Test.Arena, 0, 3);
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {1000, 1000, 0}, Test.UnitVolume);
    Monster->MonsterKind = MonsterKind_Brute;
    v3 Start = Monster->Position;
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    // NOTE(zoubir): no player at all, ten seconds is enough to pick a
    // walking direction at least once
    for(u32 Frame = 0; Frame < 600; Frame++)
    {
        UpdateMonster(Monster, Test.World, &Test.Arena, Test.Input.DeltaTime,
                      Test.AppState, &AnimationSpeed, &AnimationType,
                      &AnimationDirection);
    }
    Check(Length(Monster->Position.XY - Start.XY) > 10.f);
    DestroyTestWorld(&Test);
}

internal void
TestEachSlotFollowsItsOwnInput()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    world_entity *Still = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                          0, {300, 300, 0});
    world_entity *Runner = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 600, 0});
    AppState->Players[1].Input.Move = V2(1.f, 0.f);

    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    for(u32 Frame = 0; Frame < 60; Frame++)
    {
        for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
        {
            UpdatePlayer(&AppState->Players[SlotIndex], Test.World,
                         &Test.Arena, Test.Input.DeltaTime, AppState,
                         &AnimationSpeed, &AnimationType,
                         &AnimationDirection);
        }
    }
    Check(Still->Position.X == 300.f);
    Check(Runner->Position.X > 350.f);
    Check(Runner->Position.Y == 600.f);
    Check(GetPlayerSlot(AppState, Runner) == &AppState->Players[1]);
    DestroyTestWorld(&Test);
}

internal void
TestMonsterChasesNearestPlayer()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->PlayerCollision = Test.UnitVolume;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 1000, 0});
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {1200, 1000, 0});
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {1000, 1000, 0}, Test.UnitVolume);
    Monster->MonsterKind = MonsterKind_Brute;

    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    for(u32 Frame = 0; Frame < 30; Frame++)
    {
        UpdateMonster(Monster, Test.World, &Test.Arena, Test.Input.DeltaTime,
                      AppState, &AnimationSpeed, &AnimationType,
                      &AnimationDirection);
    }
    // NOTE(zoubir): slot 1 is 200 away, slot 0 is 700 away (out of range)
    Check(Monster->Position.X > 1010.f);
    DestroyTestWorld(&Test);
}

internal void
TestSwordHitsOtherPlayerNotOwner()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Victim = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {330, 300, 0});
    Victim->MaxHp = Victim->Hp = SWORD_DAMAGE;
    world_entity *Sword = AddSword(AppState, Test.World, &Test.Arena,
                                   {316, 300, 0}, Attacker,
                                   AnimationDirection_Right);
    // NOTE(zoubir): the swing lasts several frames but hits once
    while (UpdateSword(Sword, Test.World, &Test.Arena, AppState,
                       Test.Input.DeltaTime))
    {
    }
    Check(Attacker->Hp == Attacker->MaxHp);
    Check(Victim->Hp <= 0.f);
    Check(AppState->Players[0].Kills == 1);

    Check(AppState->Players[1].Deaths == 1);
    for(u32 Frame = 0; Frame < (u32)(PLAYER_RESPAWN_SECONDS * 60.f) + 2; Frame++)
    {
        UpdateDeadPlayer(&AppState->Players[1], Test.World, &Test.Arena,
                         AppState, 1.f / 60.f);
    }
    Check(Victim->Hp == Victim->MaxHp);
    Check(Victim->Position.X == 330.f);
    DestroyTestWorld(&Test);
}

internal void
TestSwordHitsMonster()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Attacker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                             0, {300, 300, 0});
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {330, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    world_entity *Sword = AddSword(AppState, Test.World, &Test.Arena,
                                   {316, 300, 0}, Attacker,
                                   AnimationDirection_Right);
    while (UpdateSword(Sword, Test.World, &Test.Arena, AppState,
                       Test.Input.DeltaTime))
    {
    }
    Check(Monster->Hp == 100.f - SWORD_DAMAGE);
    DestroyTestWorld(&Test);
}

internal void
TestFireBallHitsOtherPlayerNotOwner()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Caster = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Victim = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {420, 300, 0});
    // NOTE(zoubir): cast from inside the caster to prove it ignores them
    world_entity *FireBall = AddFireBall(AppState, Test.World, &Test.Arena,
                                         Caster, {300, 300, 0},
                                         {450, 0, 0});
    for(u32 Frame = 0; Frame < 40; Frame++)
    {
        float MaxDistance = 10000.f;
        MoveEntity(FireBall, Test.World, &Test.Arena, Test.Input.DeltaTime,
                   AppState, {}, &MaxDistance);
    }
    Check(Caster->Hp == Caster->MaxHp);
    Check(Victim->Hp == Victim->MaxHp - FIREBALL_DAMAGE);
    DestroyTestWorld(&Test);
}

internal void
TestShockwaveHitsOtherPlayersNotSource()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Source = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    world_entity *Other = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                          1, {350, 300, 0});
    u32 Hits = TriggerShockwave(AppState, Test.World, Source);
    Check(Hits == 1);
    Check(Source->Hp == Source->MaxHp);
    Check(Other->Hp == Other->MaxHp - SHOCKWAVE_DAMAGE);
    Check(Other->Velocity.X > 0.f);
    DestroyTestWorld(&Test);
}

internal void
TestMonsterBiteCreditsNobody()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    Player->Hp = 1.f;
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {340, 300, 0}, Test.UnitVolume);
    Monster->MonsterKind = MonsterKind_Brute;
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    UpdateMonster(Monster, Test.World, &Test.Arena, Test.Input.DeltaTime, AppState,
                  &AnimationSpeed, &AnimationType, &AnimationDirection);
    Check(Player->Hp <= 0.f);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Check(AppState->Players[SlotIndex].Kills == 0);
    }
    DestroyTestWorld(&Test);
}

internal void
TestDeadPlayerIsInertUntilRespawn()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Dead = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                         0, {400, 300, 0});
    world_entity *Walker = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           1, {300, 300, 0});
    DamageEntity(AppState, Test.World, Dead, 1000.f, Walker);
    Check(IsDeadPlayer(Dead));
    Check(AppState->Players[0].Deaths == 1);
    Check(AppState->Players[1].Kills == 1);

    // NOTE(zoubir): monsters ignore the body
    float Distance;
    Check(FindNearestPlayer(AppState, V2(410.f, 300.f), &Distance) == Walker);
    // NOTE(zoubir): the body neither blocks nor takes more hits
    Walk(&Test, Walker, {1, 0}, 120);
    Check(Walker->Position.X > 430.f);
    Check(TriggerShockwave(AppState, Test.World, Walker) == 0);
    Check(AppState->Players[0].Deaths == 1);

    float DeltaTime = 1.f / 60.f;
    u32 Frames = (u32)(PLAYER_RESPAWN_SECONDS * 60.f) - 10;
    for(u32 Frame = 0; Frame < Frames; Frame++)
    {
        Check(UpdateDeadPlayer(&AppState->Players[0], Test.World,
                               &Test.Arena, AppState, DeltaTime));
    }
    for(u32 Frame = 0; Frame < 20; Frame++)
    {
        UpdateDeadPlayer(&AppState->Players[0], Test.World, &Test.Arena,
                         AppState, DeltaTime);
    }
    Check(!IsDeadPlayer(Dead));
    Check(Dead->Hp == Dead->MaxHp);
    Check(Dead->Position.X == 400.f);
    DestroyTestWorld(&Test);
}

internal void
TestScoreboardRanksByKillsThenDeaths()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    u32 Kills[] = {1, 3, 3, 0};
    u32 Deaths[] = {0, 2, 1, 0};
    for(u32 SlotIndex = 0; SlotIndex < 4; SlotIndex++)
    {
        AppState->Players[SlotIndex].Active = true;
        AppState->Players[SlotIndex].Kills = Kills[SlotIndex];
        AppState->Players[SlotIndex].Deaths = Deaths[SlotIndex];
    }
    u32 Order[MAX_PLAYERS];
    Check(RankPlayers(AppState, Order) == 4);
    Check(Order[0] == 2 && Order[1] == 1 && Order[2] == 0 && Order[3] == 3);
    free(AppState);
}

// NOTE(zoubir): a chunk's entity list is a first block plus full blocks.
// Emptying the first block, then removing from the third, used to copy
// the third block over the first and silently drop the second.
internal void
TestCrowdedChunkRemovalKeepsEveryone()
{
    test_world Test = CreateTestWorld();
    world *World = Test.World;
    world_chunk *Chunk = 0;
    world_entity *Units[40];
    for(u32 Index = 0; Index < 40; Index++)
    {
        Units[Index] = AddTestEntity(&Test, EntityType_StaticObject,
                                     {100.f + Index, 100.f, 0},
                                     Test.FireBallVolume);
    }
    Chunk = FindChunk(World, 0, 0, 0);
    Check(Chunk != 0);
    // NOTE(zoubir): 40 = 8 in the first block + two full blocks of 16
    Check(Chunk->FirstEntityChunk.EntityCount == 8);
    for(u32 Index = 32; Index < 40; Index++)
    {
        Check(RemoveEntity(World, Chunk, Units[Index]));
    }
    Check(Chunk->FirstEntityChunk.EntityCount == 0);
    // NOTE(zoubir): Units[0] went in first, so it sits in the last block
    Check(RemoveEntity(World, Chunk, Units[0]));
    for(u32 Index = 1; Index < 32; Index++)
    {
        Check(RemoveEntity(World, Chunk, Units[Index]));
    }
    Check(Chunk->FirstEntityChunk.EntityCount == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the full simulation under random play. Seed 5 used to
// crash after about 8000 ticks on the chunk bug above.
internal void
TestRandomPlaySoak()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)),
                    Megabytes(1));
    InitSimulation(AppState, &Arena, &Constants);
    for(u32 Slot = 0; Slot < 4; Slot++)
    {
        AddPlayerToSlot(AppState, &AppState->World, &Arena, Slot,
                        PlayerSpawnPosition(&AppState->World, Slot));
    }
    random_series Series = Seed(5);
    u32 Ticks = 60 * 180;
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        for(u32 Slot = 0; Slot < 4; Slot++)
        {
            player_input *Input = &AppState->Players[Slot].Input;
            if (RandomChoice(&Series, 20) == 0)
            {
                Input->Move.X = (float)RandomChoice(&Series, 3) - 1.f;
                Input->Move.Y = (float)RandomChoice(&Series, 3) - 1.f;
            }
            Input->Pressed = RandomChoice(&Series, 10) == 0 ?
                (1u << RandomChoice(&Series, 5)) : 0;
        }
        SimulateTick(AppState, &Arena,
                     RandomBetween(&Series, 0.005f, 0.05f));
        AppState->Events.Count = 0;
    }
    Check(CountLiveMonsters(&AppState->World) > 0);
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
}

// NOTE(zoubir): switching maps (the offline map picker, joining a server
// on another map) empties the world arena first, so going round every map
// twice, playing each, ends using what one visit to the map used
internal void
TestSwitchingMapsReusesWorldMemory()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(16);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)),
                    Megabytes(1));
    InitSimulation(AppState, &Arena, &Constants);
    memory_index FirstVisit[MapId_Count] = {};
    for(u32 Round = 0; Round < 2; Round++)
    {
        for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
        {
            RebuildWorldForMap(AppState, &Arena, MapIndex);
            FillMonsterPopulation(AppState, &AppState->World, &Arena,
                                  AppState->Monsters);
            world_entity *Player =
                AddPlayerToSlot(AppState, &AppState->World, &Arena, 0,
                                PlayerSpawnPosition(&AppState->World, 0));
            AddFamiliar(AppState, &AppState->World, &Arena, Player);
            for(u32 Tick = 0; Tick < 120; Tick++)
            {
                SimulateTick(AppState, &Arena, 1.f / 60.f);
                AppState->Events.Count = 0;
            }
            Check(AppState->World.MapId == MapIndex);
            Check(AppState->Players[0].Entity != 0);
            if (Round == 0)
            {
                FirstVisit[MapIndex] = Arena.Used;
            }
            else
            {
                Check(Arena.Used <= FirstVisit[MapIndex] + Kilobytes(64));
            }
        }
    }
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
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
    animation_set Set = {};
    animation_state State = {};
    AdvanceAnimation(&State, &Set, AnimationType_Move, AnimationDirection_Up,
                     1.f / 60.f, 1.f);
    v4 Uvs = GetAnimationUvs(&State, &Set, 64, 64, 4, 4);
    Check(Uvs.X == 0.f && Uvs.Y == 0.f && Uvs.Z == 0.f && Uvs.W == 0.f);
}

internal void
TestAnimationAdvancesWithoutTexture()
{
    test_world Test = CreateTestWorld();
    animation_set Set = {};
    AddAnimation(&Set, &Test.Arena, AnimationType_Attack,
                 AnimationDirection_Right, 10, 3, 0.1f);
    animation_state State = {};
    // NOTE(zoubir): 0.25 s at 0.1 s per frame is two frames in, with
    // margin either side so float rounding cannot change the answer
    for(u32 Frame = 0; Frame < 15; Frame++)
    {
        AdvanceAnimation(&State, &Set, AnimationType_Attack,
                         AnimationDirection_Right, 1.f / 60.f, 1.f);
    }
    Check(State.SlotIndex == 2);
    Check(!IsAnimationFinished(&Set, &State, AnimationType_Attack,
                               AnimationDirection_Right));
    // NOTE(zoubir): frame 10 + 2 = 12 on a 4x4 sheet is column 0, row 3
    v4 Uvs = GetAnimationUvs(&State, &Set, 64, 64, 4, 4);
    Check(Uvs.X == 0.f && Uvs.Y == 0.75f);
    for(u32 Frame = 0; Frame < 12; Frame++)
    {
        AdvanceAnimation(&State, &Set, AnimationType_Attack,
                         AnimationDirection_Right, 1.f / 60.f, 1.f);
    }
    Check(IsAnimationFinished(&Set, &State, AnimationType_Attack,
                              AnimationDirection_Right));
    DestroyTestWorld(&Test);
}

internal void
TestSimulateTickQueuesSoundsInsteadOfPlaying()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena,
                                           0, {300, 300, 0});
    AppState->Players[0].Input.Pressed = PlayerButton_Jump;
    SimulateTick(AppState, &Test.Arena, 1.f / 60.f);
    Check(AppState->Events.Count == 1);
    Check(AppState->Events.Events[0].Sound == AssetType_ZoubirAudio);
    Check(Player->Position.Z > 0.f);
    DestroyTestWorld(&Test);
}

#include "monster_tests.cpp"
#include "terrain_tests.cpp"
#include "collision_tests.cpp"
#include "online_tests.cpp"
#include "player_ability_tests.cpp"

#define RUN(Test) printf("%s\n", #Test); Test()

int
main()
{
    // NOTE(zoubir): a failed Assert crashes, so print as we go to show
    // which test it was
    setvbuf(stdout, 0, _IONBF, 0);

    RUN(TestFireBallKillsMonsterOnce);
    RUN(TestMonsterDyingMidMoveLeavesNoGhost);
    RUN(TestRemovedSlotIsReused);
    RUN(TestShockwaveHitsOnlyNearbyMonsters);
    RUN(TestMonsterPopulationRefillsAwayFromPlayers);
    RUN(TestIdleMonsterWanders);
    RUN(TestEachSlotFollowsItsOwnInput);
    RUN(TestMonsterChasesNearestPlayer);
    RUN(TestSwordHitsOtherPlayerNotOwner);
    RUN(TestSwordHitsMonster);
    RUN(TestFireBallHitsOtherPlayerNotOwner);
    RUN(TestShockwaveHitsOtherPlayersNotSource);
    RUN(TestMonsterBiteCreditsNobody);
    RUN(TestDeadPlayerIsInertUntilRespawn);
    RUN(TestScoreboardRanksByKillsThenDeaths);
    RUN(TestCrowdedChunkRemovalKeepsEveryone);
    RUN(TestRandomPlaySoak);
    RUN(TestCopyString);
    RUN(TestEmptyAnimationSlotDoesNotCrash);
    RUN(TestAnimationAdvancesWithoutTexture);
    RUN(TestSimulateTickQueuesSoundsInsteadOfPlaying);
    RUN(TestSwitchingMapsReusesWorldMemory);

    RunMonsterTests();
    RunTerrainTests();
    RunCollisionTests();
    RunOnlineTests();
    RunPlayerAbilityTests();

    printf("%d of %d checks passed\n", TestChecks - TestFailures, TestChecks);
    return TestFailures ? 1 : 0;
}
