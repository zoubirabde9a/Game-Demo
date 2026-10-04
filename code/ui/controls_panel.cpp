/* Controls panel: every key and what it does, on the left under the HUD.
   Shows for the first CONTROLS_INTRO_SECONDS of play and whenever H is
   held; otherwise a one-line hint in the bottom-left says how to bring it
   back. Hidden while the connect screen is open (it takes the keys and
   sits on top), and the intro time only runs while playing.
   A new key goes in ControlsRows. */

#define CONTROLS_INTRO_SECONDS 10.f

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
    {"Left click",  "Fireball at a foe; on the ground, walk there"},
    {"Right click", "Sword: a wide slice that shoves"},
    {"Space",       "Jump, again in the air; clears rocks"},
    {"Alt",         "Dash; dashing through an attack dodges it"},
    {"E",           "Shockwave around you"},
    {"F",           "Blink to the cursor"},
    {"R",           "Push: throws a crowd off you"},
    {"A",           "Launch: throws foes up, stuns"},
    {"C",           "Slam down from a jump"},
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

    font *Font = AppState->Fonts.Body;
    float RowHeight = UILineHeight(Font) + 4.f;
    float KeyWidth = 0.f;
    float ActionWidth = 0.f;
    for(u32 Row = 0; Row < ArrayCount(ControlsRows); Row++)
    {
        KeyWidth = Maximum(KeyWidth, UITextWidth(Font, ControlsRows[Row].Key));
        ActionWidth = Maximum(ActionWidth,
                              UITextWidth(Font, ControlsRows[Row].Action));
    }
    float Pad = UI_GAP;
    float Width = Pad + KeyWidth + UI_GAP_LARGE + ActionWidth + Pad;
    float Height = Pad + UILineHeight(AppState->Fonts.Title) + UI_GAP_SMALL +
        ArrayCount(ControlsRows) * RowHeight + Pad;
    // NOTE(zoubir): under the HUD's stat lines
    float Left = UI_GAP_LARGE;
    float Top = 150.f;
    DrawFilledRectangle(RenderContext, Left, Top, Width, Height,
                        UI_COLOR_PANEL, 0.f);
    DrawRectangle(RenderContext, Left, Top, Width, Height, UI_COLOR_BORDER, 0.f);

    float Y = Top + Pad;
    UIText(RenderContext, AppState->Fonts.Title, Left + Pad, Y, "Controls",
           UI_COLOR_TEXT);
    Y += UILineHeight(AppState->Fonts.Title) + UI_GAP_SMALL;
    float KeyRight = Left + Pad + KeyWidth;
    for(u32 Row = 0; Row < ArrayCount(ControlsRows); Row++)
    {
        UIText(RenderContext, Font, KeyRight, Y, ControlsRows[Row].Key,
               UI_COLOR_ACCENT, UIAlign_Right);
        UIText(RenderContext, Font, KeyRight + UI_GAP_LARGE, Y,
               ControlsRows[Row].Action, UI_COLOR_TEXT);
        Y += RowHeight;
    }
}
