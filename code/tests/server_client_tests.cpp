/* Client against server: the game's own client code (net/client.cpp and
   client/online.cpp) against a real server in the same process, over
   loopback: joining, prediction agreeing with the server, leaving and
   rejoining a restarted server, giving up on a silent one, and playing
   over a lossy link. Included by server_tests.cpp, which calls
   RunServerClientTests. */

// What the connect screen does: a bad address is refused without a
// socket, a good one joins, and leaving goes back to the local game.
internal void
TestConnectAndLeaveFromTheGame()
{
    static server Server;
    Check(ServerStart(&Server, 0));

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0,
                    PlayerSpawnPosition(&Client->World, 0));

    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;
    Check(GetOnlinePhase(Online) == OnlinePhase_Offline);

    Check(!OnlineConnect(Online, "not an address", "Gary"));
    Check(Online->BadAddress);
    Check(GetOnlinePhase(Online) == OnlinePhase_Offline);

    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    Check(OnlineConnect(Online, Address, "Gary"));
    Check(!Online->BadAddress);
    Check(GetOnlinePhase(Online) == OnlinePhase_Joining);

    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < 120 && !IsOnline(Online); ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(GetOnlinePhase(Online) == OnlinePhase_Joined);
    for (int Frame = 0; Frame < 10; ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(Online->Replicas.Active);

    // Keys held while a screen is open do not move the player.
    world_entity *Player = GetLocalPlayer(Client);
    float StartX = Player->Position.X;
    Input.ButtonQ.EndedDown = true;
    for (int Frame = 0; Frame < 30; ++Frame)
    {
        UpdateOnlineSession(Online, &Input, true);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Player = GetLocalPlayer(Client);
    Check(Player->Position.X - StartX < 1.0f && Player->Position.X - StartX > -1.0f);
    Input.ButtonQ.EndedDown = false;

    OnlineDisconnect(Online);
    Check(GetOnlinePhase(Online) == OnlinePhase_Offline);
    RunWorldTick(Client, &Arena, Input.DeltaTime);
    Check(!Online->Replicas.Active);
    ServerTick(&Server);

    ServerStop(&Server);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

// A server restart (a deploy) does not need the player: the client sees
// the server close, waits, and joins the new one by itself. Leaving by
// choice does not reconnect.
internal void
TestClientRejoinsRestartedServer()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    u16 Port = NetSocketPort(&Server.Socket);

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0,
                    PlayerSpawnPosition(&Client->World, 0));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;

    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", Port);
    Check(OnlineConnect(Online, Address, "Gary"));
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < 120 && !IsOnline(Online); ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(IsOnline(Online));

    // The server goes down and comes back on the same port.
    ServerStop(&Server);
    for (int Frame = 0; Frame < 10; ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
    }
    Check(!IsOnline(Online));
    Check(WillReconnect(Online));
    char Status[128];
    GetOnlineStatusText(Online, Status, sizeof(Status));
    Check(strstr(Status, "reconnecting") != 0);
    Check(ServerStart(&Server, Port));

    // First retry after ONLINE_RECONNECT_STEP seconds.
    int Frames = (int)((ONLINE_RECONNECT_STEP + 2.f) * SERVER_TICK_RATE);
    for (int Frame = 0; Frame < Frames && !IsOnline(Online); ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(IsOnline(Online));
    Check(Online->Reconnects == 0);

    // A long outage: the client keeps retrying, waiting at most
    // ONLINE_RECONNECT_MAX_WAIT between tries, and is back soon after.
    ServerStop(&Server);
    int OutageFrames = 60 * SERVER_TICK_RATE;
    for (int Frame = 0; Frame < OutageFrames; ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
    }
    Check(!IsOnline(Online));
    Check(Online->Reconnects >= 5);
    Check(WillReconnect(Online));
    Check(ServerStart(&Server, Port));
    Frames = (int)((ONLINE_RECONNECT_MAX_WAIT + 4.f) * SERVER_TICK_RATE);
    for (int Frame = 0; Frame < Frames && !IsOnline(Online); ++Frame)
    {
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(IsOnline(Online));

    // Leaving by choice stays left.
    OnlineDisconnect(Online);
    Check(!WillReconnect(Online));

    ServerStop(&Server);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

// The real client loop (online session, replicas, prediction) against a
// real server in one process: what the client predicts for its own player
// must end up where the server puts it.
internal void
TestPredictionAgreesWithServer()
{
    static server Server;
    Check(ServerStart(&Server, 0));

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0,
                    PlayerSpawnPosition(&Client->World, 0));

    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    SetEnvironment(ONLINE_ADDRESS_ENV, Address);
    Client->Online = StartOnlineSession(&Arena);
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Check(Client->Online->Enabled);

    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < 120 && !IsOnline(Client->Online); ++Frame)
    {
        UpdateOnlineSession(Client->Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Check(IsOnline(Client->Online));
    for (int Frame = 0; Frame < 10; ++Frame)
    {
        UpdateOnlineSession(Client->Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }

    // Hold left (open ground from slot 0's spawn) for a second.
    world_entity *Predicted = GetLocalPlayer(Client);
    float StartX = Predicted->Position.X;
    Input.ButtonQ.EndedDown = true;
    for (int Frame = 0; Frame < 60; ++Frame)
    {
        UpdateOnlineSession(Client->Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Predicted = GetLocalPlayer(Client);
    world_entity *Authority = Server.Game.AppState->Players[0].Entity;
    Check(Predicted->Position.X < StartX - 50.0f);
    // Moving, the client is ahead of the server by the inputs in flight.
    // It is drawn DrawError off that while a correction blends in.
    v2 DrawError = Client->Online->Prediction.DrawError;
    Check(Predicted->Position.X - DrawError.X <= Authority->Position.X + 0.01f);
    Check(LengthSq(DrawError) < Square(4.0f));

    // Released, both come to rest at the same spot.
    Input.ButtonQ.EndedDown = false;
    for (int Frame = 0; Frame < 60; ++Frame)
    {
        UpdateOnlineSession(Client->Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        ServerTick(&Server);
    }
    Predicted = GetLocalPlayer(Client);
    Authority = Server.Game.AppState->Players[0].Entity;
    float Error = Predicted->Position.X - Authority->Position.X;
    Check(Error < 1.0f && Error > -1.0f);
    Check(Predicted->Position.Y - Authority->Position.Y < 1.0f &&
          Predicted->Position.Y - Authority->Position.Y > -1.0f);

    NetClientDisconnect(&Client->Online->Client);
    ServerStop(&Server);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

internal void
TestClientConnectsAndMoves()
{
    static server Server;
    static net_client Client;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Client, LocalServer(&Server), 1234, SimContentId()));
    Check(Client.State == NetClient_Connecting);

    for (int Frame = 0; Frame < 120 && !Client.HasSnapshot; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.State == NetClient_Connected);
    Check(Client.PlayerIndex == 0);
    Check(Client.HasSnapshot);
    float StartX = Client.Snapshot.Entities[0].X;
    u32 StartTick = Client.Snapshot.Tick;

    StepBoth(&Server, &Client, 60, NetButton_Left);
    Check(Client.Snapshot.Tick > StartTick);
    Check(Client.Snapshot.Entities[0].X < StartX - 50.0f);

    NetClientDisconnect(&Client);
    Check(Client.State == NetClient_Disconnected);
    Check(Client.EndReason == NetEnd_LeftByChoice);
    for (int Index = 0; Index < 30 && Server.Clients.Slots[0].Connected; ++Index) ServerTick(&Server);
    Check(!Server.Clients.Slots[0].Connected);
    ServerStop(&Server);
}

internal void
TestClientGivesUpWithoutServer()
{
    // Open and close a socket to get a port nobody is listening on.
    net_socket Probe = NetOpenSocket(0);
    net_address Nowhere = {0x7f000001, NetSocketPort(&Probe)};
    NetCloseSocket(&Probe);

    static net_client Client;
    Check(NetClientConnect(&Client, Nowhere, 5, SimContentId()));
    int Frames = (int)(NET_CONNECT_GIVE_UP * 60) + 2;
    for (int Frame = 0; Frame < Frames; ++Frame) NetClientUpdate(&Client, 1.0f / 60, 0, 0, 0);
    Check(Client.State == NetClient_Disconnected);
    Check(Client.EndReason == NetEnd_NoAnswer);
}

internal void
TestClientNoticesSilentServer()
{
    static server Server;
    static net_client Client;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Client, LocalServer(&Server), 9, SimContentId()));
    for (int Frame = 0; Frame < 120 && Client.State != NetClient_Connected; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.State == NetClient_Connected);

    // The server stops ticking without saying goodbye, as if it crashed.
    int Frames = (int)(NET_CLIENT_TIMEOUT * 60) + 2;
    for (int Frame = 0; Frame < Frames; ++Frame) NetClientUpdate(&Client, 1.0f / 60, 0, 0, 0);
    Check(Client.State == NetClient_Disconnected);
    Check(Client.EndReason == NetEnd_LostConnection);
    ServerStop(&Server);
}

internal void
TestPlayOverBadConnection()
{
    // Baseline over a clean link, so the bad-link numbers have something to match.
    link_result Clean = PlayThroughLink(0, 0, 0, 10);
    Check(Clean.Connected && Clean.StayedConnected);
    Check(Clean.MovedRight > 50.0f);
    Check(Clean.Fireballs == 10);

    // A quarter of packets lost each way, some doubled, delays up to 100 ms
    // that reorder packets. Every tap must still cast exactly one fireball:
    // each input packet repeats the last 8 inputs, and the server applies
    // each input tick once.
    link_result Bad = PlayThroughLink(25, 10, 6, 10);
    printf("  bad link: %u dropped, %u duplicated; moved %.0f (clean %.0f), %u of 10 fireballs\n",
           Bad.Dropped, Bad.Duplicated, Bad.MovedRight, Clean.MovedRight, Bad.Fireballs);
    Check(Bad.Dropped > 50 && Bad.Duplicated > 10);
    Check(Bad.Connected && Bad.StayedConnected);
    Check(Bad.MovedRight > 0.8f * Clean.MovedRight);
    Check(Bad.Fireballs == 10);
}

// Online parity: what the client's replicas show must match the server's
// entities. Several features worked offline and silently not online
// (sounds, elite health and tint, monster warnings) because the client
// never got or never used a field. A real client watches a real server
// with bots fighting for 30 s; at every new snapshot each replica is
// compared with its server entity, and anything the server showed at
// least once (a wind-up, a burrow, an elite) must have shown on the client
// too. A field that differs, or never reaches the client, is named.
internal void
TestReplicasMatchTheServer()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    Server.Game.BotTarget = 6;

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    Check(OnlineConnect(Online, Address, "Watcher"));

    world *ServerWorld = &Server.Game.AppState->World;
    world *ClientWorld = &Client->World;
    u32 Compared = 0, WrongType = 0, WrongKind = 0, WrongAffix = 0, WrongMaxHp = 0, WrongTint = 0;
    u32 ServerWindups = 0, ClientWindups = 0, ServerBurrows = 0, ClientBurrows = 0;
    u32 ServerElites = 0, ClientElites = 0, ServerFlashes = 0, ClientFlashes = 0;
    u32 LastTick = 0;
    u32 CooldownsCompared = 0, CooldownsOff = 0, ServerCooling = 0, ClientCooling = 0;
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < 30 * SERVER_TICK_RATE; ++Frame)
    {
        // The watcher dashes and shockwaves now and then, so its cooldown
        // bars have something to show.
        Input.AltButton.EndedDown = (Frame % 90) < 2;
        Input.ButtonE.EndedDown = (Frame % 300) < 2;
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        if (IsOnline(Online) && Online->Replicas.Active &&
            Online->Replicas.LastAppliedTick != LastTick)
        {
            LastTick = Online->Replicas.LastAppliedTick;
            // The watcher's own cooldowns: within a snapshot's time of the
            // server's (the server has ticked once more since it wrote).
            world_entity *OwnTheirs = Server.Game.AppState->Players[Online->Client.PlayerIndex].Entity;
            world_entity *OwnOurs = Client->Players[Client->LocalPlayerIndex].Entity;
            if (OwnTheirs && OwnOurs)
            {
                for (u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; ++Index)
                {
                    float Full;
                    float Theirs = *PlayerCooldown(OwnTheirs, Index, &Full);
                    float Ours = *PlayerCooldown(OwnOurs, Index, &Full);
                    ++CooldownsCompared;
                    if (Theirs - Ours > 0.1f || Ours - Theirs > 0.1f) ++CooldownsOff;
                    ServerCooling += Theirs > 0.f ? 1 : 0;
                    ClientCooling += Ours > 0.f ? 1 : 0;
                }
            }
            for (u32 Id = 0; Id < MAX_REPLICAS; ++Id)
            {
                if (!Online->Replicas.LocalIndexPlusOne[Id] || Id >= ServerWorld->EntityCount) continue;
                world_entity *Theirs = &ServerWorld->Entities[Id];
                world_entity *Ours = &ClientWorld->Entities[Online->Replicas.LocalIndexPlusOne[Id] - 1];
                if (!Theirs->IsPresent || !Ours->IsPresent) continue;
                ++Compared;
                if (Ours->Type != Theirs->Type) { ++WrongType; continue; }
                if (Theirs->Type != EntityType_Monster) continue;
                if (Ours->MonsterKind != Theirs->MonsterKind) ++WrongKind;
                if (Ours->EliteAffix != Theirs->EliteAffix) ++WrongAffix;
                if (Ours->MaxHp != Theirs->MaxHp) ++WrongMaxHp;
                if (Ours->Tint != Theirs->Tint) ++WrongTint;
                bool32 TheirWindup = Theirs->AbilityPhase == AbilityPhase_Windup || Theirs->AbilityPhase == AbilityPhase_Active;
                bool32 OurWindup = Ours->AbilityPhase == AbilityPhase_Windup || Ours->AbilityPhase == AbilityPhase_Active;
                ServerWindups += TheirWindup ? 1 : 0;
                ClientWindups += OurWindup ? 1 : 0;
                ServerBurrows += Theirs->Burrowed ? 1 : 0;
                ClientBurrows += Ours->Burrowed ? 1 : 0;
                ServerElites += Theirs->EliteAffix ? 1 : 0;
                ClientElites += Ours->EliteAffix ? 1 : 0;
                ServerFlashes += Theirs->PhaseFlash > 0.f ? 1 : 0;
                ClientFlashes += Ours->PhaseFlash > 0.f ? 1 : 0;
            }
        }
        ServerTick(&Server);
    }
    printf("  parity: %u compared; wrong type %u, kind %u, affix %u, max hp %u, tint %u; "
           "seen on server/client: wind-ups %u/%u, burrows %u/%u, elites %u/%u, enrage flashes %u/%u\n",
           Compared, WrongType, WrongKind, WrongAffix, WrongMaxHp, WrongTint,
           ServerWindups, ClientWindups, ServerBurrows, ClientBurrows,
           ServerElites, ClientElites, ServerFlashes, ClientFlashes);
    Check(Compared > 1000);
    Check(WrongType == 0 && WrongKind == 0 && WrongAffix == 0);
    Check(WrongMaxHp == 0 && WrongTint == 0);
    Check(ServerWindups == 0 || ClientWindups > 0);
    Check(ServerBurrows == 0 || ClientBurrows > 0);
    Check(ServerElites == 0 || ClientElites > 0);
    Check(ServerFlashes == 0 || ClientFlashes > 0);
    printf("  parity: own cooldowns %u compared, %u off by over 0.1 s, cooling on server/client %u/%u\n",
           CooldownsCompared, CooldownsOff, ServerCooling, ClientCooling);
    Check(CooldownsCompared > 100 && CooldownsOff == 0);
    Check(ServerCooling > 0 && ClientCooling > 0);

    NetClientDisconnect(&Online->Client);
    ServerStop(&Server);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

internal void
RunServerClientTests()
{
    TestClientConnectsAndMoves();
    TestPredictionAgreesWithServer();
    TestConnectAndLeaveFromTheGame();
    TestClientRejoinsRestartedServer();
    TestClientGivesUpWithoutServer();
    TestClientNoticesSilentServer();
    TestPlayOverBadConnection();
    TestReplicasMatchTheServer();
}
