/* Connection indicator: one line under the HUD stats with a coloured dot.
   Joined, it shows the round trip and any snapshot loss, green when the
   connection is good, amber when it is getting slow, red when it is bad
   or the server has gone quiet. Otherwise it shows what the session is
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
        snprintf(Text, sizeof(Text), "Online, nothing from the server for %.1f s",
                 Silence);
    }
    else if (Quality)
    {
        int Ms = (int)(Quality->RoundTripMs + 0.5f);
        int LossPercent = (int)(100.f * Quality->Loss + 0.5f);
        if (LossPercent >= 2)
        {
            snprintf(Text, sizeof(Text), "Online  %d ms  %d%% lost", Ms,
                     LossPercent);
        }
        else
        {
            snprintf(Text, sizeof(Text), "Online  %d ms", Ms);
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

    float LineHeight = UILineHeight(Font);
    float Dot = 8.f;
    DrawFilledRectangle(RenderContext, X, TopY + 0.5f * (LineHeight - Dot),
                        Dot, Dot, DotColor, 0.f);
    UIText(RenderContext, Font, X + Dot + UI_GAP_SMALL, TopY, Text,
           UI_COLOR_TEXT);
}
