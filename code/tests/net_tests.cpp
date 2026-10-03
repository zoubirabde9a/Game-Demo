/* Packet format tests: every packet type survives a write and read, and
   malformed datagrams are rejected. Builds on its own, without the game,
   which also proves code/net has no dependency on rendering or assets.
   Run test.bat from the repo root; exit code 0 means every check passed. */

#include <stdio.h>
#include <string.h>
#include "../net/protocol.cpp"
#include "../net/connections.cpp"
#include "../net/socket.cpp"

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
    P.Snapshot.InputTick = 0xfedcba98;
    P.Snapshot.Count = NET_MAX_SNAPSHOT_ENTITIES;
    for (u16 Index = 0; Index < NET_MAX_SNAPSHOT_ENTITIES; ++Index)
    {
        net_entity_state *E = &P.Snapshot.Entities[Index];
        E->Id = (u16)(1000 + Index);
        E->Type = (u8)(Index % 5);
        E->Facing = 1;
        E->Animation = 2;
        E->Affix = (u8)(Index % 5);
        E->Status = (u8)(Index % 8);
        E->Ability = (u8)(Index % 3);
        E->Variant = (u8)(Index % 7);
        E->Health = (i16)(Index - 10);
        E->X = 12.5f * Index;
        E->Y = -3.25f;
        E->VelX = 0.1f;
        E->VelY = -900.0f;
    }

    // Worst case: every ability slot used, every one with all its points.
    P.Snapshot.AbilityCount = NET_MAX_SNAPSHOT_ABILITIES;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_ABILITIES; ++Index)
    {
        net_ability_state *A = &P.Snapshot.Abilities[Index];
        A->EntityIndex = (u8)(NET_MAX_SNAPSHOT_ENTITIES - 1 - Index);
        A->Phase = 1;
        A->Ability = Index;
        A->TimeLeft = 0.75f;
        A->AimX = 0.6f;
        A->AimY = -0.8f;
        A->PointCount = NET_MAX_ABILITY_POINTS;
        for (u32 Point = 0; Point < NET_MAX_ABILITY_POINTS; ++Point)
        {
            A->PointX[Point] = 100.0f * Point + 0.125f;
            A->PointY[Point] = 2000.0f;
        }
    }

    // ...and every player slot's score.
    P.Snapshot.ScoreCount = NET_MAX_SNAPSHOT_SCORES;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_SCORES; ++Index)
    {
        net_score *Score = &P.Snapshot.Scores[Index];
        Score->Slot = Index;
        Score->Kills = (u16)(Index * 3);
        Score->Deaths = 65535;
        Score->MonsterKills = (u16)(1000 + Index);
    }

    // ...and every facing slot...
    P.Snapshot.FacingCount = NET_MAX_SNAPSHOT_FACINGS;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_FACINGS; ++Index)
    {
        P.Snapshot.Facings[Index].EntityIndex = Index;
        P.Snapshot.Facings[Index].Angle = (u8)(Index * 32 + 1);
    }
    P.Snapshot.SoundCount = NET_MAX_SNAPSHOT_SOUNDS;
    for (u8 Index = 0; Index < NET_MAX_SNAPSHOT_SOUNDS; ++Index)
    {
        P.Snapshot.Sounds[Index] = (u8)(200 + Index);
    }

    // ...and the longest name.
    P.Snapshot.NameSlot = NET_MAX_SNAPSHOT_SCORES - 1;
    snprintf(P.Snapshot.Name, NET_NAME_SIZE, "%s", "ABCDEFGHIJKLMNO");
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
TestNamesRoundTripAndAreCleaned()
{
    net_packet In = {};
    In.Header.Type = NetPacket_ConnectRequest;
    In.ConnectRequest.ClientSalt = 5;
    snprintf(In.ConnectRequest.Name, NET_NAME_SIZE, "Zoubir");
    net_packet Out = RoundTrip(&In, 0);
    Check(strcmp(Out.ConnectRequest.Name, "Zoubir") == 0);

    // Control characters and bytes above 126 become '?'.
    In.ConnectRequest.Name[1] = '\t';
    In.ConnectRequest.Name[2] = (char)200;
    Out = RoundTrip(&In, 0);
    Check(strcmp(Out.ConnectRequest.Name, "Z??bir") == 0);

    // A forged length longer than the buffer is refused.
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&In, Buffer, sizeof(Buffer));
    Check(Size > 0);
    Buffer[Size - 7] = NET_NAME_SIZE; // the name's length byte
    net_packet Bad;
    Check(!NetReadPacket(Buffer, Size, &Bad));

    // No name at all is fine.
    In.ConnectRequest.Name[0] = 0;
    Out = RoundTrip(&In, 0);
    Check(Out.ConnectRequest.Name[0] == 0);
}

