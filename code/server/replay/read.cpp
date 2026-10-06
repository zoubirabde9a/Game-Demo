/* Reading a replay back (replay.cpp has the format): replay_reader
   unpacks one block at a time and hands out events in order, and
   PlayReplay feeds them to a fresh game, comparing its world hash after
   every tick with the recorded one. */

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
            case ReplayOp_Held: Event->Type = ReplayEvent_Held; return true;
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
            case ReplayEvent_Held: GameHoldPlayer(Game, Event.Slot); break;
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
