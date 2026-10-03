/* Plays what the simulation asked for during the last tick (sounds for
   now), then empties the queue for the next tick. */

internal void
PlaySimEvents(app_state *AppState)
{
    sim_events *Queue = &AppState->Events;
    for(u32 EventIndex = 0; EventIndex < Queue->Count; EventIndex++)
    {
        sim_event *Event = &Queue->Events[EventIndex];
        if (Event->Type == SimEvent_Sound)
        {
            PlaySound(AppState, {Event->Sound});
        }
    }
    Queue->Count = 0;
}
