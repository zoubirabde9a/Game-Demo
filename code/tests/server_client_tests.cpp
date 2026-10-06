/* Client against server: the game's own client code (net/client.cpp and
   client/online.cpp) against a real server in the same process, over
   loopback: joining, prediction agreeing with the server, leaving and
   rejoining a restarted server, giving up on a silent one, and playing
   over a lossy link. Included by server_tests.cpp, which calls
   RunServerClientTests. */

// What the connect screen does: a bad address is refused without a
// socket, a good one joins, and leaving goes back to the local game.
// A server's name reaches the clients that join it; a server answers a
// build with another protocol with the version notice, and a client that
// gets one stops with WrongVersion instead of waiting for "no answer".
internal void
TestServerNameAndVersionNotice()
{
    static server Server;
    Check(ServerStart(&Server, 0, 0, "Named Arena"));
    net_address Address = {0x7f000001, NetSocketPort(&Server.Socket)};
    static net_client Client;
    Check(NetClientConnect(&Client, Address, 1234, Server.Clients.ContentId, "Gary"));
    for (int Frame = 0; Frame < 120 && Client.State != NetClient_Connected; ++Frame)
    {
        NetClientUpdate(&Client, 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        ServerTick(&Server);
    }
    Check(Client.State == NetClient_Connected);
    Check(strcmp(Client.ServerName, "Named Arena") == 0);
    NetClientDisconnect(&Client);

    // NOTE: a request from "another build": our bytes with the version
    // letter changed
    net_socket Old = NetOpenSocket(0);
    Check(Old.Open);
    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));
    Buffer[0] = (u8)(Buffer[0] + 1);
    Check(NetSendTo(&Old, Address, Buffer, Size));
    u32 Got = 0;
    net_address From = {};
    for (int Frame = 0; Frame < 200 && !Got; ++Frame)
    {
        ServerTick(&Server);
        Got = NetReceiveFrom(&Old, &From, Buffer, sizeof(Buffer));
    }
    u32 ServerProtocol = 0;
    Check(NetReadVersionNotice(Buffer, Got, &ServerProtocol));
    Check(ServerProtocol == NET_PROTOCOL_ID);

    // NOTE: a client joining a server that answers only with the notice
    net_address OldAddress = {0x7f000001, NetSocketPort(&Old)};
    Check(NetClientConnect(&Client, OldAddress, 99, 0, "Gary"));
    NetClientUpdate(&Client, 1.0f / 60.0f, 0, 0, 0);
    u8 Notice[NET_VERSION_NOTICE_SIZE];
    NetWriteVersionNotice(Notice);
    Notice[0] = (u8)(Notice[0] + 1);
    net_address ClientAddress = {0x7f000001, NetSocketPort(&Client.Socket)};
    Check(NetSendTo(&Old, ClientAddress, Notice, sizeof(Notice)));
    for (int Frame = 0; Frame < 200 && Client.State != NetClient_Disconnected; ++Frame)
    {
        NetClientUpdate(&Client, 1.0f / 600.0f, 0, 0, 0);
    }
    Check(Client.State == NetClient_Disconnected);
    Check(Client.EndReason == NetEnd_WrongVersion);
    NetCloseSocket(&Old);
    ServerStop(&Server);
}

