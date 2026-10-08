/* Options menu: Esc opens it over the game and Esc or "Back to game"
   closes it. Esc closes whatever else is open first (CloseTopScreen): the
   Play screen, a spell being aimed, the talent panel, the tile editor;
   only when nothing is open does it open this menu.

   It picks the keyboard layout, AZERTY (ZQSD moves) or QWERTY (WASD
   moves), and the control scheme, keys move or mouse moves
   (client/control_scheme.cpp), both saved for the next launch; switches
   between full screen and a window (client/window_mode.cpp); holds the
   vote for another game mode or map (ui/map_vote_view.cpp); and opens the
   key bindings screen (ui/key_bindings_menu.cpp), which takes the menu's
   place until Esc or Back.
   While it is open the player holds no keys and clicks do not cast
   (app.cpp). The game does not pause: online, the world goes on. */

#define OPTIONS_WIDTH 420.f
#define OPTIONS_CHOICE_HEIGHT 64.f
#define OPTIONS_BUTTON_HEIGHT 36.f

// NOTE(zoubir): the map vote's section (ui/map_vote_view.cpp, included
// after this file)
internal float MapVoteSectionHeight(app_state *AppState);
internal void DoMapVoteSection(render_context *RenderContext, app_state *AppState,
                               app_input *Input, float Left, float Top, float Width);
// NOTE(zoubir): the key bindings screen (ui/key_bindings_menu.cpp,
// included after this file)
internal bool32 KeyBindingsMenuOpen(app_state *AppState);
internal bool32 KeyBindingsMenuTakesEsc(app_state *AppState);
internal void OpenKeyBindingsMenu(app_state *AppState);
internal void CloseKeyBindingsMenu(app_state *AppState);
internal void DoKeyBindingsMenu(render_context *RenderContext, app_state *AppState,
                                app_input *Input, u32 WindowWidth, u32 WindowHeight);

// NOTE(zoubir): a rounded button; true when clicked this frame. Under the
// mouse it tells the controls the click is the screen's (MouseOnButton)
internal bool32
OptionsButton(render_context *RenderContext, app_input *Input, float X, float Y,
              float Width, float Height, bool32 Selected)
{
    bool32 Hot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height);
    if (Hot)
    {
        GlobalMouseOnButton = true;
    }
    DrawRoundRect(RenderContext, X, Y, Width, Height,
                  Hot ? UI_COLOR_CONTROL_HOT : UI_COLOR_CONTROL);
    if (Selected || Hot)
    {
        DrawRoundOutline(RenderContext, X, Y, Width, Height,
                         Selected ? UI_COLOR_ACCENT : UI_COLOR_BORDER);
    }
    bool32 Result = Hot && Input->LeftButton.Pressed;
    return Result;
}

// NOTE(zoubir): a heading over two side-by-side choices, each a name and
// a hint under it; Picked is the lit one. Returns the one clicked this
// frame that was not already picked, -1 for none, and moves Top past it
internal i32
OptionsChoicePair(render_context *RenderContext, app_state *AppState, app_input *Input,
                  float Left, float *Top, float Inner, char *Heading,
                  char **Names, char **Hints, u32 Picked)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Body, Left, *Top, Heading, UI_COLOR_TEXT);
    *Top += UILineHeight(Body) + UI_GAP_SMALL;
    i32 Result = -1;
    float ChoiceWidth = 0.5f * (Inner - UI_GAP);
    for(u32 Index = 0; Index < 2; Index++)
    {
        float ChoiceX = Left + Index * (ChoiceWidth + UI_GAP);
        bool32 Selected = Picked == Index;
        if (OptionsButton(RenderContext, Input, ChoiceX, *Top, ChoiceWidth,
                          OPTIONS_CHOICE_HEIGHT, Selected) && !Selected)
        {
            Result = (i32)Index;
            Selected = true;
        }
        float CenterX = ChoiceX + 0.5f * ChoiceWidth;
        float LinesTop = *Top + 0.5f * (OPTIONS_CHOICE_HEIGHT - UILineHeight(Body) -
                                        UILineHeight(Small));
        UIText(RenderContext, Body, CenterX, LinesTop, Names[Index],
               Selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT, UIAlign_Center);
        UIText(RenderContext, Small, CenterX, LinesTop + UILineHeight(Body), Hints[Index],
               UI_COLOR_TEXT_MUTED, UIAlign_Center);
    }
    *Top += OPTIONS_CHOICE_HEIGHT + UI_GAP_LARGE;
    return Result;
}

// NOTE(zoubir): Esc closes the screen on top, if any: true when it closed
// one, so the same press does not also open the options
internal bool32
CloseTopScreen(app_state *AppState)
{
    bool32 Result = true;
    talent_panel *Talents = GetTalentPanel(AppState);
    if (ConnectScreenTakesInput(AppState))
    {
        AppState->ConnectScreen->Open = false;
    }
    else if (CancelCastAim(AppState))
    {
    }
    else if (Talents->Open)
    {
        Talents->Open = false;
    }
    else if (AppState->TileEditing)
    {
        AppState->TileEditing = false;
    }
    else
    {
        Result = false;
    }
    return Result;
}

