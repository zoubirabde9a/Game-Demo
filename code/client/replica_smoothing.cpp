/* Replica smoothing: snapshots come 20 times a second, frames 60 or more,
   and over the internet each snapshot is a little early or late. Every
   replica keeps the positions of its last few snapshots, each with the
   server tick it was taken on, and is drawn where the server had it at a
   render tick a short delay behind the newest snapshot, on a curve
   between the two snapshots either side that matches the velocity each
   snapshot sent. So other players and monsters move evenly,
   and a late or lost snapshot is covered by the ones on each side of it
   instead of making them stop and then catch up.

   The render clock runs at the server's tick rate, a few percent faster
   or slower to follow the server as the snapshots arrive. The delay is
   one snapshot interval plus a margin that grows with how unevenly they
   arrive (REPLICA_DELAY_*): about 60 ms on a steady link. When the render
   tick passes the newest snapshot (one lost, or very late), a replica
   carries on at its last velocity for at most
   REPLICA_MAX_EXTRAPOLATE ticks, then waits.

   New replicas and jumps longer than REPLICA_SNAP_DISTANCE (respawns)
   start over from the new position. The local player is never smoothed;
   prediction (prediction.cpp) moves it.

   replicas.cpp calls BeginSmoothedSnapshot once per new snapshot,
   SetSmoothingTarget for each replica it applied, and SmoothReplica for
   each replica every frame after AdvanceSmoothing. */

#define MAX_REPLICAS ArrayCount(((world *)0)->Entities)
#define REPLICA_SNAP_DISTANCE (4.f * ARENA_TILE_SIZE)
#define REPLICA_HISTORY 6
// NOTE(zoubir): the delay, in server ticks: one snapshot interval, plus
// REPLICA_DELAY_JITTER times how far snapshots arrive from when they are
// expected, plus REPLICA_DELAY_SPARE, never outside MIN..MAX
#define REPLICA_DELAY_JITTER 2.5f
#define REPLICA_DELAY_SPARE 0.5f
#define REPLICA_DELAY_MIN 2.f
#define REPLICA_DELAY_MAX 12.f
// NOTE(zoubir): the render clock speeds up or slows down by this share per
// tick it is off, at most REPLICA_CLOCK_MAX_ADJUST; further off than
// REPLICA_CLOCK_RESYNC ticks it jumps
#define REPLICA_CLOCK_GAIN 0.08f
#define REPLICA_CLOCK_MAX_ADJUST 0.1f
#define REPLICA_CLOCK_RESYNC 30.f
#define REPLICA_MAX_EXTRAPOLATE 3.f
#define REPLICA_DEFAULT_STEP 3

struct replica_smoothing
{
    bool32 HasClock;
    // NOTE(zoubir): where the server's tick is now, by the snapshots'
    // arrivals; the tick replicas are drawn at; and how far behind
    float ServerTick;
    float RenderTick;
    float Delay;
    // NOTE(zoubir): ticks a snapshot arrives early or late, on average
    float Jitter;
    // NOTE(zoubir): server ticks between snapshots, the smallest gap seen
    // (a lost snapshot only makes a gap bigger)
    u32 SnapshotStep;
    u32 NewestTick;

    // NOTE(zoubir): per replica, a ring of its snapshot positions, the
    // newest at Head; Count 0 for the local player
    u8 Count[MAX_REPLICAS];
    u8 Head[MAX_REPLICAS];
    u32 SampleTick[MAX_REPLICAS][REPLICA_HISTORY];
    v3 SamplePosition[MAX_REPLICAS][REPLICA_HISTORY];
    // NOTE(zoubir): units a second, as the snapshot sent it
    v3 SampleVelocity[MAX_REPLICAS][REPLICA_HISTORY];
};

