/* Match screens: the scoreboard shown while Tab is held (every player's
   level, kills, deaths and monster kills, best first). The respawn
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

internal void
DrawScoreboard(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    font *Font = AppState->Fonts.Body;
    u32 Order[MAX_PLAYERS];
    u32 Count = RankPlayers(AppState, Order);

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
        u32 SlotIndex = Order[Rank];
        player_slot *Slot = &AppState->Players[SlotIndex];
        bool32 IsLocal = (SlotIndex == AppState->LocalPlayerIndex);
        u32 Color = IsLocal ? UI_COLOR_ACCENT : UI_COLOR_TEXT;
        Y += RowHeight;

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
