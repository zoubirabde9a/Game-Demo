#if !defined(SIM_TIME_REWIND_H)
/* Time rewind: the types. How it works is at the top of time_rewind.cpp. */

// NOTE(zoubir): the three rewinds, one key each; the order of
// RewindAbilities and of a player's RewindCooldowns
enum rewind_kind
{
    RewindKind_Self,   // the caster alone
    RewindKind_Bubble, // everything within the bubble around the caster
    RewindKind_World,  // the whole world, every player and monster
    RewindKind_Count
};
static_assert(RewindKind_Count <= PLAYER_REWIND_SLOTS, "one cooldown each");

enum rewind_phase
{
    RewindPhase_None,
    RewindPhase_Cast,     // the caster winds up; nothing is frozen yet
    RewindPhase_Hold,     // what the rewind takes is frozen in place
    RewindPhase_Playback, // it runs backwards through its history
    RewindPhase_Count
};

// NOTE(zoubir): how far back a rewind goes, and the three phases. The
// playback shows the REWIND_SECONDS of history at REWIND_PLAYBACK_SPEED,
// so it lasts REWIND_SECONDS / REWIND_PLAYBACK_SPEED. The cast is the
// caster's wind-up (sim/player_casts.cpp), the same for all three
#define REWIND_SECONDS 2.f
#define REWIND_CAST_SECONDS (PlayerSpells[PlayerSpell_RewindSelf].CastTime)
#define REWIND_HOLD_SECONDS 0.5f
#define REWIND_PLAYBACK_SPEED 4.f
#define REWIND_PLAYBACK_SECONDS (REWIND_SECONDS / REWIND_PLAYBACK_SPEED)

// NOTE(zoubir): the history: a frame every REWIND_RECORD_INTERVAL, of
// every moving entity. 128 frames at 30 a second are 4.2 s, enough for
// the 2 s a rewind goes back plus the second a bubble's cast and hold
// take while the rest of the world keeps being recorded. The records are
// whole entities (712 bytes): 8192 of them are 5.8 MB, 64 moving
// entities per frame on average before the oldest frames go early
#define REWIND_RECORD_INTERVAL (1.f / 30.f)
#define REWIND_HISTORY_FRAMES 128
#define REWIND_RECORD_POOL 8192
// NOTE(zoubir): the most entities one bubble takes
#define REWIND_MAX_AFFECTED 192
// NOTE(zoubir): room for the part of monster_population a world rewind
// puts back (everything before its animation tables)
#define REWIND_POPULATION_BYTES 1536

struct rewind_ability
{
    u32 Button;
    // NOTE(zoubir): seconds before the next use, counted from the press
    float Cooldown;
    // NOTE(zoubir): the bubble's reach around the caster; 0 for the others
    float Radius;
};

// NOTE(zoubir): one moment of the world. Its entities are RecordCount
// whole copies from FirstRecord on in time_rewind.Records (never split
// across the end of the ring)
struct rewind_frame
{
    // NOTE(zoubir): time_rewind.Clock when it was taken
    float Time;
    u32 FirstRecord;
    u32 RecordCount;
    // NOTE(zoubir): what a world rewind puts back besides the entities
    float RespawnTimers[MAX_PLAYERS];
    u8 Population[REWIND_POPULATION_BYTES];
};

// NOTE(zoubir): one player's rewind, from the press to the end of its
// playback
struct rewind_cast
{
    rewind_kind Kind;
    rewind_phase Phase;
    float PhaseLeft;
    // NOTE(zoubir): the caster's feet; a bubble takes everything within
    // Radius of it when the hold starts, and stays there
    v3 Centre;
    float Radius;
    // NOTE(zoubir): time_rewind.Clock when the hold started: the playback
    // runs back from there to REWIND_SECONDS before it, and Cursor is the
    // moment it shows now
    float HoldClock;
    float Cursor;
    // NOTE(zoubir): what the hold took, by entity ID and RewindSerial
    // (0 once it is gone). Empty for a world rewind, which takes all
    u32 AffectedCount;
    u32 AffectedID[REWIND_MAX_AFFECTED];
    u32 AffectedSerial[REWIND_MAX_AFFECTED];
};

struct time_rewind
{
    // NOTE(zoubir): seconds of simulation since the world was made; a
    // world rewind sets it back with everything else
    float Clock;
    float SinceRecord;
    u32 NextSerial;
    // NOTE(zoubir): oldest at FirstFrame
    rewind_frame Frames[REWIND_HISTORY_FRAMES];
    u32 FirstFrame;
    u32 FrameCount;
    world_entity *Records;
    u32 RecordCapacity;
    // NOTE(zoubir): where the next frame's records go
    u32 RecordHead;
    rewind_cast Casts[MAX_PLAYERS];
    // NOTE(zoubir): by entity ID, the RewindSerial a hold or playback has
    // frozen there, 0 for none: that entity is outside time, it neither
    // acts nor can be hurt (IsTimeLocked)
    u32 LockedSerial[ArrayCount(((world *)0)->Entities)];
    // NOTE(zoubir): a world rewind is holding or playing back: the whole
    // simulation stops but for it (SimulateTick)
    bool32 WorldFrozen;
};

#define SIM_TIME_REWIND_H
#endif
