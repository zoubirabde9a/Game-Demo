/* Class refusals (role_kits/class_kits.cpp): one place every class says
   a spell was pressed without the resource it costs (a Berserker's Rage,
   a Stormcaller's Charge, a Shadowblade's combo points). RefuseForResource
   sets the shared ClassFlags bits (class_refusal.h), which snapshots carry,
   so the local player's HUD shows it and plays the denied sound the same
   online as offline (ui/resource_refusal.cpp). The server decides; a
   client predicting its own player leaves it to the server. */

internal void
StampRefusalFlags(player_slot *Slot)
{
    class_refusal_slot *Refusal = &Slot->Refusal;
    u32 Flags = Slot->ClassFlags & ~(u32)CLASS_FLAG_REFUSED;
    Flags |= Refusal->Seconds > 0.f ? CLASS_FLAG_REFUSED : 0;
    Slot->ClassFlags = (u8)Flags;
}

// NOTE(zoubir): Slot pressed a spell short of its class's resource. A
// fresh press flips the beat (at most once in CLASS_REFUSAL_GAP, so
// mashing does not spam the sound) and holds the notice up; the same
// press tried again while it is kept (key_presses.cpp) changes nothing
internal void
RefuseForResource(player_slot *Slot)
{
    class_refusal_slot *Refusal = &Slot->Refusal;
    if (Slot->Predicted || !Refusal->Armed)
    {
        return;
    }
    Refusal->Armed = false;
    Refusal->Seconds = CLASS_REFUSED_SECONDS;
    if (Refusal->Gap <= 0.f)
    {
        Refusal->Gap = CLASS_REFUSAL_GAP;
        Slot->ClassFlags ^= CLASS_FLAG_REFUSAL_BEAT;
    }
    StampRefusalFlags(Slot);
}

// NOTE(zoubir): once a tick, after every class set its flags: the notice
// and the gap run out
internal void
UpdateClassRefusals(app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Predicted)
        {
            continue;
        }
        class_refusal_slot *Refusal = &Slot->Refusal;
        Refusal->Seconds = Maximum(0.f, Refusal->Seconds - DeltaTime);
        Refusal->Gap = Maximum(0.f, Refusal->Gap - DeltaTime);
        StampRefusalFlags(Slot);
    }
}
