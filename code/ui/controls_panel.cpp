/* Controls panel: every key and what it does, on the left under the HUD.
   Shows for the first CONTROLS_INTRO_SECONDS of play and whenever H is
   held; otherwise a one-line hint in the bottom-left says how to bring it
   back. Hidden while the connect screen is open (it takes the keys and
   sits on top), and the intro time only runs while playing.
   The panel stays above the ability bar: in a short window it switches to
   the small font, moves up toward the HUD lines, then splits into two
   columns.
   A new key goes in ControlsRows. */

#define CONTROLS_INTRO_SECONDS 10.f
// NOTE(zoubir): under the HUD's stat lines; CONTROLS_TOP_MIN when the
// window is too short for that
#define CONTROLS_TOP 150.f
#define CONTROLS_TOP_MIN 64.f

internal float AbilityBarPlateTop(u32 WindowHeight);

struct controls_panel
{
    float SecondsShown;
};

struct controls_row
{
    char *Key;
    char *Action;
};

global_variable controls_row ControlsRows[] =
{
    {"ZQSD",        "Move"},
    {"Mouse",       "Aim; you face the cursor"},
    {"Left click",  "Fireball at the cursor, every 6 s"},
    {"Space",       "Jump; fireballs and blasts pass under"},
    {"Alt",         "Dash; dashing through an attack dodges it"},
    {"E",           "Shield: nothing hurts you for 2 s"},
    {"F",           "Blink to the cursor after 0.5 s"},
    {"A",           "Launch: throws foes up, stuns"},
    {"V",           "Rewind the whole world"},
    {"Tab",         "Scoreboard"},
    {"F4",          "Play: map and server"},
    {"F1",          "Fullscreen"},
    {"Hold H",      "This panel"},
};

internal void
DrawControlsPanel(render_context *RenderContext, app_state *AppState,
                  app_input *Input, u32 WindowHeight)
{
    if (ConnectScreenTakesInput(AppState) || !GetLocalPlayer(AppState))
    {
        return;
    }
    if (!AppState->ControlsPanel)
    {
        AppState->ControlsPanel =
            AllocateStruct(&AppState->MemoryArena, controls_panel);
        *AppState->ControlsPanel = {};
    }
    controls_panel *Panel = AppState->ControlsPanel;
    bool32 Intro = Panel->SecondsShown < CONTROLS_INTRO_SECONDS;
    if (Intro)
    {
        Panel->SecondsShown += Input->DeltaTime;
    }

    font *Small = AppState->Fonts.Small;
    if (!Intro && !Input->ButtonH.EndedDown)
    {
        UIText(RenderContext, Small, UI_GAP_LARGE,
               (float)WindowHeight - UI_GAP_LARGE - UILineHeight(Small),
               "Hold H for controls", UI_COLOR_TEXT_MUTED);
        return;
    }

    float Bottom = AbilityBarPlateTop(WindowHeight) - UI_GAP;
    float Pad = UI_GAP;
    float TitleHeight = UILineHeight(AppState->Fonts.Title) + UI_GAP_SMALL;
    u32 RowCount = ArrayCount(ControlsRows);
    font *Font = AppState->Fonts.Body;
    float RowHeight = UILineHeight(Font) + 4.f;
    u32 Columns = 1;
    if (CONTROLS_TOP + Pad + TitleHeight + RowCount * RowHeight + Pad > Bottom)
    {
        Font = Small;
        RowHeight = UILineHeight(Font) + 2.f;
        if (CONTROLS_TOP_MIN + Pad + TitleHeight + RowCount * RowHeight + Pad > Bottom)
        {
            Columns = 2;
        }
    }
    u32 RowsPerColumn = (RowCount + Columns - 1) / Columns;
    float Height = Pad + TitleHeight + RowsPerColumn * RowHeight + Pad;
    float KeyWidth = 0.f;
    float ActionWidth = 0.f;
    for(u32 Row = 0; Row < RowCount; Row++)
    {
        KeyWidth = Maximum(KeyWidth, UITextWidth(Font, ControlsRows[Row].Key));
        ActionWidth = Maximum(ActionWidth,
                              UITextWidth(Font, ControlsRows[Row].Action));
    }
    float ColumnWidth = KeyWidth + UI_GAP_LARGE + ActionWidth;
    float Width = Pad + Columns * ColumnWidth + (Columns - 1) * UI_GAP_LARGE + Pad;
    float Left = UI_GAP_LARGE;
    float Top = Maximum(CONTROLS_TOP_MIN,
                        Minimum(CONTROLS_TOP, Bottom - Height));
    DrawFilledRectangle(RenderContext, Left, Top, Width, Height,
                        UI_COLOR_PANEL, 0.f);
    DrawRectangle(RenderContext, Left, Top, Width, Height, UI_COLOR_BORDER, 0.f);

    UIText(RenderContext, AppState->Fonts.Title, Left + Pad, Top + Pad,
           "Controls", UI_COLOR_TEXT);
    float RowsTop = Top + Pad + TitleHeight;
    for(u32 Row = 0; Row < RowCount; Row++)
    {
        u32 Column = Row / RowsPerColumn;
        float KeyRight = Left + Pad + Column * (ColumnWidth + UI_GAP_LARGE) + KeyWidth;
        float Y = RowsTop + (Row % RowsPerColumn) * RowHeight;
        UIText(RenderContext, Font, KeyRight, Y, ControlsRows[Row].Key,
               UI_COLOR_ACCENT, UIAlign_Right);
        UIText(RenderContext, Font, KeyRight + UI_GAP_LARGE, Y,
               ControlsRows[Row].Action, UI_COLOR_TEXT);
    }
}
