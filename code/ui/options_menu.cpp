/* Options menu: Esc opens it over the game and Esc or "Back to game"
   closes it. It picks the keyboard layout, AZERTY (ZQSD moves) or QWERTY
   (WASD moves), saved for the next launch (client/keyboard_layout.cpp),
   and holds the map vote (ui/map_vote_view.cpp).
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

struct options_choice
{
    keyboard_layout Layout;
    char *Name;
    char *Hint;
};

// NOTE(zoubir): a rounded button; true when clicked this frame
internal bool32
OptionsButton(render_context *RenderContext, app_input *Input, float X, float Y,
              float Width, float Height, bool32 Selected)
{
    bool32 Hot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height);
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

internal void
DoOptionsMenu(render_context *RenderContext, app_state *AppState, app_input *Input,
              u32 WindowWidth, u32 WindowHeight)
{
    // NOTE(zoubir): the layout lives in AppState, which survives the game
    // code being reloaded in development; the global does not
    GlobalKeyboardLayout = (keyboard_layout)AppState->KeyboardLayout;
    if (ConnectScreenTakesInput(AppState))
    {
        AppState->OptionsOpen = false;
        return;
    }
    if (Input->EscapeButton.Pressed)
    {
        AppState->OptionsOpen = !AppState->OptionsOpen;
    }
    if (!AppState->OptionsOpen)
    {
        return;
    }

    font *Title = AppState->Fonts.Title;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float Pad = UI_GAP_LARGE;
    float Width = Minimum(OPTIONS_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float Height = Pad + UILineHeight(Title) + UI_GAP + UILineHeight(Body) + UI_GAP_SMALL +
        OPTIONS_CHOICE_HEIGHT + UI_GAP_LARGE +
        UILineHeight(Body) + UI_GAP_SMALL + MapVoteSectionHeight(AppState) + UI_GAP_LARGE +
        OPTIONS_BUTTON_HEIGHT + Pad;
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
    UIText(RenderContext, Body, Left, Top, "Keyboard", UI_COLOR_TEXT);
    Top += UILineHeight(Body) + UI_GAP_SMALL;

    options_choice Choices[] =
    {
        {KeyboardLayout_Azerty, "AZERTY", "ZQSD to move"},
        {KeyboardLayout_Qwerty, "QWERTY", "WASD to move"},
    };
    float ChoiceWidth = 0.5f * (Inner - UI_GAP);
    for(u32 Index = 0; Index < ArrayCount(Choices); Index++)
    {
        options_choice *Choice = Choices + Index;
        float ChoiceX = Left + Index * (ChoiceWidth + UI_GAP);
        bool32 Selected = AppState->KeyboardLayout == (u32)Choice->Layout;
        if (OptionsButton(RenderContext, Input, ChoiceX, Top, ChoiceWidth,
                          OPTIONS_CHOICE_HEIGHT, Selected) && !Selected)
        {
            AppState->KeyboardLayout = Choice->Layout;
            GlobalKeyboardLayout = Choice->Layout;
            SaveKeyboardLayout();
            Selected = true;
        }
        float CenterX = ChoiceX + 0.5f * ChoiceWidth;
        float LinesTop = Top + 0.5f * (OPTIONS_CHOICE_HEIGHT - UILineHeight(Body) -
                                       UILineHeight(Small));
        UIText(RenderContext, Body, CenterX, LinesTop, Choice->Name,
               Selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT, UIAlign_Center);
        UIText(RenderContext, Small, CenterX, LinesTop + UILineHeight(Body), Choice->Hint,
               UI_COLOR_TEXT_MUTED, UIAlign_Center);
    }
    Top += OPTIONS_CHOICE_HEIGHT + UI_GAP_LARGE;

    UIText(RenderContext, Body, Left, Top, "Map", UI_COLOR_TEXT);
    UIText(RenderContext, Small, Left + Inner, Top + UILineHeight(Body) - UILineHeight(Small),
           GetMapDef((map_id)AppState->World.MapId)->Name, UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Top += UILineHeight(Body) + UI_GAP_SMALL;
    DoMapVoteSection(RenderContext, AppState, Input, Left, Top, Inner);
    Top += MapVoteSectionHeight(AppState) + UI_GAP_LARGE;

    if (OptionsButton(RenderContext, Input, Left, Top, Inner, OPTIONS_BUTTON_HEIGHT, false))
    {
        AppState->OptionsOpen = false;
    }
    UIText(RenderContext, Body, Left + 0.5f * Inner,
           Top + 0.5f * (OPTIONS_BUTTON_HEIGHT - UILineHeight(Body)), "Back to game",
           UI_COLOR_TEXT, UIAlign_Center);
}
