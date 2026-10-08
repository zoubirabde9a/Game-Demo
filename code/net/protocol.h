#if !defined(NET_PROTOCOL_H)
#define NET_PROTOCOL_H
/* Packet format shared by the game client and the dedicated server.

   Every packet is one UDP datagram: a header, then one body chosen by
   Header.Type. Clients send ConnectRequest, Input, Chat and Disconnect.
   The server sends ConnectAccepted, ConnectDenied, Snapshot, ChatLines
   and Disconnect.

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
#define NET_PROTOCOL_ID 0x47444d71u // "GDMq", change it whenever the layout changes
// (GDMq: the dungeon role request is a byte of its own, net_input.Role, as eight classes
// and "none" do not fit the three spare bits of the held buttons)
// (GDMo: 30 talent ranks, the class talents in 3 bits, for the deeper class trees)
// (GDMm: a dungeon run sends one player's meter a snapshot)
// (GDMl: an open map vote sends every player's answer)
// (GDMk: two dungeon casts came in before the rewinds, so cast ids moved,
// and two bursts for the Giant Fireball)
// A player's health is sent in hundredths: the duel gives a player one
// point, and burns take fractions of it, which whole points would hide.
#define NET_PLAYER_HEALTH_STEPS 100.f
#define NET_MAX_PACKET_SIZE 1200    // stays under a typical internet MTU
#define NET_MAX_INPUTS_PER_PACKET 8
// The server's ticks a second. Each net_input is one tick's worth: the
// client sends one per tick, whatever its frame rate, and the server
// applies one per tick.
#define NET_TICK_RATE 60
#define NET_MAX_SNAPSHOT_ENTITIES 42 // moving things only; walls and trees are never sent (42 leaves room for the map vote, a dungeon run with its sanctuaries and infernos, and each player's class bytes)
#define NET_MAX_SNAPSHOT_ABILITIES 8 // monsters winding up or striking at once
#define NET_MAX_ABILITY_POINTS 4    // matches MAX_ABILITY_POINTS in entity.h
#define NET_MAX_SNAPSHOT_SCORES 8   // one per player slot (MAX_PLAYERS)
#define NET_MAX_SNAPSHOT_FACINGS 8  // front-armoured monsters per snapshot
#define NET_MAX_SNAPSHOT_SOUNDS 8   // sounds heard since the last snapshot
#define NET_MAX_SNAPSHOT_KILLS 4    // player deaths since the last snapshot
#define NET_MAX_SNAPSHOT_BURSTS 8   // visual bursts seen since the last snapshot
#define NET_MAX_SNAPSHOT_REWINDS 4  // time rewinds under way the viewer can see
#define NET_MAX_SNAPSHOT_CASTS 8    // players winding up a spell (MAX_PLAYERS)
#define NET_COOLDOWN_COUNT 16       // the viewer's own ability cooldowns
// NOTE(zoubir): ranks before NET_TALENT_WIDE_FIRST travel in 2 bits (a
// duel talent has at most 3), the class talents from it in 3 (at most 4)
#define NET_TALENT_WIDE_FIRST 18
#define NET_TALENT_BYTES 9
#define NET_TALENT_COUNT 30         // the viewer's own talent ranks (sim/progression/talents.cpp)
#define NET_STATUS_COUNT 10        // status effects (sim/status_effects.cpp)
#define NET_NAME_SIZE 16            // player name, 15 characters plus the terminator
#define NET_SERVER_NAME_SIZE 24     // server name, 23 characters plus the terminator
#define NET_NO_NAME_SLOT 0xff
#define NET_NO_VOTE 0xff
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
    // Anyone to the server, without joining: who is playing, on which map
    // and build. The request is padded to at least the reply's size, so
    // the server cannot be used to multiply someone else's traffic.
    NetPacket_InfoRequest,
    NetPacket_InfoReply,
    NetPacket_Chat,      // client to server: a line said, and lines heard (protocol/chat.h)
    NetPacket_ChatLines, // server to client: lines said by anyone
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
    NetButton_Blink     = 1 << 9,
    NetButton_Push      = 1 << 10,
    NetButton_Launch    = 1 << 11,
    NetButton_Slam      = 1 << 12,
    NetButton_RewindSelf   = 1 << 13,
    NetButton_RewindBubble = 1 << 14,
    NetButton_RewindWorld  = 1 << 15,
    NetButton_Shield       = 1 << 16,
    NetButton_FrostNova    = 1 << 17,
    NetButton_GravityWell  = 1 << 18,
    NetButton_Kunai        = 1 << 19,
};
// The action buttons, Jump onward, are the simulation's player_button bits
// moved up by PLAYER_BUTTON_NET_SHIFT (sim/player.h); keep the two orders
// the same.
//
// Bits NET_LEARN_SHIFT and up are not a button but a number: the talent
// the player is spending a point on, its talent_id + 1, or 0. The client
// holds it for a few inputs and lets go; the server spends a point each
// time it changes to something other than 0 (sim_game.cpp), so a lost
// input loses nothing and a replay spends the same points.
#define NET_LEARN_SHIFT 24
#define NET_LEARN_MASK 0x1fu
// Bits NET_VOTE_SHIFT to NET_LEARN_SHIFT are a map vote request
// (map_vote_request, sim/map_vote.cpp): a map asked for or an answer,
// held and let go like the talent field.
#define NET_VOTE_SHIFT 20
#define NET_VOTE_MASK 0xfu
#define NET_NO_BOSS 0xFFu
#define NET_BOSS_ENRAGED 0xFFu
#define NET_MAX_SANCTUARIES 4
#define NET_MAX_INFERNOS 4
#define NET_MAX_FOE_MARKS 4
#define NET_ADD_BURSTS 0x80u
#define NET_MARK_STACKS 7
#define NET_MARK_SUNDER 8
#define NET_ZONE_WIDE 0x80u
#define NET_INFERNO_FALLING 0x40u

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
    u32 Buttons;   // net_button bits held down
    float AimX;    // aim direction in -1..1, sent at 1/32767 precision
    float AimY;
    u16 Target;    // the unit the cursor is on: its entity Id + 1, 0 for none
    u8 Role;       // a dungeon role request (sim/dungeon/roles.cpp): the player_role
                   // picked + 1, or 0; held and let go like the talent field
};

struct net_entity_state
{
    u16 Id;
    u8 Type;
    // Facing and Animation share one byte on the wire, as do Affix,
    // Status and Ability, so each must stay within its bit count.
    u8 Facing;     // 2 bits: animation_direction
    u8 Animation;  // 4 bits: animation_type
    u8 Flash;      // 1 bit: a monster's enrage burst is playing
    u8 Variant;    // which look within the type: monster kind (also for
                   // hazards), shot style, player slot
    u8 Affix;      // 3 bits: elite affix of a monster, shot or hazard
    u16 Status;    // 11 bits: bit N set while status effect N + 1 is active
    u8 Ability;    // 2 bits: AbilityIndex of a monster shot or hazard;
                   // for a player, PLAYER_FLASH_* bits (sim/player.h)
    i16 Health;    // a player's in 1/NET_PLAYER_HEALTH_STEPS of a point
    // Sent as 16-bit fixed point: positions to 1/8 unit within +-4096,
    // velocities to 1/4 unit per second within +-8192. Values outside are
    // clamped. Z and velocity are left out when they are zero, and VelZ
    // travels with Z (24 bytes an entity at most, with the second status byte; 12 for one standing on
    // the ground). Type fits 5 bits.
    float X, Y, Z; // Z is height above the floor (jumps)
    float VelX, VelY;
    float VelZ;    // vertical speed, while off the ground
    // The unit's last hit, sent only while it is fresh (sim/hit.cpp,
    // HitFresh): 3 bytes more for a unit hit in the last quarter second.
    u8 Hit;        // 1 bit: the hit fields below are sent
    u8 HitStop;    // hit-pause left, in milliseconds
    u8 HitAngle;   // the way the hit threw it, a whole turn in 256 steps, 0 = +X
    u8 HitBy;      // 4 bits: player slot + 1 that landed it, 0 for a monster
    u8 HitThrown;  // 1 bit: the hit lifted it off its feet
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

// Padding bytes after the nonce: the largest reply body is 164 bytes
// (12 + a server name of up to 1 + 23 + 8 names of up to 1 + 15), so a
// request is never the smaller one.
#define NET_INFO_PADDING 168
struct net_info_request { u32 Nonce; };
struct net_info_reply
{
    u32 Nonce;      // echoes the request's
    u32 ContentId;  // the server build's SimContentId()
    u8 MapId;
    u8 PlayerCount;
    u8 MaxPlayers;
    char ServerName[NET_SERVER_NAME_SIZE]; // what the server calls itself (server --name)
    u8 NameCount;   // names of the connected players, in slot order
    char Names[NET_MAX_SNAPSHOT_SCORES][NET_NAME_SIZE];
};
// NOTE(zoubir): MapId is the server's map_id; the client builds the same
// ground from it (terrain never crosses the wire)
struct net_connect_accepted
{
    u32 ClientSalt;
    u8 PlayerIndex;
    u32 ServerTick;
    u8 MapId;
    char ServerName[NET_SERVER_NAME_SIZE]; // shown in the HUD while joined
};
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

// A player winding up a spell (sim/player_casts.cpp), so every client can
// draw the cast bar over it. Spell is a player_spell; Done is how much of
// the wind-up has passed, 0..255.
struct net_player_cast
{
    u8 EntityIndex; // index into net_snapshot.Entities
    u8 Spell;
    u8 Done;
};

// One connected player's score, so every client can show the scoreboard.
struct net_score
{
    u8 Slot;
    u16 Kills;        // other players killed
    u16 Deaths;
    u16 MonsterKills;
    u8 Level;         // sim/progression/experience.cpp
    u8 Ward;          // 1 bit: the Ward talent's charge is up
    // In a dungeon run (sim/dungeon/), 0 elsewhere: player_role in bits
    // 0-1, Shield Wall bit 2, a ward bit 3, revive progress bits 4-7.
    u8 Dungeon;
    // A rally bit 0, Renewal bit 1, monsters after them (0..7) bits 2-4,
    // player_role's third bit in bit 5 (sim/dungeon/role_abilities.cpp).
    u8 DungeonMore;
    u8 ClassMeter; // player_slot's (sim/dungeon/dungeon_slot_fields.inc)
    u8 ClassFlags;
};

struct net_kill
{
    u8 Killer;        // player slot, 0xFF for none
    u8 Victim;        // player slot
    u8 KillerMonster; // monster kind, 0xFF for none
};

// A visual burst (sim_burst in sim/events.h) the simulation asked for.
struct net_burst
{
    u8 Kind;
    u8 Slot;        // player that caused it, 0xFF for none
    u8 Angle;       // a whole turn in 256 steps, 0 = +X, 64 = +Y
    float X, Y, Z;
};

// A time rewind under way (sim/time_rewind/), so clients can draw its
// cast, its freeze and its playback, and leave a frozen local player
// where the server puts it. 13 bytes.
struct net_rewind
{
    u8 Slot;        // 3 bits: the caster's player slot
    u8 Kind;        // 2 bits: rewind_kind
    u8 Phase;       // 2 bits: rewind_phase, never None
    float PhaseLeft; // seconds left in the phase, sent in 4 ms steps up to 1.02 s
    float X, Y;     // the caster's feet, or the bubble's centre once it holds
    float Radius;   // the bubble's, sent in 2-unit steps up to 510
    // Bit N set: the snapshot's entity N is frozen by this rewind. A world
    // rewind freezes everything and sets none.
    u8 Frozen[(NET_MAX_SNAPSHOT_ENTITIES + 7) / 8];
};

struct net_snapshot
{
    u32 Tick;
    // Newest net_input.Tick from this client that the server had applied
    // when it wrote the snapshot; the client replays its inputs after it.
    u32 InputTick;
    // This client's inputs that had arrived but were still waiting for
    // their tick; the client paces its inputs to keep one or two waiting.
    u8 InputBuffered;
    // Where the snapshot's positions are measured from on the wire: the
    // viewer's player, or the map's middle without one. Positions in this
    // struct are whole-map ones; the client needs nothing from this.
    float OriginX, OriginY;
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
    // The viewer's own ability cooldowns, each 0..255 of that ability's full
    // cooldown (0 = ready), for the HUD; abilities run only on the server.
    // Which is which: PlayerCooldown in sim/player_cooldowns.cpp.
    u8 Cooldowns[NET_COOLDOWN_COUNT];
    // The viewer's own experience and talent ranks (sim/progression/),
    // each rank 0..3 and sent in 2 bits. Everyone's level is in Scores.
    u16 Xp;
    u8 TalentRanks[NET_TALENT_COUNT];
    // The viewer's own stagger from a shove (sim/hit.cpp), 0..255 of
    // PlayerStats.StaggerSeconds; its prediction replays from it, or every
    // shove would be braked away on the client and pulled back.
    u8 Stagger;
    // The break between rounds (sim/round_break.cpp): tenths of a second
    // left, 0 while a round is played. The same for every viewer.
    u8 RoundBreak;
    // The map being played (sim/maps/); it changes between rounds, and the
    // client builds the new one's ground when it does (client/online.cpp).
    u8 MapId;
    // The map vote (sim/map_vote.cpp): the map asked for, NET_NO_VOTE
    // while none is open; who asked; whole seconds left; the yes and no
    // answers so far; the viewer's own answer (map_vote_request); and
    // every slot's answer, two bits each, slot 0 lowest, so each player
    // sees who has voted what. Sent only while a vote is open, the slot,
    // answer and counts in 4 bits.
    u8 VoteMap;
    u8 VoteBy;
    u8 VoteSeconds;
    u8 VoteYes;
    u8 VoteNo;
    u8 OwnVote;
    u16 VoteAnswers;
    // A dungeon run (sim/dungeon/), HasDungeon 0 on any other map: the
    // room being fought (0 for none), the rooms cleared (room N in bit
    // N - 1), the wipes, the fight's boss kind (NET_NO_BOSS for none) and
    // its health in 0..255 of its most, and the monsters left in the
    // fight. The same for every viewer; the rest is sent only in a run.
    u8 HasDungeon;
    u8 FightingRoom;
    u8 RoomsCleared;
    u8 Wipes;
    u8 BossKind;
    u8 BossHealth;
    u8 FoesLeft;
    // The boss's enrage timer (sim/dungeon/boss_clock.cpp): whole seconds
    // left, NET_BOSS_ENRAGED once it ran out, 0 with no timer running.
    u8 BossClock;
    // The boss's timed add soonest to go (sim/dungeon/boss_clock.cpp):
    // whole seconds left in bits 0-6, 0 for none, and NET_ADD_BURSTS when
    // it erupts on the party as it goes.
    u8 AddClock;
    // The healers' sanctuaries on the ground (sim/dungeon/
    // role_abilities.cpp): whole-unit positions and tenths of a second
    // left (bits 0-6), so clients draw them; bit 7 is a sanctuary a
    // talent widened (NET_ZONE_WIDE).
    u8 SanctuaryCount;
    i16 SanctuaryX[NET_MAX_SANCTUARIES];
    i16 SanctuaryY[NET_MAX_SANCTUARIES];
    u8 SanctuaryTenths[NET_MAX_SANCTUARIES];
    // The damage role's Meteors: where, and tenths of a second (bits
    // 0-5) until the meteor lands while bit 6 (NET_INFERNO_FALLING) is
    // set, else of burning ground left.
    u8 InfernoCount;
    i16 InfernoX[NET_MAX_INFERNOS];
    i16 InfernoY[NET_MAX_INFERNOS];
    u8 InfernoTenths[NET_MAX_INFERNOS];
    // The foe marks (sim/dungeon/role_kits/foe_marks.cpp) on the monsters
    // nearest the viewer (four, all a full packet has room for): the
    // entity Id (13 bits) and the mark bits, the
    // striker's Searing stacks (NET_MARK_STACKS) and the tank's Sunder
    // (NET_MARK_SUNDER), two bytes each.
    u8 MarkCount;
    u16 MarkId[NET_MAX_FOE_MARKS];
    u8 MarkBits[NET_MAX_FOE_MARKS];
    // With HasMeter, one player's meter (sim/dungeon/meter.cpp): slot, fights started
    // (0..31), the fight's tenths of a second, and damage, healing, taken, whole points.
    u8 HasMeter, MeterSlot, MeterFight;
    u16 MeterTenths, MeterDamage, MeterHealing, MeterTaken;
    // The viewer's own player exactly: position and velocity as floats.
    // Entities are sent rounded to 1/8 unit, and a prediction replayed
    // from a rounded start went round a wall's corner the other way from
    // the server, so the player shook along the map's edges. HasOwnBody is
    // 0 when the viewer has no player.
    u8 HasOwnBody;
    float OwnPosition[3];
    float OwnVelocity[3];
    // The viewer's own status clocks exactly (sim/status_effects.cpp),
    // entry N for status effect N + 1, below 0 while it is shrugged off;
    // only the nonzero ones are sent. Its prediction replays from them,
    // so a haste or a soak ends on the same tick as on the server.
    float OwnStatus[NET_STATUS_COUNT];
    // Bursts seen near this player since its previous snapshot; cosmetic,
    // lost with their snapshot like sounds.
    u8 BurstCount;
    net_burst Bursts[NET_MAX_SNAPSHOT_BURSTS];
    // Rewinds under way near this player (all of a world rewind's), so a
    // lost snapshot loses nothing: the next one says the same.
    u8 RewindCount;
    net_rewind Rewinds[NET_MAX_SNAPSHOT_REWINDS];
    // Players in the snapshot winding up a spell.
    u8 CastCount;
    net_player_cast Casts[NET_MAX_SNAPSHOT_CASTS];
};

#include "protocol/chat.h"

struct net_packet
{
    net_header Header;
    union
    {
        net_connect_request ConnectRequest;
        net_connect_accepted ConnectAccepted;
        net_connect_denied ConnectDenied;
        net_connect_challenge ConnectChallenge;
        net_info_request InfoRequest;
        net_info_reply InfoReply;
        net_input_batch Input;
        net_snapshot Snapshot;
        net_chat_say ChatSay;
        net_chat_lines ChatLines;
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

#include "protocol/version_notice.h"

// True if sequence A is newer than B, treating the u16 counter as wrapping.
internal bool32 NetSequenceNewer(u16 A, u16 B);

#endif
