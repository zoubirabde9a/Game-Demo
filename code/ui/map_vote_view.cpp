/* Map vote on screen (sim/map_vote.cpp). In the Escape menu
   (options_menu.cpp), a Mode and map section: a row with the two game
   modes, Duel and Dungeon, where the other one asks everyone to move to
   its first map, then a button per other map of the mode being played,
   which asks everyone to move there. While a vote is open: who asked for
   which map (and mode, when it changes), the answers so far and Yes / No
   buttons. Over the game, while a
   vote is open, a slim plate at the top says so and how to answer.
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

struct game_mode_choice
{
    bool32 Dungeon;
    char *Name;
};

global_variable game_mode_choice GameModeChoices[] =
{
    {false, "Duel"},
    {true, "Dungeon"},
};

// NOTE(zoubir): the maps a button is drawn for: the other maps of the mode
// being played (a mode change goes through its own row)
inline bool32
IsMapButtonShown(app_state *AppState, u32 MapId)
{
    bool32 Result = IsVotableMap(AppState, MapId) &&
        IsDungeonMap(MapId) == IsDungeonMap(AppState->World.MapId);
    return Result;
}

// NOTE(zoubir): "Sunken Crypt", or "Dungeon: Sunken Crypt" when the map
// is of the other mode, so the vote says the mode changes
internal void
VoteMapText(app_state *AppState, char *Out, u32 OutSize)
{
    u32 MapId = AppState->VoteMap;
    char *Name = GetMapDef((map_id)MapId)->Name;
    if (IsDungeonMap(MapId) != IsDungeonMap(AppState->World.MapId))
    {
        snprintf(Out, OutSize, "%s: %s", GameModeChoices[IsDungeonMap(MapId) ? 1 : 0].Name,
                 Name);
    }
    else
    {
        snprintf(Out, OutSize, "%s", Name);
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
        Result = UILineHeight(Body) + UILineHeight(Small) + UI_GAP_SMALL +
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
        Result = (MAP_VOTE_BUTTON_HEIGHT + UI_GAP) +
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

    // NOTE(zoubir): the modes; the one being played is lit, the other
    // asks for its first map
    bool32 InDungeon = IsDungeonMap(AppState->World.MapId);
    float ModeWidth = 0.5f * (Width - UI_GAP);
    for(u32 Index = 0; Index < ArrayCount(GameModeChoices); Index++)
    {
        game_mode_choice *Mode = GameModeChoices + Index;
        float X = Left + Index * (ModeWidth + UI_GAP);
        bool32 Selected = (Mode->Dungeon != 0) == (InDungeon != 0);
        u32 First = FirstMapOfMode(Mode->Dungeon);
        if (OptionsButton(RenderContext, Input, X, Top, ModeWidth,
                          MAP_VOTE_BUTTON_HEIGHT, Selected) &&
            !Selected && IsVotableMap(AppState, First))
        {
            RequestVote(AppState, MapVoteAsk(First));
        }
        UIText(RenderContext, Body, X + 0.5f * ModeWidth,
               Top + 0.5f * (MAP_VOTE_BUTTON_HEIGHT - UILineHeight(Body)),
               Mode->Name, Selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT, UIAlign_Center);
    }
    Top += MAP_VOTE_BUTTON_HEIGHT + UI_GAP;

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

// NOTE(zoubir): over the game while a vote is open and the menu is not
internal void
DrawMapVotePlate(render_context *RenderContext, app_state *AppState, u32 WindowWidth)
{
    if (!AppState->VoteOpen || AppState->OptionsOpen)
    {
        return;
    }
    font *Body = AppState->Fonts.Body;
    float Width = Minimum(MAP_VOTE_PLATE_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float Height = UILineHeight(Body) + 2.f * UI_GAP_SMALL;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = UI_GAP_LARGE;
    DrawUIPanel(RenderContext, X, Y, Width, Height, UI_COLOR_ACCENT);
    char Who[48];
    GetPlayerName(AppState, AppState->VoteBy, Who, sizeof(Who));
    char Text[128];
    bool32 Answered = AppState->Votes[AppState->LocalPlayerIndex] != MapVote_None;
    char Map[64];
    VoteMapText(AppState, Map, sizeof(Map));
    snprintf(Text, sizeof(Text), "%s wants %s  -  %s  (%.0f s)", Who, Map,
             Answered ? "waiting for the others" : "Esc to vote",
             Maximum(0.f, AppState->VoteSeconds));
    UIText(RenderContext, Body, X + 0.5f * Width, Y + UI_GAP_SMALL, Text,
           UI_COLOR_TEXT, UIAlign_Center);
}
