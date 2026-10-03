/* Replica smoothing: snapshots come 20 times a second and frames 60 or
   more, so a replica put straight at each snapshot's position steps every
   50 ms. Instead each replica glides from where it is drawn to the newest
   snapshot's position over one snapshot interval (measured, not assumed):
   other players and monsters show about one interval late but move
   evenly. New replicas and jumps longer than REPLICA_SNAP_DISTANCE
   (respawns) snap. The local player is never smoothed; prediction
   (prediction.cpp) moves it.

   replicas.cpp calls BeginSmoothedSnapshot once per new snapshot,
   SetSmoothingTarget for each replica it applied, and SmoothReplica for
   each replica every frame after AdvanceSmoothing. */

#define MAX_REPLICAS ArrayCount(((world *)0)->Entities)
#define REPLICA_SNAP_DISTANCE (4.f * ARENA_TILE_SIZE)
#define REPLICA_DEFAULT_INTERVAL 0.05f

struct replica_smoothing
{
    float SinceSnapshot;
    // NOTE(zoubir): seconds between snapshots, a running average
    float Interval;
    bool32 Smooth[MAX_REPLICAS];
    v3 From[MAX_REPLICAS];
    v3 To[MAX_REPLICAS];
};

internal void
BeginSmoothedSnapshot(replica_smoothing *Smoothing)
{
    if (Smoothing->Interval == 0.f)
    {
        Smoothing->Interval = REPLICA_DEFAULT_INTERVAL;
    }
    else
    {
        float Measured = Smoothing->SinceSnapshot;
        if (Measured < 1.f / 120.f) Measured = 1.f / 120.f;
        if (Measured > 0.25f) Measured = 0.25f;
        Smoothing->Interval += 0.2f * (Measured - Smoothing->Interval);
    }
    Smoothing->SinceSnapshot = 0.f;
}

inline void
AdvanceSmoothing(replica_smoothing *Smoothing, float DeltaTime)
{
    Smoothing->SinceSnapshot += DeltaTime;
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

// NOTE(zoubir): call after the snapshot's state was applied, so Replica
// sits at the snapshot position; Drawn is where it was drawn before.
// Smooth is false for new replicas and the local player.
internal void
SetSmoothingTarget(app_state *AppState, memory_arena *Arena,
                   replica_smoothing *Smoothing, u32 Id,
                   world_entity *Replica, v3 Drawn, bool32 Smooth)
{
    v3 Target = Replica->Position;
    Smoothing->To[Id] = Target;
    Smoothing->Smooth[Id] = Smooth &&
        LengthSq(Target - Drawn) <= Square(REPLICA_SNAP_DISTANCE);
    Smoothing->From[Id] = Smoothing->Smooth[Id] ? Drawn : Target;
    if (Smoothing->Smooth[Id])
    {
        MoveReplicaTo(AppState, Arena, Replica, Drawn);
    }
}

internal void
SmoothReplica(app_state *AppState, memory_arena *Arena,
              replica_smoothing *Smoothing, u32 Id, world_entity *Replica)
{
    if (!Smoothing->Smooth[Id])
    {
        return;
    }
    float T = Smoothing->SinceSnapshot / Smoothing->Interval;
    if (T >= 1.f)
    {
        T = 1.f;
        Smoothing->Smooth[Id] = false;
    }
    v3 From = Smoothing->From[Id];
    MoveReplicaTo(AppState, Arena, Replica,
                  From + T * (Smoothing->To[Id] - From));
}
