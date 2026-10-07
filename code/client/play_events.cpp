/* Handles what the simulation reported during the last tick: plays its
   sounds (a made sound at its level in the mix and a slightly different
   pitch each time, client/sounds/sounds.cpp), adds its kills to the kill feed (kill_feed.cpp) and starts its
   bursts (fx_bursts.cpp), then empties
   the queue for the next tick. Online the queue holds what the server's
   snapshot carried. */

// NOTE(zoubir): the pitch wander's dice, the client's alone
global_variable u32 GlobalSoundDice = 0x1234567u;

// NOTE(zoubir): Type at its level and a pitch that wanders a little;
// sounds read from the pack play as they are
internal void
PlayGameSound(app_state *AppState, asset_type_id Type)
{
    sound_effect *Effect = FindSoundEffect(Type);
    if (!Effect && Type >= AssetType_PackCount)
    {
        return;
    }
    playing_sound *Sound = PlaySound(AppState, {Type});
    if (Sound && Effect)
    {
        GlobalSoundDice = GlobalSoundDice * 1664525u + 1013904223u;
        float Roll = (float)(GlobalSoundDice >> 8) / 16777216.f;
        ChangeVolume(Sound, 0.f, V2(Effect->Gain, Effect->Gain));
        ChangePitch(Sound, 1.f + Effect->PitchJitter * (2.f * Roll - 1.f));
    }
}

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
            PlayGameSound(AppState, Event->Sound);
        }
        else if (Event->Type == SimEvent_Kill)
        {
            AddToKillFeed(AppState->KillFeed, Event);
        }
        else if (Event->Type == SimEvent_Burst)
        {
            AddBurst(AppState, Event->Burst, Event->Slot, Event->Position,
                     Event->Angle);
        }
    }
    Queue->Count = 0;
}
