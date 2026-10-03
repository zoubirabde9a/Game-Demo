#if !defined(NET_CLIENT_H)
#define NET_CLIENT_H
/* The client end of a connection to the dedicated server. The game calls
   NetClientConnect once, NetClientUpdate every frame with the buttons the
   player holds, and reads Snapshot to draw the world.

   While connecting, the request is resent every NET_CONNECT_RETRY seconds
   until the server answers or NET_CONNECT_GIVE_UP seconds pass. Once
   connected, every update sends the last few inputs, so a lost packet is
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
};

struct net_client
{
    net_client_state State;
    net_client_end EndReason;
    u8 PlayerIndex;        // our slot on the server, valid once connected

    bool32 HasSnapshot;
    net_snapshot Snapshot; // newest world state from the server

    // Implementation detail below; read the fields above.
    net_socket Socket;
    net_address Server;
    u32 Salt;
    float SecondsSinceHeard;
    float SecondsConnecting;
    float RetryTimer;
    u16 NextSequence;
    u16 NewestReceived;
    u32 InputTick;
    u32 RecentInputCount;
    net_input RecentInputs[NET_MAX_INPUTS_PER_PACKET]; // newest first
};

// Opens a socket and starts connecting. Salt should be random per launch.
internal bool32 NetClientConnect(net_client *Client, net_address Server, u32 Salt);

// Reads everything from the server, then sends this frame's input (or a
// connect retry). Call once per frame with the buttons held this frame.
internal void NetClientUpdate(net_client *Client, float Dt, u16 Buttons, float AimX, float AimY);

// Tells the server we are leaving and closes the socket.
internal void NetClientDisconnect(net_client *Client);

#endif
