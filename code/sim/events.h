#if !defined(SIM_EVENTS_H)
/* Simulation events: things the simulation wants the outside world to
   know about without doing them itself: a sound to play, a player killed.
   The client handles them after each tick (client/play_events.cpp); the
   server forwards them to clients in its snapshots. */

#define MAX_SIM_EVENTS 64

// NOTE(zoubir): a kill's Killer or KillerMonster when there is none
#define SIM_NOBODY 0xFF

enum sim_event_type
{
    SimEvent_Sound,
    // NOTE(zoubir): a player died: Victim is its slot; Killer is the slot
    // of the player behind the hit, KillerMonster the monster_kind of a
    // monster that did it, either SIM_NOBODY
    SimEvent_Kill,
};

struct sim_event
{
    sim_event_type Type;
    asset_type_id Sound;
    v3 Position;
    u8 Killer;
    u8 Victim;
    u8 KillerMonster;
};

struct sim_events
{
    sim_event Events[MAX_SIM_EVENTS];
    u32 Count;
};

// NOTE(zoubir): drops the event when the queue is full; sounds are not
// worth stalling the simulation for
inline void
EmitSound(sim_events *Queue, asset_type_id Sound, v3 Position)
{
    if (Queue->Count < MAX_SIM_EVENTS)
    {
        sim_event *Event = &Queue->Events[Queue->Count++];
        Event->Type = SimEvent_Sound;
        Event->Sound = Sound;
        Event->Position = Position;
    }
}

inline void
EmitKill(sim_events *Queue, u8 Killer, u8 Victim, u8 KillerMonster)
{
    if (Queue->Count < MAX_SIM_EVENTS)
    {
        sim_event *Event = &Queue->Events[Queue->Count++];
        *Event = {};
        Event->Type = SimEvent_Kill;
        Event->Killer = Killer;
        Event->Victim = Victim;
        Event->KillerMonster = KillerMonster;
    }
}

#define SIM_EVENTS_H
#endif
