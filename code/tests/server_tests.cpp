/* Server tests: a real server and a fake client talk over loopback
   sockets in one process. Covers join, input, snapshots, restart and
   leave. Run test.bat from the repo root; exit code 0 means every check
   passed. */

#include <stdio.h>
#include "../server/server.cpp"

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
    Check(Reply.Snapshot.Count == 1);
    float StartX = Reply.Snapshot.Entities[0].X;

    // Hold right; the player should move right in later snapshots.
    SendInput(&Client, 1, NetButton_Right);
    for (int Index = 0; Index < 30; ++Index) ServerTick(&Server);
    Check(TickUntil(&Server, &Client, NetPacket_Snapshot, &Reply));
    Check(Reply.Snapshot.Entities[0].X > StartX);
    Check(Reply.Snapshot.Entities[0].VelX > 0);

    // A restart from the same address gets the same slot, reset to the start.
    Request.ConnectRequest.ClientSalt = 78;
    ClientSend(&Client, &Request);
    Check(TickUntil(&Server, &Client, NetPacket_ConnectAccepted, &Reply));
    Check(Reply.ConnectAccepted.PlayerIndex == 0);
    Check(Server.Game.Players[0].X == StartX);
    Check(Server.Game.Players[0].Buttons == 0);

    net_packet Bye = {};
    Bye.Header.Type = NetPacket_Disconnect;
    ClientSend(&Client, &Bye);
    for (int Index = 0; Index < 50 && Server.Clients.Slots[0].Connected; ++Index) ServerTick(&Server);
    Check(!Server.Clients.Slots[0].Connected);
    Check(!Server.Game.Players[0].Present);

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
    Check(Server.Game.Players[0].Present);

    int Ticks = (int)(NET_CLIENT_TIMEOUT * SERVER_TICK_RATE) + 2;
    for (int Index = 0; Index < Ticks; ++Index) ServerTick(&Server);
    Check(!Server.Clients.Slots[0].Connected);
    Check(!Server.Game.Players[0].Present);

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
TestClientConnectsAndMoves()
{
    static server Server;
    static net_client Client;
    Check(ServerStart(&Server, 0));
    Check(NetClientConnect(&Client, LocalServer(&Server), 1234));
    Check(Client.State == NetClient_Connecting);

    for (int Frame = 0; Frame < 120 && !Client.HasSnapshot; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.State == NetClient_Connected);
    Check(Client.PlayerIndex == 0);
    Check(Client.HasSnapshot);
    float StartX = Client.Snapshot.Entities[0].X;
    u32 StartTick = Client.Snapshot.Tick;

    StepBoth(&Server, &Client, 60, NetButton_Right);
    Check(Client.Snapshot.Tick > StartTick);
    Check(Client.Snapshot.Entities[0].X > StartX + 100.0f); // about 200 units after a second

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
        Check(NetClientConnect(&Clients[Index], LocalServer(&Server), 100 + Index));
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
    Check(NetClientConnect(&Client, Nowhere, 5));
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
    Check(NetClientConnect(&Client, LocalServer(&Server), 9));
    for (int Frame = 0; Frame < 120 && Client.State != NetClient_Connected; ++Frame) StepBoth(&Server, &Client, 1, 0);
    Check(Client.State == NetClient_Connected);

    // The server stops ticking without saying goodbye, as if it crashed.
    int Frames = (int)(NET_CLIENT_TIMEOUT * 60) + 2;
    for (int Frame = 0; Frame < Frames; ++Frame) NetClientUpdate(&Client, 1.0f / 60, 0, 0, 0);
    Check(Client.State == NetClient_Disconnected);
    Check(Client.EndReason == NetEnd_LostConnection);
    ServerStop(&Server);
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
    TestNinthClientIsTurnedAway();
    TestClientGivesUpWithoutServer();
    TestClientNoticesSilentServer();
    NetSocketsShutdown();

    printf("server tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
