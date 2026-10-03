/* Heads-up display: always-on screen-space widgets drawn on top of the
   world each frame: player health and ability cooldowns. */

#define RGBA8_HUD_BACKGROUND (0xC0202020)
#define RGBA8_HUD_HEALTH (0xFF3040D0)
#define RGBA8_HUD_ABILITY_READY (0xFF30C8F0)
#define RGBA8_HUD_ABILITY_CHARGING (0xFF808080)

// NOTE(zoubir): Fraction is 0..1, filled from the left
internal void
DrawHudBar(render_context *RenderContext, float X, float Y,
           float Width, float Height, float Fraction, u32 FillColor)
{
    Fraction = Minimum(1.f, Maximum(0.f, Fraction));
    DrawFilledRectangle(RenderContext, X, Y, Width, Height,
                        RGBA8_HUD_BACKGROUND, 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Fraction * Width, Height,
                        FillColor, 0.f);
    DrawRectangle(RenderContext, X, Y, Width, Height,
                  RGBA8_WHITE, 0.f);
}

internal void
DrawHud(render_context *RenderContext, app_state *AppState)
{
    world_entity *Player = AppState->Player;
    if (!Player || Player->MaxHp <= 0.f)
    {
        return;
    }

    float X = 20.f;
    float Y = 20.f;
    DrawHudBar(RenderContext, X, Y, 200.f, 14.f,
               Player->Hp / Player->MaxHp, RGBA8_HUD_HEALTH);

    // Dash (Alt): fills back up while recharging
    float DashCharge = 1.f - Player->DashCooldown / PLAYER_DASH_COOLDOWN;
    DrawHudBar(RenderContext, X, Y + 20.f, 60.f, 6.f, DashCharge,
               DashCharge >= 1.f ?
               RGBA8_HUD_ABILITY_READY : RGBA8_HUD_ABILITY_CHARGING);

    font *Font = AppState->DefaultFont;
    if (Font)
    {
        char Text[32];
        snprintf(Text, sizeof(Text), "Kills: %u", AppState->KillCount);
        v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
        // NOTE(zoubir): Y is the baseline, so drop it by the font ascent
        RenderText(RenderContext, X, Y + 32.f + Font->UpperLimit, Font,
                   RenderContext->TextureProgram, Text, RGBA8_WHITE,
                   1.f, 1.f, NoClip, 0.f);
    }
}
