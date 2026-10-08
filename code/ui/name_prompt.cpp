/* Name prompt: the connect screen's first step when the player has no
   usable name yet (client/player_name.cpp). One field, filled with a
   suggested name already selected, so Enter plays with it and typing
   replaces it; Shuffle offers another. Play online saves the name and
   joins the server the session points at; Play offline leaves it offline
   until the next launch or F4. F4 switches to the full connect screen.
   Included by connect_screen.cpp, which opens it (OpenConnectScreen). */

#define NAME_PROMPT_WIDTH 440.f

// NOTE(zoubir): fills the field with a new suggestion, selected so the
// first key typed replaces it
internal void
SuggestNameInPrompt(connect_screen *Screen, ui_context *UIContext)
{
    char Name[NET_NAME_SIZE];
    Screen->SuggestionSeed++;
    SuggestPlayerName(Name, sizeof(Name), Screen->SuggestionSeed, Screen->Name.Text);
    SetEditBoxText(&Screen->Name, Name);
    UISelectAllText(UIContext, &Screen->Name);
    Screen->ShowNameProblem = false;
}

// NOTE(zoubir): the field's own line under it: what is wrong with the
// name, or what Enter does, on the left; characters used on the right
internal void
DrawNameHint(render_context *RenderContext, app_state *AppState,
             connect_screen *Screen, float X, float Y, float Width, char *Fine)
{
    font *Small = AppState->Fonts.Small;
    char Name[NET_NAME_SIZE];
    CleanPlayerName(Name, sizeof(Name), Screen->Name.Text);
    char *Problem = PlayerNameProblem(Name);
    bool32 Loud = Problem && (Screen->ShowNameProblem || Name[0]);
    UIText(RenderContext, Small, X, Y, Problem ? Problem : Fine,
           Loud ? UI_COLOR_HEALTH : UI_COLOR_TEXT_MUTED);
    char Count[16];
    snprintf(Count, sizeof(Count), "%u/%u", Screen->Name.TextCount, NET_NAME_SIZE - 1);
    UIText(RenderContext, Small, X + Width, Y, Count,
           Screen->Name.TextCount >= NET_NAME_SIZE - 1 ? UI_COLOR_ACCENT : UI_COLOR_TEXT_MUTED,
           UIAlign_Right);
}

// NOTE(zoubir): the full screen's note after "Your name", right-aligned
// at Right: why the name was refused, else the name the server gave a
// taken one ("Ash Fox 2"), else the characters used
internal void
DrawNameLabelNote(render_context *RenderContext, app_state *AppState,
                  connect_screen *Screen, float Right, float Y)
{
    font *Small = AppState->Fonts.Small;
    online_session *Online = AppState->Online;
    char Name[NET_NAME_SIZE];
    CleanPlayerName(Name, sizeof(Name), Screen->Name.Text);
    char *Problem = PlayerNameProblem(Name);
    char *Given = AppState->Players[AppState->LocalPlayerIndex].Name;
    char Note[64];
    u32 Color = UI_COLOR_TEXT_MUTED;
    if (Problem && Screen->ShowNameProblem)
    {
        snprintf(Note, sizeof(Note), "%s", Problem);
        Color = UI_COLOR_HEALTH;
    }
    else if (IsOnline(Online) && Given[0] && strcmp(Given, Online->NameText) != 0)
    {
        snprintf(Note, sizeof(Note), "taken here, you are %s", Given);
        Color = UI_COLOR_ACCENT;
    }
    else
    {
        snprintf(Note, sizeof(Note), "%u/%u", Screen->Name.TextCount, NET_NAME_SIZE - 1);
    }
    UIText(RenderContext, Small, Right, Y, Note, Color, UIAlign_Right);
}

// NOTE(zoubir): how far the field is pushed sideways while it shakes
// after a refused name
internal float
NameFieldShake(connect_screen *Screen, float DeltaTime)
{
    Screen->Shake = Maximum(0.f, Screen->Shake - DeltaTime);
    float Fade = Screen->Shake / NAME_PROMPT_SHAKE_SECONDS;
    float Result = 7.f * Fade * sinf(60.f * Screen->Shake);
    return Result;
}

