/* Packet format tests: every packet type survives a write and read, and
   malformed datagrams are rejected. Builds on its own, without the game,
   which also proves code/net has no dependency on rendering or assets.
   Run test.bat from the repo root; exit code 0 means every check passed. */

#include <stdio.h>
#include "../net/protocol.cpp"
#include "../net/connections.cpp"

global_variable int TestFailures;
global_variable int TestChecks;

#define Check(Expression) CheckImpl((Expression) != 0, #Expression, __LINE__)

internal void
CheckImpl(bool Passed, char *Expression, int Line)
{
    TestChecks++;
    if (!Passed)
    {
        TestFailures++;
        printf("  FAILED net_tests.cpp(%d): %s\n", Line, Expression);
    }
}

internal net_packet
RoundTrip(net_packet *In, u32 *SizeOut)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_packet Out = {};
    u32 Size = NetWritePacket(In, Buffer, sizeof(Buffer));
    Check(Size > 0);
    Check(NetReadPacket(Buffer, Size, &Out));
    if (SizeOut) *SizeOut = Size;
    return Out;
}

internal net_packet
FullSnapshot()
{
    net_packet P = {};
    P.Header = {NetPacket_Snapshot, 7, 3};
    P.Snapshot.Tick = 123456;
    P.Snapshot.Count = NET_MAX_SNAPSHOT_ENTITIES;
    for (u16 Index = 0; Index < NET_MAX_SNAPSHOT_ENTITIES; ++Index)
    {
        net_entity_state *E = &P.Snapshot.Entities[Index];
        E->Id = (u16)(1000 + Index);
        E->Type = (u8)(Index % 5);
        E->Facing = 1;
        E->Animation = 2;
        E->Health = (i16)(Index - 10);
        E->X = 12.5f * Index;
        E->Y = -3.25f;
        E->VelX = 0.1f;
        E->VelY = -900.0f;
    }
    return P;
}

internal void
TestConnectRoundTrip()
{
    net_packet In = {};
    In.Header = {NetPacket_ConnectAccepted, 65535, 1};
    In.ConnectAccepted = {0xdeadbeef, 3, 99};
    net_packet Out = RoundTrip(&In, 0);
    Check(Out.Header.Type == NetPacket_ConnectAccepted);
    Check(Out.Header.Sequence == 65535);
    Check(Out.Header.Ack == 1);
    Check(Out.ConnectAccepted.ClientSalt == 0xdeadbeef);
    Check(Out.ConnectAccepted.PlayerIndex == 3);
    Check(Out.ConnectAccepted.ServerTick == 99);
}

internal void
TestInputRoundTrip()
{
    net_packet In = {};
    In.Header = {NetPacket_Input, 1, 2};
    In.Input.Count = 2;
    In.Input.Inputs[0] = {50, NetButton_Left | NetButton_Shockwave, 0.5f, -1.0f};
    In.Input.Inputs[1] = {49, NetButton_Jump, 3.0f, -7.0f}; // aim out of range
    net_packet Out = RoundTrip(&In, 0);
    Check(Out.Input.Count == 2);
    Check(Out.Input.Inputs[0].Tick == 50);
    Check(Out.Input.Inputs[0].Buttons == (NetButton_Left | NetButton_Shockwave));
    Check(Out.Input.Inputs[0].AimX > 0.4999f && Out.Input.Inputs[0].AimX < 0.5001f);
    Check(Out.Input.Inputs[0].AimY == -1.0f);
    Check(Out.Input.Inputs[1].AimX == 1.0f);
    Check(Out.Input.Inputs[1].AimY == -1.0f);
}

internal void
TestFullSnapshotFits()
{
    net_packet In = FullSnapshot();
    u32 Size = 0;
    net_packet Out = RoundTrip(&In, &Size);
    Check(Size <= NET_MAX_PACKET_SIZE);
    Check(Out.Snapshot.Tick == 123456);
    Check(Out.Snapshot.Count == NET_MAX_SNAPSHOT_ENTITIES);
    net_entity_state *Last = &Out.Snapshot.Entities[NET_MAX_SNAPSHOT_ENTITIES - 1];
    Check(Last->Id == 1000 + NET_MAX_SNAPSHOT_ENTITIES - 1);
    Check(Out.Snapshot.Entities[0].Health == -10);
    Check(Last->VelY == -900.0f);
}

