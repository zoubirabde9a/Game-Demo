/* Round rules and the map vote in the simulation alone: on a map without
   monsters a dead player stays out until one player or nobody is left
   standing (sim/round_break.cpp); on a map with monsters each respawns on
   their own; a map vote passes at more than half yes and closes when it
   cannot (sim/map_vote.cpp). The test world is the Old Arena, which has
   no monsters. Included by sim_tests.cpp, which calls RunRoundRulesTests. */

internal void
TestRoundEndsAtLastOneStanding()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    world_entity *C = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 2, {500, 300, 0});
    A->SpawnShield = B->SpawnShield = C->SpawnShield = 0.f;

    // NOTE(zoubir): two still stand: the round goes on and B stays out
    DamageEntity(AppState, Test.World, B, B->MaxHp, A);
    Check(IsDeadPlayer(B));
    Check(AppState->RoundBreak == 0.f);
    float Dt = Test.Input.DeltaTime;
    for(u32 Tick = 0; Tick < 10 * 60; Tick++)
    {
        SimulateTick(AppState, &Test.Arena, Dt);
    }
    Check(IsDeadPlayer(B));
    Check(AppState->RoundBreak == 0.f);

    // NOTE(zoubir): one left: the round is over
    DamageEntity(AppState, Test.World, C, C->MaxHp, A);
    Check(AppState->RoundBreak == ROUND_BREAK_SECONDS + FINAL_BLOW_SECONDS);
    Check(AppState->Players[1].RespawnTimer <= ROUND_BREAK_SECONDS + FINAL_BLOW_SECONDS);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

internal void
TestLeavingCanEndTheRound()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 2, {500, 300, 0});
    A->SpawnShield = B->SpawnShield = 0.f;
    DamageEntity(AppState, Test.World, B, B->MaxHp, A);
    Check(AppState->RoundBreak == 0.f);
    RemovePlayerFromSlot(AppState, Test.World, 2);
    SimulateTick(AppState, &Test.Arena, Test.Input.DeltaTime);
    Check(AppState->RoundBreak > 0.f);
    // NOTE(zoubir): nobody died to end it, so no slow motion
    Check(FinalBlowLeft(AppState) == 0.f);
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

internal void
TestMonsterMapsRespawnAlone()
{
    GameRules = DuelRules;
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    u32 MonsterMap = 0;
    while (MonsterMap < MapId_Count && GetMapDef((map_id)MonsterMap)->MonsterPopulation == 0)
    {
        MonsterMap++;
    }
    Check(MonsterMap < MapId_Count);
    AppState->World.MapId = MonsterMap;
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    A->SpawnShield = B->SpawnShield = 0.f;
    DamageEntity(AppState, Test.World, B, B->MaxHp, A);
    Check(IsDeadPlayer(B));
    Check(AppState->RoundBreak == 0.f);
    Check(AppState->Players[1].RespawnTimer <= PLAYER_RESPAWN_SECONDS);
    AppState->World.MapId = MapId_Arena;
    DestroyTestWorld(&Test);
    GameRules = ClassicRules;
}

internal void
TestMapVotePassesAtMoreThanHalf()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    for(u32 SlotIndex = 0; SlotIndex < 3; SlotIndex++)
    {
        AddPlayerToSlot(AppState, Test.World, &Test.Arena, SlotIndex,
                        {300.f + 100.f * SlotIndex, 300, 0});
    }
    u32 Target = (MapId_Arena + 1) % MapId_Count;
    float Dt = Test.Input.DeltaTime;

    // NOTE(zoubir): the current map cannot be asked for
    AppState->Players[0].Input.Vote = MapVoteAsk(MapId_Arena);
    UpdateMapVote(AppState, Dt);
    Check(!AppState->VoteOpen);

    AppState->Players[0].Input.Vote = MapVoteAsk(Target);
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteOpen && AppState->VoteMap == Target && AppState->VoteBy == 0);
    Check(AppState->VoteYes == 1);
    // NOTE(zoubir): a second ask while one is open changes nothing
    AppState->Players[1].Input.Vote = MapVoteAsk((Target + 1) % MapId_Count);
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteMap == Target && AppState->VoteBy == 0);
    AppState->Players[1].Input.Vote = MapVote_Yes;
    UpdateMapVote(AppState, Dt);
    Check(!AppState->VoteOpen);
    Check(AppState->NextMapVoted && AppState->NextMap == Target);
    Check(AppState->RoundMapDue);
    DestroyTestWorld(&Test);
}

internal void
TestMapVoteClosesWhenItCannotPass()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    for(u32 SlotIndex = 0; SlotIndex < 4; SlotIndex++)
    {
        AddPlayerToSlot(AppState, Test.World, &Test.Arena, SlotIndex,
                        {300.f + 100.f * SlotIndex, 300, 0});
    }
    u32 Target = (MapId_Arena + 1) % MapId_Count;
    float Dt = Test.Input.DeltaTime;
    AppState->Players[0].Input.Vote = MapVoteAsk(Target);
    UpdateMapVote(AppState, Dt);
    AppState->Players[1].Input.Vote = MapVote_No;
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteOpen);
    AppState->Players[2].Input.Vote = MapVote_No;
    UpdateMapVote(AppState, Dt);
    Check(!AppState->VoteOpen && !AppState->NextMapVoted);

    // NOTE(zoubir): and when nobody answers in time
    AppState->Players[0].Input.Vote = MapVoteAsk(Target);
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteOpen);
    for(u32 Tick = 0; Tick < (u32)(MAP_VOTE_SECONDS * 60.f) + 2; Tick++)
    {
        UpdateMapVote(AppState, Dt);
    }
    Check(!AppState->VoteOpen && !AppState->NextMapVoted);
    DestroyTestWorld(&Test);
}

#define ROUND_RULES_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunRoundRulesTests()
{
    ROUND_RULES_TEST(TestRoundEndsAtLastOneStanding);
    ROUND_RULES_TEST(TestLeavingCanEndTheRound);
    ROUND_RULES_TEST(TestMonsterMapsRespawnAlone);
    ROUND_RULES_TEST(TestMapVotePassesAtMoreThanHalf);
    ROUND_RULES_TEST(TestMapVoteClosesWhenItCannotPass);
}
