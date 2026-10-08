/* Health bar: the local player's health above the ability bar's slots
   (ability_bar.cpp). A recessed track, a red fill that glides to the real
   health rather than jumping, a pale trail that drains after a hit so the
   size of the hit can be read, a tick at each quarter, and the numbers.
   Under ABILITY_HEALTH_LOW the fill pulses, faster as health runs out.
   Every part is a rounded shape (build/shaders/fx/round_rect.frag). */

#define ABILITY_HEALTH_HEIGHT 14.f
// NOTE(zoubir): the pale trail catches up at this share of the gap per
// second
#define ABILITY_HEALTH_TRAIL_RATE 3.f
// NOTE(zoubir): the fill glides to the real health this fast (share of the
// gap per second)
#define ABILITY_HEALTH_GLIDE_RATE 18.f
#define ABILITY_HEALTH_LOW 0.3f

struct health_bar
{
    float Trail; // share of health the trail shows
    float Shown; // share the fill shows, gliding to the real one
};

internal void
DrawHealthBar(render_context *RenderContext, app_state *AppState,
              health_bar *Bar, world_entity *Player,
              float X, float Y, float Width, float DeltaTime)
{
    float Share = Player->MaxHp > 0.f ?
        Maximum(0.f, Minimum(1.f, Player->Hp / Player->MaxHp)) : 0.f;
    if (Share >= Bar->Trail)
    {
        Bar->Trail = Share;
    }
    else
    {
        Bar->Trail = Maximum(Share, Bar->Trail -
                        ABILITY_HEALTH_TRAIL_RATE * DeltaTime *
                        Maximum(0.08f, Bar->Trail - Share));
    }
    Bar->Shown += (Share - Bar->Shown) *
        Minimum(1.f, ABILITY_HEALTH_GLIDE_RATE * DeltaTime);
    float Shown = Bar->Shown;
    float Height = ABILITY_HEALTH_HEIGHT;
    // NOTE(zoubir): a recessed track, the pale trail of the last hit, then
    // the fill, each a rounded part (round_rect.frag)
    DrawRoundRect(RenderContext, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f,
                  UI_RGBA(4, 5, 8, 220));
    DrawRoundRect(RenderContext, X, Y, Width, Height, UI_RGBA(46, 20, 22, 240));
    if (Bar->Trail > Shown + 0.002f)
    {
        DrawRoundRect(RenderContext, X, Y, Bar->Trail * Width, Height,
                      UI_RGBA(255, 214, 190, 210));
    }
    u32 Fill = UI_COLOR_HEALTH;
    if (Share < ABILITY_HEALTH_LOW && Share > 0.f)
    {
        // NOTE(zoubir): faster as health runs out
        float Rate = 4.f + 8.f * (1.f - Share / ABILITY_HEALTH_LOW);
        float Beat = 0.5f + 0.5f * sinf(Rate * RenderContext->Time);
        Fill = UI_RGBA(208 + (int)(47.f * Beat), 64 + (int)(40.f * Beat),
                       48 + (int)(30.f * Beat), 255);
    }
    if (Shown > 0.f)
    {
        DrawRoundRect(RenderContext, X, Y, Maximum(Shown * Width, Height), Height, Fill);
    }
    // NOTE(zoubir): a tick at each quarter, so a share can be read at a
    // glance
    for(u32 Quarter = 1; Quarter < 4; Quarter++)
    {
        DrawFilledRectangle(RenderContext, X + 0.25f * (float)Quarter * Width - 0.5f,
                            Y + 3.f, 1.f, Height - 6.f, UI_RGBA(0, 0, 0, 70), 0.f);
    }
    // NOTE(zoubir): with one point of health (the duel rules) the bar is
    // full or empty and a "1 / 1" says nothing more
    if (Player->MaxHp <= 1.f)
    {
        return;
    }
    char Text[32];
    // NOTE(zoubir): an overkill leaves Hp below zero; the bar says 0
    snprintf(Text, sizeof(Text), "%d / %d", (int)(Maximum(0.f, Player->Hp) + 0.5f),
             (int)(Player->MaxHp + 0.5f));
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Small, X + 0.5f * Width,
           Y + 0.5f * (Height - UILineHeight(Small)) - 1.f, Text, UI_COLOR_TEXT,
           UIAlign_Center);
}

