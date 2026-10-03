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
    NetSocketsShutdown();

    printf("server tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
