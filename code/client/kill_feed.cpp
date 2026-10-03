/* Kill feed: the last few player deaths, newest first, for the UI to
   list ("Gary killed Mira", "a Bat killed Mira"). Filled from SimEvent_Kill
   by PlaySimEvents, offline from the local simulation and online from the
   server's snapshots; entries drop out after KILL_FEED_SECONDS. Names come
   from AppState->Players[Slot].Name (empty: "Player N"). */

#define KILL_FEED_SIZE 5
#define KILL_FEED_SECONDS 6.f

struct kill_feed_entry
{
    u8 Killer;        // player slot or SIM_NOBODY
    u8 Victim;        // player slot
    u8 KillerMonster; // monster_kind or SIM_NOBODY
    float Age;        // seconds since it happened
};

struct kill_feed
{
    kill_feed_entry Entries[KILL_FEED_SIZE]; // newest first
    u32 Count;
};

internal void
AddToKillFeed(kill_feed *Feed, sim_event *Kill)
{
    u32 Keep = (Feed->Count < KILL_FEED_SIZE) ? Feed->Count : KILL_FEED_SIZE - 1;
    for(u32 Index = Keep; Index > 0; Index--)
    {
        Feed->Entries[Index] = Feed->Entries[Index - 1];
    }
    kill_feed_entry *Entry = &Feed->Entries[0];
    Entry->Killer = Kill->Killer;
    Entry->Victim = Kill->Victim;
    Entry->KillerMonster = Kill->KillerMonster;
    Entry->Age = 0.f;
    Feed->Count = Keep + 1;
}

internal void
AgeKillFeed(kill_feed *Feed, float DeltaTime)
{
    for(u32 Index = 0; Index < Feed->Count; Index++)
    {
        Feed->Entries[Index].Age += DeltaTime;
        if (Feed->Entries[Index].Age > KILL_FEED_SECONDS)
        {
            Feed->Count = Index;
            break;
        }
    }
}
