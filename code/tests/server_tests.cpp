/* Server tests: a real server and a fake client talk over loopback
   sockets in one process. Covers join, input, snapshots, restart and
   leave. Run test.bat from the repo root; exit code 0 means every check
   passed. */

#include <stdio.h>
#include "../server/server.cpp"
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
};

internal void
ClientSend(test_client *Client, net_packet *Packet)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    Packet->Header.Sequence = ++Client->Sequence;
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    NetSendTo(&Client->Socket, Client->Server, Buffer, Size);
}

// Ticks the server until the client receives a packet of the wanted type.
// Gives up after a few hundred ticks, which is several simulated seconds.
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
            if (NetReadPacket(Buffer, Size, Out) && Out->Header.Type == Type) return true;
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
    Check(Reply.Snapshot.Count > 1); // the player plus the arena's monsters
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

// Front-armoured monsters send their facing, a whole turn in 256 steps.
internal void
TestArmoredMonstersSendFacing()
{
    u32 Kind = 0;
    while (Kind < MonsterKind_Count && GetMonsterDef((monster_kind)Kind)->FrontArmor <= 0.f) ++Kind;
    if (Kind == MonsterKind_Count) return; // no armoured kind in this build
    u32 Plain = 0;
    while (Plain < MonsterKind_Count && GetMonsterDef((monster_kind)Plain)->FrontArmor > 0.f) ++Plain;

    static net_snapshot Out;
    Out = {};
    world_entity Monster = {};
    Monster.Type = EntityType_Monster;
    Monster.MonsterKind = (monster_kind)Kind;
    Monster.Direction = V2(0.f, -1.f);
    SimGameWriteFacing(&Monster, 5, &Out);
    Check(Out.FacingCount == 1);
    Check(Out.Facings[0].EntityIndex == 5);
    Check(Out.Facings[0].Angle == 192); // -90 degrees

    Monster.Direction = V2(-1.f, 0.f);
    SimGameWriteFacing(&Monster, 6, &Out);
    Check(Out.Facings[1].Angle == 128);

    // Unarmoured monsters and players send nothing.
    Monster.MonsterKind = (monster_kind)Plain;
    SimGameWriteFacing(&Monster, 7, &Out);
    Monster.Type = EntityType_Player;
    SimGameWriteFacing(&Monster, 8, &Out);
    Check(Out.FacingCount == 2);
}

// More moving things than fit in a snapshot: the viewer still gets its own
// player first and everything near it, and the rest nearest first.
internal void
TestSnapshotPrefersWhatIsNear()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    world *World = &AppState->World;
    GamePlayerJoined(&Game, 0);
    v3 Center = AppState->Players[0].Entity->Position;

    // Far ones first, so sending in entity order would cut the near ones.
    for (u32 Index = 0; Index < 70; ++Index)
    {
        v3 Spot = V3(2400.f, 100.f + 15.f * Index, 0.f);
        AddMonster(AppState, World, Game.Arena, Spot, (monster_kind)0);
    }
    u32 NearIds[4];
    for (u32 Index = 0; Index < 4; ++Index)
    {
        v3 Spot = Center + V3(40.f + 25.f * Index, 30.f, 0.f);
        NearIds[Index] = AddMonster(AppState, World, Game.Arena, Spot, (monster_kind)0)->ID;
    }

    static net_snapshot Out;
    Out = {};
    GameWriteSnapshot(&Game, 0, &Out);
    Check(Out.Count == NET_MAX_SNAPSHOT_ENTITIES);
    Check(Out.Entities[0].Id == AppState->Players[0].Entity->ID);
    for (u32 Near = 0; Near < 4; ++Near)
    {
        bool32 Found = false;
        for (u32 Index = 0; Index < Out.Count; ++Index)
        {
            if (Out.Entities[Index].Id == NearIds[Near]) Found = true;
        }
        Check(Found);
    }
    bool32 Ordered = true;
    float Previous = 0.f;
    for (u32 Index = 1; Index < Out.Count; ++Index)
    {
        float Dx = Out.Entities[Index].X - Center.X;
        float Dy = Out.Entities[Index].Y - Center.Y;
        float DistanceSq = Dx * Dx + Dy * Dy;
        // positions are sent to 1/8 unit, so allow a little slack
        if (DistanceSq + 1.0f < Previous) Ordered = false;
        Previous = DistanceSq;
    }
    Check(Ordered);
    GameShutdown(&Game);
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
        for (int Frame = 0; Frame < SERVER_TICK_RATE / 2; ++Frame) FRAME(0);
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

internal void
TestSnapshotCarriesMonsterWindup()
{
    static server_game Game;
    GameInit(&Game);
    GamePlayerJoined(&Game, 0);

    world *World = &Game.AppState->World;
    world_entity *Monster = 0;
    for (u32 Index = 0; Index < World->EntityCount && !Monster; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster) Monster = Entity;
    }
    Check(Monster != 0);
    if (!Monster) { GameShutdown(&Game); return; }

    static net_snapshot Snapshot;
    GameWriteSnapshot(&Game, 0, &Snapshot);
    Check(Snapshot.AbilityCount == 0); // monsters start ready, nothing to warn about

    Monster->AbilityPhase = AbilityPhase_Windup;
    Monster->AbilityIndex = 1;
    Monster->AbilityTimer = 0.4f;
    Monster->AbilityAim = {0, 1};
    Monster->AbilityPointCount = 2;
    Monster->AbilityPoints[0] = {500, 600};
    Monster->AbilityPoints[1] = {700, 800};
    GameWriteSnapshot(&Game, 0, &Snapshot);

    Check(Snapshot.AbilityCount == 1);
    net_ability_state *A = &Snapshot.Abilities[0];
    net_entity_state *Owner = &Snapshot.Entities[A->EntityIndex];
    Check(Owner->Type == EntityType_Monster);
    Check(Owner->X == Monster->Position.X && Owner->Y == Monster->Position.Y);
    Check(A->Phase == AbilityPhase_Windup && A->Ability == 1);
    Check(A->TimeLeft == 0.4f);
    Check(A->PointCount == 2 && A->PointX[1] == 700.0f && A->PointY[1] == 800.0f);

    Monster->AbilityPhase = AbilityPhase_Recover;
    GameWriteSnapshot(&Game, 0, &Snapshot);
    Check(Snapshot.AbilityCount == 0);
    GameShutdown(&Game);
}

int
main()
{
    if (!NetSocketsStartup())
    {
        printf("server tests: could not start networking\n");
        return 1;
    }
    TestJoinMoveAndLeave();
    TestQuietClientTimesOut();
    TestClientConnectsAndMoves();
    TestSnapshotPrefersWhatIsNear();
    TestArmoredMonstersSendFacing();
    TestPredictionAgreesWithServer();
    TestSnapshotsAcknowledgeInputs();
    TestPlayerNamesReachEveryone();
    TestNinthClientIsTurnedAway();
    TestClientGivesUpWithoutServer();
    TestClientNoticesSilentServer();
    TestSnapshotCarriesMonsterWindup();
    TestStatsCountTrafficAndTicks();
    TestPlayOverBadConnection();
    TestDifferentBuildIsRefused();
    printf("  content id %08x\n", SimContentId());
    NetSocketsShutdown();

    printf("server tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
