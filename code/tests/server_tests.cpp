/* Server tests: a real server and a fake client talk over loopback
   sockets in one process. This file has the harness and the server loop
   (join, input, timeouts, full server, stats, version check). What the
   game puts in snapshots is in server_game_tests.cpp; the real client
   against the server is in server_client_tests.cpp. Add a test to the
   file of its area and call it from that file's Run function, so two
   people adding tests rarely touch the same lines. Run test.bat from the
   repo root; exit code 0 means every check passed. */

#include <stdio.h>
#include "../server/server.cpp"
// NOTE: the server builds only the simulation (app_sim.cpp); these tests
// also run the real client against it, so they add the client on top.
#include "../engine/engine_module.cpp"
#include "../ui/ui_ids.h"
#include "../client/client_module.cpp"
#include "lossy_link.h"

global_variable int TestFailures;
global_variable int TestChecks;

#define Check(Expression) CheckImpl((Expression) != 0, #Expression, __LINE__)

internal void
CheckImpl(bool Passed, const char *Expression, int Line)
{
    TestChecks++;
    if (!Passed)
    {
        TestFailures++;
        printf("  FAILED server_tests.cpp(%d): %s\n", Line, Expression);
    }
}

struct test_client
{
    net_socket Socket;
    net_address Server;
    u16 Sequence;
    // NOTE: the newest connect request, sent again with the cookie when the
    // server answers it with a challenge (TickUntil)
    bool32 HasRequest;
    net_packet Request;
};

internal void
ClientSend(test_client *Client, net_packet *Packet)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    if (Packet->Header.Type == NetPacket_ConnectRequest)
    {
        Client->HasRequest = true;
        Client->Request = *Packet;
    }
    // NOTE: like net_client, every packet carries the salt as its token
    if (Client->HasRequest) Packet->Header.Token = Client->Request.ConnectRequest.ClientSalt;
    Packet->Header.Sequence = ++Client->Sequence;
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    NetSendTo(&Client->Socket, Client->Server, Buffer, Size);
}

// Ticks the server until the client receives a packet of the wanted type.
// Gives up after a few hundred ticks, which is several simulated seconds.
// A connect challenge on the way is answered, as the real client does.
internal bool32
TickUntil(server *Server, test_client *Client, u8 Type, net_packet *Out)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    for (int Tick = 0; Tick < 300; ++Tick)
    {
        ServerTick(Server);
        net_address From;
        u32 Size;
        while ((Size = NetReceiveFrom(&Client->Socket, &From, Buffer, sizeof(Buffer))) != 0)
        {
            if (!NetReadPacket(Buffer, Size, Out)) continue;
            if (Out->Header.Type == Type) return true;
            if (Out->Header.Type == NetPacket_ConnectChallenge && Client->HasRequest)
            {
                net_packet Again = Client->Request;
                Again.ConnectRequest.Cookie = Out->ConnectChallenge.Cookie;
                ClientSend(Client, &Again);
            }
        }
    }
    return false;
}

internal void
SendInput(test_client *Client, u32 Tick, u16 Buttons)
{
    net_packet Input = {};
    Input.Header.Type = NetPacket_Input;
    Input.Input.Count = 1;
    Input.Input.Inputs[0].Tick = Tick;
    Input.Input.Inputs[0].Buttons = Buttons;
    ClientSend(Client, &Input);
}

