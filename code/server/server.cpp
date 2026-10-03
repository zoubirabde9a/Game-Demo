/* Server tick, declared in server.h. Includes everything the server
   needs, so a program only has to include this one file. */

#include <stdio.h>
#include <stdarg.h>
#include "../net/protocol.cpp"
#include "../net/connections.cpp"
#include "../net/socket.cpp"
#include "../net/client.cpp" // not used by the server itself; compiled here so tests can drive both ends
#include "game_api.h"
#include "placeholder_game.cpp" // the game implementation; must come before server.h
#include "server.h"

internal void
ServerSend(server *Server, net_address To, net_packet *Packet)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    if (Size) NetSendTo(&Server->Socket, To, Buffer, Size);
}

// One line per event on stdout; a service manager (systemd) adds the timestamps.
internal void
ServerLog(server *Server, const char *Format, ...)
{
    if (!Server->Logging) return;
    va_list Args;
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
    printf("\n");
    fflush(stdout);
}

internal u32
ServerPlayerCount(server *Server)
{
    u32 Count = 0;
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        if (Server->Clients.Slots[Index].Connected) ++Count;
    }
    return Count;
}

#define ADDRESS_FORMAT "%u.%u.%u.%u:%u"
#define ADDRESS_ARGS(A) (A).Ip >> 24, ((A).Ip >> 16) & 255, ((A).Ip >> 8) & 255, (A).Ip & 255, (A).Port

internal bool32
ServerStart(server *Server, u16 Port)
{
    *Server = {};
    Server->Socket = NetOpenSocket(Port);
    GameInit(&Server->Game);
    return Server->Socket.Open;
}

internal void
ServerReceiveAll(server *Server)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    while ((Size = NetReceiveFrom(&Server->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        net_packet Packet;
        if (!NetReadPacket(Buffer, Size, &Packet)) continue;

        net_receive_result Result = NetServerReceive(&Server->Clients, From, &Packet, Server->Tick);
        switch (Result.Event)
        {
            case NetReceive_Joined:
            {
                GamePlayerJoined(&Server->Game, Result.SlotIndex);
                ServerLog(Server, "player %u joined from " ADDRESS_FORMAT " (%u/%u)", Result.SlotIndex,
                          ADDRESS_ARGS(From), ServerPlayerCount(Server), NET_MAX_CLIENTS);
            } break;
            case NetReceive_Denied:
            {
                ServerLog(Server, "refused " ADDRESS_FORMAT ": server full", ADDRESS_ARGS(From));
            } break;
            case NetReceive_Left:
            {
                GamePlayerLeft(&Server->Game, Result.SlotIndex);
                ServerLog(Server, "player %u left (%u/%u)", Result.SlotIndex,
                          ServerPlayerCount(Server), NET_MAX_CLIENTS);
            } break;
            case NetReceive_Inputs:
            {
                for (u32 Index = 0; Index < Result.NewInputCount; ++Index)
                {
                    GameApplyInput(&Server->Game, Result.SlotIndex, &Result.NewInputs[Index]);
                }
            } break;
            default: break;
        }

        if (Result.HasReply) ServerSend(Server, From, &Result.Reply);
    }
}

internal void
ServerSendSnapshots(server *Server)
{
    net_packet Packet = {};
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_client_slot *Slot = &Server->Clients.Slots[Index];
        if (!Slot->Connected) continue;
        NetServerStampHeader(Slot, &Packet, NetPacket_Snapshot);
        Packet.Snapshot.Tick = Server->Tick;
        GameWriteSnapshot(&Server->Game, Index, &Packet.Snapshot);
        ServerSend(Server, Slot->Address, &Packet);
    }
}

internal void
ServerTick(server *Server)
{
    float Dt = 1.0f / SERVER_TICK_RATE;
    ServerReceiveAll(Server);
    GameTick(&Server->Game, Dt);
    Server->Tick++;
    if (Server->Tick % SERVER_SNAPSHOT_INTERVAL == 0) ServerSendSnapshots(Server);

    u32 TimedOut = NetServerAdvance(&Server->Clients, Dt);
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        if (TimedOut & (1u << Index))
        {
            GamePlayerLeft(&Server->Game, Index);
            ServerLog(Server, "player %u timed out (%u/%u)", Index, ServerPlayerCount(Server), NET_MAX_CLIENTS);
        }
    }
}

internal void
ServerStop(server *Server)
{
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_client_slot *Slot = &Server->Clients.Slots[Index];
        if (!Slot->Connected) continue;
        net_packet Bye = {};
        NetServerStampHeader(Slot, &Bye, NetPacket_Disconnect);
        ServerSend(Server, Slot->Address, &Bye);
        *Slot = {};
    }
    NetCloseSocket(&Server->Socket);
}