internal void
BeginSmoothedSnapshot(replica_smoothing *Smoothing, u32 Tick)
{
    if (!Smoothing->HasClock)
    {
        Smoothing->HasClock = true;
        Smoothing->SnapshotStep = REPLICA_DEFAULT_STEP;
        Smoothing->ServerTick = (float)Tick;
        Smoothing->Delay = (float)REPLICA_DEFAULT_STEP + REPLICA_DELAY_SPARE;
        Smoothing->RenderTick = (float)Tick - Smoothing->Delay;
        Smoothing->NewestTick = Tick;
        return;
    }
    if (Tick > Smoothing->NewestTick)
    {
        u32 Gap = Tick - Smoothing->NewestTick;
        if (Gap < Smoothing->SnapshotStep)
        {
            Smoothing->SnapshotStep = Gap;
        }
        Smoothing->NewestTick = Tick;
    }
    float Error = (float)Tick - Smoothing->ServerTick;
    if (Error > REPLICA_CLOCK_RESYNC || Error < -REPLICA_CLOCK_RESYNC)
    {
        Smoothing->ServerTick = (float)Tick;
        Smoothing->RenderTick = (float)Tick - Smoothing->Delay;
        Smoothing->Jitter = 0.f;
        return;
    }
    Smoothing->ServerTick += 0.1f * Error;
    float Off = Error < 0.f ? -Error : Error;
    Smoothing->Jitter += 0.1f * (Off - Smoothing->Jitter);
    float Wanted = (float)Smoothing->SnapshotStep +
        REPLICA_DELAY_JITTER * Smoothing->Jitter + REPLICA_DELAY_SPARE;
    Wanted = Maximum(REPLICA_DELAY_MIN, Minimum(REPLICA_DELAY_MAX, Wanted));
    Smoothing->Delay += 0.05f * (Wanted - Smoothing->Delay);
}

// NOTE(zoubir): TickRate is the server's ticks a second
inline void
AdvanceSmoothing(replica_smoothing *Smoothing, float DeltaTime, float TickRate)
{
    if (!Smoothing->HasClock)
    {
        return;
    }
    // NOTE(zoubir): how far off the render tick is before this frame moves
    // both clocks on
    float Ticks = DeltaTime * TickRate;
    float Off = (Smoothing->ServerTick - Smoothing->Delay) - Smoothing->RenderTick;
    Smoothing->ServerTick += Ticks;
    if (Off > REPLICA_CLOCK_RESYNC || Off < -REPLICA_CLOCK_RESYNC)
    {
        Smoothing->RenderTick = Smoothing->ServerTick - Smoothing->Delay;
        return;
    }
    float Adjust = REPLICA_CLOCK_GAIN * Off;
    Adjust = Maximum(-REPLICA_CLOCK_MAX_ADJUST, Minimum(REPLICA_CLOCK_MAX_ADJUST, Adjust));
    Smoothing->RenderTick += Ticks * (1.f + Adjust);
}

// NOTE(zoubir): moves a replica, keeping the world's chunk lists right
internal void
MoveReplicaTo(app_state *AppState, memory_arena *Arena, world_entity *Replica,
              v3 Position)
{
    v3 OldPosition = Replica->Position;
    Replica->Position = Position;
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Replica);
}

inline u32
ReplicaSampleIndex(replica_smoothing *Smoothing, u32 Id, u32 Back)
{
    u32 Result = (Smoothing->Head[Id] + REPLICA_HISTORY - Back) % REPLICA_HISTORY;
    return Result;
}

// NOTE(zoubir): call after the snapshot's state was applied, so Replica
// sits at the snapshot position; Drawn is where it was drawn before.
// Reused is false for a replica made by this snapshot.
internal void
SetSmoothingTarget(app_state *AppState, memory_arena *Arena,
                   replica_smoothing *Smoothing, u32 Id, u32 Tick,
                   world_entity *Replica, v3 Drawn, bool32 Reused,
                   bool32 IsLocalPlayer)
{
    if (IsLocalPlayer)
    {
        Smoothing->Count[Id] = 0;
        return;
    }
    v3 Target = Replica->Position;
    u32 Count = Smoothing->Count[Id];
    bool32 Restart = !Reused || Count == 0;
    if (!Restart)
    {
        u32 Newest = Smoothing->Head[Id];
        v3 Last = Smoothing->SamplePosition[Id][Newest];
        Restart = LengthSq(Target - Last) > Square(REPLICA_SNAP_DISTANCE) ||
            Tick <= Smoothing->SampleTick[Id][Newest];
    }
    if (Restart)
    {
        Smoothing->Count[Id] = 1;
        Smoothing->Head[Id] = 0;
        Smoothing->SampleTick[Id][0] = Tick;
        Smoothing->SamplePosition[Id][0] = Target;
        Smoothing->SampleVelocity[Id][0] = Replica->Velocity;
        return;
    }
    u32 Head = (Smoothing->Head[Id] + 1) % REPLICA_HISTORY;
    Smoothing->Head[Id] = (u8)Head;
    Smoothing->SampleTick[Id][Head] = Tick;
    Smoothing->SamplePosition[Id][Head] = Target;
    Smoothing->SampleVelocity[Id][Head] = Replica->Velocity;
    if (Count < REPLICA_HISTORY)
    {
        Smoothing->Count[Id] = (u8)(Count + 1);
    }
    // NOTE(zoubir): drawn where it was until SmoothReplica places it
    MoveReplicaTo(AppState, Arena, Replica, Drawn);
}

