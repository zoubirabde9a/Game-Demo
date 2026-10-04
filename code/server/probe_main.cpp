/* Checks that a game server answers. Usage: probe [host:port] [content-id]
   Connects like a real client, waits for the first snapshot, then says
   goodbye. With a content id (hex, as the server's log prints it) it
   joins as that game build, so it also checks the version gate.
   Exit codes: 0 joined and got a snapshot, 1 bad usage or no network,
   2 no answer, 3 server full, 4 connection lost, 5 wrong version.
   The deploy script runs it after every install, and it works as a
   health check from anywhere.

   probe --info host:port asks who is playing without joining: prints
   the server's name, build, map and the connected players' names. Exit 0
   with an answer, 2 without one, 5 when the server runs another protocol.
   The host may be a DNS name or a.b.c.d. Built by build_server.sh / .bat. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../net/protocol.cpp"
#include "../net/socket.cpp"
#include "../net/client.cpp"
#include "platform/clock.cpp"

// The server's info reply, without taking a slot.
internal int
ProbeInfo(const char *Text)
{
    net_address Server;
    if (!NetSocketsStartup() || !NetResolveServer(Text, &Server))
    {
        fprintf(stderr, "usage: probe --info host:port\n");
        return 1;
    }
    net_socket Socket = NetOpenSocket(0);
    u32 Nonce = (u32)(ClockSeconds() * 1000.0) ^ 0x6a09e667u;
    static net_packet Request, Reply;
    Request.Header.Type = NetPacket_InfoRequest;
    Request.InfoRequest.Nonce = Nonce;
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));

    int Result = 2;
    double Start = ClockSeconds();
    double NextSend = Start;
    while (Result == 2 && ClockSeconds() - Start < 2.0)
    {
        if (ClockSeconds() >= NextSend)
        {
            NetSendTo(&Socket, Server, Buffer, Size);
            NextSend += 0.25;
        }
        net_address From;
        u32 Got;
        while ((Got = NetReceiveFrom(&Socket, &From, Buffer + Size, sizeof(Buffer) - Size)) != 0)
        {
            u32 ServerProtocol;
            if (NetReadVersionNotice(Buffer + Size, Got, &ServerProtocol))
            {
                printf("version: %s speaks protocol %08x, this probe %08x\n", Text,
                       ServerProtocol, NET_PROTOCOL_ID);
                Result = 5;
                break;
            }
            if (NetReadPacket(Buffer + Size, Got, &Reply) &&
                Reply.Header.Type == NetPacket_InfoReply && Reply.InfoReply.Nonce == Nonce)
            {
                net_info_reply *Info = &Reply.InfoReply;
                printf("ok: %s \"%s\" build %08x, map %u, %u/%u players", Text, Info->ServerName,
                       Info->ContentId, Info->MapId, Info->PlayerCount, Info->MaxPlayers);
                for (u32 Index = 0; Index < Info->NameCount; ++Index)
                {
                    printf("%s%s", Index ? ", " : ": ", Info->Names[Index]);
                }
                printf("\n");
                Result = 0;
                break;
            }
        }
        ClockSleep(1.0 / 60.0);
    }
    if (Result == 2) printf("down: no answer from %s\n", Text);
    NetCloseSocket(&Socket);
    NetSocketsShutdown();
    return Result;
}

int
main(int ArgCount, char **Args)
{
    if (ArgCount > 2 && strcmp(Args[1], "--info") == 0) return ProbeInfo(Args[2]);
    const char *Text = ArgCount > 1 ? Args[1] : "127.0.0.1:27015";
    if (!NetSocketsStartup())
    {
        fprintf(stderr, "could not start networking\n");
        return 1;
    }
    net_address Server;
    if (!NetResolveServer(Text, &Server))
    {
        fprintf(stderr, "usage: probe [host:port] [content-id-hex]\n");
        return 1;
    }
    u32 ContentId = ArgCount > 2 ? (u32)strtoul(Args[2], 0, 16) : 0;

    static net_client Client;
    u32 Salt = (u32)(ClockSeconds() * 1000.0) ^ 0x9e3779b9u;
    if (!NetClientConnect(&Client, Server, Salt, ContentId))
    {
        fprintf(stderr, "could not open a socket\n");
        return 1;
    }

    // Same rate as the game, until a snapshot arrives or the client gives up.
    double Start = ClockSeconds();
    while (Client.State != NetClient_Disconnected && !Client.HasSnapshot)
    {
        NetClientUpdate(&Client, 1.0f / 60.0f, 0, 0, 0);
        ClockSleep(1.0 / 60.0);
    }

    int Result = 0;
    if (Client.HasSnapshot)
    {
        printf("ok: %s \"%s\" answered in %.0f ms, slot %u, %u entities in view\n", Text,
               Client.ServerName, (ClockSeconds() - Start) * 1000.0, Client.PlayerIndex,
               Client.Snapshot.Count);
        NetClientDisconnect(&Client);
    }
    else
    {
        switch (Client.EndReason)
        {
            case NetEnd_ServerFull: printf("full: %s refused the probe\n", Text); Result = 3; break;
            case NetEnd_NoAnswer: printf("down: no answer from %s\n", Text); Result = 2; break;
            case NetEnd_WrongVersion: printf("version: %s runs a different game build than %08x\n", Text, ContentId); Result = 5; break;
            default: printf("lost: connection to %s dropped\n", Text); Result = 4; break;
        }
    }
    NetSocketsShutdown();
    return Result;
}
