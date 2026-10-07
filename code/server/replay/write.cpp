/* Writing a replay (replay.cpp has the format): replay_writer collects
   events into a block, LZMA-packs each full block and appends it to the
   file or to memory, and stops at the size cap. The server calls
   ReplayWriteInput, ReplayWriteTick and the other ReplayWrite* from
   sim_game.cpp. */

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
        u8 Op[11] = {(u8)(ReplayOp_Input | Slot)};
        memcpy(Op + 1, &Event->Input.Buttons, 4);
        memcpy(Op + 5, &AimX, 2);
        memcpy(Op + 7, &AimY, 2);
        memcpy(Op + 9, &Event->Input.Target, 2);
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
        u8 Kind = (u8)(Event->Type == ReplayEvent_Joined ? ReplayOp_Joined :
                       (Event->Type == ReplayEvent_Held ? ReplayOp_Held : ReplayOp_Left));
        u8 Op = (u8)(Kind | Slot);
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

