/* Rewind history: a ring of frames, each a copy of every moving entity
   at one moment, plus the respawn timers and the monster population's
   bookkeeping a world rewind puts back. Recorded at the end of every
   tick that crosses REWIND_RECORD_INTERVAL, never while a world rewind
   holds the simulation. */

// NOTE(zoubir): the part of monster_population a world rewind puts back:
// the timers, the random series and the pending deaths, not the animation
// tables that come after them
#define REWIND_POPULATION_PREFIX offsetof(monster_population, AnimationSets)
static_assert(REWIND_POPULATION_PREFIX <= REWIND_POPULATION_BYTES,
              "raise REWIND_POPULATION_BYTES");

// NOTE(zoubir): what the history keeps: everything that moves, comes and
// goes. Walls, trees and tiles never change
inline bool32
IsRewindRecorded(world_entity *Entity)
{
    bool32 Result = Entity->IsPresent &&
        (Entity->Type == EntityType_Player ||
         Entity->Type == EntityType_Monster ||
         Entity->Type == EntityType_FireBall ||
         Entity->Type == EntityType_Sword ||
         Entity->Type == EntityType_Familiar ||
         Entity->Type == EntityType_MonsterShot ||
         Entity->Type == EntityType_MonsterHazard);
    return Result;
}

internal time_rewind *
CreateTimeRewind(memory_arena *Arena)
{
    time_rewind *Result = AllocateStruct(Arena, time_rewind);
    ZeroSize(Result, sizeof(*Result));
    Result->RecordCapacity = REWIND_RECORD_POOL;
    Result->Records = AllocateArray(Arena, Result->RecordCapacity, world_entity);
    return Result;
}

// NOTE(zoubir): forgets every frame and every rewind under way; the world
// it described is gone (another map, or the server's world replaced it)
internal void
ResetTimeRewind(time_rewind *Rewind)
{
    if (Rewind)
    {
        world_entity *Records = Rewind->Records;
        u32 Capacity = Rewind->RecordCapacity;
        ZeroSize(Rewind, sizeof(*Rewind));
        Rewind->Records = Records;
        Rewind->RecordCapacity = Capacity;
    }
}

inline rewind_frame *
GetRewindFrame(time_rewind *Rewind, u32 Age)
{
    rewind_frame *Result =
        &Rewind->Frames[(Rewind->FirstFrame + Age) % REWIND_HISTORY_FRAMES];
    return Result;
}

inline void
DropOldestRewindFrame(time_rewind *Rewind)
{
    Assert(Rewind->FrameCount > 0);
    Rewind->FirstFrame = (Rewind->FirstFrame + 1) % REWIND_HISTORY_FRAMES;
    Rewind->FrameCount--;
}

// NOTE(zoubir): whether record ranges [A, A + ACount) and [B, B + BCount)
// share a record
inline bool32
RecordRangesOverlap(u32 A, u32 ACount, u32 B, u32 BCount)
{
    bool32 Result = ACount > 0 && BCount > 0 && A < B + BCount && B < A + ACount;
    return Result;
}

