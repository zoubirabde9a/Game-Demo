/* Team duels in the simulation alone (sim/teams/): the vote that turns
   them on and off, players placed on the smaller team, teammates who
   cannot hurt each other, switches that keep the teams even (a bot making
   way for a human), a switch mid-round costing the round, bots evening
   the teams when players leave, and a round on the Old Arena ending when
   one team is left. Included by sim_tests.cpp, which calls RunTeamTests. */

// NOTE(zoubir): the real Old Arena, as the game builds it, with Count
// players in slots 0 up
struct team_test_world
{
    app_state *AppState;
    memory_arena Arena;
    memory_arena Constants;
};

internal team_test_world
CreateTeamTestWorld(u32 Count, bool32 TeamDuel)
{
    team_test_world Result = {};
    Result.AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(16);
    InitializeArena(&Result.Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Result.Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Result.AppState, &Result.Arena, &Result.Constants);
    RebuildWorldForMap(Result.AppState, &Result.Arena, MapId_Arena);
    Result.AppState->TeamDuel = TeamDuel;
    for(u32 SlotIndex = 0; SlotIndex < Count; SlotIndex++)
    {
        world_entity *Player = AddPlayerToSlot(Result.AppState, &Result.AppState->World,
                                               &Result.Arena, SlotIndex,
                                               PlayerSpawnPosition(&Result.AppState->World,
                                                                   SlotIndex));
        Player->SpawnShield = 0.f;
    }
    UpdateTeams(Result.AppState, &Result.Arena);
    return Result;
}

internal void
DestroyTeamTestWorld(team_test_world *Test)
{
    free(Test->Arena.Base);
    free(Test->Constants.Base);
    free(Test->AppState);
}

internal void
TestTeamDuelVoteTurnsTeamsOnAndOff()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(2, false);
    app_state *AppState = Test.AppState;
    float Dt = 1.f / 60.f;
    Check(!IsTeamDuel(AppState) && AppState->Players[0].Team == Team_None);
    AppState->Players[0].Kills = 3;

    // NOTE(zoubir): asked for on the map being played, which stays
    AppState->Players[0].Input.Vote = MapVote_Teams;
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteOpen && AppState->VoteTeams && AppState->VoteMap == MapId_Arena);
    AppState->Players[1].Input.Vote = MapVote_Yes;
    SimulateTick(AppState, &Test.Arena, Dt);
    SimulateTick(AppState, &Test.Arena, Dt);
    Check(IsTeamDuel(AppState) && AppState->World.MapId == MapId_Arena);
    u32 Red = PlayerTeam(AppState, 0);
    u32 Blue = PlayerTeam(AppState, 1);
    Check(Red != Team_None && Blue != Team_None && Red != Blue);
    // NOTE(zoubir): a new match: the free-for-all's kills are gone
    Check(AppState->Players[0].Kills == 0);
    // NOTE(zoubir): each on its own half of the map
    float RedX = AppState->Players[Red == Team_Red ? 0 : 1].Entity->Position.X;
    float BlueX = AppState->Players[Red == Team_Red ? 1 : 0].Entity->Position.X;
    Check(RedX < BlueX);

    // NOTE(zoubir): another duel map keeps the teams; MapVote_Teams again
    // goes back to free for all
    AppState->Players[0].Input.Vote = MapVote_Teams;
    UpdateMapVote(AppState, Dt);
    Check(AppState->VoteOpen && !AppState->VoteTeams);
    AppState->Players[1].Input.Vote = MapVote_Yes;
    SimulateTick(AppState, &Test.Arena, Dt);
    SimulateTick(AppState, &Test.Arena, Dt);
    Check(!IsTeamDuel(AppState));
    Check(AppState->Players[0].Team == Team_None && AppState->Players[1].Team == Team_None);
    DestroyTeamTestWorld(&Test);
}

internal void
TestTeammatesCannotHurtEachOther()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(4, true);
    app_state *AppState = Test.AppState;
    world *World = &AppState->World;
    Check(CountTeam(AppState, Team_Red) == 2 && CountTeam(AppState, Team_Blue) == 2);
    u32 Mate = MAX_PLAYERS;
    u32 Foe = MAX_PLAYERS;
    for(u32 SlotIndex = 1; SlotIndex < 4; SlotIndex++)
    {
        if (AreTeammates(AppState, 0, SlotIndex)) Mate = SlotIndex;
        else Foe = SlotIndex;
    }
    Check(Mate < MAX_PLAYERS && Foe < MAX_PLAYERS);
    world_entity *Caster = AppState->Players[0].Entity;
    world_entity *Friend = AppState->Players[Mate].Entity;
    world_entity *Enemy = AppState->Players[Foe].Entity;

    world_entity *FireBall = AddFireBall(AppState, World, &Test.Arena, Caster,
                                         Friend->Position + V3(-30.f, 0.f, 30.f),
                                         V3(600.f, 0.f, 0.f));
    FireBallHit(AppState, World, FireBall, Friend);
    Check(Friend->Hp == Friend->MaxHp);
    DamageEntity(AppState, World, Friend, Friend->MaxHp, Caster);
    Check(!IsDeadPlayer(Friend));

    FireBall = AddFireBall(AppState, World, &Test.Arena, Caster,
                           Enemy->Position + V3(-30.f, 0.f, 30.f), V3(600.f, 0.f, 0.f));
    FireBallHit(AppState, World, FireBall, Enemy);
    Check(Enemy->Hp < Enemy->MaxHp);
    DestroyTeamTestWorld(&Test);
}

