/* Touch controls: on a phone or tablet the page (web/shell.html) draws a
   joystick on the left and the player's abilities as buttons on the
   right, over the canvas, and this file turns what they report into the
   keys and the cursor a keyboard and mouse would give. The game itself
   never knows a finger did it, so key bindings, both cast modes and
   online play work as they do on the desktop.

   The joystick holds the bound movement keys, in eight directions like
   the keys. An ability button holds that ability's bound key. Dragging
   from a button aims: the cursor goes to the player plus the drag, a
   full drag reaching PLAYER_AIM_REACH. A press with no drag aims at the
   nearest foe in reach (or the last direction dragged), so a tap throws
   at someone.

   Cast modes (client/cast_targeting.cpp): in standard cast the key goes
   down when the finger does, so an aimed ability shows where it would
   land while the finger drags, and lifting the finger presses the key
   again to cast it. In quick cast an aimed ability's key is pressed when
   the finger lifts, at the spot aimed.

   Taps elsewhere on the canvas reach the game as mouse clicks (the page
   sends them), so menus, the talent panel and a free click's fireball
   work as with a mouse. On a phone the game lays out a window larger
   than the page (WebTouchUiScale), so its screens, made for a desktop,
   leave room for the controls. Each frame the page gets the abilities the bar
   shows, with their names and cooldowns, and whether any screen covers
   the play (WebShowTouch). */

#define WEB_TOUCH_STICK_DEAD 0.3f
// NOTE(zoubir): cos 67.5 degrees: past this share of the stick's push an
// axis counts, which splits the circle into eight equal directions
#define WEB_TOUCH_STICK_AXIS 0.38f
#define WEB_TOUCH_AIM_DEAD 0.2f

struct web_touch
{
    bool32 On;
    float MoveX, MoveY;
    u32 Held;     // player_buttons whose button a finger is on
    u32 WasHeld;  // the same, last frame
    u32 Dragging; // the player_button whose button the finger left, 0 for none
    float AimX, AimY;
    // NOTE(zoubir): player_buttons to press once next frame, after their
    // key was seen up
    u32 Queued;
    v2 LastAim; // the last direction dragged, length 1; zero for none yet
};

global_variable web_touch WebTouch;

// NOTE(zoubir): what the page's controls report, as ints: the stick and
// the drag in thousandths, the held buttons as player_button bits. 0
// while the page shows no touch controls
EM_JS(int, WebReadTouch, (int *Out), {
    var Touch = Module.touch;
    if (!Touch || !Touch.on) return 0;
    var At = Number(Out) >> 2;
    HEAP32[At + 0] = Math.round(Touch.moveX * 1000);
    HEAP32[At + 1] = Math.round(Touch.moveY * 1000);
    HEAP32[At + 2] = Touch.read();
    HEAP32[At + 3] = Touch.dragging;
    HEAP32[At + 4] = Math.round(Touch.aimX * 1000);
    HEAP32[At + 5] = Math.round(Touch.aimY * 1000);
    return 1;
});

EM_JS(void, WebSendTouchState, (const char *State), {
    if (Module.touch && Module.touch.show) Module.touch.show(UTF8ToString(Number(State)));
});

// NOTE(zoubir): game pixels per CSS pixel: 1, except with touch controls
// on a screen less than WEB_TOUCH_UI_HEIGHT tall either way up, where the
// game lays out that many and draws them smaller
#define WEB_TOUCH_UI_HEIGHT 480.0
internal float
WebTouchUiScale(double CssWidth, double CssHeight)
{
    double Short = Minimum(CssWidth, CssHeight);
    float Result = 1.f;
    if (WebTouch.On && Short > 1.0 && Short < WEB_TOUCH_UI_HEIGHT)
    {
        Result = (float)Minimum(WEB_TOUCH_UI_HEIGHT / Short, 1.5);
    }
    return Result;
}

// NOTE(zoubir): the web_input index of a key code's button
internal int
WebButtonIndexOfKey(app_input *Input, u32 Code)
{
    app_button_state *State = KeyCodeState(Input, Code);
    for (u32 Index = 0; Index < WEB_BUTTON_COUNT; Index++)
    {
        if (WebButton(Input, Index) == State) return (int)Index;
    }
    return -1;
}

// NOTE(zoubir): the binding row of a player_button, -1 for none
internal int
WebActionRow(u32 Button)
{
    for (u32 Row = 0; Row < ACTION_KEY_COUNT; Row++)
    {
        if (BindingDefs[Row].Button == Button) return (int)Row;
    }
    return -1;
}