internal void
TestRejectsBadPackets()
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_packet P = FullSnapshot();
    net_packet Out;

    // Too small a buffer to write into.
    Check(NetWritePacket(&P, Buffer, 100) == 0);

    u32 Size = NetWritePacket(&P, Buffer, sizeof(Buffer));
    Check(Size > 0);

    // Truncated anywhere.
    bool32 AnyTruncatedAccepted = false;
    for (u32 Cut = 0; Cut < Size; ++Cut)
    {
        if (NetReadPacket(Buffer, Cut, &Out)) AnyTruncatedAccepted = true;
    }
    Check(!AnyTruncatedAccepted);

    // Trailing garbage.
    Buffer[Size] = 0;
    Check(!NetReadPacket(Buffer, Size + 1, &Out));

    // Wrong protocol id.
    Buffer[0] ^= 0xff;
    Check(!NetReadPacket(Buffer, Size, &Out));
    Buffer[0] ^= 0xff;
    Check(NetReadPacket(Buffer, Size, &Out));

    // Unknown packet type.
    Buffer[4] = NetPacket_Count;
    Check(!NetReadPacket(Buffer, Size, &Out));
    Buffer[4] = NetPacket_Snapshot;

    // Entity count above the limit. Count sits after the 9-byte header and 4-byte tick.
    Buffer[13] = (u8)(NET_MAX_SNAPSHOT_ENTITIES + 1);
    Check(!NetReadPacket(Buffer, Size, &Out));

    // Input batches must hold 1..NET_MAX_INPUTS_PER_PACKET inputs.
    net_packet Input = {};
    Input.Header.Type = NetPacket_Input;
    Check(NetWritePacket(&Input, Buffer, sizeof(Buffer)) == 0);
    Input.Input.Count = NET_MAX_INPUTS_PER_PACKET + 1;
    Check(NetWritePacket(&Input, Buffer, sizeof(Buffer)) == 0);
}

internal void
TestSequenceWraparound()
{
    Check(NetSequenceNewer(1, 0));
    Check(!NetSequenceNewer(0, 1));
    Check(!NetSequenceNewer(5, 5));
    Check(NetSequenceNewer(2, 65530));
    Check(!NetSequenceNewer(65530, 2));
}

internal net_packet
ConnectRequest(u32 Salt)
{
    net_packet P = {};
    P.Header.Type = NetPacket_ConnectRequest;
    P.ConnectRequest.ClientSalt = Salt;
    return P;
}

internal net_packet
InputPacket(u16 Sequence, u32 NewestTick, u8 Count)
{
    net_packet P = {};
    P.Header = {NetPacket_Input, Sequence, 0};
    P.Input.Count = Count;
    for (u8 Index = 0; Index < Count; ++Index)
    {
        P.Input.Inputs[Index].Tick = NewestTick - Index;
    }
    return P;
}

internal void
TestClientsJoinAndRejoin()
{
    net_server_clients Clients = {};
    net_address A = {0x7f000001, 4000};
    net_address B = {0x7f000001, 4001};

    net_packet Request = ConnectRequest(111);
    net_receive_result R = NetServerReceive(&Clients, A, &Request, 10);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 0);
    Check(R.HasReply && R.Reply.Header.Type == NetPacket_ConnectAccepted);
    Check(R.Reply.ConnectAccepted.ClientSalt == 111);
    Check(R.Reply.ConnectAccepted.ServerTick == 10);

    // The client did not hear back and asks again: same slot, no new player.
    R = NetServerReceive(&Clients, A, &Request, 11);
    Check(R.Event == NetReceive_Rejoined);
    Check(R.SlotIndex == 0);
    Check(R.Reply.Header.Sequence == 1);

    net_packet Other = ConnectRequest(222);
    R = NetServerReceive(&Clients, B, &Other, 12);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 1);

    // Same address with a new salt means the client restarted; it gets a fresh slot.
    net_packet Restart = ConnectRequest(333);
    R = NetServerReceive(&Clients, A, &Restart, 13);
    Check(R.Event == NetReceive_Joined);
    Check(R.SlotIndex == 0);
    Check(Clients.Slots[0].Salt == 333);
}