internal void
TestTeamSwitchKeepsTeamsEven()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(3, true);
    app_state *AppState = Test.AppState;
    // NOTE(zoubir): slot order, each to the smaller team
    Check(PlayerTeam(AppState, 0) == Team_Red && PlayerTeam(AppState, 1) == Team_Blue &&
          PlayerTeam(AppState, 2) == Team_Red);

    // NOTE(zoubir): Blue's only player cannot leave it empty against three
    u32 MakesWay;
    Check(CanJoinTeam(AppState, 1, Team_Red, &MakesWay) == TeamRefusal_Uneven);
    AppState->Players[1].Input.Team = Team_Red;
    UpdateTeams(AppState, &Test.Arena);
    Check(PlayerTeam(AppState, 1) == Team_Blue);

    // NOTE(zoubir): a Red player may cross; mid-round it sits out, and
    // nobody's score moves
    AppState->Players[0].Input.Team = Team_Blue;
    UpdateTeams(AppState, &Test.Arena);
    Check(PlayerTeam(AppState, 0) == Team_Blue);
    Check(IsDeadPlayer(AppState->Players[0].Entity));
    Check(AppState->Players[0].Deaths == 0 && AppState->Players[1].Kills == 0);
    Check(AppState->RoundBreak == 0.f);
    DestroyTeamTestWorld(&Test);
}

internal void
TestBotMakesWayForHuman()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(4, true);
    app_state *AppState = Test.AppState;
    AppState->Players[1].Bot = AppState->Players[2].Bot = true;
    // NOTE(zoubir): Red 0 and 2 (a bot), Blue 1 (a bot) and 3
    Check(PlayerTeam(AppState, 1) == Team_Blue && PlayerTeam(AppState, 3) == Team_Blue);
    u32 MakesWay;
    Check(CanJoinTeam(AppState, 0, Team_Blue, &MakesWay) == TeamRefusal_None && MakesWay == 1);
    // NOTE(zoubir): a bot gets no such favour
    Check(CanJoinTeam(AppState, 2, Team_Blue, &MakesWay) == TeamRefusal_Uneven);
    AppState->Players[0].Input.Team = Team_Blue;
    UpdateTeams(AppState, &Test.Arena);
    Check(PlayerTeam(AppState, 0) == Team_Blue && PlayerTeam(AppState, 1) == Team_Red);
    Check(CountTeam(AppState, Team_Red) == 2 && CountTeam(AppState, Team_Blue) == 2);
    DestroyTeamTestWorld(&Test);
}

internal void
TestBotsEvenTeamsWhenPlayersLeave()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(4, true);
    app_state *AppState = Test.AppState;
    AppState->Players[2].Bot = AppState->Players[3].Bot = true;
    // NOTE(zoubir): Red 0 and 2 (a bot), Blue 1 and 3 (a bot); Blue's go
    RemovePlayerFromSlot(AppState, &AppState->World, 1);
    RemovePlayerFromSlot(AppState, &AppState->World, 3);
    UpdateTeams(AppState, &Test.Arena);
    Check(PlayerTeam(AppState, 0) == Team_Red && PlayerTeam(AppState, 2) == Team_Blue);
    // NOTE(zoubir): a newcomer goes to the smaller team, here the one with
    // fewer kills
    AppState->Players[0].Kills = 2;
    AddPlayerToSlot(AppState, &AppState->World, &Test.Arena, 1,
                    PlayerSpawnPosition(&AppState->World, 1));
    UpdateTeams(AppState, &Test.Arena);
    Check(PlayerTeam(AppState, 1) == Team_Blue);
    DestroyTeamTestWorld(&Test);
}

internal void
TestTeamRoundEndsWhenOneTeamStands()
{
    GameRules = DuelRules;
    team_test_world Test = CreateTeamTestWorld(4, true);
    app_state *AppState = Test.AppState;
    world *World = &AppState->World;
    u32 Reds[2];
    u32 Blue = MAX_PLAYERS;
    u32 RedCount = 0;
    for(u32 SlotIndex = 0; SlotIndex < 4; SlotIndex++)
    {
        if (PlayerTeam(AppState, SlotIndex) == Team_Red) Reds[RedCount++] = SlotIndex;
        else Blue = SlotIndex;
    }
    Check(RedCount == 2 && Blue < MAX_PLAYERS);
    world_entity *Killer = AppState->Players[Blue].Entity;
    DamageEntity(AppState, World, AppState->Players[Reds[0]].Entity, 1000.f, Killer);
    Check(AppState->RoundBreak == 0.f);
    DamageEntity(AppState, World, AppState->Players[Reds[1]].Entity, 1000.f, Killer);
    Check(AppState->RoundBreak > 0.f);
    Check(AppState->Players[Blue].Kills == 2);
    DestroyTeamTestWorld(&Test);
}

internal void
RunTeamTests()
{
    RUN(TestTeamDuelVoteTurnsTeamsOnAndOff);
    RUN(TestTeammatesCannotHurtEachOther);
    RUN(TestTeamSwitchKeepsTeamsEven);
    RUN(TestBotMakesWayForHuman);
    RUN(TestBotsEvenTeamsWhenPlayersLeave);
    RUN(TestTeamRoundEndsWhenOneTeamStands);
    GameRules = ClassicRules;
}
