/* Final blow view: the screen while the duel's final blow plays
   (client/final_blow.cpp). Black bars slide in at the top and bottom like
   a film, the edges of the world darken, a red flash marks the blow, and
   a title lands in the lower bar: "Defeated" for the player who fell,
   "Victory" for the one left standing, "Round over" for anyone watching,
   with who won the round under it. The bars slide away as the slow motion
   eases out, and the round break's own screens take over. */

// NOTE(zoubir): each bar's height, as a share of the window's
#define FINAL_BLOW_BAR_SHARE 0.12f
// NOTE(zoubir): the red flash fades over this many seconds
#define FINAL_BLOW_FLASH_SECONDS 0.5f
#define FINAL_BLOW_TITLE_DELAY 0.35f

// NOTE(zoubir): the one player standing, or MAX_PLAYERS for none
internal u32
FinalBlowWinner(app_state *AppState)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity))
        {
            return SlotIndex;
        }
    }
    return MAX_PLAYERS;
}

internal void
DrawFinalBlow(render_context *RenderContext, app_state *AppState,
              u32 WindowWidth, u32 WindowHeight)
{
    kill_feed_entry *Kill = FinalBlowKill(AppState);
    if (!Kill)
    {
        return;
    }
    float Width = (float)WindowWidth;
    float Height = (float)WindowHeight;
    float Elapsed = FinalBlowElapsed(AppState);
    float Shown = FinalBlowPush(AppState);

    // NOTE(zoubir): the world's edges darken under everything else
    DrawShaderQuad(RenderContext, Shader_ScreenEdge, 0.f, 0.f, Width, Height,
                   WithAlpha(UI_RGBA(6, 2, 4, 255), 0.7f * Shown));
    float Flash = Maximum(0.f, 1.f - Elapsed / FINAL_BLOW_FLASH_SECONDS);
    if (Flash > 0.f)
    {
        DrawFilledRectangle(RenderContext, 0.f, 0.f, Width, Height,
                            WithAlpha(UI_COLOR_HEALTH, 0.35f * Flash * Flash), 0.f);
    }

    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Body = AppState->Fonts.Body;
    float Bar = Maximum(FINAL_BLOW_BAR_SHARE * Height,
                        UILineHeight(Title) + UILineHeight(Body) + 3.f * UI_GAP_SMALL);
    float BarShown = Bar * Shown;
    u32 BarColor = UI_RGBA(0, 0, 0, 255);
    DrawFilledRectangle(RenderContext, 0.f, 0.f, Width, BarShown, BarColor, 0.f);
    DrawFilledRectangle(RenderContext, 0.f, Height - BarShown, Width, BarShown,
                        BarColor, 0.f);

    float Alpha = Minimum(SmoothStep01((Elapsed - FINAL_BLOW_TITLE_DELAY) / 0.3f), Shown);
    if (Alpha <= 0.f)
    {
        return;
    }
    u32 Local = AppState->LocalPlayerIndex;
    u32 Winner = FinalBlowWinner(AppState);
    char *Heading = (char *)"Round over";
    u32 HeadingColor = UI_COLOR_TEXT;
    if (Kill->Victim == Local)
    {
        Heading = (char *)"Defeated";
        HeadingColor = UI_COLOR_HEALTH;
    }
    else if (Winner == Local)
    {
        Heading = (char *)"Victory";
        HeadingColor = UI_COLOR_ACCENT;
    }
    char Line[96];
    if (Winner < MAX_PLAYERS)
    {
        char Name[48];
        GetPlayerName(AppState, Winner, Name, sizeof(Name));
        snprintf(Line, sizeof(Line), "%s wins the round", Name);
    }
    else
    {
        snprintf(Line, sizeof(Line), "Nobody is left standing");
    }
    // NOTE(zoubir): the title settles into the lower bar from a little
    // above it
    float Drop = 12.f * (1.f - Alpha);
    float Top = Height - BarShown + 0.5f * (BarShown - UILineHeight(Title) -
                                            UILineHeight(Body) - UI_GAP_SMALL);
    float CentreX = 0.5f * Width;
    UIText(RenderContext, Title, CentreX, Top - Drop, Heading,
           WithAlpha(HeadingColor, Alpha), UIAlign_Center);
    UIText(RenderContext, Body, CentreX, Top + UILineHeight(Title) + UI_GAP_SMALL,
           Line, WithAlpha(UI_COLOR_TEXT_MUTED, Alpha), UIAlign_Center);
}
