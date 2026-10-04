/* Connection indicator: one line under the HUD stats with signal bars.
   Joined, it names the server and shows the round trip and any snapshot
   loss: three green bars when the connection is good, two amber when it
   is getting slow, one red when it is bad or the server has gone quiet. Otherwise it shows what the session is
   doing (connecting, reconnecting in N s, why it is offline). Nothing
   when playing offline by choice. */

#define CONNECTION_GOOD_MS 100.f
#define CONNECTION_SLOW_MS 200.f
#define CONNECTION_GOOD_LOSS 0.05f
#define CONNECTION_SLOW_LOSS 0.15f
// NOTE(zoubir): snapshots come 20 a second, so this much silence is
// several lost in a row
#define CONNECTION_QUIET_SECONDS 0.5f

internal void
DrawConnectionIndicator(render_context *RenderContext, app_state *AppState,
                        float X, float TopY)
{
    online_session *Online = AppState->Online;
    font *Font = AppState->Fonts.Body;
    char Text[128];
    u32 DotColor = UI_COLOR_HEALTH;

    online_quality *Quality = GetOnlineQuality(Online);
    float Silence = GetOnlineSilence(Online);
    if (IsOnline(Online) && Silence >= CONNECTION_QUIET_SECONDS)
    {
        snprintf(Text, sizeof(Text), "%s: nothing heard for %.1f s",
                 GetOnlineServerName(Online), Silence);
    }
    else if (Quality)
    {
        int Ms = (int)(Quality->RoundTripMs + 0.5f);
        int LossPercent = (int)(100.f * Quality->Loss + 0.5f);
        if (LossPercent >= 2)
        {
            snprintf(Text, sizeof(Text), "%s   %d ms   %d%% lost",
                     GetOnlineServerName(Online), Ms, LossPercent);
        }
        else
        {
            snprintf(Text, sizeof(Text), "%s   %d ms", GetOnlineServerName(Online), Ms);
        }
        if (Quality->RoundTripMs < CONNECTION_GOOD_MS &&
            Quality->Loss < CONNECTION_GOOD_LOSS)
        {
            DotColor = UI_COLOR_GOOD;
        }
        else if (Quality->RoundTripMs < CONNECTION_SLOW_MS &&
                 Quality->Loss < CONNECTION_SLOW_LOSS)
        {
            DotColor = UI_COLOR_ACCENT;
        }
    }
    else
    {
        GetOnlineStatusText(Online, Text, sizeof(Text));
        if (!Text[0])
        {
            return;
        }
        online_phase Phase = GetOnlinePhase(Online);
        DotColor = (Phase == OnlinePhase_Joining || Phase == OnlinePhase_Joined) ?
            UI_COLOR_ACCENT : UI_COLOR_HEALTH;
    }

    // NOTE(zoubir): signal bars: three for good, two for slow, one for bad
    u32 Bars = (DotColor == UI_COLOR_GOOD) ? 3 : ((DotColor == UI_COLOR_ACCENT) ? 2 : 1);
    float LineHeight = UILineHeight(Font);
    float BarWidth = 4.f;
    float BarsBottom = TopY + 0.5f * LineHeight + 7.f;
    for(u32 Index = 0; Index < 3; Index++)
    {
        float BarHeight = 5.f + 4.f * (float)Index;
        u32 Color = Index < Bars ? DotColor : UI_RGBA(0, 0, 0, 120);
        DrawFilledRectangle(RenderContext, X + (float)Index * (BarWidth + 2.f),
                            BarsBottom - BarHeight, BarWidth, BarHeight, Color, 0.f);
    }
    UIText(RenderContext, Font, X + 3.f * (BarWidth + 2.f) + UI_GAP_SMALL, TopY, Text,
           UI_COLOR_TEXT);
}
