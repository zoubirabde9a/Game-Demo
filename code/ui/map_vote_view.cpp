/* Map vote on screen (sim/map_vote.cpp). In the Escape menu
   (options_menu.cpp), a Mode and map section: a row with the game
   modes, Duel, Team duel and Dungeon, where another one asks everyone to
   move to it (a team duel stays on the duel map being played), in a
   team duel a button that opens the team panel (ui/teams/), then a
   button per other map of the mode being played, which asks everyone to
   move there. While a vote is open: who asked for
   which map (and mode, when it changes), the answers so far and Yes / No
   buttons. Over the game, while a vote is open, a plate at the top says
   so, with its own Yes / No buttons (and keys 1 and 2), so nobody has to
   open the menu to answer. Both
   list every player with their answer (DrawVoteAnswers), so everyone
   sees who has voted and who is still deciding.
   Clicks become requests (client/vote_requests.cpp). */

#define MAP_VOTE_COLUMNS 3
#define MAP_VOTE_BUTTON_HEIGHT 36.f
#define MAP_VOTE_PLATE_WIDTH 460.f

internal u32
CountActivePlayers(app_state *AppState)
{
    u32 Result = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Result += AppState->Players[SlotIndex].Active ? 1 : 0;
    }
    return Result;
}

enum game_mode
{
    GameMode_Duel,
    GameMode_TeamDuel,
    GameMode_Dungeon,
    GameMode_Count
};

global_variable char *GameModeNames[GameMode_Count] = {"Duel", "Team duel", "Dungeon"};

inline u32
GameModeOf(u32 MapId, bool32 Teams)
{
    u32 Result = IsDungeonMap(MapId) ? GameMode_Dungeon :
        Teams ? GameMode_TeamDuel : GameMode_Duel;
    return Result;
}

inline u32
CurrentGameMode(app_state *AppState)
{
    u32 Result = GameModeOf(AppState->World.MapId, IsTeamDuel(AppState));
    return Result;
}

// NOTE(zoubir): the vote request that asks for Mode: a team duel, or
// back to a free-for-all, through MapVote_Teams, else the mode's first
// map; 0 for the mode being played
internal u32
GameModeRequest(app_state *AppState, u32 Mode)
{
    u32 Current = CurrentGameMode(AppState);
    u32 Result = 0;
    if (Mode == Current)
    {
    }
    else if (Mode == GameMode_TeamDuel ||
             (Mode == GameMode_Duel && Current == GameMode_TeamDuel))
    {
        Result = MapVote_Teams;
    }
    else
    {
        u32 First = FirstMapOfMode(Mode == GameMode_Dungeon);
        Result = IsVotableMap(AppState, First) ? MapVoteAsk(First) : 0;
    }
    return Result;
}

// NOTE(zoubir): ui/teams/team_panel.cpp, included after this
internal void OpenTeamPanel(app_state *AppState);

// NOTE(zoubir): the maps a button is drawn for: the other maps of the mode
// being played (a mode change goes through its own row)
inline bool32
IsMapButtonShown(app_state *AppState, u32 MapId)
{
    bool32 Result = IsVotableMap(AppState, MapId) &&
        IsDungeonMap(MapId) == IsDungeonMap(AppState->World.MapId);
    return Result;
}

// NOTE(zoubir): "Sunken Crypt", or "Dungeon: Sunken Crypt" when the vote
// is for another mode ("Team duel: Old Arena"), so it says the mode changes
internal void
VoteMapText(app_state *AppState, char *Out, u32 OutSize)
{
    u32 MapId = AppState->VoteMap;
    char *Name = MapId < MapId_Count ? GetMapDef((map_id)MapId)->Name : (char *)"";
    u32 Mode = GameModeOf(MapId, AppState->VoteTeams);
    if (Mode != CurrentGameMode(AppState))
    {
        snprintf(Out, OutSize, "%s: %s", GameModeNames[Mode], Name);
    }
    else
    {
        snprintf(Out, OutSize, "%s", Name);
    }
}

