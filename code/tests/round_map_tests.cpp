/* Round maps and the map vote, with a real server and client: when the
   break between rounds runs out the server plays the next round on the
   same map, everyone keeping their level (sim/setup.cpp
   StartNextRoundMap); a map vote that passes moves everyone to the map
   asked for and starts them over at level 1 (sim/map_vote.cpp), and a
   client playing there builds the new map and follows (client/online.cpp).
   Included by server_tests.cpp, which calls TestRoundMovesToNextMap. */

struct round_map_test
{
    server Server;
    app_state *Client;
    memory_arena Arena, Constants;
    online_session *Online;
};

internal void
StartRoundMapTest(round_map_test *Test, char *Name)
{
    Check(ServerStart(&Test->Server, 0, MapId_Arena));
    Test->Client = (app_state *)calloc(1, sizeof(app_state));
    app_state *Client = Test->Client;
    memory_index Size = Megabytes(48);
    InitializeArena(&Test->Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Test->Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Test->Arena, &Test->Constants);
    AddPlayerToSlot(Client, &Client->World, &Test->Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Test->Arena);
    Test->Online = Client->Online;
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Test->Server.Socket));
    Check(OnlineConnect(Test->Online, Address, Name));
}

// NOTE(zoubir): Frames client frames and server ticks, the client sending
// its vote requests as app.cpp does
internal void
RunRoundMapTest(round_map_test *Test, int Frames)
{
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < Frames; ++Frame)
    {
        UpdateOnlineSession(Test->Online, &Input, false, {},
                            OnlineVoteBits(Test->Client, Input.DeltaTime));
        RunWorldTick(Test->Client, &Test->Arena, Input.DeltaTime);
        ServerTick(&Test->Server);
    }
}

internal void
StopRoundMapTest(round_map_test *Test)
{
    NetClientDisconnect(&Test->Online->Client);
    ServerStop(&Test->Server);
    free(Test->Client->RewindFx);
    free(Test->Arena.Base);
    free(Test->Constants.Base);
    free(Test->Client);
}

// NOTE(zoubir): the player stands at its spawn on the server's map with
// full health, and the client built the same map with its player there
internal void
CheckPlayerAtSpawn(round_map_test *Test, u32 MapId)
{
    app_state *Game = Test->Server.Game.AppState;
    u32 SlotIndex = Test->Online->Client.PlayerIndex;
    player_slot *Slot = &Game->Players[SlotIndex];
    Check(Game->World.MapId == MapId);
    Check(Test->Server.Clients.MapId == MapId);
    Check(Slot->Active && Slot->Entity && Slot->Entity->IsPresent);
    if (!Slot->Entity)
    {
        return;
    }
    Check(Slot->Entity->Hp == Slot->Entity->MaxHp);
    v3 Spawn = PlayerSpawnPosition(&Game->World, SlotIndex);
    Check(Length(Slot->Entity->Position.XY - Spawn.XY) < 64.f);
    Check(Test->Client->World.MapId == MapId);
    Check(Test->Online->Replicas.Active);
    world_entity *Own = GetLocalPlayer(Test->Client);
    Check(Own && Own->IsPresent);
    if (Own)
    {
        Check(Length(Own->Position.XY - Slot->Entity->Position.XY) < 16.f);
    }
}

internal void
TestRoundReplaysTheMap()
{
    static round_map_test Test;
    StartRoundMapTest(&Test, "Rounder");
    RunRoundMapTest(&Test, 3 * SERVER_TICK_RATE);
    Check(Test.Online->Replicas.Active);
    Check(Test.Client->World.MapId == MapId_Arena);

    // NOTE(zoubir): a round ends with the player a level up; the break's
    // last moments run out on the server
    app_state *Game = Test.Server.Game.AppState;
    player_slot *Slot = &Game->Players[Test.Online->Client.PlayerIndex];
    Check(Slot->Active);
    Slot->Level = 3;
    Slot->Kills = 2;
    Game->RoundBreak = 0.05f;
    RunRoundMapTest(&Test, SERVER_TICK_RATE);

    // NOTE(zoubir): the same map again; the player kept its slot, name,
    // level and score
    CheckPlayerAtSpawn(&Test, MapId_Arena);
    Check(strcmp(Slot->Name, "Rounder") == 0);
    Check(Slot->Level == 3 && Slot->Kills == 2);
    StopRoundMapTest(&Test);
}

internal void
TestMapVoteMovesAndStartsOver()
{
    static round_map_test Test;
    StartRoundMapTest(&Test, "Voter");
    RunRoundMapTest(&Test, 3 * SERVER_TICK_RATE);
    Check(Test.Online->Replicas.Active);

    app_state *Game = Test.Server.Game.AppState;
    player_slot *Slot = &Game->Players[Test.Online->Client.PlayerIndex];
    Slot->Level = 4;
    Slot->Xp = XpToReach(4);
    Slot->Kills = 3;
    Slot->Ranks[0] = 1;
    // NOTE(zoubir): a bot joins, and answers yes for itself
    Test.Server.Game.BotTarget = 1;
    RunRoundMapTest(&Test, SERVER_TICK_RATE / 2);

    u32 Target = (MapId_Arena + 1) % MapId_Count;
    RequestVote(Test.Client, MapVoteAsk(Target));
    RunRoundMapTest(&Test, SERVER_TICK_RATE);
    CheckPlayerAtSpawn(&Test, Target);
    Check(!Game->VoteOpen);
    Check(strcmp(Slot->Name, "Voter") == 0);
    Check(Slot->Kills == 3);
    Check(Slot->Level == 1 && Slot->Xp < XpToReach(2));
    Check(Slot->Ranks[0] == 0);
    // NOTE(zoubir): the client's own progression followed
    player_slot *Own = &Test.Client->Players[Test.Online->Client.PlayerIndex];
    Check(Own->Level == 1 && Own->Ranks[0] == 0);
    StopRoundMapTest(&Test);
}

internal void
TestRoundMovesToNextMap()
{
    TestRoundReplaysTheMap();
    TestMapVoteMovesAndStartsOver();
}
