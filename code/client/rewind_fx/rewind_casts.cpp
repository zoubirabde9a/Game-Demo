/* Rewind casts as the client sees them: one per player slot, with the
   phase it is in, when this machine first saw that phase, and which
   local entities it has frozen. Offline they are read from the
   simulation every frame; online from each new snapshot (net_rewind),
   counting down between snapshots.

   The times are this machine's clock, not the server's: the hold starts
   here when the frozen things stop on this screen, and the playback runs
   back through what this screen showed before that. */

#define REWIND_FX_MAX_FROZEN 96
// NOTE(zoubir): seconds of flash after a playback lands
#define REWIND_FX_AFTERGLOW 0.6f

struct rewind_fx_cast
{
    bool32 Active;
    rewind_kind Kind;
    rewind_phase Phase;
    float PhaseLeft;
    // NOTE(zoubir): rewind_fx.Clock when this phase, and the hold, were
    // first seen here
    float PhaseStart;
    float HoldStart;
    v3 Centre;
    float Radius;
    // NOTE(zoubir): a world rewind past its cast: every entity is frozen
    bool32 Everything;
    u32 FrozenCount;
    u32 Frozen[REWIND_FX_MAX_FROZEN];
    // NOTE(zoubir): once it is over, when it ended, and whether its
    // playback ran to the end (the afterglow shows only then)
    float EndedAt;
    bool32 Landed;
};

internal float
RewindPhaseSeconds(rewind_phase Phase)
{
    float Result = Phase == RewindPhase_Cast ? REWIND_CAST_SECONDS :
        Phase == RewindPhase_Hold ? REWIND_HOLD_SECONDS :
        Phase == RewindPhase_Playback ? REWIND_PLAYBACK_SECONDS : 0.f;
    return Result;
}

// NOTE(zoubir): 0 at the start of the phase, 1 at its end
inline float
RewindPhaseProgress(rewind_fx_cast *Cast)
{
    float Seconds = RewindPhaseSeconds(Cast->Phase);
    float Result = Seconds > 0.f ? RewindClamp01(1.f - Cast->PhaseLeft / Seconds) : 1.f;
    return Result;
}

// NOTE(zoubir): the moment of the trails a playback shows now: from the
// hold's start back REWIND_SECONDS, REWIND_PLAYBACK_SPEED times as fast
inline float
RewindFxCursor(rewind_fx_cast *Cast, float Clock)
{
    float Back = REWIND_PLAYBACK_SPEED * (Clock - Cast->PhaseStart);
    float Result = Cast->HoldStart - Minimum(Back, REWIND_SECONDS);
    return Result;
}

internal void
SeeRewindPhase(rewind_fx_cast *Cast, rewind_kind Kind, rewind_phase Phase,
               float PhaseLeft, float Clock)
{
    if (!Cast->Active || Cast->Kind != Kind || Phase < Cast->Phase)
    {
        *Cast = {};
        Cast->Active = true;
        Cast->Kind = Kind;
        Cast->PhaseStart = Clock;
        Cast->HoldStart = Clock;
    }
    if (Phase != Cast->Phase)
    {
        Cast->PhaseStart = Clock;
        if (Phase == RewindPhase_Hold)
        {
            Cast->HoldStart = Clock;
        }
        else if (Phase == RewindPhase_Playback && Cast->Phase != RewindPhase_Hold)
        {
            // NOTE(zoubir): the hold was missed (lost snapshots)
            Cast->HoldStart = Clock;
        }
    }
    Cast->Phase = Phase;
    Cast->PhaseLeft = PhaseLeft;
    Cast->Everything = (Kind == RewindKind_World && Phase >= RewindPhase_Hold);
}

internal void
EndRewindFxCast(rewind_fx_cast *Cast, float Clock)
{
    if (Cast->Active)
    {
        Cast->Active = false;
        Cast->EndedAt = Clock;
        Cast->Landed = (Cast->Phase == RewindPhase_Playback);
    }
}

