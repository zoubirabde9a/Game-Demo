/* Cast targeting: the two ways an ability key can cast, picked with the
   checkbox at the top of the screen (ui/cast_mode_toggle.cpp).

   Quick cast: the key casts the moment it is pressed, toward the cursor.
   Standard cast (the default): a key whose ability covers an area or
   lands somewhere (TargetedButtons) first shows where it would hit
   (cast_targeting/previews.cpp); a left click or the same key again
   casts it, a right click cancels. Keys that only move or guard the
   player (jump, dash, shield, slam) and the two basic attacks cast at
   once in both modes.

   Everything happens before the input reaches the simulation: the keys
   of the ability being aimed, and the click that confirms or cancels it,
   are swallowed until they are released, and the confirmed ability is
   pressed for one frame. Offline that edits player_input.Pressed
   (keyboard_input.cpp); online it edits the copy of the keys the server
   gets (cursor.cpp), which sends held keys and lets the server find the
   presses, so a one-frame press and a swallowed key read the same way
   there. The simulation never knows which mode a player uses. */

enum cast_mode
{
    CastMode_Standard,
    CastMode_Quick,
};

struct cast_targeting
{
    cast_mode Mode;
    // NOTE(zoubir): the player_button being aimed, 0 for none
    u32 Aiming;
    // NOTE(zoubir): player_buttons whose keys count as up until released
    u32 Swallowed;
    // NOTE(zoubir): the button pressed this frame by a confirmed aim
    u32 Confirmed;
    // NOTE(zoubir): seconds since aiming began, for the preview's fade in
    float AimSeconds;
    // NOTE(zoubir): the mode checkbox, in window pixels, as drawn last
    // frame (ui/cast_mode_toggle.cpp); a click on it is not a cast
    float ToggleX, ToggleY, ToggleWidth, ToggleHeight;
};

// NOTE(zoubir): the abilities standard mode aims first: each hits an
// area or lands at a spot the player should see before committing
#define CAST_TARGETED_BUTTONS (PlayerButton_Shockwave | PlayerButton_Push | \
                               PlayerButton_Launch | PlayerButton_FrostNova | \
                               PlayerButton_GravityWell | PlayerButton_Blink | \
                               PlayerButton_RewindBubble)

// NOTE(zoubir): made on first use, in MemoryArena so a map switch keeps it
internal cast_targeting *
GetCastTargeting(app_state *AppState)
{
    if (!AppState->CastTargeting)
    {
        AppState->CastTargeting = AllocateStruct(&AppState->MemoryArena, cast_targeting);
        *AppState->CastTargeting = {};
    }
    return AppState->CastTargeting;
}

inline bool32
IsMouseOnCastModeToggle(cast_targeting *Targeting, app_input *Input)
{
    bool32 Result = Targeting->ToggleWidth > 0.f &&
        IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                           Targeting->ToggleX, Targeting->ToggleY,
                           Targeting->ToggleWidth, Targeting->ToggleHeight);
    return Result;
}

// NOTE(zoubir): the targeted buttons the local player can use now: alive,
// and the ability learned (a key the talent tree has not unlocked yet
// does nothing, so it aims nothing either)
internal u32
AimableButtons(app_state *AppState)
{
    u32 Result = 0;
    world_entity *Player = GetLocalPlayer(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (Player && Player->IsPresent && !IsDeadPlayer(Player))
    {
        for(u32 Bit = 1; Bit <= CAST_TARGETED_BUTTONS; Bit <<= 1)
        {
            if ((Bit & CAST_TARGETED_BUTTONS) && AbilityLevel(Slot, Bit) > 0)
            {
                Result |= Bit;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): once a frame, before the keys are read for the
// simulation: starts, confirms and cancels the aim
internal void
UpdateCastTargeting(app_input *Input, app_state *AppState)
{
    cast_targeting *Targeting = GetCastTargeting(AppState);
    u32 Held = ActionButtonsFromKeys(Input, false);
    u32 Pressed = ActionButtonsFromKeys(Input, true);
    u32 Clicks = PlayerButton_Cast | PlayerButton_Attack;
    Targeting->Confirmed = 0;
    Targeting->Swallowed &= Held | Pressed;
    // NOTE(zoubir): a click that belongs to a screen (the tile editor, the
    // talent panel, the mode checkbox) neither casts nor confirms
    if (AppState->TileEditing || TalentPanelHasMouse(AppState, Input) ||
        IsMouseOnCastModeToggle(Targeting, Input))
    {
        Targeting->Swallowed |= Pressed & PlayerButton_Cast;
        Pressed &= ~Clicks;
    }
    u32 Aimable = Targeting->Mode == CastMode_Standard ? AimableButtons(AppState) : 0;
    if (!(Aimable & Targeting->Aiming))
    {
        Targeting->Aiming = 0;
    }
    u32 Fresh = Pressed & ~Targeting->Swallowed;
    if (Targeting->Aiming)
    {
        u32 Confirm = Fresh & (PlayerButton_Cast | Targeting->Aiming);
        if (Confirm)
        {
            Targeting->Confirmed = Targeting->Aiming;
            Targeting->Swallowed |= Confirm;
            Targeting->Aiming = 0;
            Fresh &= ~Confirm;
        }
        else if (Fresh & PlayerButton_Attack)
        {
            Targeting->Swallowed |= PlayerButton_Attack;
            Targeting->Aiming = 0;
            Fresh &= ~(u32)PlayerButton_Attack;
        }
    }
    u32 Start = Fresh & Aimable;
    if (Start)
    {
        // NOTE(zoubir): two at once: the lowest bit wins, the other is
        // dropped rather than cast blind
        Targeting->Aiming = Start & (~Start + 1);
        Targeting->AimSeconds = 0.f;
        Targeting->Swallowed |= Start;
    }
    if (Targeting->Aiming)
    {
        Targeting->AimSeconds += Input->DeltaTime;
    }
}

// NOTE(zoubir): Buttons (pressed or held player_button bits) as the
// simulation should see them this frame
inline u32
FilterCastButtons(app_state *AppState, u32 Buttons)
{
    cast_targeting *Targeting = GetCastTargeting(AppState);
    u32 Result = (Buttons & ~Targeting->Swallowed) | Targeting->Confirmed;
    return Result;
}

// NOTE(zoubir): the same, on a copy of the keys (the online session reads
// the action keys from it, cursor.cpp)
internal void
FilterCastKeys(app_input *Keys, app_state *AppState)
{
    cast_targeting *Targeting = GetCastTargeting(AppState);
    action_key Table[ACTION_KEY_COUNT];
    GetActionKeys(Keys, Table);
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        app_button_state *Key = Table[Index].Key;
        if (Table[Index].Button & Targeting->Swallowed)
        {
            Key->Pressed = false;
            Key->EndedDown = false;
        }
        if (Table[Index].Button & Targeting->Confirmed)
        {
            Key->Pressed = true;
            Key->EndedDown = true;
        }
    }
}

