/* Key bindings menu: the "Key bindings" button in the Esc options menu
   (options_menu.cpp) swaps the options for this screen, where the player
   puts each action on the key they want (client/key_bindings.cpp).

   Two tabs, Duels and Dungeons, each with its own keys; the tab of the
   game being played opens first. The rows are the ones the control
   scheme in use has (the mouse-moves scheme has no movement keys and its
   own set). In a dungeon run the class's rows say the class's spell names.

   A click on a row's key waits for the next key or mouse button (not the
   left one, which clicks the screen), Backspace puts back the row's
   default and Esc stops waiting. A key another row had swaps with it, and
   the line under the rows says so. H, N and J stay the game's. Every
   change is saved to keys.txt at once. Esc, or Back, goes back to the
   options. */

#define KEY_BINDINGS_WIDTH 460.f
#define KEY_BINDINGS_KEY_WIDTH 112.f
#define KEY_BINDINGS_NOTICE_SECONDS 4.f

struct key_bindings_menu
{
    bool32 Open;
    // NOTE(zoubir): the binding_mode the tab shows
    u32 Mode;
    // NOTE(zoubir): the row waiting for a key, plus one; 0 for none
    u32 Waiting;
    char Notice[96];
    float NoticeLeft;
};

internal key_bindings_menu *
GetKeyBindingsMenu(app_state *AppState)
{
    if (!AppState->KeyBindingsMenu)
    {
        AppState->KeyBindingsMenu = AllocateStruct(&AppState->MemoryArena, key_bindings_menu);
        *AppState->KeyBindingsMenu = {};
    }
    return AppState->KeyBindingsMenu;
}

internal bool32
KeyBindingsMenuOpen(app_state *AppState)
{
    bool32 Result = AppState->KeyBindingsMenu && AppState->KeyBindingsMenu->Open;
    return Result;
}

internal void
OpenKeyBindingsMenu(app_state *AppState)
{
    key_bindings_menu *Menu = GetKeyBindingsMenu(AppState);
    Menu->Open = true;
    Menu->Mode = IsDungeon(AppState) ? BindingMode_Dungeon : BindingMode_Duel;
    Menu->Waiting = 0;
    Menu->NoticeLeft = 0.f;
}

// NOTE(zoubir): the options menu closed under it
internal void
CloseKeyBindingsMenu(app_state *AppState)
{
    if (AppState->KeyBindingsMenu)
    {
        AppState->KeyBindingsMenu->Open = false;
        AppState->KeyBindingsMenu->Waiting = 0;
    }
}

// NOTE(zoubir): Esc while the screen is open: stops waiting for a key, or
// else goes back to the options. False when the screen is not open
internal bool32
KeyBindingsMenuTakesEsc(app_state *AppState)
{
    bool32 Result = KeyBindingsMenuOpen(AppState);
    if (Result)
    {
        key_bindings_menu *Menu = AppState->KeyBindingsMenu;
        if (Menu->Waiting)
        {
            Menu->Waiting = 0;
        }
        else
        {
            Menu->Open = false;
        }
    }
    return Result;
}

internal void
KeyBindingsNotice(key_bindings_menu *Menu, char *Format, char *A, char *B = (char *)"")
{
    snprintf(Menu->Notice, sizeof(Menu->Notice), Format, A, B);
    Menu->NoticeLeft = KEY_BINDINGS_NOTICE_SECONDS;
}