internal void
DoNamePrompt(render_context *RenderContext, app_state *AppState,
             ui_context *UIContext, app_input *Input,
             u32 WindowWidth, u32 WindowHeight)
{
    connect_screen *Screen = AppState->ConnectScreen;
    online_session *Online = AppState->Online;
    font *Font = AppState->DefaultFont;
    font *Small = AppState->Fonts.Small;
    font *Title = AppState->Fonts.Title;

    float Width = NAME_PROMPT_WIDTH;
    float Pad = UI_GAP_LARGE;
    float FieldWidth = Width - 2.f * Pad;
    float RowHeight = UI_ROW_HEIGHT;
    online_phase Phase = GetOnlinePhase(Online);
    char Status[128];
    GetOnlineStatusText(Online, Status, sizeof(Status));
    float Height = Pad + UILineHeight(Title) + UI_GAP_SMALL + UILineHeight(Small) + UI_GAP +
        RowHeight + UI_GAP_SMALL + UILineHeight(Small) + UI_GAP_LARGE +
        RowHeight + Pad;
    if (Status[0])
    {
        Height += UI_GAP + UILineHeight(Small);
    }
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = Maximum(8.f, 0.42f * ((float)WindowHeight - Height));
    DrawUIPanel(RenderContext, Left - 6.f, Top - 6.f, Width + 12.f, Height + 12.f);

    float Y = Pad;
    UIText(RenderContext, Title, Left + Pad, Top + Y, "Pick your name", UI_COLOR_TEXT);
    UIText(RenderContext, Small, Left + Width - Pad, Top + Y + 8.f,
           "F4 for all servers", UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Y += UILineHeight(Title) + UI_GAP_SMALL;
    UIText(RenderContext, Small, Left + Pad, Top + Y,
           "Other players see it over your head and on the scoreboard.",
           UI_COLOR_TEXT_MUTED);
    Y += UILineHeight(Small) + UI_GAP;
    float FieldY = Y;
    Y += RowHeight + UI_GAP_SMALL;
    DrawNameHint(RenderContext, AppState, Screen, Left + Pad, Top + Y, FieldWidth,
                 (char *)"Enter to play with this name");
    Y += UILineHeight(Small) + UI_GAP_LARGE;
    float ButtonY = Y;
    Y += RowHeight + UI_GAP;

    if (Status[0])
    {
        UIText(RenderContext, Small, Left + Pad, Top + Y, Status,
               Phase == OnlinePhase_Ended || Online->BadAddress ?
               UI_COLOR_HEALTH : UI_COLOR_TEXT_MUTED);
    }

    BeginContainer(UIContext, Left, Top, Width, Height);
    float ShuffleWidth = 96.f;
    float Shake = NameFieldShake(Screen, Input->DeltaTime);
    DoEditBox(&Screen->Name, AppState, UIContext, Pad + Shake, FieldY,
              FieldWidth - ShuffleWidth - UI_GAP_SMALL, RowHeight, NET_NAME_SIZE - 1);
    if (DoButton(&Screen->ShuffleButton, AppState, UIContext,
                 Width - Pad - ShuffleWidth, FieldY, ShuffleWidth, RowHeight, "Shuffle"))
    {
        SuggestNameInPrompt(Screen, UIContext);
    }

    char Name[NET_NAME_SIZE];
    CleanPlayerName(Name, sizeof(Name), Screen->Name.Text);
    bool32 Ready = !PlayerNameProblem(Name);
    bool32 Trying = (Phase == OnlinePhase_Joining);
    float HalfWidth = 0.5f * (FieldWidth - UI_GAP_SMALL);
    char *PlayText = (char *)(Trying ? "Joining..." : "Play online");
    bool32 Play = DoButton(&Screen->JoinButton, AppState, UIContext, Pad, ButtonY,
                           HalfWidth, RowHeight, PlayText, Ready && !Trying);
    if ((Play || Input->TextSubmit) && !Trying)
    {
        char *Address = Online->AddressText[0] ? Online->AddressText : ServerList[0].Address;
        ConnectScreenJoin(Screen, Online, UIContext, Address);
    }
    if (DoButton(&Screen->RightButton, AppState, UIContext,
                 Width - Pad - HalfWidth, ButtonY, HalfWidth, RowHeight, "Play offline"))
    {
        OnlineDisconnect(Online);
        Online->NeedsName = false;
        Screen->WaitingToJoin = false;
        Screen->Open = false;
    }
    EndContainer(UIContext);
}
