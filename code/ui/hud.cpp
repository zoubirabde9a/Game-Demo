/* Heads-up display: always-on screen-space widgets drawn on top of the
   world each frame (player health for now). */

#define RGBA8_HUD_BACKGROUND (0xC0202020)
#define RGBA8_HUD_HEALTH (0xFF3040D0)

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
    float Width = 200.f;
    float Height = 14.f;
    float Fraction = Minimum(1.f, Maximum(0.f, Player->Hp / Player->MaxHp));

    DrawFilledRectangle(RenderContext, X, Y, Width, Height,
                        RGBA8_HUD_BACKGROUND, 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Fraction * Width, Height,
                        RGBA8_HUD_HEALTH, 0.f);
    DrawRectangle(RenderContext, X, Y, Width, Height,
                  RGBA8_WHITE, 0.f);
}