// NOTE(zoubir): a server started on another map tells the client which,
// and a client rebuilding its world from that id gets the same ground
internal void
TestServerSendsItsMap()
{
    static server Server;
    Check(ServerStart(&Server, 0, MapId_Wastes));
    Check(Server.Game.AppState->World.MapId == MapId_Wastes);
    Check(Server.Game.AppState->World.Unbounded);
    test_client Client = {NetOpenSocket(0), {0x7f000001, NetSocketPort(&Server.Socket)}, 0};
    static net_packet Reply;
    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    Request.ConnectRequest.ClientSalt = 91;
    ClientSend(&Client, &Request);
    Check(TickUntil(&Server, &Client, NetPacket_ConnectAccepted, &Reply));
    Check(Reply.ConnectAccepted.MapId == MapId_Wastes);

    // NOTE(zoubir): the joining side, as client/online.cpp does it
    app_state *Joiner = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(64);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Joiner, &Arena, &Constants);
    Check(Joiner->World.MapId == MapId_Arena);
    RebuildWorldForMap(Joiner, &Arena, Reply.ConnectAccepted.MapId);
    Check(Joiner->World.MapId == MapId_Wastes);
    Check(Joiner->World.Unbounded);
    Check(Joiner->Monsters != 0);
    v3 Spawn = PlayerSpawnPosition(&Joiner->World, 0);
    Check(Spawn.X == PlayerSpawnPosition(&Server.Game.AppState->World, 0).X);
    free(Arena.Base);
    free(Constants.Base);
    free(Joiner);

    NetCloseSocket(&Client.Socket);
    ServerStop(&Server);
}

internal void
TestJoinMoveAndLeave()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    test_client Client = {NetOpenSocket(0), {0x7f000001, NetSocketPort(&Server.Socket)}, 0};
    static net_packet Reply;

    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    Request.ConnectRequest.ClientSalt = 77;
    ClientSend(&Client, &Request);
    Check(TickUntil(&Server, &Client, NetPacket_ConnectAccepted, &Reply));
    Check(Reply.ConnectAccepted.ClientSalt == 77);
    Check(Reply.ConnectAccepted.PlayerIndex == 0);

    Check(TickUntil(&Server, &Client, NetPacket_Snapshot, &Reply));
    Check(Reply.Snapshot.Count >= 1); // the player; the Old Arena has no monsters
    Check(Reply.Snapshot.Entities[0].Type == EntityType_Player); // own player first
    Check(Reply.Snapshot.Entities[0].Variant == 0); // a player's Variant is its slot
    Check(Reply.Snapshot.ScoreCount == 1 && Reply.Snapshot.Scores[0].Slot == 0);
    float StartX = Reply.Snapshot.Entities[0].X;

    // Hold left; the player should move left in later snapshots.
    SendInput(&Client, 1, NetButton_Left);
    for (int Index = 0; Index < 30; ++Index) ServerTick(&Server);
    Check(TickUntil(&Server, &Client, NetPacket_Snapshot, &Reply));
    Check(Reply.Snapshot.Entities[0].X < StartX);
    Check(Reply.Snapshot.Entities[0].VelX < 0);

    // A restart from the same address gets the same slot, reset to the start.
    Request.ConnectRequest.ClientSalt = 78;
    ClientSend(&Client, &Request);
    Check(TickUntil(&Server, &Client, NetPacket_ConnectAccepted, &Reply));
    Check(Reply.ConnectAccepted.PlayerIndex == 0);
    Check(Server.Game.AppState->Players[0].Active);
    Check(Server.Game.AppState->Players[0].Entity->Position.X == PlayerSpawnPosition(&Server.Game.AppState->World, 0).X);
    Check(Server.Game.HeldButtons[0] == 0);

    net_packet Bye = {};
    Bye.Header.Type = NetPacket_Disconnect;
    ClientSend(&Client, &Bye);
    for (int Index = 0; Index < 50 && Server.Clients.Slots[0].Connected; ++Index) ServerTick(&Server);
    Check(!Server.Clients.Slots[0].Connected);
    Check(!Server.Game.AppState->Players[0].Active);

    NetCloseSocket(&Client.Socket);
    ServerStop(&Server);
}

internal void
TestQuietClientTimesOut()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    test_client Client = {NetOpenSocket(0), {0x7f000001, NetSocketPort(&Server.Socket)}, 0};
    static net_packet Reply;

    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    ClientSend(&Client, &Request);
    Check(TickUntil(&Server, &Client, NetPacket_ConnectAccepted, &Reply));
    Check(Server.Game.AppState->Players[0].Active);

    int Ticks = (int)(NET_CLIENT_TIMEOUT * SERVER_TICK_RATE) + 2;
    for (int Index = 0; Index < Ticks; ++Index) ServerTick(&Server);
    Check(!Server.Clients.Slots[0].Connected);
    Check(!Server.Game.AppState->Players[0].Active);

    NetCloseSocket(&Client.Socket);
    ServerStop(&Server);
}

