/* Round break: while the break between rounds runs (sim/round_break.cpp)
   the talent panel opens on its own, so every player can spend the level
   the round gave them, and closes again when the next round starts
   (unless the player closed it first). Under it, above the ability bar,
   a slim plate counts down to the next round with a bar that drains as
   the break runs out. Both wait for the final blow (client/final_blow.cpp)
   to finish playing. */

#define ROUND_BREAK_PLATE_WIDTH 380.f

// NOTE(zoubir): before the talent panel draws, every frame
internal void
OpenTalentsForRoundBreak(app_state *AppState)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    if (AppState->RoundBreak > 0.f && FinalBlowLeft(AppState) <= 0.f &&
        !Panel->OpenedForBreak)
    {
        Panel->Open = true;
        Panel->OpenedForBreak = true;
    }
    else if (AppState->RoundBreak <= 0.f && Panel->OpenedForBreak)
    {
        Panel->Open = false;
        Panel->OpenedForBreak = false;
    }
}

// NOTE(zoubir): after the talent panel, just above the ability bar, so it
// covers neither the bar nor the talents
internal void
DrawRoundBreak(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (AppState->RoundBreak <= 0.f || FinalBlowLeft(AppState) > 0.f || !Slot->Active)
    {
        return;
    }
    font *Body = AppState->Fonts.Body;
    float Width = ROUND_BREAK_PLATE_WIDTH;
    float Height = UILineHeight(Body) + 2.f * UI_GAP_SMALL + 4.f;
    float X = 0.5f * ((float)WindowWidth - Width);
    // NOTE(zoubir): sitting on the ability bar's plate; the talent panel
    // ends a little above it, by less on short windows
    float Y = AbilityBarPlateTop(WindowHeight) - 4.f - Height;
    DrawUIPanel(RenderContext, X, Y, Width, Height, UI_COLOR_ACCENT);

    char Text[96];
    snprintf(Text, sizeof(Text), "Next round in %.0f  -  pick your talents",
             Maximum(1.f, RoundBreakLeft(AppState) + 0.5f));
    UIText(RenderContext, Body, X + 0.5f * Width, Y + UI_GAP_SMALL, Text,
           UI_COLOR_TEXT, UIAlign_Center);

    float Share = Clamp01(RoundBreakLeft(AppState) / ROUND_BREAK_SECONDS);
    float BarX = X + UI_GAP_LARGE;
    float BarWidth = Width - 2.f * UI_GAP_LARGE;
    float BarY = Y + Height - 6.f;
    DrawRoundRect(RenderContext, BarX, BarY, BarWidth, 3.f, UI_COLOR_TRACK);
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, BarX, BarY, Maximum(3.f, Share * BarWidth), 3.f,
                      UI_COLOR_ACCENT);
    }
}