internal void
TestPackedEntityFields()
{
    net_packet In = {};
    In.Header.Type = NetPacket_Snapshot;
    In.Snapshot.Count = 2;
    In.Snapshot.NameSlot = NET_NO_NAME_SLOT;
    net_entity_state *A = &In.Snapshot.Entities[0];
    A->Facing = 3; A->Animation = 6; A->Affix = 4; A->Status = 5; A->Ability = 2;
    net_entity_state *B = &In.Snapshot.Entities[1];
    B->Facing = 1; B->Animation = 15; B->Affix = 7; B->Status = 7; B->Ability = 3;
    net_packet Out = RoundTrip(&In, 0);
    net_entity_state *OutA = &Out.Snapshot.Entities[0];
    net_entity_state *OutB = &Out.Snapshot.Entities[1];
    Check(OutA->Facing == 3 && OutA->Animation == 6);
    Check(OutA->Affix == 4 && OutA->Status == 5 && OutA->Ability == 2);
    Check(OutB->Facing == 1 && OutB->Animation == 15);
    Check(OutB->Affix == 7 && OutB->Status == 7 && OutB->Ability == 3);

    // Values wider than their bits are cut, not spilled into neighbours.
    A->Facing = 5; A->Affix = 9; A->Status = 0; A->Ability = 0;
    Out = RoundTrip(&In, 0);
    Check(Out.Snapshot.Entities[0].Facing == 1);
    Check(Out.Snapshot.Entities[0].Animation == 6);
    Check(Out.Snapshot.Entities[0].Affix == 1);
    Check(Out.Snapshot.Entities[0].Status == 0);
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
    Check(Out.Snapshot.InputTick == 0xfedcba98);
    Check(Out.Snapshot.Count == NET_MAX_SNAPSHOT_ENTITIES);
    net_entity_state *Last = &Out.Snapshot.Entities[NET_MAX_SNAPSHOT_ENTITIES - 1];
    Check(Last->Id == 1000 + NET_MAX_SNAPSHOT_ENTITIES - 1);
    Check(Out.Snapshot.Entities[0].Health == -10);
    Check(Last->Variant == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 7);
    Check(Last->Affix == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 5);
    Check(Last->Status == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 8);
    Check(Last->Ability == (NET_MAX_SNAPSHOT_ENTITIES - 1) % 3);
    Check(Last->VelY == -900.0f);
    Check(Last->X == 12.5f * (NET_MAX_SNAPSHOT_ENTITIES - 1)); // multiples of 1/8 are exact
    Check(Out.Snapshot.Entities[3].Y == -3.25f);

    Check(Out.Snapshot.AbilityCount == NET_MAX_SNAPSHOT_ABILITIES);
    Check(Out.Snapshot.ScoreCount == NET_MAX_SNAPSHOT_SCORES);
    net_score *LastScore = &Out.Snapshot.Scores[NET_MAX_SNAPSHOT_SCORES - 1];
    Check(LastScore->Slot == NET_MAX_SNAPSHOT_SCORES - 1);
    Check(LastScore->Kills == 3 * (NET_MAX_SNAPSHOT_SCORES - 1));
    Check(LastScore->Deaths == 65535);
    Check(LastScore->MonsterKills == 1000 + NET_MAX_SNAPSHOT_SCORES - 1);
    Check(Out.Snapshot.NameSlot == NET_MAX_SNAPSHOT_SCORES - 1);
    Check(Out.Snapshot.FacingCount == NET_MAX_SNAPSHOT_FACINGS);
    Check(Out.Snapshot.SoundCount == NET_MAX_SNAPSHOT_SOUNDS);
    Check(Out.Snapshot.Sounds[NET_MAX_SNAPSHOT_SOUNDS - 1] == 200 + NET_MAX_SNAPSHOT_SOUNDS - 1);
    Check(Out.Snapshot.Facings[NET_MAX_SNAPSHOT_FACINGS - 1].Angle ==
          (NET_MAX_SNAPSHOT_FACINGS - 1) * 32 + 1);
    Check(strcmp(Out.Snapshot.Name, "ABCDEFGHIJKLMNO") == 0);
    net_ability_state *A = &Out.Snapshot.Abilities[2];
    Check(A->EntityIndex == NET_MAX_SNAPSHOT_ENTITIES - 3);
    Check(A->Phase == 1 && A->Ability == 2);
    Check(A->TimeLeft == 0.75f);
    Check(A->AimX > 0.5999f && A->AimX < 0.6001f);
    Check(A->PointCount == NET_MAX_ABILITY_POINTS);
    Check(A->PointX[3] == 300.125f && A->PointY[3] == 2000.0f);
}

internal void
TestFixedPointPrecisionAndClamping()
{
    net_packet In = {};
    In.Header.Type = NetPacket_Snapshot;
    In.Snapshot.Count = 3;
    In.Snapshot.Entities[0].X = 1234.56f;      // rounds to the nearest 1/8
    In.Snapshot.Entities[0].VelX = -77.3f;     // rounds to the nearest 1/4
    In.Snapshot.Entities[1].X = 99999.0f;      // beyond +-4096: clamped
    In.Snapshot.Entities[1].VelY = -99999.0f;  // beyond +-8192: clamped
    float NotANumber = 0.0f;
    NotANumber = NotANumber / NotANumber;
    In.Snapshot.Entities[2].Z = NotANumber;    // NaN is sent as 0
    net_packet Out = RoundTrip(&In, 0);

    float X = Out.Snapshot.Entities[0].X;
    Check(X > 1234.56f - 0.0626f && X < 1234.56f + 0.0626f);
    float VelX = Out.Snapshot.Entities[0].VelX;
    Check(VelX > -77.3f - 0.126f && VelX < -77.3f + 0.126f);
    Check(Out.Snapshot.Entities[1].X > 4095.0f && Out.Snapshot.Entities[1].X < 4096.0f);
    Check(Out.Snapshot.Entities[1].VelY < -8191.0f && Out.Snapshot.Entities[1].VelY > -8192.0f);
    Check(Out.Snapshot.Entities[2].Z == 0.0f);
}

