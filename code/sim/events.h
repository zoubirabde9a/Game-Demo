#if !defined(SIM_EVENTS_H)
/* Simulation events: things the simulation wants the outside world to
   know about without doing them itself, like playing a sound. The client
   plays them after each tick; the server can forward them to clients. */

#define MAX_SIM_EVENTS 64

enum sim_event_type
{
    SimEvent_Sound,
};

struct sim_event
{
    sim_event_type Type;
    asset_type_id Sound;
    v3 Position;
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

#define SIM_EVENTS_H
#endif
