/* Team duels online (sim/teams/, net/protocol/teams.h), with a real
   server on the Old Arena, three bots and a real client: the client asks
   for a team duel through the vote bits, sees every player's team and
   which are bots as the server has them, then switches sides through the
   role byte, a bot crossing over to keep the teams even. Included by
   server_tests.cpp after round_map_tests.cpp, whose harness it borrows. */

// NOTE(zoubir): like RunRoundMapTest, with the role byte that carries a
// team request, as app.cpp sends it
internal void
RunTeamOnlineTest(round_map_test *Test, int Frames)
{
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < Frames; ++Frame)
    {
        UpdateOnlineSession(Test->Online, &Input, false, {},
                            OnlineVoteBits(Test->Client, Input.DeltaTime), 0,
                            OnlineRoleRequest(Test->Client, Input.DeltaTime));
        RunWorldTick(Test->Client, &Test->Arena, Input.DeltaTime);
        ServerTick(&Test->Server);
    }
}

internal void
TestTeamDuelOnline()
{
    static round_map_test Test;
    StartRoundMapTest(&Test, "Captain");
    Test.Client->TeamUi = (team_ui *)calloc(1, sizeof(team_ui));
    Test.Server.Game.BotTarget = 3;
    RunTeamOnlineTest(&Test, 2 * SERVER_TICK_RATE);
    app_state *Game = Test.Server.Game.AppState;
    app_state *Client = Test.Client;
    u32 SlotIndex = Test.Online->Client.PlayerIndex;
    Check(!Client->TeamDuel);

    // NOTE(zoubir): bots answer yes, so the vote passes
    RequestVote(Client, MapVote_Teams);
    RunTeamOnlineTest(&Test, 3 * SERVER_TICK_RATE);
    Check(IsTeamDuel(Game) && IsTeamDuel(Client));
    Check(CountTeam(Game, Team_Red) == 2 && CountTeam(Game, Team_Blue) == 2);
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        Check(PlayerTeam(Client, Slot) == PlayerTeam(Game, Slot));
        Check(Client->Players[Slot].Bot == (Game->Players[Slot].Active && Game->Players[Slot].Bot));
    }
    Check(!Client->Players[SlotIndex].Bot);

    // NOTE(zoubir): to the other side; teams are even, so a bot swaps
    u32 Mine = PlayerTeam(Game, SlotIndex);
    RequestTeam(Client, OtherTeam(Mine));
    RunTeamOnlineTest(&Test, SERVER_TICK_RATE);
    Check(PlayerTeam(Game, SlotIndex) == OtherTeam(Mine));
    Check(LocalTeam(Client) == OtherTeam(Mine));
    Check(CountTeam(Game, Team_Red) == 2 && CountTeam(Game, Team_Blue) == 2);
    free(Test.Client->TeamUi);
    StopRoundMapTest(&Test);
}
