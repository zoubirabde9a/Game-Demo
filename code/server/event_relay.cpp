/* Event relay: what the simulation reported (sounds, player deaths,
   visual bursts) reaches
   the clients through their snapshots. The server has no speakers and no
   screen, so after each tick RelayKeep copies the tick's events into a
   ring; each snapshot then carries, for its player, the events it has not
   been sent yet: every death, and the sounds and bursts within
   RELAY_HEARING_DISTANCE of it. A lost snapshot loses its events; they are
   cosmetic and not resent. Bursts past the NET_MAX_SNAPSHOT_BURSTS a
   snapshot holds wait for the next one, up to RELAY_BURST_WAIT ticks old,
   so a Chain Lightning through a pack or a Starfall shows every strike. */

// Sounds farther than this from a player are not sent to it (about a
// screen and a half).
#define RELAY_HEARING_DISTANCE 900.0f
// Events kept for the snapshots; at 20 snapshots a second this is several
// snapshots' worth even in a busy fight.
#define RELAY_SIZE 64
// A burst that did not fit its snapshot goes in a later one while it is
// at most this many ticks old (0.2 s), then is dropped.
#define RELAY_BURST_WAIT 12

struct relay_entry
{
    u32 Tick; // RelayKeep call that kept it; 0 = empty
    // NOTE(zoubir): bit N set once slot N has been sent it, or passed it
    // by; what SentTick leaves open is sent unless this says otherwise
    u8 SentTo;
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
        Entry->SentTo = 0;
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
    Out->BurstCount = 0;
    u8 Bit = (u8)(1u << Slot);
    // NOTE(zoubir): the tick of the first burst held for the next
    // snapshot, 0 for none
    u32 Held = 0;
    for (u32 Age = RELAY_SIZE; Age > 0; --Age)
    {
        relay_entry *Entry = &Relay->Entries[(Relay->Cursor + RELAY_SIZE - Age) % RELAY_SIZE];
        if (Entry->Tick == 0 || Entry->Tick <= Relay->SentTick[Slot] || (Entry->SentTo & Bit)) continue;
        Entry->SentTo |= Bit;
        sim_event *Event = &Entry->Event;
        bool32 Heard = !HasCenter || LengthSq(Event->Position.XY - Center) <=
            Square(RELAY_HEARING_DISTANCE);
        if (Event->Type == SimEvent_Sound)
        {
            if (!Heard) continue;
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
        else if (Event->Type == SimEvent_Burst)
        {
            if (Heard && Out->BurstCount == NET_MAX_SNAPSHOT_BURSTS &&
                Relay->Tick - Entry->Tick < RELAY_BURST_WAIT)
            {
                if (!Held) Held = Entry->Tick;
                Entry->SentTo &= (u8)~Bit;
                continue;
            }
            if (Heard && Out->BurstCount < NET_MAX_SNAPSHOT_BURSTS)
            {
                net_burst *Burst = &Out->Bursts[Out->BurstCount++];
                Burst->Kind = (u8)Event->Burst;
                Burst->Slot = Event->Slot;
                Burst->Angle = (u8)(i32)(Event->Angle * (128.f / Pi32));
                Burst->X = Event->Position.X;
                Burst->Y = Event->Position.Y;
                Burst->Z = Event->Position.Z;
            }
        }
    }
    Relay->SentTick[Slot] = Held ? Held - 1 : Relay->Tick;
}
