/* Replays: every call the server makes into its game (a player joining,
   being named, leaving, each input, each tick), in order, with the world
   hash (sim/world_hash.cpp) after every tick. The simulation decides
   everything from those calls alone: its random numbers come from the
   monster population's seeded series, never the clock. So feeding the
   same calls to a fresh game rebuilds the match tick by tick, and the
   hashes prove it did, or name the first tick where it did not.

   A player held for a tick because its input had not arrived
   (server/input_queue.cpp) is recorded too, since it changes the world.

   Bots are recorded like humans: what they decide arrives as inputs, so
   a replay needs no bot code. Rewinds are part of the simulation and
   replay like everything else.

   server --record <file> writes one; build\replay.exe <file> plays it
   back and checks it (tools/replay_main.cpp). The tests record into
   memory (tests/replay_tests.cpp).

   Size. A full server (8 players) makes about 4 MB an hour: an input is
   written only when it changes what the game keeps (GameApplyInput asks
   the game, not the last input), aims at the wire's precision
   (GameApplyInput rounds every aim to it, bots' too), ticks in runs, and the whole stream is LZMA-compressed
   (third_party/lzma) in blocks of REPLAY_BLOCK_SIZE, about half a minute
   of play each. A block goes to disk as soon as it is full, so a crash
   loses at most that. Measured on a two-minute 8-bot recording: the
   earlier uncompressed layout made 29.6 MB an hour, LZMA alone on it
   7.9, the lean layout alone 7.4, both 4.6 (zlib would make 5.3). A
   recording stops at its cap (REPLAY_DEFAULT_CAP, server --record-cap):
   a replay must start from the server's first tick, so it cannot drop
   its oldest part to stay under one.

   The file, little-endian: a header (REPLAY_MAGIC, REPLAY_VERSION, the
   build's content id, the map id), then blocks: raw size u32, packed
   size u32, the 5 LZMA properties bytes, the packed bytes. Unpacked, a
   block is whole events, each starting with an op byte:
     0x10 | slot  input: buttons u32, aim x i16, aim y i16 (1/32767 steps),
                  the cursor's unit u16 (entity Id + 1, 0 for none)
     0x20 | slot  joined
     0x30 | slot  left
     0x40 | slot  named: name 16 bytes
     0x50         seconds per tick from here on: f32
     0x60 | slot  held: no input for the slot this tick, its player waits
     0x80 | n     n ticks (1..127), then n world hashes u32

   This file holds the format and what both sides share; writing is
   replay/write.cpp, reading and PlayReplay are replay/read.cpp. */

#include "../third_party/lzma/lzma.cpp"

#define REPLAY_MAGIC 0x50524447u // "GDRP"
#define REPLAY_VERSION 5u
#define REPLAY_NAME_SIZE 16
#define REPLAY_BLOCK_SIZE (64 * 1024)
#define REPLAY_PACKED_SIZE (REPLAY_BLOCK_SIZE + REPLAY_BLOCK_SIZE / 8 + 1024)
#define REPLAY_BLOCK_HEADER 13
#define REPLAY_MAX_TICK_RUN 127
// NOTE(zoubir): what LZMA may use at once: 1.3 MB to pack a block with a
// 64 KB dictionary, 16 KB to unpack one (measured)
#define REPLAY_SCRATCH_SIZE (2 * 1024 * 1024)
#define REPLAY_DEFAULT_CAP (30u * 1024 * 1024)

enum replay_op
{
    ReplayOp_Input = 0x10,
    ReplayOp_Joined = 0x20,
    ReplayOp_Left = 0x30,
    ReplayOp_Named = 0x40,
    ReplayOp_Seconds = 0x50,
    ReplayOp_Held = 0x60,
    ReplayOp_Ticks = 0x80,
};

enum replay_event_type
{
    ReplayEvent_Joined = 1,
    ReplayEvent_Named,
    ReplayEvent_Left,
    ReplayEvent_Input,
    ReplayEvent_Tick,
    ReplayEvent_Held,
};

// NOTE(zoubir): one call into the game, as written and as read back
struct replay_event
{
    u8 Type; // replay_event_type
    u8 Slot;
    net_input Input; // buttons and aim; the tick is not kept
    char Name[REPLAY_NAME_SIZE];
    float Seconds;
    u32 Hash;
};

// NOTE(zoubir): LZMA's memory: a fixed buffer, handed out front to back
// and emptied after each block, so a running server never calls malloc
struct replay_scratch
{
    ISzAlloc Alloc; // first, so LZMA's pointer to it is a pointer to this
    u32 Used;
    u8 Memory[REPLAY_SCRATCH_SIZE];
};

internal void *
ReplayScratchAlloc(ISzAllocPtr Alloc, size_t Size)
{
    replay_scratch *Scratch = (replay_scratch *)Alloc;
    size_t Start = (Scratch->Used + 15) & ~(size_t)15;
    if (Start + Size > sizeof(Scratch->Memory))
    {
        return 0;
    }
    Scratch->Used = (u32)(Start + Size);
    return Scratch->Memory + Start;
}

static void
ReplayScratchFree(ISzAllocPtr Alloc, void *Address)
{
}

inline void
ResetReplayScratch(replay_scratch *Scratch)
{
    Scratch->Alloc.Alloc = ReplayScratchAlloc;
    Scratch->Alloc.Free = ReplayScratchFree;
    Scratch->Used = 0;
}

#include "replay/write.cpp"
#include "replay/read.cpp"
