/* Server tick, declared in server.h. Includes everything the server
   needs, so a program only has to include this one file. */

#include <stdio.h>
#include <stdarg.h>
#include "../net/protocol.cpp"
#include "../net/connections.cpp"
#include "../net/socket.cpp"
#include "../net/client.cpp" // not used by the server itself; compiled here so tests can drive both ends
#include "game_api.h"
#include "sim_game.cpp" // the game implementation; must come before server.h
#include "input_queue.cpp" // inputs waiting for their tick; also before server.h
#include "chat_relay.cpp" // lines players say, relayed to everyone; also before server.h
#include "server.h"

internal void
ServerSend(server *Server, net_address To, net_packet *Packet)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    if (Size && NetSendTo(&Server->Socket, To, Buffer, Size))
    {
        Server->Stats.PacketsOut++;
        Server->Stats.BytesOut += Size;
    }
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

internal void
ServerRecordTick(server *Server, double Seconds, bool32 Late)
{
    server_stats *Stats = &Server->Stats;
    Stats->Ticks++;
    Stats->TickSecondsTotal += Seconds;
    if (Seconds > Stats->TickSecondsMax) Stats->TickSecondsMax = Seconds;
    if (Late) Stats->LateTicks++;
}

internal void
ServerFormatStats(server *Server, double IntervalSeconds, char *Out, u32 OutSize)
{
    server_stats *S = &Server->Stats;
    double PerSecond = IntervalSeconds > 0 ? 1.0 / IntervalSeconds : 0;
    double AverageMs = S->Ticks ? 1000.0 * S->TickSecondsTotal / S->Ticks : 0;
    snprintf(Out, OutSize,
             "stats over %.0f s: %u/%u players, tick avg %.2f ms max %.2f ms of %.1f, %u late, "
             "in %u pkt %.1f KB (%u bad, %u full ticks), out %u pkt %.1f KB (%.1f KB/s), %u trimmed, %u capped",
             IntervalSeconds, ServerPlayerCount(Server), NET_MAX_CLIENTS,
             AverageMs, 1000.0 * S->TickSecondsMax, 1000.0 / SERVER_TICK_RATE, S->LateTicks,
             S->PacketsIn, S->BytesIn / 1024.0, S->BadPacketsIn, S->FullReceiveTicks,
             S->PacketsOut, S->BytesOut / 1024.0, S->BytesOut * PerSecond / 1024.0,
             S->TrimmedSnapshots, S->CappedSnapshots);
    *S = {};
}

#define ADDRESS_FORMAT "%u.%u.%u.%u:%u"
#define ADDRESS_ARGS(A) (A).Ip >> 24, ((A).Ip >> 16) & 255, ((A).Ip >> 8) & 255, (A).Ip & 255, (A).Port

// NOTE(zoubir): MapId picks the map (sim/maps/); 0 is the Old Arena.
// Name is what players see in their server list and HUD
internal bool32
ServerStart(server *Server, u16 Port, u32 MapId, const char *Name)
{
    *Server = {};
    snprintf(Server->Clients.ServerName, NET_SERVER_NAME_SIZE, "%s",
             Name ? Name : SERVER_DEFAULT_NAME);
    Server->Socket = NetOpenSocket(Port);
    GameInit(&Server->Game, MapId);
    Server->Clients.ContentId = GameContentId(&Server->Game);
    Server->Clients.MapId = (u8)Server->Game.AppState->World.MapId;
    // Joining takes the cookie handshake (net/connections.h); the secret
    // only has to be unguessable from outside, not strong.
    Server->Clients.Strict = true;
    Server->Clients.Secret = (u32)time(0) ^ (u32)(size_t)Server ^ ((u32)clock() << 16) ^ 0x5bd1e995u;
    return Server->Socket.Open;
}

// Who is playing, for anyone who asks (probe --info); no slot is taken.
internal void
ServerAnswerInfo(server *Server, net_address From, net_packet *Request)
{
    net_packet Reply = {};
    Reply.Header.Type = NetPacket_InfoReply;
    Reply.Header.Token = Request->Header.Token;
    Reply.InfoReply.Nonce = Request->InfoRequest.Nonce;
    Reply.InfoReply.ContentId = Server->Clients.ContentId;
    Reply.InfoReply.MapId = Server->Clients.MapId;
    Reply.InfoReply.PlayerCount = (u8)ServerPlayerCount(Server);
    Reply.InfoReply.MaxPlayers = NET_MAX_CLIENTS;
    for (u32 Index = 0; Index < NET_SERVER_NAME_SIZE; ++Index)
    {
        Reply.InfoReply.ServerName[Index] = Server->Clients.ServerName[Index];
    }
    GameListPlayers(&Server->Game, &Reply.InfoReply);
    ServerSend(Server, From, &Reply);
}

// NOTE(zoubir): the name the line goes out under is the one the game
// shows, made unique when two players picked the same
internal void
ServerTakeChat(server *Server, u32 Slot, net_chat_say *Say)
{
    char Name[NET_NAME_SIZE];
    char *GameName = Server->Game.AppState->Players[Slot].Name;
    if (GameName[0]) snprintf(Name, sizeof(Name), "%s", GameName);
    else snprintf(Name, sizeof(Name), "Player %u", Slot + 1);
    net_chat_line *Line = ChatReceive(&Server->Chat, Slot, Say, Name);
    if (Line) ServerLog(Server, "chat %u %s: %s", Slot, Line->Name, Line->Text);
}