// NOTE(zoubir): the frame taken nearest to Time (the oldest one when the
// history does not reach back that far), 0 when it is empty. Nearest, not
// the newest at or before: 120 ticks of 1/60 s add up to a hair either
// side of 2 s in floats, and "at or before" then skipped the right frame
internal rewind_frame *
FindRewindFrame(time_rewind *Rewind, float Time)
{
    rewind_frame *Result = 0;
    float Best = 0.f;
    for(u32 Age = 0; Age < Rewind->FrameCount; Age++)
    {
        rewind_frame *Frame = GetRewindFrame(Rewind, Age);
        float Distance = Absolute(Frame->Time - Time);
        if (!Result || Distance < Best)
        {
            Result = Frame;
            Best = Distance;
        }
        if (Frame->Time > Time)
        {
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): Frame's copy of the entity with this ID and serial, or 0
internal world_entity *
FindRewindRecord(time_rewind *Rewind, rewind_frame *Frame, u32 ID, u32 Serial)
{
    for(u32 Index = 0; Index < Frame->RecordCount; Index++)
    {
        world_entity *Record = &Rewind->Records[Frame->FirstRecord + Index];
        if (Record->ID == ID && Record->RewindSerial == Serial)
        {
            return Record;
        }
    }
    return 0;
}

// NOTE(zoubir): gives the entity its RewindSerial if it has none yet
inline u32
GetRewindSerial(time_rewind *Rewind, world_entity *Entity)
{
    if (!Entity->RewindSerial)
    {
        Entity->RewindSerial = ++Rewind->NextSerial;
        if (!Entity->RewindSerial)
        {
            Entity->RewindSerial = ++Rewind->NextSerial;
        }
    }
    return Entity->RewindSerial;
}

// NOTE(zoubir): takes a frame of the world as it is now. The oldest
// frames go first when the ring of frames or of records is full
internal void
RecordRewindFrame(app_state *AppState, time_rewind *Rewind, world *World)
{
    u32 Count = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        Count += IsRewindRecorded(&World->Entities[Index]) ? 1 : 0;
    }
    if (Count > Rewind->RecordCapacity)
    {
        Count = Rewind->RecordCapacity;
    }
    u32 Start = Rewind->RecordHead;
    if (Start + Count > Rewind->RecordCapacity)
    {
        Start = 0;
    }
    if (Rewind->FrameCount == REWIND_HISTORY_FRAMES)
    {
        DropOldestRewindFrame(Rewind);
    }
    // NOTE(zoubir): the records are written in frame order, so the ones
    // about to be overwritten belong to the oldest frames
    for(;;)
    {
        bool32 Overlaps = false;
        for(u32 Age = 0; Age < Rewind->FrameCount && !Overlaps; Age++)
        {
            rewind_frame *Frame = GetRewindFrame(Rewind, Age);
            Overlaps = RecordRangesOverlap(Frame->FirstRecord, Frame->RecordCount,
                                           Start, Count);
        }
        if (!Overlaps)
        {
            break;
        }
        DropOldestRewindFrame(Rewind);
    }

    rewind_frame *Frame = GetRewindFrame(Rewind, Rewind->FrameCount++);
    Frame->Time = Rewind->Clock;
    Frame->FirstRecord = Start;
    Frame->RecordCount = 0;
    for(u32 Index = 0; Index < World->EntityCount && Frame->RecordCount < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (IsRewindRecorded(Entity))
        {
            GetRewindSerial(Rewind, Entity);
            Rewind->Records[Start + Frame->RecordCount++] = *Entity;
        }
    }
    Rewind->RecordHead = Start + Frame->RecordCount;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Frame->RespawnTimers[SlotIndex] = AppState->Players[SlotIndex].RespawnTimer;
    }
    ZeroSize(Frame->Population, sizeof(Frame->Population));
    if (AppState->Monsters)
    {
        memcpy(Frame->Population, AppState->Monsters, REWIND_POPULATION_PREFIX);
    }
}

// NOTE(zoubir): every tick, first: the clock says when the state this tick
// makes is, so a frame taken at its end, and a hold begun during it, carry
// the same time
inline void
AdvanceRewindClock(time_rewind *Rewind, float DeltaTime)
{
    Rewind->Clock += DeltaTime;
}

// NOTE(zoubir): every tick, last: takes a frame when one is due. Two
// 60 Hz ticks add up to a hair under REWIND_RECORD_INTERVAL in floats, so
// the test allows for that, or a frame would come every third tick
internal void
RecordRewindHistory(app_state *AppState, time_rewind *Rewind, world *World,
                    float DeltaTime)
{
    Rewind->SinceRecord += DeltaTime;
    if (Rewind->FrameCount == 0 ||
        Rewind->SinceRecord >= REWIND_RECORD_INTERVAL - 0.0005f)
    {
        Rewind->SinceRecord -= REWIND_RECORD_INTERVAL;
        if (Rewind->SinceRecord < 0.f || Rewind->SinceRecord >= REWIND_RECORD_INTERVAL)
        {
            Rewind->SinceRecord = 0.f;
        }
        RecordRewindFrame(AppState, Rewind, World);
    }
}

// NOTE(zoubir): after a world rewind lands on Frame: what came after it
// never happened, so its frames go, and the clock is Frame's again
internal void
TruncateRewindHistory(time_rewind *Rewind, rewind_frame *Frame)
{
    while (Rewind->FrameCount > 0 &&
           GetRewindFrame(Rewind, Rewind->FrameCount - 1) != Frame)
    {
        Rewind->FrameCount--;
    }
    if (Rewind->FrameCount > 0)
    {
        Rewind->RecordHead = Frame->FirstRecord + Frame->RecordCount;
        Rewind->Clock = Frame->Time;
    }
    Rewind->SinceRecord = 0.f;
}
