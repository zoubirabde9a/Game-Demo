/* Handles what the simulation reported during the last tick: plays its
   sounds and adds its kills to the kill feed (kill_feed.cpp), then empties
   the queue for the next tick. Online the queue holds what the server's
   snapshot carried. */

internal void
PlaySimEvents(app_state *AppState, float DeltaTime)
{
    if (!AppState->KillFeed)
    {
        AppState->KillFeed = AllocateStruct(&AppState->MemoryArena, kill_feed);
        *AppState->KillFeed = {};
    }
    AgeKillFeed(AppState->KillFeed, DeltaTime);

    sim_events *Queue = &AppState->Events;
    for(u32 EventIndex = 0; EventIndex < Queue->Count; EventIndex++)
    {
        sim_event *Event = &Queue->Events[EventIndex];
        if (Event->Type == SimEvent_Sound)
        {
            PlaySound(AppState, {Event->Sound});
        }
        else if (Event->Type == SimEvent_Kill)
        {
            AddToKillFeed(AppState->KillFeed, Event);
        }
    }
    Queue->Count = 0;
}