internal void
DoOptionsMenu(render_context *RenderContext, app_state *AppState, app_input *Input,
              u32 WindowWidth, u32 WindowHeight)
{
    // NOTE(zoubir): the settings live in AppState, which survives the game
    // code being reloaded in development; the globals do not
    GlobalKeyboardLayout = (keyboard_layout)AppState->KeyboardLayout;
    GlobalControlScheme = (control_scheme)AppState->ControlScheme;
    SyncKeyBindings(AppState);
    if (Input->EscapeButton.Pressed)
    {
        if (AppState->OptionsOpen && KeyBindingsMenuTakesEsc(AppState))
        {
        }
        else if (AppState->OptionsOpen)
        {
            AppState->OptionsOpen = false;
        }
        else if (!CloseTopScreen(AppState))
        {
            AppState->OptionsOpen = true;
        }
    }
    if (ConnectScreenTakesInput(AppState))
    {
        AppState->OptionsOpen = false;
    }
    if (!AppState->OptionsOpen)
    {
        CloseKeyBindingsMenu(AppState);
        return;
    }
    if (KeyBindingsMenuOpen(AppState))
    {
        DoKeyBindingsMenu(RenderContext, AppState, Input, WindowWidth, WindowHeight);
        return;
    }

    font *Title = AppState->Fonts.Title;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float Pad = UI_GAP_LARGE;
    float Width = Minimum(OPTIONS_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float ChoiceRow = UILineHeight(Body) + UI_GAP_SMALL + OPTIONS_CHOICE_HEIGHT + UI_GAP_LARGE;
    float Height = Pad + UILineHeight(Title) + UI_GAP + 3.f * ChoiceRow +
        UILineHeight(Body) + UI_GAP_SMALL + MapVoteSectionHeight(AppState) + UI_GAP_LARGE +
        2.f * OPTIONS_BUTTON_HEIGHT + UI_GAP + Pad;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = Maximum(UI_GAP, 0.5f * ((float)WindowHeight - Height));

    DrawRoundRect(RenderContext, 0.f, 0.f, (float)WindowWidth, (float)WindowHeight,
                  UI_RGBA(0, 0, 0, 120));
    DrawUIPanel(RenderContext, X, Y, Width, Height);
    float Left = X + Pad;
    float Inner = Width - 2.f * Pad;
    float Top = Y + Pad;
    UIText(RenderContext, Title, Left, Top, "Options", UI_COLOR_TEXT);
    UIText(RenderContext, Small, X + Width - Pad, Top + UILineHeight(Title) - UILineHeight(Small),
           "Esc to close", UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Top += UILineHeight(Title) + UI_GAP;

    char *LayoutNames[] = {"AZERTY", "QWERTY"};
    char *LayoutHints[] = {"ZQSD to move", "WASD to move"};
    i32 Layout = OptionsChoicePair(RenderContext, AppState, Input, Left, &Top, Inner,
                                   "Keyboard", LayoutNames, LayoutHints,
                                   AppState->KeyboardLayout == KeyboardLayout_Qwerty ? 1 : 0);
    if (Layout >= 0)
    {
        AppState->KeyboardLayout = Layout ? KeyboardLayout_Qwerty : KeyboardLayout_Azerty;
        GlobalKeyboardLayout = (keyboard_layout)AppState->KeyboardLayout;
        SaveLocalSettings();
    }

    // NOTE(zoubir): the hints name the keys on the layout in use
    char KeysHint[48];
    char MouseHint[32];
    char MoveKeys[40];
    MoveKeysText(MoveKeys, sizeof(MoveKeys));
    snprintf(KeysHint, sizeof(KeysHint), "%s, click fireball", MoveKeys);
    LayoutKeyText(MouseHint, sizeof(MouseHint), (char *)"Click, spells AZER");
    char *SchemeNames[] = {"Keys move", "Mouse moves"};
    char *SchemeHints[] = {KeysHint, MouseHint};
    i32 Scheme = OptionsChoicePair(RenderContext, AppState, Input, Left, &Top, Inner,
                                   "Controls", SchemeNames, SchemeHints,
                                   AppState->ControlScheme == ControlScheme_Mouse ? 1 : 0);
    if (Scheme >= 0)
    {
        AppState->ControlScheme = Scheme ? ControlScheme_Mouse : ControlScheme_Keys;
        GlobalControlScheme = (control_scheme)AppState->ControlScheme;
        SaveLocalSettings();
    }

    char *WindowNames[] = {"Full screen", "Window"};
    char *WindowHints[] = {"F1 switches too", "Resizable"};
    i32 WindowPick = OptionsChoicePair(RenderContext, AppState, Input, Left, &Top, Inner,
                                       "Display", WindowNames, WindowHints,
                                       IsFullscreen(AppState) ? 0 : 1);
    if (WindowPick >= 0)
    {
        RequestFullscreen(AppState, WindowPick == 0);
    }

    UIText(RenderContext, Body, Left, Top, "Mode and map", UI_COLOR_TEXT);
    UIText(RenderContext, Small, Left + Inner, Top + UILineHeight(Body) - UILineHeight(Small),
           GetMapDef((map_id)AppState->World.MapId)->Name, UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Top += UILineHeight(Body) + UI_GAP_SMALL;
    DoMapVoteSection(RenderContext, AppState, Input, Left, Top, Inner);
    Top += MapVoteSectionHeight(AppState) + UI_GAP_LARGE;

    if (OptionsButton(RenderContext, Input, Left, Top, Inner, OPTIONS_BUTTON_HEIGHT, false))
    {
        OpenKeyBindingsMenu(AppState);
    }
    UIText(RenderContext, Body, Left + 0.5f * Inner,
           Top + 0.5f * (OPTIONS_BUTTON_HEIGHT - UILineHeight(Body)), "Key bindings",
           UI_COLOR_TEXT, UIAlign_Center);
    Top += OPTIONS_BUTTON_HEIGHT + UI_GAP;

    if (OptionsButton(RenderContext, Input, Left, Top, Inner, OPTIONS_BUTTON_HEIGHT, false))
    {
        AppState->OptionsOpen = false;
    }
    UIText(RenderContext, Body, Left + 0.5f * Inner,
           Top + 0.5f * (OPTIONS_BUTTON_HEIGHT - UILineHeight(Body)), "Back to game",
           UI_COLOR_TEXT, UIAlign_Center);
}
