#if !defined(NET_CLIENT_H)
#define NET_CLIENT_H
/* The client end of a connection to the dedicated server. The game calls
   NetClientConnect once, NetClientPoll every frame, NetClientQueueInput
   once per game tick with the buttons the player holds, NetClientFlushInputs
   after them, and reads Snapshot to draw the world.

   While connecting, the request is resent every NET_CONNECT_RETRY seconds
   until the server answers or NET_CONNECT_GIVE_UP seconds pass. Once
   connected, every input packet carries the last few inputs, so a lost packet is
   covered by the next one. Only snapshots newer than the one held are
   kept. If the server goes quiet for NET_CLIENT_TIMEOUT seconds the client
   gives up and reports why in EndReason. */

#include "protocol.h"
#include "address.h"
#include "socket.h"

#define NET_CONNECT_RETRY 0.25f
#define NET_CONNECT_GIVE_UP 5.0f

enum net_client_state
{
    NetClient_Disconnected,
    NetClient_Connecting,
    NetClient_Connected,
};

enum net_client_end
{
    NetEnd_None,
    NetEnd_NoAnswer,       // the server never replied to the connect request
    NetEnd_ServerFull,
    NetEnd_ServerClosed,   // the server said goodbye
    NetEnd_LostConnection, // nothing heard from the server for too long
    NetEnd_LeftByChoice,
    NetEnd_WrongVersion,   // the server runs a build with different content or protocol
};

struct net_client
{
    net_client_state State;
    net_client_end EndReason;
    u8 PlayerIndex;        // our slot on the server, valid once connected
    u8 MapId;              // the server's map_id, valid once connected
    char ServerName[NET_SERVER_NAME_SIZE]; // what the server calls itself, valid once connected

    bool32 HasSnapshot;
    net_snapshot Snapshot; // newest world state from the server

    // Implementation detail below; read the fields above.
    net_socket Socket;
    net_address Server;
    u32 Salt;
    u32 ContentId;
    u32 Cookie; // from the server's ConnectChallenge, sent back in requests
    char Name[NET_NAME_SIZE];
    float SecondsSinceHeard;
    float SecondsConnecting;
    float RetryTimer;
    u16 NextSequence;
    u16 NewestReceived;
    u32 InputTick;
    u32 RecentInputCount;
    net_input RecentInputs[NET_MAX_INPUTS_PER_PACKET]; // newest first
    u32 InputsUnsent; // queued since the last input packet
};

// Resolves "host:port", "host" (SERVER port 27015) or "a.b.c.d:port" to
// an address. A host name needs a DNS lookup, which blocks until it
// answers, so call it when the player asks to connect, not every frame.
#define NET_DEFAULT_PORT 27015
internal bool32 NetResolveServer(const char *Text, net_address *Out);

// Opens a socket and starts connecting. Salt should be random per launch.
// ContentId is SimContentId() for a game client, 0 for a tool.
// Name is what other players see; empty shows as "Player N".
internal bool32 NetClientConnect(net_client *Client, net_address Server, u32 Salt, u32 ContentId,
                                 const char *Name = "");

// Reads everything from the server, resends the connect request when it
// is due, and gives up on a silent server. Call once per frame.
internal void NetClientPoll(net_client *Client, float Dt);

// Adds one input, one server tick's worth (NET_TICK_RATE), to the inputs
// the next packet carries, and returns its tick (0 when not connected).
internal u32 NetClientQueueInput(net_client *Client, u32 Buttons, float AimX, float AimY);

// Sends the queued inputs, with the ones before them again, in one packet.
// Nothing is sent when none was queued since the last call.
internal void NetClientFlushInputs(net_client *Client);

// Poll, one input and flush: for tools and tests that run one frame per
// server tick.
internal void NetClientUpdate(net_client *Client, float Dt, u32 Buttons, float AimX, float AimY);

// Tells the server we are leaving and closes the socket.
internal void NetClientDisconnect(net_client *Client);

#endif
