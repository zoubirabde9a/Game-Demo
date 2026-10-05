/* Rewind effects: how the three time rewinds (sim/time_rewind/) look.

   - The cast: a clock sigil under the caster, its hands turning
     backwards faster and faster, motes spiralling in; a bubble's edge
     drawn on the ground, a world rewind's rings sweeping out
     (rewind_draw.cpp).
   - The hold: the post-process (time_warp.cpp, build/shaders/fx/
     time_warp.frag) drains the colour out of what is frozen into a cold
     silver-blue, a shock ring bends the picture as it sweeps out, and
     each frozen thing shows its way back as a row of ghosts, ending in a
     bright ghost where it will land.
   - The playback: tape-rewind tearing, colour split and a zoom pull
     toward the centre inside the bubble (all of the screen for a world
     rewind), afterimages trailing each body as it runs backwards, and a
     timecode counting down at the top of the screen.
   - The landing: a flash and a ring on everything that came back, and
     the colour floods back in.

   Online a frozen entity's playback is drawn from this machine's own
   trails (rewind_trails.cpp), sampled every frame, rather than from the
   server's snapshots, 20 a second; the server's restored state arrives
   as the playback ends, where the trail already put it.

   Entry points, from app.cpp: UpdateRewindFx after the world tick;
   BeginTimeWarp and EndTimeWarp around the world pass; DrawRewindFx and
   DrawRewindHud in client/screen_pass.inc. Online, client/online.cpp
   calls ReadRewindsFromSnapshot on each new snapshot, and
   client/prediction.cpp asks IsLocalPlayerTimeLocked. */

inline float
RewindClamp01(float Value)
{
    float Result = Value < 0.f ? 0.f : (Value > 1.f ? 1.f : Value);
    return Result;
}

#include "rewind_trails.cpp"
#include "rewind_casts.cpp"

struct rewind_fx
{
    float Clock;
    rewind_fx_trails Trails;
    rewind_fx_cast Casts[MAX_PLAYERS];
    // NOTE(zoubir): the world is drawn into it while a rewind shows
    // (time_warp.cpp)
    render_target Target;
    bool32 Capturing;
};

// NOTE(zoubir): in MemoryArena, so a map switch does not drop it. 0 when
// there is no room there: a test's client, which keeps its memory in an
// arena of its own, has no rewind effects, and every caller copes
internal rewind_fx *
GetRewindFx(app_state *AppState)
{
    memory_arena *Arena = &AppState->MemoryArena;
    if (!AppState->RewindFx && Arena->Size - Arena->Used >= sizeof(rewind_fx) + 64)
    {
        AppState->RewindFx = AllocateStruct(Arena, rewind_fx);
        ZeroSize(AppState->RewindFx, sizeof(rewind_fx));
    }
    return AppState->RewindFx;
}

inline bool32
IsReplicaWorld(app_state *AppState)
{
    bool32 Result = AppState->Online && AppState->Online->Replicas.Active;
    return Result;
}

// NOTE(zoubir): client/online.cpp, on a new snapshot, before prediction
internal void
ReadRewindsFromSnapshot(app_state *AppState, replica_table *Replicas,
                        net_snapshot *Snapshot)
{
    rewind_fx *Fx = GetRewindFx(AppState);
    if (Fx)
    {
        ReadSnapshotRewinds(Fx->Casts, Replicas, Snapshot, Fx->Clock);
    }
}

// NOTE(zoubir): the local player is frozen by a rewind, so prediction
// leaves it where the server puts it (client/prediction.cpp)
internal bool32
IsLocalPlayerTimeLocked(app_state *AppState)
{
    world_entity *Player = GetLocalPlayer(AppState);
    rewind_fx *Fx = AppState->RewindFx;
    if (!Player || !Fx)
    {
        return false;
    }
    u32 Index = (u32)(Player - AppState->World.Entities);
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        if (IsFrozenByRewind(&Fx->Casts[Slot], Index))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): online only: what a playback has frozen is put where this
// screen showed it at the cursor. Offline the simulation moves it itself
internal void
PlayRewindsFromTrails(rewind_fx *Fx, app_state *AppState)
{
    world *World = &AppState->World;
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        rewind_fx_cast *Cast = &Fx->Casts[Slot];
        if (!Cast->Active || Cast->Phase != RewindPhase_Playback)
        {
            continue;
        }
        float Cursor = RewindFxCursor(Cast, Fx->Clock);
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Entity = &World->Entities[EntityIndex];
            if (!IsTrailedEntity(Entity) || !IsFrozenByRewind(Cast, EntityIndex))
            {
                continue;
            }
            rewind_fx_sample Sample;
            if (SampleRewindTrack(GetRewindTrack(&Fx->Trails, EntityIndex), Cursor, &Sample))
            {
                v3 OldPosition = Entity->Position;
                Entity->Position = Sample.Position;
                CheckAndChangeEntityChunk(AppState, World, &AppState->WorldArena,
                                          OldPosition, Entity);
            }
        }
    }
}

// NOTE(zoubir): app.cpp, once a frame after the world tick
internal void
UpdateRewindFx(app_state *AppState, float DeltaTime)
{
    rewind_fx *Fx = GetRewindFx(AppState);
    if (!Fx)
    {
        return;
    }
    Fx->Clock += DeltaTime;
    if (IsReplicaWorld(AppState))
    {
        for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
        {
            rewind_fx_cast *Cast = &Fx->Casts[Slot];
            Cast->PhaseLeft = Maximum(0.f, Cast->PhaseLeft - DeltaTime);
        }
        PlayRewindsFromTrails(Fx, AppState);
    }
    else
    {
        ReadSimRewinds(Fx->Casts, AppState, Fx->Clock);
    }
    UpdateRewindTrails(&Fx->Trails, AppState, Fx->Clock, DeltaTime);
}

#include "time_warp.cpp"
#include "rewind_draw.cpp"