// Steps one client and the server together, one 60 Hz frame at a time.
internal void
StepBoth(server *Server, net_client *Client, int Frames, u16 Buttons)
{
    for (int Frame = 0; Frame < Frames; ++Frame)
    {
        NetClientUpdate(Client, 1.0f / SERVER_TICK_RATE, Buttons, 0, 0);
        ServerTick(Server);
    }
}

internal net_address
LocalServer(server *Server)
{
    net_address Address = {0x7f000001, NetSocketPort(&Server->Socket)};
    return Address;
}

internal void
TestPlayerNamesReachEveryone()
{
    static server Server;
    static net_client Named, Plain;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Named, LocalServer(&Server), 41, SimContentId(), "Zoubir"));
    Check(NetClientConnect(&Plain, LocalServer(&Server), 42, SimContentId()));

    // Names take turns in the snapshots, so within a couple of rounds both
    // clients have seen slot 0's name.
    bool32 NamedSaw = false, PlainSaw = false;
    for (int Frame = 0; Frame < 240 && !(NamedSaw && PlainSaw); ++Frame)
    {
        NetClientUpdate(&Named, 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        NetClientUpdate(&Plain, 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        ServerTick(&Server);
        net_snapshot *A = &Named.Snapshot, *B = &Plain.Snapshot;
        if (Named.HasSnapshot && A->NameSlot == Named.PlayerIndex)
            NamedSaw = strcmp(A->Name, "Zoubir") == 0;
        if (Plain.HasSnapshot && B->NameSlot == Named.PlayerIndex)
            PlainSaw = strcmp(B->Name, "Zoubir") == 0;
    }
    Check(NamedSaw);
    Check(PlainSaw);
    NetClientDisconnect(&Named);
    NetClientDisconnect(&Plain);
    ServerStop(&Server);
}

// A flood of junk does not stall the server: one tick reads at most
// SERVER_MAX_PACKETS_PER_TICK packets, counts the junk as bad, and a real
// player still joins afterwards.
internal void
TestFloodDoesNotStallTheServer()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    net_socket Flood = NetOpenSocket(0);
    net_address To = {0x7f000001, NetSocketPort(&Server.Socket)};
    u8 Junk[64] = {1, 2, 3};
    for (u32 Index = 0; Index < 3 * SERVER_MAX_PACKETS_PER_TICK; ++Index)
    {
        NetSendTo(&Flood, To, Junk, sizeof(Junk));
    }
    ServerTick(&Server);
    Check(Server.Stats.PacketsIn <= SERVER_MAX_PACKETS_PER_TICK);
    Check(Server.Stats.BadPacketsIn == Server.Stats.PacketsIn);
    Check(Server.Stats.FullReceiveTicks == 1);
    for (u32 Tick = 0; Tick < 4; ++Tick) ServerTick(&Server);

    static net_client Player;
    Check(NetClientConnect(&Player, To, 515, SimContentId(), "Late"));
    for (int Frame = 0; Frame < 120 && Player.State != NetClient_Connected; ++Frame)
    {
        NetClientUpdate(&Player, 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        ServerTick(&Server);
    }
    Check(Player.State == NetClient_Connected);
    NetClientDisconnect(&Player);
    NetCloseSocket(&Flood);
    ServerStop(&Server);
}

// Asking who is playing takes no slot and lists the players' names.
internal void
TestInfoQueryListsPlayers()
{
    static server Server;
    Check(ServerStart(&Server, 0));
    net_address To = {0x7f000001, NetSocketPort(&Server.Socket)};
    static net_client Player;
    Check(NetClientConnect(&Player, To, 616, SimContentId(), "Gary"));
    for (int Frame = 0; Frame < 120 && Player.State != NetClient_Connected; ++Frame)
    {
        NetClientUpdate(&Player, 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        ServerTick(&Server);
    }
    Check(Player.State == NetClient_Connected);

    net_socket Asker = NetOpenSocket(0);
    static net_packet Request, Reply;
    Request.Header.Type = NetPacket_InfoRequest;
    Request.InfoRequest.Nonce = 4242;
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    NetSendTo(&Asker, To, Buffer, NetWritePacket(&Request, Buffer, sizeof(Buffer)));
    bool32 Answered = false;
    for (int Tick = 0; Tick < 60 && !Answered; ++Tick)
    {
        ServerTick(&Server);
        net_address From;
        u32 Size;
        while ((Size = NetReceiveFrom(&Asker, &From, Buffer, sizeof(Buffer))) != 0)
        {
            if (NetReadPacket(Buffer, Size, &Reply) && Reply.Header.Type == NetPacket_InfoReply)
            {
                Answered = true;
            }
        }
    }
    Check(Answered);
    Check(Reply.InfoReply.Nonce == 4242);
    Check(Reply.InfoReply.PlayerCount == 1 && Reply.InfoReply.MaxPlayers == NET_MAX_CLIENTS);
    Check(Reply.InfoReply.NameCount == 1 && strcmp(Reply.InfoReply.Names[0], "Gary") == 0);
    Check(Reply.InfoReply.ContentId == SimContentId());
    Check(ServerPlayerCount(&Server) == 1);

    NetCloseSocket(&Asker);
    NetClientDisconnect(&Player);
    ServerStop(&Server);
}

internal void
TestSnapshotsAcknowledgeInputs()
{
    static server Server;
    static net_client Client;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Client, LocalServer(&Server), 77, SimContentId()));
    for (int Frame = 0; Frame < 120 && !Client.HasSnapshot; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.State == NetClient_Connected);

    // After a few frames the server has applied the client's recent inputs,
    // and never claims one the client has not sent.
    StepBoth(&Server, &Client, 30, NetButton_Left);
    Check(Client.Snapshot.InputTick > 0);
    Check(Client.Snapshot.InputTick <= Client.InputTick);
    Check(Client.InputTick - Client.Snapshot.InputTick <= 3);

    NetClientDisconnect(&Client);
    ServerStop(&Server);
}