internal void
ServerSendChat(server *Server)
{
    ChatAdvance(&Server->Chat, 1.0f / SERVER_TICK_RATE);
    net_packet Packet = {};
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_client_slot *Slot = &Server->Clients.Slots[Index];
        if (!Slot->Connected || !ChatPacketFor(&Server->Chat, Index, &Packet.ChatLines)) continue;
        NetServerStampHeader(Slot, &Packet, NetPacket_ChatLines);
        ServerSend(Server, Slot->Address, &Packet);
    }
}

internal void
ServerReceiveAll(server *Server)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    u32 Read = 0;
    while (Read < SERVER_MAX_PACKETS_PER_TICK &&
           (Size = NetReceiveFrom(&Server->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        if (++Read == SERVER_MAX_PACKETS_PER_TICK) Server->Stats.FullReceiveTicks++;
        Server->Stats.PacketsIn++;
        Server->Stats.BytesIn += Size;
        net_packet Packet;
        if (!NetReadPacket(Buffer, Size, &Packet))
        {
            Server->Stats.BadPacketsIn++;
            // NOTE(zoubir): another build of the game: say so, or its
            // player only ever sees "no answer"
            if (NetIsOtherVersion(Buffer, Size) && Size > NET_VERSION_NOTICE_SIZE)
            {
                u8 Notice[NET_VERSION_NOTICE_SIZE];
                NetWriteVersionNotice(Notice);
                if (NetSendTo(&Server->Socket, From, Notice, sizeof(Notice)))
                {
                    Server->Stats.PacketsOut++;
                    Server->Stats.BytesOut += sizeof(Notice);
                }
            }
            continue;
        }

        if (Packet.Header.Type == NetPacket_InfoRequest)
        {
            ServerAnswerInfo(Server, From, &Packet);
            continue;
        }

        net_receive_result Result = NetServerReceive(&Server->Clients, From, &Packet, Server->Tick);
        switch (Result.Event)
        {
            case NetReceive_Joined:
            {
                ClearInputQueue(&Server->InputQueues[Result.SlotIndex]);
                GamePlayerJoined(&Server->Game, Result.SlotIndex);
                GamePlayerNamed(&Server->Game, Result.SlotIndex, Result.Name);
                ChatJoined(&Server->Chat, Result.SlotIndex);
                ServerLog(Server, "player %u joined from " ADDRESS_FORMAT " (%u/%u)", Result.SlotIndex,
                          ADDRESS_ARGS(From), ServerPlayerCount(Server), NET_MAX_CLIENTS);
            } break;
            case NetReceive_Denied:
            {
                ServerLog(Server, "refused " ADDRESS_FORMAT ": %s", ADDRESS_ARGS(From),
                          Result.Reply.ConnectDenied.Reason == NetDeny_WrongVersion ?
                          "different game version" : "server full");
            } break;
            case NetReceive_Left:
            {
                ClearInputQueue(&Server->InputQueues[Result.SlotIndex]);
                GamePlayerLeft(&Server->Game, Result.SlotIndex);
                ServerLog(Server, "player %u left (%u/%u)", Result.SlotIndex,
                          ServerPlayerCount(Server), NET_MAX_CLIENTS);
            } break;
            case NetReceive_Inputs:
            {
                for (u32 Index = 0; Index < Result.NewInputCount; ++Index)
                {
                    PushInput(&Server->InputQueues[Result.SlotIndex], &Result.NewInputs[Index]);
                }
            } break;
            case NetReceive_Chat:
            {
                ServerTakeChat(Server, Result.SlotIndex, &Packet.ChatSay);
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
        u32 Waiting = Server->InputQueues[Index].Count;
        Packet.Snapshot.InputBuffered = (u8)(Waiting < 255 ? Waiting : 255);
        if (Packet.Snapshot.Count == NET_MAX_SNAPSHOT_ENTITIES) Server->Stats.CappedSnapshots++;
        // A busy moment can fill every list at once; then the farthest
        // entities wait for the next snapshot rather than the whole
        // snapshot being lost.
        u8 Buffer[NET_MAX_PACKET_SIZE];
        u32 Dropped = 0;
        u32 Size = NetWriteSnapshotFitting(&Packet, Buffer, sizeof(Buffer), &Dropped);
        if (Dropped) Server->Stats.TrimmedSnapshots++;
        if (Size && NetSendTo(&Server->Socket, Slot->Address, Buffer, Size))
        {
            Server->Stats.PacketsOut++;
            Server->Stats.BytesOut += Size;
        }
    }
}

internal void
ServerTick(server *Server)
{
    float Dt = 1.0f / SERVER_TICK_RATE;
    ServerReceiveAll(Server);
    u32 ConnectedSlots = 0;
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        if (Server->Clients.Slots[Index].Connected) ConnectedSlots |= 1u << Index;
    }
    ApplyQueuedInputs(&Server->Game, Server->InputQueues, NET_MAX_CLIENTS, ConnectedSlots);
    GameKeepBots(&Server->Game, ConnectedSlots, Dt);
    GameTick(&Server->Game, Dt);
    // NOTE(zoubir): each round may be on another map; joiners and the
    // server list are told the one being played now
    Server->Clients.MapId = (u8)Server->Game.AppState->World.MapId;
    Server->Tick++;
    if (Server->Tick % SERVER_SNAPSHOT_INTERVAL == 0) ServerSendSnapshots(Server);
    ServerSendChat(Server);

    u32 TimedOut = NetServerAdvance(&Server->Clients, Dt);
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        if (TimedOut & (1u << Index))
        {
            ClearInputQueue(&Server->InputQueues[Index]);
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
    GameShutdown(&Server->Game);
}
