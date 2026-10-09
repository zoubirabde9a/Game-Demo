/* Match screens: the scoreboard shown while Tab is held (every player's
   level, kills, deaths and monster kills, best first). In a team duel the
   players are listed under their team, the local player's team first,
   each team headed by its name and kills in its colour. The respawn
   countdown is the death plate in kill_feed_view.cpp. */


// NOTE(zoubir): more kills first, then fewer deaths, then slot order
inline bool32
RanksAbove(player_slot *A, player_slot *B)
{
    if (A->Kills != B->Kills) return A->Kills > B->Kills;
    return A->Deaths < B->Deaths;
}

// NOTE(zoubir): fills Order with active slot indices, best first; returns
// how many
internal u32
RankPlayers(app_state *AppState, u32 *Order)
{
    u32 Count = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (AppState->Players[SlotIndex].Active)
        {
            u32 Insert = Count++;
            while (Insert > 0 &&
                   RanksAbove(&AppState->Players[SlotIndex],
                              &AppState->Players[Order[Insert - 1]]))
            {
                Order[Insert] = Order[Insert - 1];
                Insert--;
            }
            Order[Insert] = SlotIndex;
        }
    }
    return Count;
}

// NOTE(zoubir): in a team duel, Order regrouped by team, the local
// player's first, with a header row (MAX_PLAYERS + team) before each
// team; returns the rows. Otherwise Order as it was
internal u32
GroupByTeam(app_state *AppState, u32 *Order, u32 Count, u32 *Rows)
{
    u32 Mine = PlayerTeam(AppState, AppState->LocalPlayerIndex);
    if (Mine == Team_None)
    {
        for(u32 Index = 0; Index < Count; Index++)
        {
            Rows[Index] = Order[Index];
        }
        return Count;
    }
    u32 Result = 0;
    u32 Teams[2] = {Mine, OtherTeam(Mine)};
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        Rows[Result++] = MAX_PLAYERS + Teams[Pass];
        for(u32 Index = 0; Index < Count; Index++)
        {
            if (PlayerTeam(AppState, Order[Index]) == Teams[Pass])
            {
                Rows[Result++] = Order[Index];
            }
        }
    }
    return Result;
}

// NOTE(zoubir): a team's header row: a bar in its colour, its name and
// its kills under the Kills column
internal void
DrawScoreboardTeamRow(render_context *RenderContext, app_state *AppState, u32 Team,
                      float Left, float Width, float Y, float KillsRight)
{
    font *Font = AppState->Fonts.Body;
    DrawRoundRect(RenderContext, Left + 8.f, Y + 2.f, Width - 16.f, UILineHeight(Font) + 2.f,
                  WithAlpha(TeamColor(Team), 0.22f));
    DrawRoundRect(RenderContext, Left + 8.f, Y + 2.f, 4.f, UILineHeight(Font) + 2.f,
                  TeamColor(Team));
    UIText(RenderContext, Font, Left + UI_GAP_LARGE, Y + 2.f, TeamName(Team),
           TeamStrongColor(Team));
    char Text[16];
    snprintf(Text, sizeof(Text), "%u", TeamKills(AppState, Team));
    UIText(RenderContext, Font, KillsRight, Y + 2.f, Text, TeamStrongColor(Team), UIAlign_Right);
}

internal void
DrawScoreboard(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    font *Font = AppState->Fonts.Body;
    u32 Order[MAX_PLAYERS];
    u32 Ranked = RankPlayers(AppState, Order);
    u32 Rows[MAX_PLAYERS + 2];
    u32 Count = GroupByTeam(AppState, Order, Ranked, Rows);

    float RowHeight = 30.f;
    float Width = 520.f;
    float Height = RowHeight * (Count + 1) + 24.f;
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = 0.5f * ((float)WindowHeight - Height);
    // NOTE(zoubir): the name is left-aligned, the numbers right-aligned
    // on these edges so digits line up
    float Columns[] = {Left + UI_GAP_LARGE, Left + 270.f, Left + 330.f, Left + 410.f,
                       Left + Width - UI_GAP_LARGE};

    DrawUIPanel(RenderContext, Left, Top, Width, Height);

    char *Headers[] = {"Player", "Level", "Kills", "Deaths", "Mobs"};
    float Y = Top + 12.f;
    for(u32 Column = 0; Column < ArrayCount(Headers); Column++)
    {
        UIText(RenderContext, Font, Columns[Column], Y, Headers[Column],
               UI_COLOR_TEXT_MUTED, Column ? UIAlign_Right : UIAlign_Left);
    }

    for(u32 Rank = 0; Rank < Count; Rank++)
    {
        u32 SlotIndex = Rows[Rank];
        Y += RowHeight;
        if (SlotIndex >= MAX_PLAYERS)
        {
            DrawScoreboardTeamRow(RenderContext, AppState, SlotIndex - MAX_PLAYERS, Left, Width,
                                  Y - 4.f, Columns[2]);
            continue;
        }
        player_slot *Slot = &AppState->Players[SlotIndex];
        bool32 IsLocal = (SlotIndex == AppState->LocalPlayerIndex);
        u32 Color = IsLocal ? UI_COLOR_ACCENT : UI_COLOR_TEXT;

        char Text[32];
        if (IsLocal)
        {
            snprintf(Text, sizeof(Text), "You");
        }
        else
        {
            GetPlayerName(AppState, SlotIndex, Text, sizeof(Text));
        }
        UIText(RenderContext, Font, Columns[0], Y, Text, Color);
        u32 Values[] = {Slot->Level ? Slot->Level : 1, Slot->Kills, Slot->Deaths,
                        Slot->MonsterKills};
        for(u32 Column = 0; Column < ArrayCount(Values); Column++)
        {
            snprintf(Text, sizeof(Text), "%u", Values[Column]);
            UIText(RenderContext, Font, Columns[Column + 1], Y, Text, Color,
                   UIAlign_Right);
        }
    }
}
