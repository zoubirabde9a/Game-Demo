/* Map vote on screen (sim/map_vote.cpp). In the Escape menu
   (options_menu.cpp), a Map section: a button per other map, which asks
   everyone to move there, or, while a vote is open, who asked for which
   map, the answers so far and Yes / No buttons. Over the game, while a
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

// NOTE(zoubir): the section's height under its "Map" heading
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
        u32 Others = MapId_Count - 1;
        u32 Rows = (Others + MAP_VOTE_COLUMNS - 1) / MAP_VOTE_COLUMNS;
        Result = (float)Rows * (MAP_VOTE_BUTTON_HEIGHT + UI_GAP_SMALL) +
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
        snprintf(Text, sizeof(Text), "%s wants %s", Who,
                 GetMapDef((map_id)AppState->VoteMap)->Name);
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

    float ButtonWidth = (Width - (MAP_VOTE_COLUMNS - 1) * UI_GAP_SMALL) / MAP_VOTE_COLUMNS;
    u32 Column = 0;
    for(u32 MapId = 0; MapId < MapId_Count; MapId++)
    {
        if (MapId == AppState->World.MapId)
        {
            continue;
        }
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
    char *Hint = "Starts over there at level 1";
    if (CountActivePlayers(AppState) > 1)
    {
        Hint = "Asks for a vote. The new map starts everyone over at level 1";
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
    snprintf(Text, sizeof(Text), "%s wants %s  -  %s  (%.0f s)", Who,
             GetMapDef((map_id)AppState->VoteMap)->Name,
             Answered ? "waiting for the others" : "Esc to vote",
             Maximum(0.f, AppState->VoteSeconds));
    UIText(RenderContext, Body, X + 0.5f * Width, Y + UI_GAP_SMALL, Text,
           UI_COLOR_TEXT, UIAlign_Center);
}
