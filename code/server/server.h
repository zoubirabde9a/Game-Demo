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
#define SERVER_TICK_RATE 60
#define SERVER_SNAPSHOT_INTERVAL 3 // 20 snapshots a second

struct server
{
    net_socket Socket;
    net_server_clients Clients;
    server_game Game;
    u32 Tick;
};

// Opens the socket. Port 0 picks a free one (tests use this).
internal bool32 ServerStart(server *Server, u16 Port);
internal void ServerTick(server *Server);
// Tells every client the server is going away, then closes the socket.
internal void ServerStop(server *Server);

#endif
