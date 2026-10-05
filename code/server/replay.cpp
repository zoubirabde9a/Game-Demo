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

   The file, little-endian: a header (REPLAY_MAGIC, REPLAY_VERSION, the
   build's content id, the map id), then events, each a type byte:
     Joined  slot u8
     Named   slot u8, name 16 bytes
     Left    slot u8
     Input   slot u8, tick u32, buttons u16, aim x f32, aim y f32
     Tick    seconds f32, world hash u32 */

#define REPLAY_MAGIC 0x50524447u // "GDRP"
#define REPLAY_VERSION 1u
#define REPLAY_NAME_SIZE 16
#define REPLAY_BUFFER_SIZE (64 * 1024)

enum replay_event
{
    ReplayEvent_Joined = 1,
    ReplayEvent_Named,
    ReplayEvent_Left,
    ReplayEvent_Input,
    ReplayEvent_Tick,
};

// NOTE(zoubir): writes into Memory (tests) or, a buffer at a time, File
struct replay_writer
{
    FILE *File;
    u8 *Memory;
    u32 Capacity;
    u32 Used;
    bool32 Failed;
    u32 Ticks;
    u8 Buffer[REPLAY_BUFFER_SIZE];
};

internal void
ReplayFlush(replay_writer *Writer)
{
    if (Writer->File && Writer->Used)
    {
        if (fwrite(Writer->Buffer, 1, Writer->Used, Writer->File) != Writer->Used)
        {
            Writer->Failed = true;
        }
        Writer->Used = 0;
    }
}

internal void
ReplayWrite(replay_writer *Writer, void *Data, u32 Size)
{
    u8 *Target = Writer->File ? Writer->Buffer : Writer->Memory;
    u32 Capacity = Writer->File ? (u32)sizeof(Writer->Buffer) : Writer->Capacity;
    if (Writer->File && Writer->Used + Size > Capacity)
    {
        ReplayFlush(Writer);
    }
    if (!Target || Writer->Used + Size > Capacity)
    {
        Writer->Failed = true;
        return;
    }
    memcpy(Target + Writer->Used, Data, Size);
    Writer->Used += Size;
}

#define ReplayWriteValue(Writer, Value) ReplayWrite(Writer, &(Value), sizeof(Value))

internal void
ReplayWriteHeader(replay_writer *Writer, u32 ContentId, u32 MapId)
{
    u32 Header[4] = {REPLAY_MAGIC, REPLAY_VERSION, ContentId, MapId};
    ReplayWrite(Writer, Header, sizeof(Header));
}

internal void
ReplayWriteSlotEvent(replay_writer *Writer, u8 Type, u32 Slot)
{
    if (Writer)
    {
        u8 Bytes[2] = {Type, (u8)Slot};
        ReplayWrite(Writer, Bytes, sizeof(Bytes));
    }
}

internal void
ReplayWriteNamed(replay_writer *Writer, u32 Slot, char *Name)
{
    if (Writer)
    {
        ReplayWriteSlotEvent(Writer, ReplayEvent_Named, Slot);
        char Padded[REPLAY_NAME_SIZE] = {};
        for (u32 Index = 0; Name && Name[Index] && Index + 1 < REPLAY_NAME_SIZE; ++Index)
        {
            Padded[Index] = Name[Index];
        }
        ReplayWrite(Writer, Padded, sizeof(Padded));
    }
}

internal void
ReplayWriteInput(replay_writer *Writer, u32 Slot, net_input *Input)
{
    if (Writer)
    {
        ReplayWriteSlotEvent(Writer, ReplayEvent_Input, Slot);
        ReplayWriteValue(Writer, Input->Tick);
        ReplayWriteValue(Writer, Input->Buttons);
        ReplayWriteValue(Writer, Input->AimX);
        ReplayWriteValue(Writer, Input->AimY);
    }
}

internal void
ReplayWriteTick(replay_writer *Writer, float Seconds, u32 Hash)
{
    if (Writer)
    {
        u8 Type = ReplayEvent_Tick;
        ReplayWriteValue(Writer, Type);
        ReplayWriteValue(Writer, Seconds);
        ReplayWriteValue(Writer, Hash);
        Writer->Ticks++;
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

struct replay_reader
{
    u8 *Data;
    u32 Size;
    u32 At;
    bool32 Failed;
};

internal void
ReplayRead(replay_reader *Reader, void *Out, u32 Size)
{
    if (Reader->At + Size > Reader->Size)
    {
        Reader->Failed = true;
        memset(Out, 0, Size);
        return;
    }
    memcpy(Out, Reader->Data + Reader->At, Size);
    Reader->At += Size;
}

#define ReplayReadValue(Reader, Value) ReplayRead(Reader, &(Value), sizeof(Value))

// NOTE(zoubir): feeds a recorded match to a fresh game, which must not be
// recording, comparing each tick's world hash with the recorded one.
// Game is shut down afterwards
internal replay_result
PlayReplay(server_game *Game, u8 *Data, u32 Size)
{
    replay_result Result = {};
    replay_reader Reader = {Data, Size, 0, false};
    u32 Header[4] = {};
    ReplayRead(&Reader, Header, sizeof(Header));
    if (Reader.Failed || Header[0] != REPLAY_MAGIC || Header[1] != REPLAY_VERSION ||
        Header[3] >= MapId_Count)
    {
        return Result;
    }
    Result.ContentId = Header[2];
    Result.MapId = Header[3];
    GameInit(Game, Header[3]);
    while (Reader.At < Reader.Size && !Reader.Failed)
    {
        u8 Type = 0;
        ReplayReadValue(&Reader, Type);
        u8 Slot = 0;
        if (Type != ReplayEvent_Tick)
        {
            ReplayReadValue(&Reader, Slot);
            if (Slot >= MAX_PLAYERS)
            {
                Reader.Failed = true;
                break;
            }
        }
        switch (Type)
        {
            case ReplayEvent_Joined: GamePlayerJoined(Game, Slot); break;
            case ReplayEvent_Left: GamePlayerLeft(Game, Slot); break;
            case ReplayEvent_Named:
            {
                char Name[REPLAY_NAME_SIZE];
                ReplayRead(&Reader, Name, sizeof(Name));
                Name[REPLAY_NAME_SIZE - 1] = 0;
                GamePlayerNamed(Game, Slot, Name);
            } break;
            case ReplayEvent_Input:
            {
                net_input Input = {};
                ReplayReadValue(&Reader, Input.Tick);
                ReplayReadValue(&Reader, Input.Buttons);
                ReplayReadValue(&Reader, Input.AimX);
                ReplayReadValue(&Reader, Input.AimY);
                GameApplyInput(Game, Slot, &Input);
            } break;
            case ReplayEvent_Tick:
            {
                float Seconds = 0.f;
                u32 Recorded = 0;
                ReplayReadValue(&Reader, Seconds);
                ReplayReadValue(&Reader, Recorded);
                if (Reader.Failed)
                {
                    break;
                }
                GameTick(Game, Seconds);
                Result.Ticks++;
                Result.FinalHash = HashWorldState(Game->AppState);
                if (Result.FinalHash != Recorded)
                {
                    Result.Mismatches++;
                    if (!Result.FirstMismatch)
                    {
                        Result.FirstMismatch = Result.Ticks;
                    }
                }
            } break;
            default:
            {
                Reader.Failed = true;
            } break;
        }
    }
    Result.Readable = !Reader.Failed;
    GameShutdown(Game);
    return Result;
}