// Server addresses as players type them: a.b.c.d:port, a host name with
// or without a port (27015 when left out), and the ones to refuse.
internal void
TestResolveServer()
{
    Check(NetSocketsStartup());
    net_address A = {};
    Check(NetResolveServer("127.0.0.1:27015", &A));
    Check(A.Ip == 0x7f000001 && A.Port == 27015);
    Check(NetResolveServer("localhost:1234", &A));
    Check(A.Ip == 0x7f000001 && A.Port == 1234);
    Check(NetResolveServer("localhost", &A));
    Check(A.Port == NET_DEFAULT_PORT);
    Check(!NetResolveServer("", &A));
    Check(!NetResolveServer("bad host:80", &A));
    Check(!NetResolveServer("localhost:0", &A));
    Check(!NetResolveServer("localhost:65536", &A));
    Check(!NetResolveServer("localhost:80x", &A));
    Check(!NetResolveServer(":80", &A));
    Check(!NetResolveServer("no-such-host.invalid", &A));
}

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
    // It is drawn DrawError off that while a correction blends in. The
    // slack is a small share of one tick's walk (about 4.3 at full speed):
    // where the blend stands at this frame moves with the walk speed.
    v2 DrawError = Client->Online->Prediction.DrawError;
    Check(Predicted->Position.X - DrawError.X <= Authority->Position.X + 0.1f);
    Check(LengthSq(DrawError) < Square(8.0f));

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
    link_result Clean = PlayThroughLink(0, 0, 0, 4);
    Check(Clean.Connected && Clean.StayedConnected);
    Check(Clean.MovedRight > 50.0f);
    Check(Clean.Fireballs == 4);

    // A quarter of packets lost each way, some doubled, delays up to 100 ms
    // that reorder packets. Every tap must still cast exactly one fireball:
    // each input packet repeats the last 8 inputs, and the server applies
    // each input tick once.
    link_result Bad = PlayThroughLink(25, 10, 6, 4);
    printf("  bad link: %u dropped, %u duplicated; moved %.0f (clean %.0f), %u of 4 fireballs\n",
           Bad.Dropped, Bad.Duplicated, Bad.MovedRight, Clean.MovedRight, Bad.Fireballs);
    Check(Bad.Dropped > 50 && Bad.Duplicated > 10);
    Check(Bad.Connected && Bad.StayedConnected);
    Check(Bad.MovedRight > 0.8f * Clean.MovedRight);
    Check(Bad.Fireballs == 4);
}