internal void
TestServerFullDenies()
{
    net_server_clients Clients = {};
    for (u16 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_packet Request = ConnectRequest(Index);
        NetServerReceive(&Clients, {1, Index}, &Request, 0);
    }
    net_packet Late = ConnectRequest(99);
    net_receive_result R = NetServerReceive(&Clients, {2, 0}, &Late, 0);
    Check(R.Event == NetReceive_Denied);
    Check(R.Reply.Header.Type == NetPacket_ConnectDenied);
    Check(R.Reply.ConnectDenied.Reason == NetDeny_ServerFull);
    Check(R.Reply.ConnectDenied.ClientSalt == 99);
}

internal void
TestInputsAppliedOnceInOrder()
{
    net_server_clients Clients = {};
    net_address A = {5, 5};
    net_packet Request = ConnectRequest(1);
    NetServerReceive(&Clients, A, &Request, 0);

    net_packet First = InputPacket(1, 3, 3); // ticks 3, 2, 1
    net_receive_result R = NetServerReceive(&Clients, A, &First, 0);
    Check(R.Event == NetReceive_Inputs);
    Check(R.NewInputCount == 3);
    Check(R.NewInputs[0].Tick == 1 && R.NewInputs[2].Tick == 3);

    net_packet Overlap = InputPacket(2, 5, 4); // ticks 5, 4, 3, 2
    R = NetServerReceive(&Clients, A, &Overlap, 0);
    Check(R.NewInputCount == 2);
    Check(R.NewInputs[0].Tick == 4 && R.NewInputs[1].Tick == 5);
    Check(Clients.Slots[0].NewestReceived == 2);

    // A late, reordered packet brings nothing new and does not move the ack back.
    R = NetServerReceive(&Clients, A, &First, 0);
    Check(R.Event == NetReceive_Ignored);
    Check(Clients.Slots[0].NewestReceived == 2);

    // Strangers cannot inject inputs.
    R = NetServerReceive(&Clients, {6, 6}, &Overlap, 0);
    Check(R.Event == NetReceive_Ignored);
    Check(R.NewInputCount == 0);
}

internal void
TestClientsLeaveAndTimeOut()
{
    net_server_clients Clients = {};
    net_packet Request = ConnectRequest(1);
    NetServerReceive(&Clients, {1, 1}, &Request, 0);
    NetServerReceive(&Clients, {2, 2}, &Request, 0);

    net_packet Bye = {};
    Bye.Header.Type = NetPacket_Disconnect;
    net_receive_result R = NetServerReceive(&Clients, {1, 1}, &Bye, 0);
    Check(R.Event == NetReceive_Left);
    Check(!Clients.Slots[0].Connected);

    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.5f) == 0);
    net_packet Ping = InputPacket(1, 1, 1);
    NetServerReceive(&Clients, {2, 2}, &Ping, 0); // hearing from it resets the clock
    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.9f) == 0);
    Check(NetServerAdvance(&Clients, NET_CLIENT_TIMEOUT * 0.2f) == (1u << 1));
    Check(!Clients.Slots[1].Connected);
}

int
main()
{
    TestConnectRoundTrip();
    TestInputRoundTrip();
    TestFullSnapshotFits();
    TestRejectsBadPackets();
    TestSequenceWraparound();
    TestClientsJoinAndRejoin();
    TestServerFullDenies();
    TestInputsAppliedOnceInOrder();
    TestClientsLeaveAndTimeOut();

    printf("net tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