internal void
SetEnvironment(const char *Name, const char *Value)
{
#if defined(_WIN32)
    _putenv_s(Name, Value);
#else
    setenv(Name, Value, 1);
#endif
}

internal void
TestNinthClientIsTurnedAway()
{
    static server Server;
    static net_client Clients[NET_MAX_CLIENTS + 1];
    Check(ServerStart(&Server, 0));
    for (u32 Index = 0; Index < ArrayCount(Clients); ++Index)
    {
        Check(NetClientConnect(&Clients[Index], LocalServer(&Server), 100 + Index, SimContentId()));
    }
    for (int Frame = 0; Frame < 120; ++Frame)
    {
        for (u32 Index = 0; Index < ArrayCount(Clients); ++Index)
        {
            NetClientUpdate(&Clients[Index], 1.0f / SERVER_TICK_RATE, 0, 0, 0);
        }
        ServerTick(&Server);
    }

    u32 Connected = 0, Full = 0;
    for (u32 Index = 0; Index < ArrayCount(Clients); ++Index)
    {
        if (Clients[Index].State == NetClient_Connected) ++Connected;
        if (Clients[Index].EndReason == NetEnd_ServerFull) ++Full;
    }
    Check(Connected == NET_MAX_CLIENTS);
    Check(Full == 1);

    // Stopping the server tells everyone.
    ServerStop(&Server);
    for (u32 Index = 0; Index < ArrayCount(Clients); ++Index)
    {
        for (int Try = 0; Try < 1000 && Clients[Index].State == NetClient_Connected; ++Try)
        {
            NetClientUpdate(&Clients[Index], 0, 0, 0, 0);
        }
        if (Clients[Index].EndReason != NetEnd_ServerFull)
        {
            Check(Clients[Index].EndReason == NetEnd_ServerClosed);
        }
    }
}