// Online parity: what the client's replicas show must match the server's
// entities. Several features worked offline and silently not online
// (sounds, elite health and tint, monster warnings) because the client
// never got or never used a field. A real client watches a real server
// with bots fighting for 30 s; at every new snapshot each replica is
// compared with its server entity, and anything the server showed at
// least once (a wind-up, a burrow, an elite) must have shown on the client
// too. A field that differs, or never reaches the client, is named.
// NOTE: the snapshot is written at the end of a server tick and compared
// before the next one, so the server's state is the one that was sent and
// the comparisons can be exact. Run on the arena and on an infinite map,
// where the client first has to rebuild the world for the server's map.
internal void
TestReplicasMatchTheServer(u32 MapId, int Seconds)
{
    static server Server;
    Check(ServerStart(&Server, 0, MapId));
    Server.Game.BotTarget = 6;

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    // NOTE(zoubir): as in the game, which makes it in MemoryArena: bots
    // cast time rewinds, and a watcher one freezes is not predicted
    // (client/rewind_fx/)
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;
    char Address[32];
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", NetSocketPort(&Server.Socket));
    Check(OnlineConnect(Online, Address, "Watcher"));

    world *ServerWorld = &Server.Game.AppState->World;
    world *ClientWorld = &Client->World;
    u32 Compared = 0, WrongType = 0, WrongKind = 0, WrongAffix = 0, WrongMaxHp = 0, WrongTint = 0;
    u32 Players = 0, WrongPlayerHp = 0, WrongDead = 0, WrongSlot = 0;
    // NOTE(zoubir): levels and wards (sim/progression/) reach every
    // client through the scores; the watcher's own experience and ranks
    // through its snapshot
    u32 WrongLevel = 0, WrongWard = 0, ServerLevelUps = 0, OwnCompared = 0, WrongOwn = 0;
    u32 ServerWindups = 0, ClientWindups = 0, ServerBurrows = 0, ClientBurrows = 0;
    u32 ServerElites = 0, ClientElites = 0, ServerFlashes = 0, ClientFlashes = 0;
    u32 LastTick = 0;
    u32 CooldownsCompared = 0, CooldownsOff = 0, ServerCooling = 0, ClientCooling = 0;
    // NOTE(zoubir): the watcher's stagger from a shove (sim/hit.cpp), which
    // its prediction replays from the snapshot's
    u32 StaggersCompared = 0, StaggersOff = 0, ServerStaggered = 0, ClientStaggered = 0;
    u32 ShieldPresses = 0, ShieldsSeenAtOnce = 0;
    // NOTE(zoubir): a press a moment after the shield is ready again, held
    // back while the watcher is dead or held so the bots killing it does
    // not use up the presses
    int ShieldEvery = (int)(PlayerMovements[PlayerMove_Shield].Cooldown * SERVER_TICK_RATE) + 10;
    int LastShieldFrame = -ShieldEvery;
    u32 JumpPresses = 0, JumpsSeenAtOnce = 0;
    // NOTE(zoubir): the last hit (sim/hit.cpp): units hit lately on each
    // side, and of those on both, how many disagree on the hit's angle,
    // thrower, lift or hit-pause
    u32 ServerHits = 0, ClientHits = 0, BothHit = 0, WrongHit = 0;
    u32 ServerStops = 0, ClientStops = 0, WrongStop = 0;
    // NOTE(zoubir): players winding up a spell (sim/player_casts.cpp) on
    // each side, and of those on both, how many show another spell
    u32 ServerCasts = 0, ClientCasts = 0, BothCast = 0, WrongCast = 0;
    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    for (int Frame = 0; Frame < Seconds * SERVER_TICK_RATE; ++Frame)
    {
        // The watcher shields now and then, so its cooldown bars have
        // something to show.
        world_entity *Before = Client->Players[Client->LocalPlayerIndex].Entity;
        if (Frame - LastShieldFrame >= ShieldEvery && IsOnline(Online) &&
            Online->Replicas.Active && Before && Before->IsPresent &&
            !IsDeadPlayer(Before) && !IsLocalPlayerTimeLocked(Client) &&
            !HasStatus(Before, StatusEffect_Stunned))
        {
            LastShieldFrame = Frame;
        }
        Input.ButtonE.EndedDown = (Frame - LastShieldFrame) < 2;
        // NOTE(zoubir): and jumps, so its own jump arc can be compared
        Input.SpaceButton.EndedDown = (Frame % 120) == 45;
        UpdateOnlineSession(Online, &Input);
        RunWorldTick(Client, &Arena, Input.DeltaTime);
        UpdateRewindFx(Client, Input.DeltaTime);
        // NOTE(zoubir): the shield is predicted, so the client's player has
        // already raised it on the frame the key goes down (its cooldown
        // has started). Not while stunned, or frozen by a bot's time
        // rewind: then it does neither, on the server or here
        world_entity *Own = Client->Players[Client->LocalPlayerIndex].Entity;
        bool32 Held = IsLocalPlayerTimeLocked(Client) ||
            (Own && HasStatus(Own, StatusEffect_Stunned));
        if ((Frame % 120) == 45 && IsOnline(Online) && Online->Replicas.Active &&
            Own && Own->IsPresent && !IsDeadPlayer(Own) && !Held)
        {
            ++JumpPresses;
            JumpsSeenAtOnce += Own->Velocity.Z > 0.f ? 1 : 0;
        }
        if (Frame == LastShieldFrame && IsOnline(Online) && Online->Replicas.Active &&
            Own && Own->IsPresent && !IsDeadPlayer(Own) && !Held)
        {
            ++ShieldPresses;
            ShieldsSeenAtOnce += Own->MovementCooldowns[PlayerMove_Shield] >
                0.9f * PlayerMovements[PlayerMove_Shield].Cooldown ? 1 : 0;
        }
        if (IsOnline(Online) && Online->Replicas.Active &&
            Online->Replicas.LastAppliedTick != LastTick)
        {
            LastTick = Online->Replicas.LastAppliedTick;
            // The watcher's own cooldowns: within a snapshot's time of the
            // server's (the server has ticked once more since it wrote).
            world_entity *OwnTheirs = Server.Game.AppState->Players[Online->Client.PlayerIndex].Entity;
            world_entity *OwnOurs = Client->Players[Client->LocalPlayerIndex].Entity;
            // NOTE(zoubir): a predicted press (a dash, a cast) the server has
            // not had yet starts its cooldown on the client first; skip
            // those frames
            bool32 PressInFlight = false;
            for (u32 Index = 0; Index < Online->Prediction.Count; ++Index)
            {
                u32 Pressed = (u32)GetPredictedInput(&Online->Prediction, Index)->Pressed >>
                    PLAYER_BUTTON_NET_SHIFT;
                PressInFlight = PressInFlight || (Pressed & PredictedButtons());
            }
            if (OwnTheirs && OwnOurs && !PressInFlight)
            {
                for (u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; ++Index)
                {
                    float Full;
                    float Theirs = *PlayerCooldown(Server.Game.AppState, OwnTheirs, Index, &Full);
                    float Ours = *PlayerCooldown(Client, OwnOurs, Index, &Full);
                    ++CooldownsCompared;
                    if (Theirs - Ours > 0.1f || Ours - Theirs > 0.1f) ++CooldownsOff;
                    ServerCooling += Theirs > 0.f ? 1 : 0;
                    ClientCooling += Ours > 0.f ? 1 : 0;
                }
                ++StaggersCompared;
                StaggersOff += Absolute(OwnTheirs->Stagger - OwnOurs->Stagger) > 0.1f ? 1 : 0;
                ServerStaggered += OwnTheirs->Stagger > 0.f ? 1 : 0;
                ClientStaggered += OwnOurs->Stagger > 0.f ? 1 : 0;
            }
            for (u32 Id = 0; Id < MAX_REPLICAS; ++Id)
            {
                if (!Online->Replicas.LocalIndexPlusOne[Id] || Id >= ServerWorld->EntityCount) continue;
                world_entity *Theirs = &ServerWorld->Entities[Id];
                world_entity *Ours = &ClientWorld->Entities[Online->Replicas.LocalIndexPlusOne[Id] - 1];
                if (!Theirs->IsPresent || !Ours->IsPresent) continue;
                ++Compared;
                if (Ours->Type != Theirs->Type) { ++WrongType; continue; }
                ServerHits += Theirs->HitFresh > 0.f ? 1 : 0;
                ClientHits += Ours->HitFresh > 0.f ? 1 : 0;
                ServerStops += Theirs->HitStop > 0.f ? 1 : 0;
                ClientStops += Ours->HitStop > 0.f ? 1 : 0;
                if (Theirs->HitFresh > 0.f && Ours->HitFresh > 0.f)
                {
                    ++BothHit;
                    // NOTE(zoubir): the angle travels in 256 steps
                    float Turn = (Theirs->HitAngle - Ours->HitAngle) / (2.f * Pi32);
                    Turn -= floorf(Turn + 0.5f);
                    bool32 SameHit = Absolute(Turn) < 1.5f / 256.f &&
                        Ours->HitBySlot == Theirs->HitBySlot &&
                        (Ours->HitThrown != 0) == (Theirs->HitThrown != 0);
                    WrongHit += SameHit ? 0 : 1;
                    // NOTE(zoubir): the replica's pause has run down one
                    // frame since; in milliseconds on the wire
                    float Stop = Maximum(0.f, Theirs->HitStop);
                    WrongStop += Absolute(Stop - Ours->HitStop) > 0.02f ? 1 : 0;
                }
                if (Theirs->Type == EntityType_Player)
                {
                    ++Players;
                    // NOTE(zoubir): sent in hundredths, rounded up while
                    // alive (SimGameHealth, pack.cpp)
                    float Sent = (float)SimGameHealth(Theirs) / NET_PLAYER_HEALTH_STEPS;
                    if (Absolute(Ours->Hp - Sent) > 0.001f) ++WrongPlayerHp;
                    if (IsDeadPlayer(Ours) != IsDeadPlayer(Theirs)) ++WrongDead;
                    if (Ours->PlayerIndex != Theirs->PlayerIndex) ++WrongSlot;
                    player_slot *OurSlot = &Client->Players[Ours->PlayerIndex];
                    player_slot *TheirSlot = &Server.Game.AppState->Players[Theirs->PlayerIndex];
                    WrongLevel += OurSlot->Level != TheirSlot->Level ? 1 : 0;
                    ServerLevelUps += TheirSlot->Level > 1 ? 1 : 0;
                    bool32 TheirWard = TheirSlot->WardReady && TheirSlot->Ranks[Talent_Ward];
                    WrongWard += (OurSlot->WardReady != 0) != (TheirWard != 0) ? 1 : 0;
                    if (Ours->PlayerIndex == Client->LocalPlayerIndex)
                    {
                        ++OwnCompared;
                        bool32 Same = OurSlot->Xp == TheirSlot->Xp;
                        for (u32 Talent = 0; Talent < Talent_Count; ++Talent)
                        {
                            Same = Same && OurSlot->Ranks[Talent] == TheirSlot->Ranks[Talent];
                        }
                        WrongOwn += Same ? 0 : 1;
                    }
                    ServerCasts += IsPlayerCasting(Theirs) ? 1 : 0;
                    ClientCasts += IsPlayerCasting(Ours) ? 1 : 0;
                    if (IsPlayerCasting(Theirs) && IsPlayerCasting(Ours))
                    {
                        ++BothCast;
                        WrongCast += Ours->CastSpell != Theirs->CastSpell ? 1 : 0;
                    }
                    continue;
                }
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
    printf("  parity on %s: %u players, wrong health %u, dead %u, slot %u\n",
           GetMapDef((map_id)MapId)->Name, Players, WrongPlayerHp, WrongDead, WrongSlot);
    Check(Players > 100 && WrongPlayerHp == 0 && WrongDead == 0 && WrongSlot == 0);
    printf("  parity: players past level 1 on the server %u; wrong level %u, ward %u; "
           "own experience and ranks off %u of %u\n",
           ServerLevelUps, WrongLevel, WrongWard, WrongOwn, OwnCompared);
    // NOTE(zoubir): the server ticks once after the snapshot it compares
    // against, so a level, a ward or the trickle of experience may move
    // in between now and then
    Check(ServerLevelUps > 0);
    Check(WrongLevel * 50 <= Players && WrongWard * 50 <= Players);
    Check(OwnCompared > 100 && WrongOwn * 3 <= OwnCompared);
    printf("  parity: players casting on server/client %u/%u, %u on both, %u show another spell\n",
           ServerCasts, ClientCasts, BothCast, WrongCast);
    Check(ServerCasts == 0 || ClientCasts > 0);
    Check(WrongCast * 20 <= BothCast);
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
    printf("  parity: units hit on server/client %u/%u, %u on both, %u disagree on the hit, "
           "%u on the pause; paused on server/client %u/%u\n",
           ServerHits, ClientHits, BothHit, WrongHit, WrongStop, ServerStops, ClientStops);
    // NOTE(zoubir): a pause lasts a few ticks and lands between snapshots
    // as often as not, and a short game may see none
    Check(ServerHits > 0 && ClientHits > 0);
    Check(ServerStops == 0 || ClientStops > 0);
    Check(BothHit > 0 && WrongHit * 50 <= BothHit && WrongStop * 50 <= BothHit);
    printf("  parity: own cooldowns %u compared, %u off by over 0.1 s, cooling on server/client %u/%u\n",
           CooldownsCompared, CooldownsOff, ServerCooling, ClientCooling);
    Check(CooldownsCompared > 100 && CooldownsOff == 0);
    printf("  parity: own stagger %u compared, %u off by over 0.1 s, staggered on server/client %u/%u\n",
           StaggersCompared, StaggersOff, ServerStaggered, ClientStaggered);
    Check(StaggersCompared > 100 && StaggersOff == 0);
    Check(ServerStaggered == 0 || ClientStaggered > 0);
    printf("  shield presses %u, shielded on the press frame %u\n", ShieldPresses, ShieldsSeenAtOnce);
    // NOTE(zoubir): a 6 s shield gives a short game only a press or two
    Check(ShieldPresses >= 1 && ShieldsSeenAtOnce >= ShieldPresses - 1);
    // NOTE(zoubir): jump is predicted too: rising on the press frame
    printf("  jump presses %u, rising on the press frame %u\n",
           JumpPresses, JumpsSeenAtOnce);
    Check(JumpPresses > 1 && JumpsSeenAtOnce >= JumpPresses);
    Check(ServerCooling > 0 && ClientCooling > 0);

    NetClientDisconnect(&Online->Client);
    ServerStop(&Server);
    free(Client->RewindFx);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
}

internal void
RunServerClientTests()
{
    TestServerNameAndVersionNotice();
    TestResolveServer();
    TestClientConnectsAndMoves();
    TestPredictionAgreesWithServer();
    TestConnectAndLeaveFromTheGame();
    TestClientRejoinsRestartedServer();
    TestClientGivesUpWithoutServer();
    TestClientNoticesSilentServer();
    TestPlayOverBadConnection();
    TestReplicasMatchTheServer(MapId_Arena, 30);
    TestReplicasMatchTheServer(MapId_Wilds, 15);
}