internal void
TestRejectsAbilityForMissingEntity()
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_packet P = {};
    P.Header.Type = NetPacket_Snapshot;
    P.Snapshot.Count = 2;
    P.Snapshot.AbilityCount = 1;
    P.Snapshot.Abilities[0].EntityIndex = 2; // only 0 and 1 exist
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    P.Snapshot.Abilities[0].EntityIndex = 1;
    P.Snapshot.Abilities[0].PointCount = NET_MAX_ABILITY_POINTS + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    P.Snapshot.Abilities[0].PointCount = 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) > 0);

    P.Snapshot.AbilityCount = NET_MAX_SNAPSHOT_ABILITIES + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);

    // Facings must point at an entity that is in the snapshot too.
    P.Snapshot.AbilityCount = 0;
    P.Snapshot.FacingCount = 1;
    P.Snapshot.Facings[0].EntityIndex = 2;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
    P.Snapshot.Facings[0].EntityIndex = 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) > 0);
    P.Snapshot.FacingCount = NET_MAX_SNAPSHOT_FACINGS + 1;
    Check(NetWritePacket(&P, Buffer, sizeof(Buffer)) == 0);
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

    // Entity count above the limit. Count sits after the 9-byte header,
    // the 4-byte tick and the 4-byte input tick.
    Buffer[17] = (u8)(NET_MAX_SNAPSHOT_ENTITIES + 1);
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

internal void
TestParseAddress()
{
    net_address A = {};
    Check(NetParseAddress("127.0.0.1:27015", &A));
    Check(A.Ip == 0x7f000001 && A.Port == 27015);
    Check(NetParseAddress("255.255.255.255:65535", &A));
    Check(A.Ip == 0xffffffff && A.Port == 65535);
    Check(!NetParseAddress("127.0.0.1", &A));
    Check(!NetParseAddress("127.0.0:80", &A));
    Check(!NetParseAddress("256.0.0.1:80", &A));
    Check(!NetParseAddress("1.2.3.4:65536", &A));
    Check(!NetParseAddress("1..3.4:80", &A));
    Check(!NetParseAddress("1.2.3.4:", &A));
    Check(!NetParseAddress("1.2.3.4:80x", &A));
    Check(!NetParseAddress("", &A));
}

// Sends a real packet between two sockets over the loopback interface.
internal void
TestLoopbackPacket()
{
    Check(NetSocketsStartup());
    net_socket Server = NetOpenSocket(0);
    net_socket Client = NetOpenSocket(0);
    Check(Server.Open && Client.Open);
    u16 ServerPort = NetSocketPort(&Server);
    Check(ServerPort != 0);

    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From = {};
    Check(NetReceiveFrom(&Server, &From, Buffer, sizeof(Buffer)) == 0); // nothing yet, and no blocking

    net_packet Request = {};
    Request.Header.Type = NetPacket_ConnectRequest;
    Request.ConnectRequest.ClientSalt = 4242;
    u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));
    Check(NetSendTo(&Client, {0x7f000001, ServerPort}, Buffer, Size));

    // Loopback delivery is not instant; poll briefly.
    u32 Received = 0;
    for (int Try = 0; Try < 1000000 && !Received; ++Try)
    {
        Received = NetReceiveFrom(&Server, &From, Buffer, sizeof(Buffer));
    }
    net_packet Got = {};
    Check(Received == Size);
    Check(NetReadPacket(Buffer, Received, &Got));
    Check(Got.ConnectRequest.ClientSalt == 4242);
    Check(From.Ip == 0x7f000001 && From.Port == NetSocketPort(&Client));

    NetCloseSocket(&Client);
    NetCloseSocket(&Server);
    Check(!Server.Open);
    NetSocketsShutdown();
}

int
main()
{
    TestConnectRoundTrip();
    TestPackedEntityFields();
    TestNamesRoundTripAndAreCleaned();
    TestInputRoundTrip();
    TestFullSnapshotFits();
    TestFixedPointPrecisionAndClamping();
    TestRejectsAbilityForMissingEntity();
    TestRejectsBadPackets();
    TestSequenceWraparound();
    TestClientsJoinAndRejoin();
    TestServerFullDenies();
    TestInputsAppliedOnceInOrder();
    TestClientsLeaveAndTimeOut();
    TestParseAddress();
    TestLoopbackPacket();

    printf("net tests: %d checks, %d failed\n", TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