// Counts fireballs that appeared this tick: an entity slot that holds a
// present fireball now and did not on the previous call.
internal u32
CountNewFireballs(world *World, bool32 *WasFireball)
{
    u32 New = 0;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        bool32 IsFireball = Entity->IsPresent && Entity->Type == EntityType_FireBall;
        if (IsFireball && !WasFireball[Index]) ++New;
        WasFireball[Index] = IsFireball;
    }
    return New;
}

// Network tests measure the network, not the fight: an empty arena keeps a
// monster that happens to spawn nearby from killing the player or eating
// fireballs, which made the count depend on which monster kinds exist.
internal void
ClearMonsters(server *Server)
{
    app_state *AppState = Server->Game.AppState;
    if (AppState->Monsters) AppState->Monsters->Target = 0;
    world *World = &AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster) RemoveEntity(World, Entity);
    }
}

struct link_result
{
    bool32 Connected;
    bool32 StayedConnected;
    float MovedRight;
    u32 Fireballs;
    u32 Dropped, Duplicated;
};

// One player joins through a link with the given faults, walks right for
// two seconds, then taps fireball Taps times, half a second apart.
internal link_result
PlayThroughLink(u32 DropPercent, u32 DuplicatePercent, u32 MaxDelayFrames, u32 Taps)
{
    link_result Result = {};
    static server Server;
    static net_client Client;
    static lossy_link Link;
    static bool32 WasFireball[4096];
    memset(WasFireball, 0, sizeof(WasFireball));

    Check(ServerStart(&Server, 0));
    ClearMonsters(&Server);
    Check(LossyOpen(&Link, LocalServer(&Server), DropPercent, DuplicatePercent, MaxDelayFrames, 7));
    Check(NetClientConnect(&Client, LossyAddress(&Link), 99, SimContentId()));

    // One frame: client sends, the relay carries it, the server ticks, the relay carries replies.
    #define FRAME(Buttons) do { NetClientUpdate(&Client, 1.0f / SERVER_TICK_RATE, (Buttons), 0, 0); \
                                LossyPump(&Link); ServerTick(&Server); LossyPump(&Link); \
                                Result.Fireballs += CountNewFireballs(&Server.Game.AppState->World, WasFireball); } while (0)

    for (int Frame = 0; Frame < 5 * SERVER_TICK_RATE && Client.State != NetClient_Connected; ++Frame) FRAME(0);
    Result.Connected = Client.State == NetClient_Connected;
    if (!Result.Connected) { ServerStop(&Server); return Result; }

    world_entity *Player = Server.Game.AppState->Players[Client.PlayerIndex].Entity;
    float StartX = Player->Position.X;
    for (int Frame = 0; Frame < 2 * SERVER_TICK_RATE; ++Frame) FRAME(NetButton_Right);
    for (int Frame = 0; Frame < SERVER_TICK_RATE / 2; ++Frame) FRAME(0);
    Result.MovedRight = Player->Position.X - StartX;

    Result.Fireballs = 0;
    for (u32 Tap = 0; Tap < Taps; ++Tap)
    {
        FRAME(NetButton_Fireball); // held for a single frame
        // a moment past the fireball's interval, so every tap may fire
        int Wait = (int)(PlayerStats.FireballInterval * SERVER_TICK_RATE) + 6;
        for (int Frame = 0; Frame < Wait; ++Frame) FRAME(0);
    }
    #undef FRAME

    Result.StayedConnected = Client.State == NetClient_Connected;
    Result.Dropped = Link.Dropped;
    Result.Duplicated = Link.Duplicated;
    NetClientDisconnect(&Client);
    NetCloseSocket(&Link.Socket);
    ServerStop(&Server);
    return Result;
}