// NOTE(zoubir): one player's part of the answers line: "Gary: yes", and
// "You: ..." for the local player
internal u32
VoteAnswerText(app_state *AppState, u32 SlotIndex, char *Out, u32 OutSize)
{
    char Name[24];
    GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
    if (SlotIndex == AppState->LocalPlayerIndex)
    {
        snprintf(Name, sizeof(Name), "You");
    }
    u8 Answer = AppState->Votes[SlotIndex];
    char *Word = (char *)(Answer == MapVote_Yes ? "yes" : Answer == MapVote_No ? "no" : "...");
    snprintf(Out, OutSize, "%s: %s", Name, Word);
    u32 Result = Answer == MapVote_Yes ? UI_COLOR_GOOD :
        Answer == MapVote_No ? UI_COLOR_HEALTH : UI_COLOR_TEXT_MUTED;
    return Result;
}

// NOTE(zoubir): every player in the game and their answer, green for
// yes, red for no, grey while they decide, centred on CenterX
internal void
DrawVoteAnswers(render_context *RenderContext, app_state *AppState, float CenterX, float Y)
{
    font *Small = AppState->Fonts.Small;
    float Spacing = UITextWidth(Small, "   ");
    char Text[48];
    float Total = 0.f;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (AppState->Players[SlotIndex].Active)
        {
            VoteAnswerText(AppState, SlotIndex, Text, sizeof(Text));
            Total += (Total > 0.f ? Spacing : 0.f) + UITextWidth(Small, Text);
        }
    }
    float X = CenterX - 0.5f * Total;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (AppState->Players[SlotIndex].Active)
        {
            u32 Color = VoteAnswerText(AppState, SlotIndex, Text, sizeof(Text));
            UIText(RenderContext, Small, X, Y, Text, Color);
            X += UITextWidth(Small, Text) + Spacing;
        }
    }
}

// NOTE(zoubir): the section's height under its heading
internal float
MapVoteSectionHeight(app_state *AppState)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float Result;
    if (AppState->VoteOpen)
    {
        Result = UILineHeight(Body) + 2.f * UILineHeight(Small) + UI_GAP_SMALL +
            MAP_VOTE_BUTTON_HEIGHT;
    }
    else
    {
        u32 Others = 0;
        for(u32 MapId = 0; MapId < MapId_Count; MapId++)
        {
            Others += IsMapButtonShown(AppState, MapId) ? 1 : 0;
        }
        u32 Rows = (Others + MAP_VOTE_COLUMNS - 1) / MAP_VOTE_COLUMNS;
        Result = (IsTeamDuel(AppState) ? MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL : 0.f) +
            (MAP_VOTE_BUTTON_HEIGHT + UI_GAP) +
            (float)Rows * (MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL) +
            (Rows ? 0.f : UILineHeight(Small) + UI_GAP_SMALL) +
            UILineHeight(Small);
    }
    return Result;
}

