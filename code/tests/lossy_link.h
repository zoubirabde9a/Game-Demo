/* A bad internet connection for tests: a UDP relay between one client and
   a server that drops, duplicates and delays datagrams. The client talks
   to the relay's port as if it were the server; the server sees the relay
   as the client. Call LossyPump once per simulated frame; a datagram's
   delay is counted in frames, so delays also reorder packets.
   MinDelayFrames and BurstFrames, set after LossyOpen, make it a long
   link (every datagram waits at least that long) and one whose losses
   come in runs (a drop also drops everything in the same direction for
   up to BurstFrames frames), as a busy wifi or mobile link does.
   ClumpFrames hands datagrams to the client only every that many
   frames, several at once, as wifi saving power does.
   Needs code/net/socket.cpp compiled in first. */

#define LOSSY_QUEUE_SIZE 512

struct lossy_datagram
{
    u8 Data[NET_MAX_PACKET_SIZE];
    u32 Size;
    u32 DeliverAtFrame;
    bool32 ToServer;
};

struct lossy_link
{
    net_socket Socket;
    net_address Server;
    net_address Client;
    bool32 HasClient;

    u32 DropPercent;
    u32 DuplicatePercent;
    u32 MaxDelayFrames;
    u32 MinDelayFrames;
    u32 BurstFrames;
    u32 ClumpFrames;
    // NOTE: a run of losses drops this direction until this frame
    u32 DropUntilFrame[2];
    u32 Random;
    u32 Frame;

    lossy_datagram Queue[LOSSY_QUEUE_SIZE];
    u32 QueueCount;

    u32 Forwarded, Dropped, Duplicated;
};

internal u32
LossyRandom(lossy_link *Link, u32 Range)
{
    u32 X = Link->Random;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    Link->Random = X;
    return Range ? X % Range : 0;
}

internal bool32
LossyOpen(lossy_link *Link, net_address Server, u32 DropPercent,
          u32 DuplicatePercent, u32 MaxDelayFrames, u32 Seed)
{
    *Link = {};
    Link->Socket = NetOpenSocket(0);
    Link->Server = Server;
    Link->DropPercent = DropPercent;
    Link->DuplicatePercent = DuplicatePercent;
    Link->MaxDelayFrames = MaxDelayFrames;
    Link->Random = Seed * 2654435761u + 1;
    return Link->Socket.Open;
}

internal net_address
LossyAddress(lossy_link *Link)
{
    net_address Result = {0x7f000001, NetSocketPort(&Link->Socket)};
    return Result;
}

internal void
LossyQueue(lossy_link *Link, u8 *Data, u32 Size, bool32 ToServer)
{
    if (Link->QueueCount == LOSSY_QUEUE_SIZE) { Link->Dropped++; return; }
    lossy_datagram *D = &Link->Queue[Link->QueueCount++];
    memcpy(D->Data, Data, Size);
    D->Size = Size;
    D->ToServer = ToServer;
    D->DeliverAtFrame = Link->Frame + Link->MinDelayFrames +
        LossyRandom(Link, Link->MaxDelayFrames + 1);
}

internal void
LossyPump(lossy_link *Link)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    while ((Size = NetReceiveFrom(&Link->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        bool32 ToServer = !NetAddressEqual(From, Link->Server);
        if (ToServer) { Link->Client = From; Link->HasClient = true; }

        if (Link->DropUntilFrame[ToServer ? 1 : 0] > Link->Frame) { Link->Dropped++; continue; }
        if (LossyRandom(Link, 100) < Link->DropPercent)
        {
            Link->Dropped++;
            if (Link->BurstFrames)
            {
                Link->DropUntilFrame[ToServer ? 1 : 0] =
                    Link->Frame + 1 + LossyRandom(Link, Link->BurstFrames);
            }
            continue;
        }
        LossyQueue(Link, Buffer, Size, ToServer);
        if (LossyRandom(Link, 100) < Link->DuplicatePercent)
        {
            Link->Duplicated++;
            LossyQueue(Link, Buffer, Size, ToServer);
        }
    }

    for (u32 Index = 0; Index < Link->QueueCount;)
    {
        lossy_datagram *D = &Link->Queue[Index];
        if (D->DeliverAtFrame > Link->Frame) { ++Index; continue; }
        if (!D->ToServer && Link->ClumpFrames && Link->Frame % Link->ClumpFrames) { ++Index; continue; }
        if (D->ToServer || Link->HasClient)
        {
            NetSendTo(&Link->Socket, D->ToServer ? Link->Server : Link->Client, D->Data, D->Size);
            Link->Forwarded++;
        }
        *D = Link->Queue[--Link->QueueCount];
    }
    Link->Frame++;
}
