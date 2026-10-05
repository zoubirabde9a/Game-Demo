/* Rewind trails: where each moving entity was drawn, sixty times a second
   for the last four seconds, with the sprite frame it showed. The ghosts
   of a rewind's path are drawn from them, and online a frozen entity's
   playback is: the server's snapshots come 20 times a second, the trail
   has every frame the player actually saw, so the run backwards is as
   smooth as the run forwards was. */

#define REWIND_FX_TRACKS 192
#define REWIND_FX_SAMPLES 256
#define REWIND_FX_SAMPLE_INTERVAL (1.f / 60.f)
// NOTE(zoubir): a track whose entity has been gone this long is reused
#define REWIND_FX_TRACK_GRACE 1.f

struct rewind_fx_sample
{
    float Time;
    v3 Position;
    // NOTE(zoubir): the sprite as it was drawn then
    asset_id Texture;
    v2 Dimensions;
    v4 Uvs;
};

struct rewind_fx_track
{
    bool32 Used;
    u32 EntityIndex;
    entity_type Type;
    float LastSeen;
    // NOTE(zoubir): a ring, newest at Head - 1
    u32 Head;
    u32 Count;
    rewind_fx_sample Samples[REWIND_FX_SAMPLES];
};

struct rewind_fx_trails
{
    float SinceSample;
    rewind_fx_track Tracks[REWIND_FX_TRACKS];
    // NOTE(zoubir): by local entity index, its track + 1, 0 for none
    u16 TrackOf[ArrayCount(((world *)0)->Entities)];
};

inline bool32
IsTrailedEntity(world_entity *Entity)
{
    bool32 Result = Entity->IsPresent && !IsDeadPlayer(Entity) &&
        (Entity->Type == EntityType_Player ||
         Entity->Type == EntityType_Monster ||
         Entity->Type == EntityType_FireBall ||
         Entity->Type == EntityType_Familiar ||
         Entity->Type == EntityType_MonsterShot);
    return Result;
}

inline rewind_fx_track *
GetRewindTrack(rewind_fx_trails *Trails, u32 EntityIndex)
{
    u32 Track = EntityIndex < ArrayCount(Trails->TrackOf) ?
        Trails->TrackOf[EntityIndex] : 0;
    rewind_fx_track *Result = Track ? &Trails->Tracks[Track - 1] : 0;
    return Result;
}

internal rewind_fx_track *
StartRewindTrack(rewind_fx_trails *Trails, world_entity *Entity, u32 EntityIndex,
                 float Clock)
{
    for(u32 Index = 0; Index < REWIND_FX_TRACKS; Index++)
    {
        rewind_fx_track *Track = &Trails->Tracks[Index];
        bool32 Stale = Track->Used && Clock - Track->LastSeen > REWIND_FX_TRACK_GRACE;
        if (!Track->Used || Stale)
        {
            if (Track->Used && Trails->TrackOf[Track->EntityIndex] == Index + 1)
            {
                Trails->TrackOf[Track->EntityIndex] = 0;
            }
            Track->Used = true;
            Track->EntityIndex = EntityIndex;
            Track->Type = Entity->Type;
            Track->LastSeen = Clock;
            Track->Head = 0;
            Track->Count = 0;
            Trails->TrackOf[EntityIndex] = (u16)(Index + 1);
            return Track;
        }
    }
    return 0;
}

inline rewind_fx_sample *
GetRewindSample(rewind_fx_track *Track, u32 Age)
{
    u32 Index = (Track->Head + REWIND_FX_SAMPLES - 1 - Age) % REWIND_FX_SAMPLES;
    rewind_fx_sample *Result = &Track->Samples[Index];
    return Result;
}

// NOTE(zoubir): once a frame, after the world tick: a sample of every
// trailed entity each REWIND_FX_SAMPLE_INTERVAL
internal void
UpdateRewindTrails(rewind_fx_trails *Trails, app_state *AppState, float Clock,
                   float DeltaTime)
{
    Trails->SinceSample += DeltaTime;
    if (Trails->SinceSample < REWIND_FX_SAMPLE_INTERVAL)
    {
        return;
    }
    Trails->SinceSample -= REWIND_FX_SAMPLE_INTERVAL;
    if (Trails->SinceSample > REWIND_FX_SAMPLE_INTERVAL)
    {
        Trails->SinceSample = 0.f;
    }
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!IsTrailedEntity(Entity))
        {
            continue;
        }
        rewind_fx_track *Track = GetRewindTrack(Trails, EntityIndex);
        if (Track && (Track->Type != Entity->Type || Track->EntityIndex != EntityIndex))
        {
            Trails->TrackOf[EntityIndex] = 0;
            Track = 0;
        }
        if (!Track)
        {
            Track = StartRewindTrack(Trails, Entity, EntityIndex, Clock);
        }
        if (!Track)
        {
            continue;
        }
        Track->LastSeen = Clock;
        rewind_fx_sample *Sample = &Track->Samples[Track->Head];
        Track->Head = (Track->Head + 1) % REWIND_FX_SAMPLES;
        Track->Count = Minimum(Track->Count + 1, (u32)REWIND_FX_SAMPLES);
        Sample->Time = Clock;
        Sample->Position = Entity->Position;
        Sample->Texture = Entity->Texture;
        Sample->Dimensions = Entity->Dimensions;
        Sample->Uvs = Entity->Uvs;
    }
}

// NOTE(zoubir): where the track's entity was at Time, between the two
// samples around it; the oldest sample when the track does not reach
// back that far. False for an empty track
internal bool32
SampleRewindTrack(rewind_fx_track *Track, float Time, rewind_fx_sample *Out)
{
    if (!Track || Track->Count == 0)
    {
        return false;
    }
    rewind_fx_sample *Newer = GetRewindSample(Track, 0);
    if (Time >= Newer->Time)
    {
        *Out = *Newer;
        return true;
    }
    for(u32 Age = 1; Age < Track->Count; Age++)
    {
        rewind_fx_sample *Older = GetRewindSample(Track, Age);
        if (Older->Time <= Time)
        {
            float Span = Newer->Time - Older->Time;
            float T = Span > 0.f ? (Time - Older->Time) / Span : 0.f;
            *Out = *Older;
            Out->Position = Older->Position + T * (Newer->Position - Older->Position);
            return true;
        }
        Newer = Older;
    }
    *Out = *Newer;
    return true;
}
