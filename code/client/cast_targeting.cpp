/* Cast targeting: the two ways an ability key can cast, picked with the
   checkbox at the top of the screen (ui/cast_mode_toggle.cpp).

   Quick cast: the key casts the moment it is pressed, toward the cursor.
   Standard cast (the default): a key whose ability covers an area or
   lands somewhere (TargetedButtons) first shows where it would hit
   (cast_targeting/previews.cpp); a left click or the same key again
   casts it, a right click cancels. The kunai aims the same way: its key
   shows its reach, and the click throws it at the unit the cursor is on
   (targeting.cpp). The fireball aims too, showing the line it will fly
   along; it used to cast at once like a basic attack, which read as
   quick cast being on when it was off. Keys that only move or guard the
   player (jump, dash, shield, slam) and the sword cast at once in both
   modes.
   In a dungeon run a role's spells aim only when they land on the
   ground (dungeon/role_targeting.cpp).

   A targeted key pressed while its ability recharges aims nothing: it
   is swallowed and the cursor says "Not ready" (targeting.cpp), so no
   preview promises a cast the simulation would refuse. A confirm the
   caller blocks (a kunai with no unit in reach) keeps the aim and says
   why instead of spending the press.

   Everything happens before the input reaches the simulation: the keys
   of the ability being aimed, and the right click that cancels it, are
   swallowed until they are released, and the confirmed ability is
   pressed for one frame. The left click casts nothing by itself (the
   fireball is on X), so it is read straight from the mouse. Offline that edits player_input.Pressed
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
                               PlayerButton_RewindBubble | PlayerButton_Kunai |                                PlayerButton_Cast)

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

// NOTE(zoubir): whether Button's cooldown has run out (or is close enough
// that a press now still counts, CanUseEarly)
internal bool32
IsCastReady(app_state *AppState, world_entity *Player, u32 Button)
{
    bool32 Result = true;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        float *Left = PlayerCooldown(AppState, Player, Index, &Full);
        if (Left && PlayerCooldownButton(Index) == Button && !CanUseEarly(*Left))
        {
            Result = false;
        }
    }
    return Result;
}

// NOTE(zoubir): the targeted buttons the local player has: alive, and
// the ability learned (a key the talent tree has not unlocked yet does
// nothing, so it aims nothing either)
internal u32
LearnedTargetedButtons(app_state *AppState)
{
    u32 Result = 0;
    world_entity *Player = GetLocalPlayer(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (Player && Player->IsPresent && !IsDeadPlayer(Player))
    {
        u32 Targeted = RoleTargetedButtons(AppState, CAST_TARGETED_BUTTONS);
        // NOTE(zoubir): in a dungeon run, what the class casts
        // (sim/dungeon/role_abilities.cpp, RunAllowedButtons)
        u32 Run = RunAllowedButtons(AppState, Slot, PLAYER_ALL_BUTTONS);
        for(u32 Bit = 1; Bit <= Targeted; Bit <<= 1)
        {
            if ((Bit & Targeted & Run) &&
                (IsDungeon(AppState) || AbilityLevel(Slot, Bit) > 0))
            {
                Result |= Bit;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the learned targeted buttons whose cooldown is still
// running
internal u32
RechargingButtons(app_state *AppState, u32 Learned)
{
    u32 Result = 0;
    world_entity *Player = GetLocalPlayer(AppState);
    for(u32 Bit = 1; Player && Bit <= Learned; Bit <<= 1)
    {
        if ((Bit & Learned) && !IsCastReady(AppState, Player, Bit))
        {
            Result |= Bit;
        }
    }
    return Result;
}

// NOTE(zoubir): once a frame, before the keys are read for the
// simulation: starts, confirms and cancels the aim. Blocked are buttons
// whose cast would be refused now for another reason than the cooldown
// (the kunai with no unit in reach). Returns the buttons pressed this
// frame that cannot go: recharging, or blocked, for the cursor's note
internal u32
UpdateCastTargeting(app_input *Input, app_state *AppState, u32 Blocked)
{
    cast_targeting *Targeting = GetCastTargeting(AppState);
    u32 Held = ActionButtonsFromKeys(Input, false);
    u32 Pressed = ActionButtonsFromKeys(Input, true);
    Targeting->Confirmed = 0;
    Targeting->Swallowed &= Held | Pressed;
    bool32 Click = Input->LeftButton.Pressed;
    // NOTE(zoubir): a click that belongs to a screen (the tile editor, the
    // talent panel, the mode checkbox) neither casts nor confirms
    if (AppState->TileEditing || TalentPanelHasMouse(AppState, Input) ||
        IsMouseOnCastModeToggle(Targeting, Input) || PartyFramesHaveMouse(AppState, Input))
    {
        Click = false;
        Pressed &= ~(u32)PlayerButton_Attack;
    }
    u32 Learned = LearnedTargetedButtons(AppState);
    u32 Recharging = RechargingButtons(AppState, Learned);
    u32 Refused = 0;
    u32 Aimable = Targeting->Mode == CastMode_Standard ? Learned : 0;
    if (!(Aimable & Targeting->Aiming))
    {
        Targeting->Aiming = 0;
    }
    u32 Fresh = Pressed & ~Targeting->Swallowed;
    if (Targeting->Aiming)
    {
        u32 Confirm = Fresh & Targeting->Aiming;
        if (Click)
        {
            Confirm |= Targeting->Aiming;
        }
        if (Confirm && (Targeting->Aiming & (Blocked | Recharging)))
        {
            // NOTE(zoubir): it would not go: the aim stays, the press is
            // spent on nothing, and the cursor says why
            Refused |= Targeting->Aiming;
            Targeting->Swallowed |= Confirm;
            Fresh &= ~Confirm;
        }
        else if (Confirm)
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
    // NOTE(zoubir): a key still recharging: standard mode shows no aim
    // for it and holds it back; quick mode lets it through (the
    // simulation ignores it). Both say "Not ready"
    u32 Early = Fresh & Recharging;
    Refused |= Early;
    Targeting->Swallowed |= Early & Aimable;
    Fresh &= ~(Early & Aimable);
    if (!Aimable)
    {
        Refused |= Fresh & Blocked & Learned;
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
    return Refused;
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