// NOTE(zoubir): draws the section at Top, Width wide, and turns clicks
// into vote requests
internal void
DoMapVoteSection(render_context *RenderContext, app_state *AppState, app_input *Input,
                 float Left, float Top, float Width)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    char Text[128];
    if (AppState->VoteOpen)
    {
        char Who[48];
        GetPlayerName(AppState, AppState->VoteBy, Who, sizeof(Who));
        char Map[64];
        VoteMapText(AppState, Map, sizeof(Map));
        snprintf(Text, sizeof(Text), "%s wants %s", Who, Map);
        UIText(RenderContext, Body, Left, Top, Text, UI_COLOR_TEXT);
        snprintf(Text, sizeof(Text), "%.0f s", Maximum(0.f, AppState->VoteSeconds));
        UIText(RenderContext, Body, Left + Width, Top, Text, UI_COLOR_TEXT_MUTED,
               UIAlign_Right);
        Top += UILineHeight(Body);
        snprintf(Text, sizeof(Text), "Yes %u, no %u of %u. More than half yes moves "
                 "everyone, back to level 1", AppState->VoteYes, AppState->VoteNo,
                 CountActivePlayers(AppState));
        UIText(RenderContext, Small, Left, Top, Text, UI_COLOR_TEXT_MUTED);
        Top += UILineHeight(Small);
        DrawVoteAnswers(RenderContext, AppState, Left + 0.5f * Width, Top);
        Top += UILineHeight(Small) + UI_GAP_SMALL;

        u8 Own = AppState->Votes[AppState->LocalPlayerIndex];
        char *Labels[] = {"Yes", "No"};
        u32 Answers[] = {MapVote_Yes, MapVote_No};
        float ButtonWidth = 0.5f * (Width - UI_GAP);
        for(u32 Index = 0; Index < 2; Index++)
        {
            float X = Left + Index * (ButtonWidth + UI_GAP);
            bool32 Selected = Own == Answers[Index];
            if (OptionsButton(RenderContext, Input, X, Top, ButtonWidth,
                              MAP_VOTE_BUTTON_HEIGHT, Selected) && !Selected)
            {
                RequestVote(AppState, Answers[Index]);
            }
            UIText(RenderContext, Body, X + 0.5f * ButtonWidth,
                   Top + 0.5f * (MAP_VOTE_BUTTON_HEIGHT - UILineHeight(Body)),
                   Labels[Index], Selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT,
                   UIAlign_Center);
        }
        return;
    }

    // NOTE(zoubir): the modes; the one being played is lit, another asks
    // everyone to move to it
    u32 Current = CurrentGameMode(AppState);
    float ModeWidth = (Width - (GameMode_Count - 1) * UI_GAP_SMALL) / GameMode_Count;
    for(u32 Mode = 0; Mode < GameMode_Count; Mode++)
    {
        float X = Left + Mode * (ModeWidth + UI_GAP_SMALL);
        bool32 Selected = Mode == Current;
        u32 Request = GameModeRequest(AppState, Mode);
        if (OptionsButton(RenderContext, Input, X, Top, ModeWidth,
                          MAP_VOTE_BUTTON_HEIGHT, Selected) && Request)
        {
            RequestVote(AppState, Request);
        }
        UIText(RenderContext, Body, X + 0.5f * ModeWidth,
               Top + 0.5f * (MAP_VOTE_BUTTON_HEIGHT - UILineHeight(Body)),
               GameModeNames[Mode], Selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT, UIAlign_Center);
    }
    Top += MAP_VOTE_BUTTON_HEIGHT + UI_GAP;
    // NOTE(zoubir): in a team duel, the way to the team panel
    u32 Mine = PlayerTeam(AppState, AppState->LocalPlayerIndex);
    if (Mine != Team_None)
    {
        if (OptionsButton(RenderContext, Input, Left, Top, Width, MAP_VOTE_BUTTON_HEIGHT, false))
        {
            AppState->OptionsOpen = false;
            OpenTeamPanel(AppState);
        }
        snprintf(Text, sizeof(Text), "You're on %s.  Teams (M)", TeamName(Mine));
        UIText(RenderContext, Body, Left + 0.5f * Width,
               Top + 0.5f * (MAP_VOTE_BUTTON_HEIGHT - UILineHeight(Body)), Text,
               TeamStrongColor(Mine), UIAlign_Center);
        Top += MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL;
    }

    float ButtonWidth = (Width - (MAP_VOTE_COLUMNS - 1) * UI_GAP_SMALL) / MAP_VOTE_COLUMNS;
    u32 Column = 0;
    u32 Shown = 0;
    for(u32 MapId = 0; MapId < MapId_Count; MapId++)
    {
        if (!IsMapButtonShown(AppState, MapId))
        {
            continue;
        }
        Shown++;
        float X = Left + Column * (ButtonWidth + UI_GAP_SMALL);
        if (OptionsButton(RenderContext, Input, X, Top, ButtonWidth,
                          MAP_VOTE_BUTTON_HEIGHT, false))
        {
            RequestVote(AppState, MapVoteAsk(MapId));
        }
        UIText(RenderContext, Body, X + 0.5f * ButtonWidth,
               Top + 0.5f * (MAP_VOTE_BUTTON_HEIGHT - UILineHeight(Body)),
               GetMapDef((map_id)MapId)->Name, UI_COLOR_TEXT, UIAlign_Center);
        if (++Column == MAP_VOTE_COLUMNS)
        {
            Column = 0;
            Top += MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL;
        }
    }
    if (Column)
    {
        Top += MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL;
    }
    if (!Shown)
    {
        UIText(RenderContext, Small, Left, Top, "No other map in this mode",
               UI_COLOR_TEXT_MUTED);
        Top += UILineHeight(Small) + UI_GAP_SMALL;
    }
    char *Hint = "Starts over there at level 1";
    if (CountActivePlayers(AppState) > 1)
    {
        Hint = "Asks for a vote. A new map or mode starts everyone over at level 1";
    }
    UIText(RenderContext, Small, Left, Top, Hint, UI_COLOR_TEXT_MUTED);
}

