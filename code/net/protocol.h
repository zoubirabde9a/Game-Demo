#if !defined(NET_PROTOCOL_H)
#define NET_PROTOCOL_H
/* Packet format shared by the game client and the dedicated server.

   Every packet is one UDP datagram: a header, then one body chosen by
   Header.Type. Clients send ConnectRequest, Input and Disconnect. The
   server sends ConnectAccepted, ConnectDenied, Snapshot and Disconnect.

   Input packets carry the client's last few inputs, newest first, so a
   lost packet costs nothing as long as a later one arrives. Snapshots are
   never resent; the next one replaces them.

   NetWritePacket and NetReadPacket are the whole interface. The byte
   layout (little-endian, fixed-size fields) lives in protocol.cpp.
   NetReadPacket rejects anything malformed, so the server can feed it raw
   datagrams from the internet. */

#include "../app_defs.h"

#define NET_PROTOCOL_ID 0x47444d34u // "GDM4", change it whenever the layout changes
#define NET_MAX_PACKET_SIZE 1200    // stays under a typical internet MTU
#define NET_MAX_INPUTS_PER_PACKET 8
#define NET_MAX_SNAPSHOT_ENTITIES 48 // moving things only; walls and trees are never sent
#define NET_MAX_SNAPSHOT_ABILITIES 8 // monsters winding up or striking at once
#define NET_MAX_ABILITY_POINTS 4    // matches MAX_ABILITY_POINTS in entity.h
#define NET_MAX_SNAPSHOT_SCORES 8   // one per player slot (MAX_PLAYERS)
#define NET_CLIENT_TIMEOUT 5.0f     // seconds of silence before either side gives up

enum net_packet_type
{
    NetPacket_Invalid,
    NetPacket_ConnectRequest,
    NetPacket_ConnectAccepted,
    NetPacket_ConnectDenied,
    NetPacket_Disconnect,
    NetPacket_Input,
    NetPacket_Snapshot,
    NetPacket_Count,
};

enum net_button
{
    NetButton_Left      = 1 << 0,
    NetButton_Right     = 1 << 1,
    NetButton_Up        = 1 << 2,
    NetButton_Down      = 1 << 3,
    NetButton_Jump      = 1 << 4,
    NetButton_Dash      = 1 << 5,
    NetButton_Fireball  = 1 << 6,
    NetButton_Sword     = 1 << 7,
    NetButton_Shockwave = 1 << 8,
};

enum net_deny_reason
{
    NetDeny_ServerFull = 1,
    NetDeny_WrongVersion,
};

struct net_header
{
    u8 Type;       // net_packet_type
    u16 Sequence;  // sender's packet counter, wraps around
    u16 Ack;       // newest sequence received from the other side
};

struct net_input
{
    u32 Tick;      // client tick this input was sampled on
    u16 Buttons;   // net_button bits held down
    float AimX;    // aim direction in -1..1, sent at 1/32767 precision
    float AimY;
};

struct net_entity_state
{
    u16 Id;
    u8 Type;
    u8 Facing;
    u8 Animation;
    u8 Variant;    // which look within the type: monster kind, shot style, player slot
    i16 Health;
    // Sent as 16-bit fixed point: positions to 1/8 unit within +-4096,
    // velocities to 1/4 unit per second within +-8192. Values outside are clamped.
    float X, Y, Z; // Z is height above the floor (jumps)
    float VelX, VelY;
};

// A monster ability being telegraphed or carried out, so clients can draw
// the warning (aim line, target circles) before the hit lands.
struct net_ability_state
{
    u8 EntityIndex;    // index into net_snapshot.Entities of the monster using it
    u8 Phase;          // ability_phase: windup or active
    u8 Ability;        // which of the monster kind's abilities
    float TimeLeft;    // seconds left in this phase, sent in milliseconds
    float AimX, AimY;  // unit direction
    u8 PointCount;
    float PointX[NET_MAX_ABILITY_POINTS]; // target spots on the ground
    float PointY[NET_MAX_ABILITY_POINTS];
};

// The salt is a random number the client picks; the server echoes it so a
// client can tell its own reply from a stale or spoofed one.
// ContentId: the client build's SimContentId(); 0 for tools such as the
// health probe, which never read snapshot contents.
struct net_connect_request { u32 ClientSalt; u32 ContentId; };
struct net_connect_accepted { u32 ClientSalt; u8 PlayerIndex; u32 ServerTick; };
struct net_connect_denied { u32 ClientSalt; u8 Reason; };

struct net_input_batch
{
    u8 Count;
    net_input Inputs[NET_MAX_INPUTS_PER_PACKET]; // newest first
};

// One connected player's score, so every client can show the scoreboard.
struct net_score
{
    u8 Slot;
    u16 Kills;        // other players killed
    u16 Deaths;
    u16 MonsterKills;
};

struct net_snapshot
{
    u32 Tick;
    u16 Count;
    net_entity_state Entities[NET_MAX_SNAPSHOT_ENTITIES];
    u8 AbilityCount;
    net_ability_state Abilities[NET_MAX_SNAPSHOT_ABILITIES];
    u8 ScoreCount;
    net_score Scores[NET_MAX_SNAPSHOT_SCORES];
};

struct net_packet
{
    net_header Header;
    union
    {
        net_connect_request ConnectRequest;
        net_connect_accepted ConnectAccepted;
        net_connect_denied ConnectDenied;
        net_input_batch Input;
        net_snapshot Snapshot;
    };
};

// Returns the number of bytes written, or 0 if the packet is invalid or does not fit.
internal u32 NetWritePacket(net_packet *Packet, u8 *Buffer, u32 BufferSize);

// Returns true and fills Packet if Buffer holds exactly one well-formed packet.
internal bool32 NetReadPacket(u8 *Buffer, u32 Size, net_packet *Packet);

// True if sequence A is newer than B, treating the u16 counter as wrapping.
internal bool32 NetSequenceNewer(u16 A, u16 B);

#endif