// NOTE(zoubir): what the row is called in the tab: a dungeon run's class
// spell by its name when the local player has one there, else what the
// key is for in any class
internal char *
KeyBindingRowName(app_state *AppState, u32 Mode, u32 Row)
{
    char *Result = BindingDefs[Row].Name;
    if (Mode == BindingMode_Dungeon && Row < ACTION_KEY_COUNT)
    {
        u32 Button = BindingDefs[Row].Button;
        role_spell *Spell = IsDungeon(AppState) ? LocalRoleSpell(AppState, Button) : 0;
        if (Spell)
        {
            Result = Spell->Name;
        }
        else
        {
            switch(Button)
            {
                case PlayerButton_Launch: Result = (char *)"Class spell 1"; break;
                case PlayerButton_Push: Result = (char *)"Class spell 2"; break;
                case PlayerButton_Shockwave: Result = (char *)"Class spell 3"; break;
                case PlayerButton_Slam: Result = (char *)"Tree spell 1"; break;
                case PlayerButton_Kunai: Result = (char *)"Tree spell 2"; break;
                case PlayerButton_Cast: Result = (char *)"Fireball or class spell"; break;
                case PlayerButton_Attack: Result = (char *)"Class attack"; break;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the rows the tab shows, in BindingDefs order with the
// movement first, into Rows; returns how many
internal u32
KeyBindingsMenuRows(u32 Mode, u32 *Rows)
{
    u32 Result = 0;
    for(u32 Each = 0; Each < BINDING_COUNT; Each++)
    {
        u32 Row = (Each + ACTION_KEY_COUNT) % BINDING_COUNT;
        if (BindingRowUsed(Mode, GlobalControlScheme, Row))
        {
            Rows[Result++] = Row;
        }
    }
    return Result;
}

// NOTE(zoubir): while a row waits: the key pressed this frame goes on it
internal void
TakeKeyForBinding(app_state *AppState, app_input *Input, key_bindings_menu *Menu)
{
    key_bindings *Bindings = AppState->KeyBindings;
    u32 Row = Menu->Waiting - 1;
    u32 Scheme = GlobalControlScheme;
    u32 Swapped = BINDING_COUNT;
    if (Input->BackspaceButton.Pressed)
    {
        Swapped = ResetKeyBinding(Bindings, Menu->Mode, Scheme, Row);
    }
    else
    {
        key_code Code = FirstPressedKeyCode(Input);
        if (!Code)
        {
            return;
        }
        if (KeyCodeReserved(Code))
        {
            char *What = (Code == LetterKeyCode('H')) ? (char *)"the controls panel" :
                (Code == LetterKeyCode('N')) ? (char *)"the talents" : (char *)"the battle theme";
            KeyBindingsNotice(Menu, "%s is kept for %s; pick another key", KeyCodeName(Code), What);
            return;
        }
        Swapped = SetKeyBinding(Bindings, Menu->Mode, Scheme, Row, Code);
    }
    Menu->Waiting = 0;
    Menu->NoticeLeft = 0.f;
    if (Swapped < BINDING_COUNT)
    {
        KeyBindingsNotice(Menu, "%s moved to %s, the key this one had",
                          KeyBindingRowName(AppState, Menu->Mode, Swapped),
                          KeyCodeName(BoundKeyCodeIn(Menu->Mode, Scheme, Swapped)));
    }
    SaveKeyBindings(Bindings);
}

// NOTE(zoubir): a button with its text centred; true when clicked
internal bool32
KeyBindingsButton(render_context *RenderContext, app_input *Input, font *Font,
                  float X, float Y, float Width, float Height, char *Text,
                  bool32 Selected, u32 TextColor = UI_COLOR_TEXT)
{
    bool32 Result = OptionsButton(RenderContext, Input, X, Y, Width, Height, Selected);
    UIText(RenderContext, Font, X + 0.5f * Width, Y + 0.5f * (Height - UILineHeight(Font)),
           Text, TextColor, UIAlign_Center);
    return Result;
}

internal void
DoKeyBindingsMenu(render_context *RenderContext, app_state *AppState, app_input *Input,
                  u32 WindowWidth, u32 WindowHeight)
{
    key_bindings_menu *Menu = GetKeyBindingsMenu(AppState);
    key_bindings *Bindings = AppState->KeyBindings;
    if (!Bindings)
    {
        Menu->Open = false;
        return;
    }
    if (Menu->Waiting)
    {
        TakeKeyForBinding(AppState, Input, Menu);
    }
    Menu->NoticeLeft = Maximum(0.f, Menu->NoticeLeft - Input->DeltaTime);

    font *Title = AppState->Fonts.Title;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    u32 Rows[BINDING_COUNT];
    u32 RowCount = KeyBindingsMenuRows(Menu->Mode, Rows);
    float Pad = UI_GAP_LARGE;
    float RowHeight = UILineHeight(Body) + 12.f;
    float Header = UILineHeight(Title) + UI_GAP + OPTIONS_BUTTON_HEIGHT + UI_GAP_SMALL +
        UILineHeight(Small) + UI_GAP;
    float Footer = UI_GAP + UILineHeight(Small) + UI_GAP + OPTIONS_BUTTON_HEIGHT;
    // NOTE(zoubir): two columns of rows when one does not fit the window
    u32 Columns = 1;
    if (2.f * Pad + Header + RowCount * RowHeight + Footer > (float)WindowHeight - 2.f * UI_GAP)
    {
        Columns = 2;
    }
    u32 PerColumn = (RowCount + Columns - 1) / Columns;
    float Width = Minimum(Columns * KEY_BINDINGS_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float Height = 2.f * Pad + Header + PerColumn * RowHeight + Footer;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = Maximum(UI_GAP, 0.5f * ((float)WindowHeight - Height));

    DrawRoundRect(RenderContext, 0.f, 0.f, (float)WindowWidth, (float)WindowHeight,
                  UI_RGBA(0, 0, 0, 120));
    DrawUIPanel(RenderContext, X, Y, Width, Height);
    float Left = X + Pad;
    float Inner = Width - 2.f * Pad;
    float Top = Y + Pad;
    UIText(RenderContext, Title, Left, Top, "Key bindings", UI_COLOR_TEXT);
    UIText(RenderContext, Small, Left + Inner, Top + UILineHeight(Title) - UILineHeight(Small),
           Menu->Waiting ? (char *)"Esc to stop waiting" : (char *)"Esc to go back", UI_COLOR_TEXT_MUTED,
           UIAlign_Right);
    Top += UILineHeight(Title) + UI_GAP;

    char *Tabs[BindingMode_Count] = {"Duels", "Dungeons"};
    float TabWidth = 0.5f * (Inner - UI_GAP);
    for(u32 Mode = 0; Mode < BindingMode_Count; Mode++)
    {
        bool32 Picked = Menu->Mode == Mode;
        if (KeyBindingsButton(RenderContext, Input, Body, Left + Mode * (TabWidth + UI_GAP), Top,
                              TabWidth, OPTIONS_BUTTON_HEIGHT, Tabs[Mode], Picked,
                              Picked ? UI_COLOR_ACCENT : UI_COLOR_TEXT) && !Picked)
        {
            Menu->Mode = Mode;
            Menu->Waiting = 0;
            RowCount = KeyBindingsMenuRows(Menu->Mode, Rows);
        }
    }
    Top += OPTIONS_BUTTON_HEIGHT + UI_GAP_SMALL;
    UIText(RenderContext, Small, Left, Top,
           MouseMoves() ? (char *)"Keys for the mouse-moves scheme; keys move has its own" :
           (char *)"Keys for the keys-move scheme; mouse moves has its own", UI_COLOR_TEXT_MUTED);
    Top += UILineHeight(Small) + UI_GAP;

    float ColumnWidth = (Inner - (Columns - 1) * UI_GAP_LARGE) / Columns;
    for(u32 Index = 0; Index < RowCount; Index++)
    {
        u32 Row = Rows[Index];
        float RowLeft = Left + (Index / PerColumn) * (ColumnWidth + UI_GAP_LARGE);
        float RowTop = Top + (Index % PerColumn) * RowHeight;
        float KeyX = RowLeft + ColumnWidth - KEY_BINDINGS_KEY_WIDTH;
        bool32 Waiting = Menu->Waiting == Row + 1;
        bool32 Clash = KeyBindingClashes(Menu->Mode, GlobalControlScheme, Row);
        bool32 Changed = Bindings->Keys[Menu->Mode][GlobalControlScheme][Row] != KeyCode_None;
        UIText(RenderContext, Body, RowLeft, RowTop + 0.5f * (RowHeight - 6.f - UILineHeight(Body)),
               KeyBindingRowName(AppState, Menu->Mode, Row),
               Changed ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
        char *KeyText = Waiting ? (char *)"Press a key" :
            KeyCodeName(BoundKeyCodeIn(Menu->Mode, GlobalControlScheme, Row));
        u32 KeyColor = Waiting ? UI_COLOR_ACCENT : Clash ? UI_COLOR_HEALTH : UI_COLOR_TEXT;
        if (KeyBindingsButton(RenderContext, Input, Body, KeyX, RowTop, KEY_BINDINGS_KEY_WIDTH,
                              RowHeight - 6.f, KeyText, Waiting, KeyColor))
        {
            Menu->Waiting = Waiting ? 0 : Row + 1;
        }
    }
    Top += PerColumn * RowHeight + UI_GAP;

    char *Line = Menu->Waiting ? (char *)"Press the new key or mouse button; Backspace for the default" :
        (char *)"Click a key to change it";
    u32 LineColor = UI_COLOR_TEXT_MUTED;
    if (Menu->NoticeLeft > 0.f)
    {
        Line = Menu->Notice;
        LineColor = UI_COLOR_ACCENT;
    }
    UIText(RenderContext, Small, Left, Top, Line, LineColor);
    Top += UILineHeight(Small) + UI_GAP;

    float ButtonWidth = 0.5f * (Inner - UI_GAP);
    bool32 CanReset = KeyBindingSetChanged(Bindings, Menu->Mode, GlobalControlScheme);
    if (KeyBindingsButton(RenderContext, Input, Body, Left, Top, ButtonWidth, OPTIONS_BUTTON_HEIGHT,
                          "Reset this tab", false,
                          CanReset ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED) && CanReset)
    {
        ResetKeyBindingSet(Bindings, Menu->Mode, GlobalControlScheme);
        Menu->Waiting = 0;
        KeyBindingsNotice(Menu, "%s keys back to the defaults", Tabs[Menu->Mode]);
        SaveKeyBindings(Bindings);
    }
    if (KeyBindingsButton(RenderContext, Input, Body, Left + ButtonWidth + UI_GAP, Top,
                          ButtonWidth, OPTIONS_BUTTON_HEIGHT, "Back", false))
    {
        Menu->Open = false;
    }
    // NOTE(zoubir): a click anywhere else stops waiting
    if (Menu->Waiting && Input->LeftButton.Pressed && !GlobalMouseOnButton)
    {
        Menu->Waiting = 0;
    }
}