inline bool32
IsInRewindAfterglow(rewind_fx_cast *Cast, float Clock)
{
    bool32 Result = !Cast->Active && Cast->Landed &&
        Clock - Cast->EndedAt < REWIND_FX_AFTERGLOW;
    return Result;
}

// NOTE(zoubir): offline, every frame: the simulation's rewinds
internal void
ReadSimRewinds(rewind_fx_cast *Casts, app_state *AppState, float Clock)
{
    time_rewind *Rewind = AppState->Rewind;
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        rewind_fx_cast *Cast = &Casts[Slot];
        rewind_cast *Sim = Rewind ? &Rewind->Casts[Slot] : 0;
        if (!Sim || Sim->Phase == RewindPhase_None)
        {
            EndRewindFxCast(Cast, Clock);
            continue;
        }
        SeeRewindPhase(Cast, Sim->Kind, Sim->Phase, Maximum(0.f, Sim->PhaseLeft), Clock);
        Cast->Centre = Sim->Centre;
        Cast->Radius = Sim->Radius;
        Cast->FrozenCount = 0;
        for(u32 Index = 0; Index < Sim->AffectedCount; Index++)
        {
            u32 ID = Sim->AffectedID[Index];
            u32 Serial = Sim->AffectedSerial[Index];
            if (Serial && Rewind->LockedSerial[ID] == Serial &&
                Cast->FrozenCount < REWIND_FX_MAX_FROZEN)
            {
                Cast->Frozen[Cast->FrozenCount++] = ID;
            }
        }
    }
}

// NOTE(zoubir): online, on each new snapshot (client/online.cpp): the
// rewinds it carries, its entities mapped to their replicas
internal void
ReadSnapshotRewinds(rewind_fx_cast *Casts, replica_table *Replicas,
                    net_snapshot *Snapshot, float Clock)
{
    bool32 Seen[MAX_PLAYERS] = {};
    for(u32 Index = 0; Index < Snapshot->RewindCount; Index++)
    {
        net_rewind *Sent = &Snapshot->Rewinds[Index];
        if (Sent->Slot >= MAX_PLAYERS || Sent->Kind >= RewindKind_Count ||
            Sent->Phase == RewindPhase_None || Sent->Phase >= RewindPhase_Count)
        {
            continue;
        }
        Seen[Sent->Slot] = true;
        rewind_fx_cast *Cast = &Casts[Sent->Slot];
        SeeRewindPhase(Cast, (rewind_kind)Sent->Kind, (rewind_phase)Sent->Phase,
                       Sent->PhaseLeft, Clock);
        Cast->Centre = V3(Sent->X, Sent->Y, 0.f);
        Cast->Radius = Sent->Radius;
        Cast->FrozenCount = 0;
        for(u32 Entity = 0; Entity < Snapshot->Count; Entity++)
        {
            if (!(Sent->Frozen[Entity / 8] & (1u << (Entity % 8))))
            {
                continue;
            }
            u32 Id = Snapshot->Entities[Entity].Id;
            u32 Local = Id < MAX_REPLICAS ? Replicas->LocalIndexPlusOne[Id] : 0;
            if (Local && Cast->FrozenCount < REWIND_FX_MAX_FROZEN)
            {
                Cast->Frozen[Cast->FrozenCount++] = Local - 1;
            }
        }
    }
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        if (!Seen[Slot])
        {
            EndRewindFxCast(&Casts[Slot], Clock);
        }
    }
}

// NOTE(zoubir): whether the cast has frozen local entity EntityIndex now
internal bool32
IsFrozenByRewind(rewind_fx_cast *Cast, u32 EntityIndex)
{
    if (!Cast->Active || Cast->Phase < RewindPhase_Hold)
    {
        return false;
    }
    if (Cast->Everything)
    {
        return true;
    }
    for(u32 Index = 0; Index < Cast->FrozenCount; Index++)
    {
        if (Cast->Frozen[Index] == EntityIndex)
        {
            return true;
        }
    }
    return false;
}