// NOTE(zoubir): the curve from one position to the next that leaves the
// first at its velocity and reaches the second at its own (cubic
// Hermite), T 0..1 over Seconds. A straight line changed speed all at once
// at every snapshot, which showed as a small jolt 20 times a second
inline v3
MotionCurve(v3 P0, v3 V0, v3 P1, v3 V1, float Seconds, float T)
{
    float T2 = T * T;
    float T3 = T2 * T;
    v3 Result = (2.f * T3 - 3.f * T2 + 1.f) * P0 +
        (T3 - 2.f * T2 + T) * Seconds * V0 +
        (-2.f * T3 + 3.f * T2) * P1 +
        (T3 - T2) * Seconds * V1;
    return Result;
}

// NOTE(zoubir): where the replica was at the render tick, by its samples
internal v3
ReplicaPositionAt(replica_smoothing *Smoothing, u32 Id, float RenderTick)
{
    float TickSeconds = 1.f / (float)NET_TICK_RATE;
    u32 Count = Smoothing->Count[Id];
    u32 Newest = ReplicaSampleIndex(Smoothing, Id, 0);
    float NewestTick = (float)Smoothing->SampleTick[Id][Newest];
    if (RenderTick >= NewestTick)
    {
        // NOTE(zoubir): past the newest snapshot: on at its velocity, for a
        // few ticks at most
        float Ahead = Minimum(RenderTick - NewestTick, REPLICA_MAX_EXTRAPOLATE);
        v3 Result = Smoothing->SamplePosition[Id][Newest];
        if (Count > 1)
        {
            Result += (Ahead * TickSeconds) * Smoothing->SampleVelocity[Id][Newest];
        }
        return Result;
    }
    for(u32 Back = 1; Back < Count; Back++)
    {
        u32 Older = ReplicaSampleIndex(Smoothing, Id, Back);
        u32 Newer = ReplicaSampleIndex(Smoothing, Id, Back - 1);
        float OlderTick = (float)Smoothing->SampleTick[Id][Older];
        if (RenderTick >= OlderTick)
        {
            float Span = (float)Smoothing->SampleTick[Id][Newer] - OlderTick;
            float T = Span > 0.f ? (RenderTick - OlderTick) / Span : 1.f;
            v3 Result = MotionCurve(Smoothing->SamplePosition[Id][Older],
                                     Smoothing->SampleVelocity[Id][Older],
                                     Smoothing->SamplePosition[Id][Newer],
                                     Smoothing->SampleVelocity[Id][Newer],
                                     Span * TickSeconds, T);
            return Result;
        }
    }
    // NOTE(zoubir): older than every sample kept: the oldest
    return Smoothing->SamplePosition[Id][ReplicaSampleIndex(Smoothing, Id, Count - 1)];
}

internal void
SmoothReplica(app_state *AppState, memory_arena *Arena,
              replica_smoothing *Smoothing, u32 Id, world_entity *Replica)
{
    if (!Smoothing->Count[Id] || !Smoothing->HasClock)
    {
        return;
    }
    MoveReplicaTo(AppState, Arena, Replica,
                  ReplicaPositionAt(Smoothing, Id, Smoothing->RenderTick));
}
