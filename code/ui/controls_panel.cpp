/* Controls panel: every key and what it does, on the left under the HUD.
   Shows for the first CONTROLS_INTRO_SECONDS of play, fading out over
   the last CONTROLS_FADE_SECONDS, and whenever H is held; otherwise a one-line hint in the bottom-left says how to bring it
   back. Hidden while the connect screen is open (it takes the keys and
   sits on top), and the intro time only runs while playing.
   The panel stays above the ability bar: in a short window it switches to
   the small font, moves up toward the HUD lines, then splits into two
   columns.
   A new key goes in ControlsRows; capital letters in its key text are
   shown as the keys at those places on the player's layout
   (client/keyboard_layout.cpp), so name letter keys as on AZERTY. */

#define CONTROLS_INTRO_SECONDS 10.f
// NOTE(zoubir): the intro fades out over its last this-many seconds
#define CONTROLS_FADE_SECONDS 1.f
// NOTE(zoubir): under the HUD's stat lines; CONTROLS_TOP_MIN when the
// window is too short for that
#define CONTROLS_TOP 150.f
#define CONTROLS_TOP_MIN 64.f

internal float AbilityBarPlateTop(u32 WindowHeight);

struct controls_panel
{
    float SecondsShown;
};

// NOTE(zoubir): where a row shows: everywhere, only outside a dungeon
// run, or only in one, where A, R, C and V are the class's spells
enum controls_where
{
    Controls_Always,
    Controls_Duel,
    Controls_Run,
};

struct controls_row
{
    char *Key;
    char *Action;
    controls_where Where;
};

global_variable controls_row ControlsRows[] =
{
    {"ZQSD",        "Move"},
    {"Mouse",       "Aim; you face the cursor"},
    {"X",           "Fireball at the cursor, every 6 s"},
    {"Space",       "Jump; fireballs and blasts pass under"},
    {"E",           "Shield: nothing hurts you for 2 s"},
    {"F",           "Blink to the cursor after 0.2 s"},
    {"A",           "Launch: throws foes up, stuns", Controls_Duel},
    {"V",           "Kunai at the foe under the cursor; follows it", Controls_Duel},
    {"G T W R C",   "Talent abilities, once unlocked", Controls_Duel},
    {"A",           "", Controls_Run},
    {"R",           "", Controls_Run},
    {"C",           "Your tree's first spell, once unlocked", Controls_Run},
    {"V",           "Your tree's second spell, once unlocked", Controls_Run},
    {"N",           "Talents: spend a point each level"},
    {"Tab",         "Scoreboard"},
    {"Esc",         "Options: AZERTY or QWERTY"},
    {"F4",          "Play: map and server"},
    {"F1",          "Fullscreen"},
    {"Hold H",      "This panel"},
};

// NOTE(zoubir): the rows that show now, in order, into Rows; returns how
// many
internal u32
ShownControlsRows(app_state *AppState, controls_row **Rows)
{
    controls_where Hidden = IsDungeon(AppState) ? Controls_Duel : Controls_Run;
    u32 Result = 0;
    for(u32 Row = 0; Row < ArrayCount(ControlsRows); Row++)
    {
        if (ControlsRows[Row].Where != Hidden)
        {
            Rows[Result++] = &ControlsRows[Row];
        }
    }
    return Result;
}

// NOTE(zoubir): a row's key text on the player's layout; only runs of
// capital letters are letter keys ("ZQSD", "G T W R C"), so "Alt", "Tab"
// and "F4" stay as they are
// NOTE(zoubir): a row's action: in a dungeon run the class's keys say
// what its spell does (sim/dungeon/role_abilities.cpp); C and V only once
// the tree unlocked them
internal char *
ControlsRowAction(app_state *AppState, controls_row *Row)
{
    if (Row->Where != Controls_Run)
    {
        return Row->Action;
    }
    u32 Button = 0;
    if (strcmp(Row->Key, "A") == 0) Button = PlayerButton_Launch;
    else if (strcmp(Row->Key, "R") == 0) Button = PlayerButton_Push;
    else if (strcmp(Row->Key, "C") == 0) Button = PlayerButton_Slam;
    else if (strcmp(Row->Key, "V") == 0) Button = PlayerButton_Kunai;
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 Learned = (RunAllowedButtons(AppState, Slot, 0) & Button) != 0;
    char *Result = Learned ? RoleControlsLine(AppState, Button, Row->Action) : Row->Action;
    return Result;
}

internal void
ControlsKeyText(char *Out, u32 OutSize, char *Text)
{
    bool32 Letters = true;
    for(char *At = Text; *At; At++)
    {
        if (!(*At == ' ' || (*At >= 'A' && *At <= 'Z')))
        {
            Letters = false;
        }
    }
    if (Letters)
    {
        LayoutKeyText(Out, OutSize, Text);
    }
    else
    {
        snprintf(Out, OutSize, "%s", Text);
    }
}

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
               "Hold H for controls, Esc for options", UI_COLOR_TEXT_MUTED);
        return;
    }

    float Fade = Input->ButtonH.EndedDown ? 1.f :
        Clamp01((CONTROLS_INTRO_SECONDS - Panel->SecondsShown) / CONTROLS_FADE_SECONDS);
    float Bottom = AbilityBarPlateTop(WindowHeight) - UI_GAP;
    float Pad = UI_GAP;
    float TitleHeight = UILineHeight(AppState->Fonts.Title) + UI_GAP_SMALL;
    controls_row *Rows[ArrayCount(ControlsRows)];
    u32 RowCount = ShownControlsRows(AppState, Rows);
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
    char Keys[ArrayCount(ControlsRows)][16];
    for(u32 Row = 0; Row < RowCount; Row++)
    {
        ControlsKeyText(Keys[Row], sizeof(Keys[Row]), Rows[Row]->Key);
        KeyWidth = Maximum(KeyWidth, UITextWidth(Font, Keys[Row]));
        ActionWidth = Maximum(ActionWidth,
                              UITextWidth(Font, ControlsRowAction(AppState, Rows[Row])));
    }
    float ColumnWidth = KeyWidth + UI_GAP_LARGE + ActionWidth;
    float Width = Pad + Columns * ColumnWidth + (Columns - 1) * UI_GAP_LARGE + Pad;
    float Left = UI_GAP_LARGE;
    float Top = Maximum(CONTROLS_TOP_MIN,
                        Minimum(CONTROLS_TOP, Bottom - Height));
    DrawUIPanel(RenderContext, Left, Top, Width, Height, UI_RGBA(150, 160, 190, 255), Fade);

    UIText(RenderContext, AppState->Fonts.Title, Left + Pad, Top + Pad,
           "Controls", WithAlpha(UI_COLOR_TEXT, Fade));
    float RowsTop = Top + Pad + TitleHeight;
    for(u32 Row = 0; Row < RowCount; Row++)
    {
        u32 Column = Row / RowsPerColumn;
        float KeyRight = Left + Pad + Column * (ColumnWidth + UI_GAP_LARGE) + KeyWidth;
        float Y = RowsTop + (Row % RowsPerColumn) * RowHeight;
        UIText(RenderContext, Font, KeyRight, Y, Keys[Row],
               WithAlpha(UI_COLOR_ACCENT, Fade), UIAlign_Right);
        UIText(RenderContext, Font, KeyRight + UI_GAP_LARGE, Y,
               ControlsRowAction(AppState, Rows[Row]), WithAlpha(UI_COLOR_TEXT, Fade));
    }
}