internal void
TestDifferentBuildIsRefused()
{
    static server Server;
    static net_client Stranger, Probe, Player;
    Check(ServerStart(&Server, 0));
    Check(Server.Clients.ContentId == SimContentId());
    Check(SimContentId() == SimContentId()); // stable within a build

    // A client built with other monsters: refused before taking a slot.
    Check(NetClientConnect(&Stranger, LocalServer(&Server), 1, SimContentId() ^ 0x5a5a));
    for (int Frame = 0; Frame < 120 && Stranger.State == NetClient_Connecting; ++Frame) StepBoth(&Server, &Stranger, 1, 0);
    Check(Stranger.State == NetClient_Disconnected);
    Check(Stranger.EndReason == NetEnd_WrongVersion);
    Check(!Server.Clients.Slots[0].Connected);

    // The health probe sends 0 and is let in; so is a matching game client.
    Check(NetClientConnect(&Probe, LocalServer(&Server), 2, 0));
    for (int Frame = 0; Frame < 120 && !Probe.HasSnapshot; ++Frame) StepBoth(&Server, &Probe, 1, 0);
    Check(Probe.State == NetClient_Connected);
    Check(NetClientConnect(&Player, LocalServer(&Server), 3, SimContentId()));
    for (int Frame = 0; Frame < 120 && !Player.HasSnapshot; ++Frame) StepBoth(&Server, &Player, 1, 0);
    Check(Player.State == NetClient_Connected);

    NetClientDisconnect(&Probe);
    NetClientDisconnect(&Player);
    ServerStop(&Server);
}

internal void
TestStatsCountTrafficAndTicks()
{
    static server Server;
    static net_client Client;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Client, LocalServer(&Server), 55, SimContentId()));
    for (int Frame = 0; Frame < 120 && !Client.HasSnapshot; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.HasSnapshot);

    // Junk from the internet is counted, not crashed on.
    u8 Junk[16] = {1, 2, 3};
    NetSendTo(&Client.Socket, LocalServer(&Server), Junk, sizeof(Junk));
    for (int Index = 0; Index < 20; ++Index) ServerTick(&Server);

    server_stats *S = &Server.Stats;
    Check(S->PacketsIn > 2 && S->BytesIn > 0);
    Check(S->PacketsOut > 2 && S->BytesOut > S->PacketsOut * 10);
    Check(S->BadPacketsIn == 1);

    ServerRecordTick(&Server, 0.002, false);
    ServerRecordTick(&Server, 0.010, true);
    char Line[256];
    ServerFormatStats(&Server, 2.0, Line, sizeof(Line));
    Check(strstr(Line, "1/8 players") != 0);
    Check(strstr(Line, "tick avg 6.00 ms max 10.00 ms") != 0);
    Check(strstr(Line, "1 late") != 0);
    Check(strstr(Line, "1 bad") != 0);
    Check(Server.Stats.PacketsIn == 0 && Server.Stats.Ticks == 0); // reset after the line

    NetClientDisconnect(&Client);
    ServerStop(&Server);
}

#include "server_game_tests.cpp"
#include "server_client_tests.cpp"
#include "round_map_tests.cpp"
#include "dungeon_online_tests.cpp"
#include "rewind_tests.cpp"
#include "replay_tests.cpp"
#include "motion_tests.cpp"

int
main()
{
    // NOTE(zoubir): written for every ability and 100 health
    // (sim/player_stats.cpp GameRules)
    GameRules = ClassicRules;
    if (!NetSocketsStartup())
    {
        printf("server tests: could not start networking\n");
        return 1;
    }
    TestJoinMoveAndLeave();
    TestServerSendsItsMap();
    TestQuietClientTimesOut();
    RunServerClientTests();
    TestRoundMovesToNextMap();
    RunDungeonOnlineTests();
    RunServerGameTests();
    RunRewindTests();
    RunReplayTests();
    RunMotionTests();
    TestSnapshotsAcknowledgeInputs();
    TestPlayerNamesReachEveryone();
    TestNinthClientIsTurnedAway();
    TestFloodDoesNotStallTheServer();
    TestInfoQueryListsPlayers();
    TestStatsCountTrafficAndTicks();
    TestDifferentBuildIsRefused();
    printf("  content id %08x\n", SimContentId());
    NetSocketsShutdown();

    printf("server tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
