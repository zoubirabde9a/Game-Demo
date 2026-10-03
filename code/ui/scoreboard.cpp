/* Match screens: the scoreboard shown while Tab is held (every player's
   kills, deaths and monster kills, best first) and the respawn countdown
   shown while the local player is dead. */


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
    font *Font = AppState->DefaultFont;
    if (!Font)
    {
        return;
    }

    u32 Order[MAX_PLAYERS];
    u32 Count = RankPlayers(AppState, Order);

    float RowHeight = 30.f;
    float Width = 460.f;
    float Height = RowHeight * (Count + 1) + 24.f;
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = 0.5f * ((float)WindowHeight - Height);
    float Columns[] = {Left + 20.f, Left + 230.f, Left + 310.f, Left + 390.f};

    DrawFilledRectangle(RenderContext, Left, Top, Width, Height,
                        UI_COLOR_PANEL, 0.f);
    DrawRectangle(RenderContext, Left, Top, Width, Height, UI_COLOR_BORDER, 0.f);

    char *Headers[] = {"Player", "Kills", "Deaths", "Mobs"};
    float Y = Top + 12.f;
    for(u32 Column = 0; Column < ArrayCount(Headers); Column++)
    {
        DrawScreenText(RenderContext, Font, Columns[Column], Y,
                       Headers[Column], UI_COLOR_TEXT_MUTED);
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
        DrawScreenText(RenderContext, Font, Columns[0], Y, Text, Color);
        u32 Values[] = {Slot->Kills, Slot->Deaths, Slot->MonsterKills};
        for(u32 Column = 0; Column < ArrayCount(Values); Column++)
        {
            snprintf(Text, sizeof(Text), "%u", Values[Column]);
            DrawScreenText(RenderContext, Font, Columns[Column + 1], Y,
                           Text, Color);
        }
    }
}

internal void
DrawRespawnCountdown(render_context *RenderContext, app_state *AppState,
                     u32 WindowWidth, u32 WindowHeight)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    font *Font = AppState->DefaultFont;
    if (!Font || !Slot->Active || !IsDeadPlayer(Slot->Entity))
    {
        return;
    }

    char Text[48];
    snprintf(Text, sizeof(Text), "Respawning in %.0f",
             Maximum(1.f, Slot->RespawnTimer + 0.5f));
    float TextWidth = GetTextWidth(Font, Text);
    DrawScreenText(RenderContext, Font,
                   0.5f * ((float)WindowWidth - TextWidth),
                   0.4f * (float)WindowHeight, Text, UI_COLOR_TEXT);
}
