#if !defined(SERVER_H)
#define SERVER_H
/* The dedicated server. It owns one UDP socket and the client table, and
   runs the game at a fixed SERVER_TICK_RATE. Each ServerTick:

     1. reads every waiting datagram and answers joins, leaves and inputs
     2. advances the game by one tick
     3. every SERVER_SNAPSHOT_INTERVAL ticks, sends each client a snapshot
     4. frees clients that have gone quiet

   The game itself sits behind game_api.h, so the server does not know
   whether it is running the real simulation or the placeholder. */

#include "../net/protocol.h"
#include "../net/connections.h"
#include "../net/socket.h"
#include "game_api.h" // struct server_game must be complete before this header

#define SERVER_DEFAULT_PORT 27015
// NOTE(zoubir): unnamed; clients then call it by its name in their server
// list (client/server_list.cpp), or by its address
#define SERVER_DEFAULT_NAME ""
#define SERVER_TICK_RATE 60
#define SERVER_SNAPSHOT_INTERVAL 3 // 20 snapshots a second
#define SERVER_STATS_SECONDS 60    // how often server_main logs a stats line
// Packets read per tick at most. Eight players send a few each; a flood
// beyond this waits in the socket buffer (or is dropped by the system)
// instead of keeping the tick from running.
#define SERVER_MAX_PACKETS_PER_TICK 256

// Counters since the last stats line. A tick is "late" when the loop could
// not sleep before the next one: the machine is not keeping up.
struct server_stats
{
    u32 Ticks;
    u32 LateTicks;
    double TickSecondsTotal;
    double TickSecondsMax;
    u32 PacketsIn, PacketsOut, BadPacketsIn;
    u32 TrimmedSnapshots; // snapshots that left far entities out to fit
    u32 CappedSnapshots;  // snapshots with more nearby than NET_MAX_SNAPSHOT_ENTITIES
    u32 FullReceiveTicks; // ticks that stopped reading at SERVER_MAX_PACKETS_PER_TICK
    u64 BytesIn, BytesOut;
};

struct server
{
    net_socket Socket;
    net_server_clients Clients;
    server_game Game;
    u32 Tick;
    bool32 Logging; // print joins, leaves, timeouts and stats to stdout
    server_stats Stats;
};

// Opens the socket. Port 0 picks a free one (tests use this).
// MapId picks the map (sim/maps/); 0 is the Old Arena.
internal bool32 ServerStart(server *Server, u16 Port, u32 MapId = 0, const char *Name = 0);
internal void ServerTick(server *Server);
// The caller times each ServerTick and reports it here.
internal void ServerRecordTick(server *Server, double Seconds, bool32 Late);
// Writes one stats line covering the last IntervalSeconds, then resets the counters.
internal void ServerFormatStats(server *Server, double IntervalSeconds, char *Out, u32 OutSize);
// Tells every client the server is going away, then closes the socket.
internal void ServerStop(server *Server);

#endif
