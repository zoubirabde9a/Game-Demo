#if !defined(SIM_EVENTS_H)
/* Simulation events: things the simulation wants the outside world to
   know about without doing them itself: a sound to play, a player killed,
   a visual burst.
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
    // NOTE(zoubir): a visual burst (sim_burst) at Position, facing Angle
    // (radians, from +X), caused by the player in Slot (SIM_NOBODY for
    // none); clients draw it from their own look table
    SimEvent_Burst,
};

// NOTE(zoubir): every burst the simulation can ask for; how each looks is
// client/fx_bursts.cpp. Fits a byte on the wire.
enum sim_burst
{
    SimBurst_CastGather,   // an area ability's cast starting at a player
    SimBurst_PushCone,     // Push's cone, from a player along Angle
    SimBurst_LaunchColumn, // Launch's burst on the ground
    SimBurst_AirJump,      // a ring under a player's feet, the second jump
    SimBurst_Land,         // dust where a unit lands hard
    SimBurst_ShockwaveRing, // a ring around a player, Shockwave's reach
    SimBurst_Impact,       // sparks where a thrown body hits something
    SimBurst_Finisher,     // a combo's last hit landing, along Angle
    SimBurst_SwingArc,     // a sword swing around a player, centred on Angle
    SimBurst_SwingArcBack, // the next swing of a combo, the other way round
    SimBurst_SwingArcFinisher, // a combo's last swing, wider and brighter
    SimBurst_Skid,         // dust kicked forward when a run stops, along Angle
    SimBurst_Step,         // a footstep's dust; clients make these themselves
    SimBurst_Death,        // a monster or player bursting as it dies
    SimBurst_Spawn,        // a beam where a player comes back
    SimBurst_PushMark,     // Push's cone on the ground while it is cast
    SimBurst_LaunchMark,   // Launch's circle on the ground while it is cast
    SimBurst_SlamRing,     // a ring where a slam lands
    SimBurst_Count
};

struct sim_event
{
    sim_event_type Type;
    asset_type_id Sound;
    v3 Position;
    u8 Killer;
    u8 Victim;
    u8 KillerMonster;
    sim_burst Burst;
    float Angle;
    u8 Slot;
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

inline void
EmitBurst(sim_events *Queue, sim_burst Burst, u8 Slot, v3 Position,
          float Angle = 0.f)
{
    if (Queue->Count < MAX_SIM_EVENTS)
    {
        sim_event *Event = &Queue->Events[Queue->Count++];
        *Event = {};
        Event->Type = SimEvent_Burst;
        Event->Burst = Burst;
        Event->Position = Position;
        Event->Angle = Angle;
        Event->Slot = Slot;
    }
}

#define SIM_EVENTS_H
#endif