// NOTE(zoubir): over the game while a vote is open and the menu is not:
// who wants which map, everyone's answers, the time left draining, and
// Yes / No buttons to answer without opening the menu, or keys 1 and 2.
// The local player's answer stays lit and can still be changed
internal void
DoMapVotePlate(render_context *RenderContext, app_state *AppState, app_input *Input,
               u32 WindowWidth)
{
    if (!AppState->VoteOpen || AppState->OptionsOpen)
    {
        return;
    }
    u8 Own = AppState->Votes[AppState->LocalPlayerIndex];
    if (!ConnectScreenTakesInput(AppState))
    {
        // NOTE(zoubir): a digit the player put a spell on casts it and
        // does not vote (client/key_bindings.cpp); the buttons still do
        bool32 Yes = Input->NumbersButtons[1].Pressed && !KeyCodeBoundNow(KeyCode_0 + 1);
        bool32 No = Input->NumbersButtons[2].Pressed && !KeyCodeBoundNow(KeyCode_0 + 2);
        if (Yes && Own != MapVote_Yes)
        {
            RequestVote(AppState, MapVote_Yes);
        }
        else if (No && Own != MapVote_No)
        {
            RequestVote(AppState, MapVote_No);
        }
    }
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float Width = Minimum(MAP_VOTE_PLATE_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float ButtonHeight = MAP_VOTE_BUTTON_HEIGHT - 6.f;
    float Height = UILineHeight(Body) + UILineHeight(Small) + ButtonHeight +
        3.f * UI_GAP_SMALL + UI_GAP + 4.f;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = UI_GAP_LARGE;
    DrawUIPanel(RenderContext, X, Y, Width, Height, UI_COLOR_ACCENT);
    char Who[48];
    GetPlayerName(AppState, AppState->VoteBy, Who, sizeof(Who));
    char Text[128];
    char Map[64];
    VoteMapText(AppState, Map, sizeof(Map));
    snprintf(Text, sizeof(Text), "%s wants %s", Who, Map);
    float Top = Y + UI_GAP_SMALL;
    UIText(RenderContext, Body, X + 0.5f * Width, Top, Text, UI_COLOR_TEXT, UIAlign_Center);
    Top += UILineHeight(Body);
    DrawVoteAnswers(RenderContext, AppState, X + 0.5f * Width, Top);
    Top += UILineHeight(Small) + UI_GAP_SMALL;

    char *Labels[] = {"Yes  [1]", "No  [2]"};
    u32 Answers[] = {MapVote_Yes, MapVote_No};
    u32 Colors[] = {UI_COLOR_GOOD, UI_COLOR_HEALTH};
    float Inner = Width - 2.f * UI_GAP;
    float ButtonWidth = 0.5f * (Inner - UI_GAP);
    for(u32 Index = 0; Index < 2; Index++)
    {
        float ButtonX = X + UI_GAP + Index * (ButtonWidth + UI_GAP);
        bool32 Selected = Own == Answers[Index];
        if (OptionsButton(RenderContext, Input, ButtonX, Top, ButtonWidth, ButtonHeight,
                          Selected) && !Selected)
        {
            RequestVote(AppState, Answers[Index]);
        }
        UIText(RenderContext, Body, ButtonX + 0.5f * ButtonWidth,
               Top + 0.5f * (ButtonHeight - UILineHeight(Body)), Labels[Index],
               Selected ? Colors[Index] : UI_COLOR_TEXT, UIAlign_Center);
    }
    Top += ButtonHeight + UI_GAP_SMALL;

    // NOTE(zoubir): the time left, draining, with the seconds at its end
    snprintf(Text, sizeof(Text), "%.0f s", Maximum(0.f, AppState->VoteSeconds));
    float SecondsWidth = UITextWidth(Small, Text) + UI_GAP_SMALL;
    float BarWidth = Inner - SecondsWidth;
    float BarY = Top + 0.5f * UILineHeight(Small) - 2.f;
    float Share = Clamp01(AppState->VoteSeconds / MAP_VOTE_SECONDS);
    DrawRoundRect(RenderContext, X + UI_GAP, BarY, BarWidth, 4.f, UI_COLOR_TRACK);
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, X + UI_GAP, BarY, Maximum(4.f, Share * BarWidth), 4.f,
                      AppState->VoteSeconds < 8.f ? UI_COLOR_HEALTH : UI_COLOR_ACCENT);
    }
    UIText(RenderContext, Small, X + Width - UI_GAP, Top, Text, UI_COLOR_TEXT_MUTED,
           UIAlign_Right);
}
