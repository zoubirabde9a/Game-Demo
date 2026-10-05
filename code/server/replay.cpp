/* Replays: every call the server makes into its game (a player joining,
   being named, leaving, each input, each tick), in order, with the world
   hash (sim/world_hash.cpp) after every tick. The simulation decides
   everything from those calls alone: its random numbers come from the
   monster population's seeded series, never the clock. So feeding the
   same calls to a fresh game rebuilds the match tick by tick, and the
   hashes prove it did, or name the first tick where it did not.

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
     0x10 | slot  input: buttons u32, aim x i16, aim y i16 (1/32767 steps)
     0x20 | slot  joined
     0x30 | slot  left
     0x40 | slot  named: name 16 bytes
     0x50         seconds per tick from here on: f32
     0x80 | n     n ticks (1..127), then n world hashes u32 */

#include "../third_party/lzma/lzma.cpp"

#define REPLAY_MAGIC 0x50524447u // "GDRP"
#define REPLAY_VERSION 3u
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
    ReplayOp_Ticks = 0x80,
};

enum replay_event_type
{
    ReplayEvent_Joined = 1,
    ReplayEvent_Named,
    ReplayEvent_Left,
    ReplayEvent_Input,
    ReplayEvent_Tick,
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

// NOTE(zoubir): writes to File, or into Memory (tests)
struct replay_writer
{
    FILE *File;
    u8 *Memory;
    u32 Capacity;
    // NOTE(zoubir): bytes written so far, and the most it may be, 0 for
    // REPLAY_DEFAULT_CAP; past it the writer stops and sets Full
    u32 Used;
    u32 Cap;
    bool32 Full;
    bool32 Failed;
    u32 Ticks;
    // NOTE(zoubir): the seconds per tick last written, and the ticks not
    // written yet
    float Seconds;
    u32 RunCount;
    u32 RunHashes[REPLAY_MAX_TICK_RUN];
    u32 BlockUsed;
    u8 Block[REPLAY_BLOCK_SIZE];
    u8 Packed[REPLAY_PACKED_SIZE];
    replay_scratch Scratch;
};

internal void
ReplayEmit(replay_writer *Writer, void *Data, u32 Size)
{
    u32 Cap = Writer->Cap ? Writer->Cap : REPLAY_DEFAULT_CAP;
    if (Writer->Full || Writer->Used + Size > Cap)
    {
        Writer->Full = true;
        return;
    }
    if (Writer->File)
    {
        if (fwrite(Data, 1, Size, Writer->File) != Size)
        {
            Writer->Failed = true;
        }
    }
    else if (Writer->Memory && Writer->Used + Size <= Writer->Capacity)
    {
        memcpy(Writer->Memory + Writer->Used, Data, Size);
    }
    else
    {
        Writer->Failed = true;
        return;
    }
    Writer->Used += Size;
}

// NOTE(zoubir): packs the block and writes it out
internal void
ReplayPackBlock(replay_writer *Writer)
{
    if (!Writer->BlockUsed)
    {
        return;
    }
    ResetReplayScratch(&Writer->Scratch);
    CLzmaEncProps Props;
    LzmaEncProps_Init(&Props);
    Props.level = 6;
    Props.dictSize = REPLAY_BLOCK_SIZE;
    Props.numThreads = 1;
    u8 Header[REPLAY_BLOCK_HEADER];
    size_t PropsSize = LZMA_PROPS_SIZE;
    size_t PackedSize = sizeof(Writer->Packed);
    SRes Result = LzmaEncode(Writer->Packed, &PackedSize, Writer->Block, Writer->BlockUsed,
                             &Props, Header + 8, &PropsSize, 0, 0,
                             &Writer->Scratch.Alloc, &Writer->Scratch.Alloc);
    if (Result != SZ_OK || PropsSize != LZMA_PROPS_SIZE)
    {
        Writer->Failed = true;
        Writer->BlockUsed = 0;
        return;
    }
    u32 RawSize = Writer->BlockUsed;
    u32 Packed = (u32)PackedSize;
    memcpy(Header, &RawSize, 4);
    memcpy(Header + 4, &Packed, 4);
    u32 Cap = Writer->Cap ? Writer->Cap : REPLAY_DEFAULT_CAP;
    if (Writer->Used + REPLAY_BLOCK_HEADER + Packed > Cap)
    {
        Writer->Full = true;
    }
    else
    {
        ReplayEmit(Writer, Header, REPLAY_BLOCK_HEADER);
        ReplayEmit(Writer, Writer->Packed, Packed);
        if (Writer->File)
        {
            fflush(Writer->File);
        }
    }
    Writer->BlockUsed = 0;
}

// NOTE(zoubir): an event goes into the block whole; a block too full for
// it is packed first
internal void
ReplayPut(replay_writer *Writer, void *Data, u32 Size)
{
    if (Writer->BlockUsed + Size > sizeof(Writer->Block))
    {
        ReplayPackBlock(Writer);
    }
    memcpy(Writer->Block + Writer->BlockUsed, Data, Size);
    Writer->BlockUsed += Size;
}

internal void
ReplayPutTickRun(replay_writer *Writer)
{
    if (Writer->RunCount)
    {
        u8 Run[1 + 4 * REPLAY_MAX_TICK_RUN];
        Run[0] = (u8)(ReplayOp_Ticks | Writer->RunCount);
        memcpy(Run + 1, Writer->RunHashes, 4 * Writer->RunCount);
        ReplayPut(Writer, Run, 1 + 4 * Writer->RunCount);
        Writer->RunCount = 0;
    }
}

internal void
ReplayWriteHeader(replay_writer *Writer, u32 ContentId, u32 MapId)
{
    u32 Header[4] = {REPLAY_MAGIC, REPLAY_VERSION, ContentId, MapId};
    ReplayEmit(Writer, Header, sizeof(Header));
}

// NOTE(zoubir): the aim as the wire carries it, in 1/32767 steps, rounded
// exactly as NetUnitFloat (net/serialize.cpp) rounds it, so a client's
// aim, already rounded once, comes out the same
inline i16
ReplayAimSteps(float Aim)
{
    if (!(Aim >= -1.0f)) Aim = -1.0f; // also catches NaN
    if (Aim > 1.0f) Aim = 1.0f;
    i16 Steps = (i16)(Aim * 32767.0f + (Aim < 0 ? -0.5f : 0.5f));
    if (Steps < -32767) Steps = -32767;
    return Steps;
}

internal void
ReplayWriteEvent(replay_writer *Writer, replay_event *Event)
{
    if (!Writer || Writer->Full)
    {
        return;
    }
    if (Event->Type == ReplayEvent_Tick)
    {
        if (Event->Seconds != Writer->Seconds)
        {
            ReplayPutTickRun(Writer);
            u8 Op[5] = {ReplayOp_Seconds};
            memcpy(Op + 1, &Event->Seconds, 4);
            ReplayPut(Writer, Op, sizeof(Op));
            Writer->Seconds = Event->Seconds;
        }
        Writer->RunHashes[Writer->RunCount++] = Event->Hash;
        Writer->Ticks++;
        if (Writer->RunCount == REPLAY_MAX_TICK_RUN)
        {
            ReplayPutTickRun(Writer);
        }
        return;
    }

    u8 Slot = (u8)(Event->Slot & 7);
    if (Event->Type == ReplayEvent_Input)
    {
        i16 AimX = ReplayAimSteps(Event->Input.AimX);
        i16 AimY = ReplayAimSteps(Event->Input.AimY);
        ReplayPutTickRun(Writer);
        u8 Op[9] = {(u8)(ReplayOp_Input | Slot)};
        memcpy(Op + 1, &Event->Input.Buttons, 4);
        memcpy(Op + 5, &AimX, 2);
        memcpy(Op + 7, &AimY, 2);
        ReplayPut(Writer, Op, sizeof(Op));
        return;
    }

    ReplayPutTickRun(Writer);
    if (Event->Type == ReplayEvent_Named)
    {
        u8 Op[1 + REPLAY_NAME_SIZE] = {(u8)(ReplayOp_Named | Slot)};
        memcpy(Op + 1, Event->Name, REPLAY_NAME_SIZE);
        Op[REPLAY_NAME_SIZE] = 0;
        ReplayPut(Writer, Op, sizeof(Op));
    }
    else
    {
        u8 Op = (u8)((Event->Type == ReplayEvent_Joined ? ReplayOp_Joined : ReplayOp_Left) | Slot);
        ReplayPut(Writer, &Op, 1);
    }
}

internal void
ReplayWriteSlotEvent(replay_writer *Writer, u8 Type, u32 Slot)
{
    replay_event Event = {};
    Event.Type = Type;
    Event.Slot = (u8)Slot;
    ReplayWriteEvent(Writer, &Event);
}

internal void
ReplayWriteNamed(replay_writer *Writer, u32 Slot, char *Name)
{
    replay_event Event = {};
    Event.Type = ReplayEvent_Named;
    Event.Slot = (u8)Slot;
    for (u32 Index = 0; Name && Name[Index] && Index + 1 < REPLAY_NAME_SIZE; ++Index)
    {
        Event.Name[Index] = Name[Index];
    }
    ReplayWriteEvent(Writer, &Event);
}

internal void
ReplayWriteInput(replay_writer *Writer, u32 Slot, net_input *Input)
{
    replay_event Event = {};
    Event.Type = ReplayEvent_Input;
    Event.Slot = (u8)Slot;
    Event.Input = *Input;
    ReplayWriteEvent(Writer, &Event);
}

internal void
ReplayWriteTick(replay_writer *Writer, float Seconds, u32 Hash)
{
    replay_event Event = {};
    Event.Type = ReplayEvent_Tick;
    Event.Seconds = Seconds;
    Event.Hash = Hash;
    ReplayWriteEvent(Writer, &Event);
}

// NOTE(zoubir): writes what is still held: call before closing the file
// or reading the memory
internal void
ReplayFlush(replay_writer *Writer)
{
    ReplayPutTickRun(Writer);
    ReplayPackBlock(Writer);
    if (Writer->File)
    {
        fflush(Writer->File);
    }
}

// NOTE(zoubir): reads a replay back one event at a time
struct replay_reader
{
    u8 *Data;
    u32 Size;
    u32 At;
    bool32 Failed;
    u32 ContentId;
    u32 MapId;
    float Seconds;
    u32 RunLeft;
    u32 BlockSize;
    u32 BlockAt;
    u8 Block[REPLAY_BLOCK_SIZE];
    replay_scratch Scratch;
};

// NOTE(zoubir): false when Data is not a replay this build reads
internal bool32
ReplayOpen(replay_reader *Reader, u8 *Data, u32 Size)
{
    Reader->Data = Data;
    Reader->Size = Size;
    Reader->At = 16;
    Reader->Failed = false;
    Reader->Seconds = 0.f;
    Reader->RunLeft = 0;
    Reader->BlockSize = Reader->BlockAt = 0;
    u32 Header[4] = {};
    if (Size < sizeof(Header))
    {
        return false;
    }
    memcpy(Header, Data, sizeof(Header));
    Reader->ContentId = Header[2];
    Reader->MapId = Header[3];
    return Header[0] == REPLAY_MAGIC && Header[1] == REPLAY_VERSION && Header[3] < MapId_Count;
}

internal bool32
ReplayTake(replay_reader *Reader, void *Out, u32 Size)
{
    if (Reader->BlockAt + Size > Reader->BlockSize)
    {
        Reader->Failed = true;
        return false;
    }
    memcpy(Out, Reader->Block + Reader->BlockAt, Size);
    Reader->BlockAt += Size;
    return true;
}

// NOTE(zoubir): unpacks the next block; false at the end of the file or
// at a block cut short (a recording stopped by a crash ends there)
internal bool32
ReplayNextBlock(replay_reader *Reader)
{
    if (Reader->At == Reader->Size)
    {
        return false;
    }
    u32 RawSize = 0, Packed = 0;
    if (Reader->At + REPLAY_BLOCK_HEADER > Reader->Size)
    {
        Reader->Failed = true;
        return false;
    }
    memcpy(&RawSize, Reader->Data + Reader->At, 4);
    memcpy(&Packed, Reader->Data + Reader->At + 4, 4);
    u8 *Props = Reader->Data + Reader->At + 8;
    if (RawSize == 0 || RawSize > sizeof(Reader->Block) ||
        Packed > Reader->Size - Reader->At - REPLAY_BLOCK_HEADER)
    {
        Reader->Failed = true;
        return false;
    }
    ResetReplayScratch(&Reader->Scratch);
    SizeT OutSize = RawSize;
    SizeT InSize = Packed;
    ELzmaStatus Status;
    SRes Result = LzmaDecode(Reader->Block, &OutSize,
                             Reader->Data + Reader->At + REPLAY_BLOCK_HEADER, &InSize,
                             Props, LZMA_PROPS_SIZE, LZMA_FINISH_END, &Status,
                             &Reader->Scratch.Alloc);
    if (Result != SZ_OK || OutSize != RawSize)
    {
        Reader->Failed = true;
        return false;
    }
    Reader->At += REPLAY_BLOCK_HEADER + Packed;
    Reader->BlockSize = RawSize;
    Reader->BlockAt = 0;
    return true;
}

// NOTE(zoubir): the next call into the game; false at the end, or when
// the file is damaged (then Failed is set)
internal bool32
ReplayNextEvent(replay_reader *Reader, replay_event *Event)
{
    *Event = {};
    for (;;)
    {
        if (Reader->RunLeft)
        {
            Reader->RunLeft--;
            Event->Type = ReplayEvent_Tick;
            Event->Seconds = Reader->Seconds;
            return ReplayTake(Reader, &Event->Hash, 4);
        }
        if (Reader->BlockAt == Reader->BlockSize && !ReplayNextBlock(Reader))
        {
            return false;
        }
        u8 Op = 0;
        if (!ReplayTake(Reader, &Op, 1))
        {
            return false;
        }
        Event->Slot = Op & 7;
        if (Op & ReplayOp_Ticks)
        {
            Reader->RunLeft = Op & 0x7F;
            if (!Reader->RunLeft)
            {
                Reader->Failed = true;
                return false;
            }
            continue;
        }
        switch (Op & 0xF8)
        {
            case ReplayOp_Seconds:
            {
                if (!ReplayTake(Reader, &Reader->Seconds, 4)) return false;
                continue;
            }
            case ReplayOp_Joined: Event->Type = ReplayEvent_Joined; return true;
            case ReplayOp_Left: Event->Type = ReplayEvent_Left; return true;
            case ReplayOp_Named:
            {
                Event->Type = ReplayEvent_Named;
                if (!ReplayTake(Reader, Event->Name, REPLAY_NAME_SIZE)) return false;
                Event->Name[REPLAY_NAME_SIZE - 1] = 0;
                return true;
            }
            case ReplayOp_Input:
            {
                Event->Type = ReplayEvent_Input;
                i16 AimX = 0, AimY = 0;
                if (!ReplayTake(Reader, &Event->Input.Buttons, 4) ||
                    !ReplayTake(Reader, &AimX, 2) || !ReplayTake(Reader, &AimY, 2))
                {
                    return false;
                }
                // NOTE(zoubir): as the wire turns steps back into an aim
                Event->Input.AimX = (float)AimX / 32767.0f;
                Event->Input.AimY = (float)AimY / 32767.0f;
                return true;
            }
            default:
            {
                Reader->Failed = true;
                return false;
            }
        }
    }
}

// NOTE(zoubir): what playing a replay back found
struct replay_result
{
    bool32 Readable;     // the header and every event made sense
    u32 ContentId;       // the build that recorded it
    u32 MapId;
    u32 Ticks;           // ticks played
    u32 Mismatches;      // ticks whose world hash differed
    u32 FirstMismatch;   // the first of them, counted from 1; 0 for none
    u32 FinalHash;
};

// NOTE(zoubir): feeds a recorded match to a fresh game, which must not be
// recording, comparing each tick's world hash with the recorded one.
// Game is shut down afterwards
internal replay_result
PlayReplay(server_game *Game, u8 *Data, u32 Size)
{
    replay_result Result = {};
    static replay_reader Reader;
    if (!ReplayOpen(&Reader, Data, Size))
    {
        return Result;
    }
    Result.ContentId = Reader.ContentId;
    Result.MapId = Reader.MapId;
    GameInit(Game, Reader.MapId);
    replay_event Event;
    u32 InputTick = 0;
    while (ReplayNextEvent(&Reader, &Event))
    {
        switch (Event.Type)
        {
            case ReplayEvent_Joined: GamePlayerJoined(Game, Event.Slot); break;
            case ReplayEvent_Left: GamePlayerLeft(Game, Event.Slot); break;
            case ReplayEvent_Named: GamePlayerNamed(Game, Event.Slot, Event.Name); break;
            case ReplayEvent_Input:
            {
                Event.Input.Tick = ++InputTick;
                GameApplyInput(Game, Event.Slot, &Event.Input);
            } break;
            case ReplayEvent_Tick:
            {
                GameTick(Game, Event.Seconds);
                Result.Ticks++;
                Result.FinalHash = HashWorldState(Game->AppState);
                if (Result.FinalHash != Event.Hash)
                {
                    Result.Mismatches++;
                    if (!Result.FirstMismatch)
                    {
                        Result.FirstMismatch = Result.Ticks;
                    }
                }
            } break;
        }
    }
    Result.Readable = !Reader.Failed;
    GameShutdown(Game);
    return Result;
}
