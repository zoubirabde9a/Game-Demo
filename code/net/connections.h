#if !defined(NET_CONNECTIONS_H)
#define NET_CONNECTIONS_H
/* The server's table of connected clients. No sockets here: the server
   loop hands every decoded packet to NetServerReceive and sends back
   whatever reply it fills in, then calls NetServerAdvance once per tick to
   drop clients that went quiet.

   A client joins by sending ConnectRequest with a random salt. Requests
   are resent until answered, so a repeat from the same address and salt
   gets the same slot again. A slot is freed by a Disconnect packet or after
   NET_CLIENT_TIMEOUT seconds without any packet. */

#include "protocol.h"
#include "address.h"

#define NET_MAX_CLIENTS 8
#define NET_CLIENT_TIMEOUT 5.0f

struct net_client_slot
{
    bool32 Connected;
    net_address Address;
    u32 Salt;
    float SecondsSinceHeard;
    u16 NextSequence;      // sequence for the next packet we send this client
    u16 NewestReceived;    // newest sequence we got from this client, echoed as Ack
    u32 NewestInputTick;   // inputs at or before this tick were already applied
    bool32 HasInput;
};

struct net_server_clients
{
    net_client_slot Slots[NET_MAX_CLIENTS];
};

enum net_receive_event
{
    NetReceive_Ignored,   // unknown sender, stale or server-only packet
    NetReceive_Joined,    // a new client took slot SlotIndex
    NetReceive_Rejoined,  // a repeated connect request; reply already filled
    NetReceive_Denied,    // server full; reply filled
    NetReceive_Left,      // client said goodbye; slot is free again
    NetReceive_Inputs,    // NewInputs holds inputs not seen before, oldest first
};

struct net_receive_result
{
    net_receive_event Event;
    u32 SlotIndex;
    bool32 HasReply;        // send Reply back to the sender
    net_packet Reply;
    u32 NewInputCount;
    net_input NewInputs[NET_MAX_INPUTS_PER_PACKET];
};

internal net_receive_result
NetServerReceive(net_server_clients *Clients, net_address From, net_packet *Packet, u32 ServerTick);

// Ages every slot by Dt seconds and frees the ones past the timeout.
// Returns a bit mask of the slots that timed out this call.
internal u32 NetServerAdvance(net_server_clients *Clients, float Dt);

// Fills the header of a packet about to be sent to a connected client.
internal void NetServerStampHeader(net_client_slot *Slot, net_packet *Packet, u8 Type);

#endif