internal void
WebHoldRow(app_input *Scratch, u32 Row, bool32 Press)
{
    u32 Code = BoundKeyCode(Row);
    int Index = Code != KeyCode_None ? WebButtonIndexOfKey(Scratch, Code) : -1;
    if (Index >= 0)
    {
        WebInput.TouchDown[Index] = true;
        if (Press) WebInput.WentDown[Index] = true;
    }
}

internal void
WebPressButtons(app_input *Scratch, u32 Buttons, bool32 Hold)
{
    for (u32 Bit = 1; Bit && Bit <= Buttons; Bit <<= 1)
    {
        int Row = (Buttons & Bit) ? WebActionRow(Bit) : -1;
        if (Row < 0) continue;
        if (Hold) WebHoldRow(Scratch, (u32)Row, false);
        else
        {
            u32 Code = BoundKeyCode((u32)Row);
            int Index = Code != KeyCode_None ? WebButtonIndexOfKey(Scratch, Code) : -1;
            if (Index >= 0) WebInput.WentDown[Index] = true;
        }
    }
}

// NOTE(zoubir): a world point on the window, in game pixels, with the
// camera on screen now (the inverse of CursorInWorld)
internal void
WebPutCursorAt(app_state *AppState, v2 World)
{
    float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
    WebInput.MouseX = (i32)((World.X - SnapToScreenPixel(AppState->CameraOffset.X, Zoom)) * Zoom);
    WebInput.MouseY = (i32)((World.Y - SnapToScreenPixel(AppState->CameraOffset.Y, Zoom)) * Zoom);
}

// NOTE(zoubir): where a press with no drag aims: the middle of the
// nearest foe's body in reach, else the last direction dragged at half
// reach
internal v2
WebTouchAutoAim(app_state *AppState, world_entity *Player)
{
    v2 From = Player->Position.XY;
    v2 Direction = LengthSq(WebTouch.LastAim) > 0.f ? WebTouch.LastAim : V2(1.f, 0.f);
    v2 Result = From + Direction * (0.5f * PLAYER_AIM_REACH);
    float Best = PLAYER_AIM_REACH * PLAYER_AIM_REACH;
    world *World = &AppState->World;
    for (u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (Unit == Player || !IsCursorPickable(AppState, Unit, Player)) continue;
        float Distance = LengthSq(Unit->Position.XY - From);
        if (Distance < Best)
        {
            Best = Distance;
            target_box Box = TargetBodyBox(AppState, Unit);
            Result = 0.5f * (Box.Min + Box.Max);
        }
    }
    return Result;
}

// NOTE(zoubir): before WebReadInput, so the touch keys take part in its
// presses and releases. Scratch is any app_input, to find key states in
internal void
WebApplyTouch(app_memory *Memory, app_input *Scratch)
{
    for (u32 Index = 0; Index < WEB_BUTTON_COUNT; Index++) WebInput.TouchDown[Index] = false;
    int Read[6];
    WebTouch.On = WebReadTouch(Read) != 0;
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    if (!WebTouch.On || !AppState->IsInitialized)
    {
        WebTouch.WasHeld = WebTouch.Held = WebTouch.Queued = 0;
        return;
    }
    WebTouch.MoveX = 0.001f * (float)Read[0];
    WebTouch.MoveY = 0.001f * (float)Read[1];
    WebTouch.Held = (u32)Read[2];
    WebTouch.Dragging = (u32)Read[3];
    WebTouch.AimX = 0.001f * (float)Read[4];
    WebTouch.AimY = 0.001f * (float)Read[5];

    // NOTE(zoubir): the stick, as the keys would move
    float Push = SquareRoot(WebTouch.MoveX * WebTouch.MoveX + WebTouch.MoveY * WebTouch.MoveY);
    if (Push > WEB_TOUCH_STICK_DEAD)
    {
        float X = WebTouch.MoveX / Push;
        float Y = WebTouch.MoveY / Push;
        if (Y < -WEB_TOUCH_STICK_AXIS) WebHoldRow(Scratch, BINDING_MOVE_UP, false);
        if (Y > WEB_TOUCH_STICK_AXIS) WebHoldRow(Scratch, BINDING_MOVE_DOWN, false);
        if (X < -WEB_TOUCH_STICK_AXIS) WebHoldRow(Scratch, BINDING_MOVE_LEFT, false);
        if (X > WEB_TOUCH_STICK_AXIS) WebHoldRow(Scratch, BINDING_MOVE_RIGHT, false);
    }

    cast_targeting *Cast = GetCastTargeting(AppState);
    bool32 Quick = Cast->Mode == CastMode_Quick;
    // NOTE(zoubir): a standard-cast confirm queued last frame goes only
    // if the ability is still being aimed; a quick cast always goes
    u32 Queued = WebTouch.Queued;
    WebTouch.Queued = 0;
    if (!Quick) Queued &= Cast->Aiming;
    WebPressButtons(Scratch, Queued, false);

    u32 Pressed = WebTouch.Held & ~WebTouch.WasHeld;
    u32 Released = WebTouch.WasHeld & ~WebTouch.Held;
    WebTouch.WasHeld = WebTouch.Held;
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player && Pressed)
    {
        WebPutCursorAt(AppState, WebTouchAutoAim(AppState, Player));
    }
    float Drag = SquareRoot(WebTouch.AimX * WebTouch.AimX + WebTouch.AimY * WebTouch.AimY);
    if (Player && WebTouch.Dragging && Drag > WEB_TOUCH_AIM_DEAD)
    {
        WebTouch.LastAim = V2(WebTouch.AimX / Drag, WebTouch.AimY / Drag);
        v2 Aim = V2(WebTouch.AimX, WebTouch.AimY) * (Minimum(Drag, 1.f) / Drag);
        WebPutCursorAt(AppState, Player->Position.XY + Aim * PLAYER_AIM_REACH);
    }

    // NOTE(zoubir): which presses wait for the finger to lift
    u32 OnLift = Quick ? (u32)CAST_TARGETED_BUTTONS : 0;
    WebPressButtons(Scratch, WebTouch.Held & ~OnLift, true);
    WebPressButtons(Scratch, Pressed & ~OnLift, false);
    // NOTE(zoubir): lifting casts: in standard cast it confirms what the
    // press began aiming, once the key was seen up for a frame
    WebTouch.Queued = Quick ? (Released & OnLift) : Released;
}

