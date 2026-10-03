/* Event relay: what the simulation reported (sounds, player deaths) reaches
   the clients through their snapshots. The server has no speakers and no
   screen, so after each tick RelayKeep copies the tick's events into a
   ring; each snapshot then carries, for its player, the events it has not
   been sent yet: every death, and the sounds that played within
   RELAY_HEARING_DISTANCE of it. A lost snapshot loses its events; they are
   cosmetic and not resent. */

// Sounds farther than this from a player are not sent to it (about a
// screen and a half).
#define RELAY_HEARING_DISTANCE 900.0f
// Events kept for the snapshots; at 20 snapshots a second this is several
// snapshots' worth even in a busy fight.
#define RELAY_SIZE 64

struct relay_entry
{
    u32 Tick; // RelayKeep call that kept it; 0 = empty
    sim_event Event;
};

struct event_relay
{
    u32 Tick;
    relay_entry Entries[RELAY_SIZE];
    u32 Cursor;
    // Each slot has been sent everything kept up to this tick.
    u32 SentTick[NET_MAX_CLIENTS];
};

// After every simulation tick, before the queue is emptied.
internal void
RelayKeep(event_relay *Relay, sim_events *Events)
{
    Relay->Tick++;
    for (u32 Index = 0; Index < Events->Count; ++Index)
    {
        sim_event *Event = &Events->Events[Index];
        if (Event->Type == SimEvent_Sound && (u32)Event->Sound > 255) continue;
        relay_entry *Entry = &Relay->Entries[Relay->Cursor++ % RELAY_SIZE];
        Entry->Tick = Relay->Tick;
        Entry->Event = *Event;
    }
}

// A player who just joined hears only what happens from now on.
internal void
RelayJoined(event_relay *Relay, u32 Slot)
{
    Relay->SentTick[Slot] = Relay->Tick;
}

// Fills Out's sounds and kills for the player in Slot, standing at Center
// (HasCenter false: no player, so every sound counts as heard).
internal void
RelayWrite(event_relay *Relay, u32 Slot, bool32 HasCenter, v2 Center,
           net_snapshot *Out)
{
    Out->SoundCount = 0;
    Out->KillCount = 0;
    for (u32 Age = RELAY_SIZE; Age > 0; --Age)
    {
        relay_entry *Entry = &Relay->Entries[(Relay->Cursor + RELAY_SIZE - Age) % RELAY_SIZE];
        if (Entry->Tick == 0 || Entry->Tick <= Relay->SentTick[Slot]) continue;
        sim_event *Event = &Entry->Event;
        if (Event->Type == SimEvent_Sound)
        {
            if (HasCenter && LengthSq(Event->Position.XY - Center) >
                Square(RELAY_HEARING_DISTANCE)) continue;
            if (Out->SoundCount < NET_MAX_SNAPSHOT_SOUNDS)
            {
                Out->Sounds[Out->SoundCount++] = (u8)Event->Sound;
            }
        }
        else if (Event->Type == SimEvent_Kill)
        {
            if (Out->KillCount < NET_MAX_SNAPSHOT_KILLS)
            {
                net_kill *Kill = &Out->Kills[Out->KillCount++];
                Kill->Killer = Event->Killer;
                Kill->Victim = Event->Victim;
                Kill->KillerMonster = Event->KillerMonster;
            }
        }
    }
    Relay->SentTick[Slot] = Relay->Tick;
}
