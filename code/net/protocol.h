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

// TestWireLayoutIsPinned (net_tests.cpp) fails when the bytes on the wire
// change and this does not.
#define NET_PROTOCOL_ID 0x47444d44u // "GDMD", change it whenever the layout changes
#define NET_MAX_PACKET_SIZE 1200    // stays under a typical internet MTU
#define NET_MAX_INPUTS_PER_PACKET 8
#define NET_MAX_SNAPSHOT_ENTITIES 48 // moving things only; walls and trees are never sent
#define NET_MAX_SNAPSHOT_ABILITIES 8 // monsters winding up or striking at once
#define NET_MAX_ABILITY_POINTS 4    // matches MAX_ABILITY_POINTS in entity.h
#define NET_MAX_SNAPSHOT_SCORES 8   // one per player slot (MAX_PLAYERS)
#define NET_MAX_SNAPSHOT_FACINGS 8  // front-armoured monsters per snapshot
#define NET_MAX_SNAPSHOT_SOUNDS 8   // sounds heard since the last snapshot
#define NET_MAX_SNAPSHOT_KILLS 4    // player deaths since the last snapshot
#define NET_NAME_SIZE 16            // player name, 15 characters plus the terminator
#define NET_NO_NAME_SLOT 0xff
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
    // Server to a would-be client: send your request again with this
    // cookie. Proves the sender receives at its address before it gets a
    // slot (see connections.h).
    NetPacket_ConnectChallenge,
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
    // The client's salt, on every packet both ways. It never travels
    // anywhere else, so a packet with the wrong one did not come from this
    // connection's other end, whatever its source address says.
    u32 Token;
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
    // Facing and Animation share one byte on the wire, as do Affix,
    // Status and Ability, so each must stay within its bit count.
    u8 Facing;     // 2 bits: animation_direction
    u8 Animation;  // 4 bits: animation_type
    u8 Variant;    // which look within the type: monster kind (also for
                   // hazards), shot style, player slot
    u8 Affix;      // 3 bits: elite affix of a monster, shot or hazard
    u8 Status;     // 3 bits: bit N set while status effect N + 1 is active
    u8 Ability;    // 2 bits: AbilityIndex of a monster shot or hazard;
                   // for a player, PLAYER_FLASH_* bits (sim/player.h)
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
// Cookie is 0 until the server's ConnectChallenge supplies one.
struct net_connect_request { u32 ClientSalt; u32 ContentId; u32 Cookie; char Name[NET_NAME_SIZE]; };
struct net_connect_challenge { u32 ClientSalt; u32 Cookie; };
// NOTE(zoubir): MapId is the server's map_id; the client builds the same
// ground from it (terrain never crosses the wire)
struct net_connect_accepted { u32 ClientSalt; u8 PlayerIndex; u32 ServerTick; u8 MapId; };
struct net_connect_denied { u32 ClientSalt; u8 Reason; };

struct net_input_batch
{
    u8 Count;
    net_input Inputs[NET_MAX_INPUTS_PER_PACKET]; // newest first
};

// Which way a front-armoured monster faces, so the client draws its shell
// on the right side. Angle is a whole turn in 256 steps, 0 = +X, 64 = +Y.
struct net_facing
{
    u8 EntityIndex; // index into net_snapshot.Entities
    u8 Angle;
};

// One connected player's score, so every client can show the scoreboard.
struct net_score
{
    u8 Slot;
    u16 Kills;        // other players killed
    u16 Deaths;
    u16 MonsterKills;
};

struct net_kill
{
    u8 Killer;        // player slot, 0xFF for none
    u8 Victim;        // player slot
    u8 KillerMonster; // monster kind, 0xFF for none
};

struct net_snapshot
{
    u32 Tick;
    // Newest net_input.Tick from this client that the server had applied
    // when it wrote the snapshot; the client replays its inputs after it.
    u32 InputTick;
    u16 Count;
    net_entity_state Entities[NET_MAX_SNAPSHOT_ENTITIES];
    u8 AbilityCount;
    net_ability_state Abilities[NET_MAX_SNAPSHOT_ABILITIES];
    u8 ScoreCount;
    net_score Scores[NET_MAX_SNAPSHOT_SCORES];
    // One player's name per snapshot, taking turns, so names cost a few
    // bytes a tick yet every client has them all within MAX_PLAYERS ticks.
    // NameSlot is NET_NO_NAME_SLOT when there is none.
    u8 NameSlot;
    char Name[NET_NAME_SIZE];
    u8 FacingCount;
    net_facing Facings[NET_MAX_SNAPSHOT_FACINGS];
    // Sounds the simulation played near this player since its previous
    // snapshot, as asset type ids (asset_type_id). A lost snapshot loses
    // its sounds; they are not worth resending.
    u8 SoundCount;
    u8 Sounds[NET_MAX_SNAPSHOT_SOUNDS];
    // Player deaths anywhere since this player's previous snapshot, for
    // the kill feed. Slots and monster kinds; 0xFF means nobody.
    u8 KillCount;
    net_kill Kills[NET_MAX_SNAPSHOT_KILLS];
};

struct net_packet
{
    net_header Header;
    union
    {
        net_connect_request ConnectRequest;
        net_connect_accepted ConnectAccepted;
        net_connect_denied ConnectDenied;
        net_connect_challenge ConnectChallenge;
        net_input_batch Input;
        net_snapshot Snapshot;
    };
};

// Returns the number of bytes written, or 0 if the packet is invalid or does not fit.
internal u32 NetWritePacket(net_packet *Packet, u8 *Buffer, u32 BufferSize);

// Returns true and fills Packet if Buffer holds exactly one well-formed packet.
internal bool32 NetReadPacket(u8 *Buffer, u32 Size, net_packet *Packet);

// Writes a snapshot packet, dropping its last (farthest) entities and
// whatever points at them until it fits; returns the size, 0 if even one
// entity does not fit. Sets *Dropped to how many entities were left out.
internal u32 NetWriteSnapshotFitting(net_packet *Packet, u8 *Buffer, u32 BufferSize,
                                     u32 *Dropped);

// True if sequence A is newer than B, treating the u16 counter as wrapping.
internal bool32 NetSequenceNewer(u16 A, u16 B);

#endif