// NOTE(zoubir): after the frame: what the page shows. One line of flags
// (1 controls shown, 2 a screen wants typing, 4 the movement keys are
// bound, 8 the name screen is up), then "button,group,colour,cooldown share,seconds,name" for
// each ability the bar shows, in the bar's order
internal void
WebShowTouch(app_memory *Memory)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    if (!WebTouch.On || !AppState->IsInitialized) return;
    char State[2048];
    world_entity *Player = GetLocalPlayer(AppState);
    bool32 Covered = ConnectScreenTakesInput(AppState) || AppState->OptionsOpen ||
        (AppState->TalentPanel && AppState->TalentPanel->Open);
    bool32 Typing = ConnectScreenTakesInput(AppState) || (AppState->Chat && AppState->Chat->Typing);
    bool32 Stick = BoundKeyCode(BINDING_MOVE_UP) != KeyCode_None;
    u32 Flags = (Player && !Covered ? 1 : 0) | (Typing ? 2 : 0) | (Stick ? 4 : 0) |
        (ConnectScreenTakesInput(AppState) ? 8 : 0);
    int Length = snprintf(State, sizeof(State), "%u", Flags);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    for (u32 Position = 0; Player && Position < AbilitySlotCount(AppState); Position++)
    {
        ability_slot_def *Def = &AbilitySlotDefs[AbilitySlotIndex(AppState, Position)];
        if (!IsAbilitySlotShown(AppState, Slot, Def)) continue;
        role_spell *Spell = RoleSpellOnButton(AppState, Player, Def->Button);
        float Share, Seconds;
        AbilityCooldownLeft(AppState, Player, Def->Button, &Share, &Seconds);
        u32 Colour = Def->Accent;
        // NOTE(zoubir): where the page puts it: 0 the big button, 1 the
        // ring next to it, 2 the rings further out
        u32 Near = PlayerButton_Jump | PlayerButton_Blink | PlayerButton_Shield;
        u32 Group = Def->Button == PlayerButton_Cast ? 0 : (Def->Button & Near) ? 1 : 2;
        int Wrote = snprintf(State + Length, sizeof(State) - Length,
                             "\n%u,%u,#%02x%02x%02x,%.3f,%.0f,%s", Def->Button, Group,
                             Colour & 0xFF, (Colour >> 8) & 0xFF, (Colour >> 16) & 0xFF,
                             Share, ceilf(Seconds), Spell ? Spell->Name : Def->Name);
        if (Wrote < 0 || Length + Wrote >= (int)sizeof(State)) break;
        Length += Wrote;
    }
    WebSendTouchState(State);
}
