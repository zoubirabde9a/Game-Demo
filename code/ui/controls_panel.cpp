/* Controls panel: every key and what it does, on the left under the HUD.
   Shows for the first CONTROLS_INTRO_SECONDS of play, fading out over
   the last CONTROLS_FADE_SECONDS, and whenever H is held; otherwise a one-line hint in the bottom-left says how to bring it
   back. Hidden while the connect screen is open (it takes the keys and
   sits on top), and the intro time only runs while playing.
   The panel stays above the ability bar: in a short window it switches to
   the small font, moves up toward the HUD lines, then splits into two
   columns.
   A new key goes in ControlsRows. A row for a player action names its
   button, and the key shown is the one the action table gives it on the
   player's layout and control scheme (client/action_keys.cpp); in other
   rows capital letters in the key text are shown as the keys at those
   places on the layout (client/keyboard_layout.cpp), so name letter keys
   as on AZERTY. Rows for one control scheme only say which. */

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

// NOTE(zoubir): which control scheme a row is for (client/control_scheme.cpp)
enum controls_scheme
{
    Controls_EitherScheme,
    Controls_KeysMove,
    Controls_MouseMoves,
};

struct controls_row
{
    // NOTE(zoubir): the key's text, for rows with no Buttons
    char *Key;
    char *Action;
    controls_where Where;
    // NOTE(zoubir): the player_buttons the row is about; their keys are
    // its key text
    u32 Buttons;
    controls_scheme Scheme;
};

#define CONTROLS_TALENT_BUTTONS (PlayerButton_Shockwave | PlayerButton_Push | PlayerButton_Slam | \
                                 PlayerButton_FrostNova | PlayerButton_GravityWell)

// NOTE(zoubir): one string for both schemes' fireball rows, so a class
// that owns X can put its own spell's line there (ControlsRowAction)
global_variable char ControlsFireballLine[] = "Fireball at the cursor, every 6 s";

global_variable controls_row ControlsRows[] =
{
    {"ZQSD",        "Move", Controls_Always, 0, Controls_KeysMove},
    {"Left click",  "Walk there; hold to keep walking", Controls_Always, 0, Controls_MouseMoves},
    {"Mouse",       "Aim; you face the cursor"},
    {"Click, X",    ControlsFireballLine, Controls_Always, 0, Controls_KeysMove},
    {"",            ControlsFireballLine, Controls_Always, PlayerButton_Cast, Controls_MouseMoves},
    {"",            "", Controls_Run, PlayerButton_Attack},
    {"",            "Jump; fireballs and blasts pass under", Controls_Always, PlayerButton_Jump},
    {"",            "Shield: nothing hurts you for 2 s", Controls_Always, PlayerButton_Shield},
    {"",            "Blink to the cursor after 0.2 s", Controls_Always, PlayerButton_Blink},
    {"",            "Launch: throws foes up, stuns", Controls_Duel, PlayerButton_Launch},
    {"",            "Kunai at the foe under the cursor; follows it", Controls_Duel,
                    PlayerButton_Kunai},
    {"",            "Talent abilities, once unlocked", Controls_Duel, CONTROLS_TALENT_BUTTONS},
    {"",            "", Controls_Run, PlayerButton_Launch},
    {"",            "", Controls_Run, PlayerButton_Push},
    {"",            "Your tree's first spell, once unlocked", Controls_Run, PlayerButton_Slam},
    {"",            "Your tree's second spell, once unlocked", Controls_Run, PlayerButton_Kunai},
    {"",            "", Controls_Run, PlayerButton_Shockwave},
    {"N",           "Talents: spend a point each level"},
    {"Tab",         "Scoreboard"},
    {"Esc",         "Close what is open, else options"},
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
    controls_scheme OtherScheme = MouseMoves() ? Controls_KeysMove : Controls_MouseMoves;
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Result = 0;
    for(u32 Row = 0; Row < ArrayCount(ControlsRows); Row++)
    {
        // NOTE(zoubir): a run's row shows only for a class with a spell on
        // its key, and the fireball's only for a class that keeps it
        controls_row *Each = &ControlsRows[Row];
        bool32 Empty = Each->Where == Controls_Run &&
            !RoleSpellOnButton(AppState, Slot->Entity, Each->Buttons);
        if (Each->Action == ControlsFireballLine && IsDungeon(AppState) &&
            !(RunAllowedButtons(AppState, Slot, 0) & PlayerButton_Cast))
        {
            Empty = true;
        }
        if (Each->Where != Hidden && Each->Scheme != OtherScheme && !Empty)
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
    // NOTE(zoubir): the fireball rows say what a class that owns X casts
    if (Row->Action == ControlsFireballLine)
    {
        return RoleControlsLine(AppState, PlayerButton_Cast, Row->Action);
    }
    if (Row->Where != Controls_Run)
    {
        return Row->Action;
    }
    u32 Button = Row->Buttons;
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 Learned = (RunAllowedButtons(AppState, Slot, 0) & Button) != 0;
    char *Result = Learned ? RoleControlsLine(AppState, Button, Row->Action) : Row->Action;
    return Result;
}

internal void
ControlsKeyText(char *Out, u32 OutSize, controls_row *Row)
{
    if (Row->Buttons)
    {
        // NOTE(zoubir): in the action table's order, which keeps the keys
        // of a group near each other
        static app_input NoInput;
        action_key Keys[ACTION_KEY_COUNT];
        GetActionKeys(&NoInput, Keys);
        Out[0] = 0;
        for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
        {
            if (Keys[Index].Button & Row->Buttons)
            {
                u32 Length = (u32)strlen(Out);
                snprintf(Out + Length, OutSize - Length, "%s%s", Length ? " " : "",
                         Keys[Index].Label);
            }
        }
        return;
    }
    char *Text = Row->Key;
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
        ControlsKeyText(Keys[Row], sizeof(Keys[Row]), Rows[Row]);
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
