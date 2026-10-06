/* Status strip: the local player's running status effects
   (sim/status_effects.cpp), one chip each, centred just above the ability
   bar's plate: a coloured dot, the effect's name and the seconds left.
   Boons (healing, haste) get a green rim, everything else a red one, and
   a chip blinks through its last second. Online the clocks are the
   server's exact ones (net_snapshot.OwnStatus), so the seconds are true
   there too. */

#define STATUS_STRIP_HEIGHT 24.f
#define STATUS_STRIP_GAP 6.f
#define STATUS_STRIP_LIFT 8.f
#define STATUS_STRIP_DOT 8.f
#define STATUS_STRIP_PAD 10.f

// NOTE(zoubir): heals or speeds you up, and nothing else
inline bool32
IsStatusBoon(status_def *Def)
{
    bool32 Result = (Def->DamagePerSecond < 0.f || Def->MoveScale > 1.f) &&
        Def->Flags == 0;
    return Result;
}

internal void
DrawStatusStrip(render_context *RenderContext, app_state *AppState,
                u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = GetLocalPlayer(AppState);
    font *Small = AppState->Fonts.Small;
    if (!Player || !Small || IsDeadPlayer(Player))
    {
        return;
    }

    char Labels[StatusEffect_Count][32];
    float Widths[StatusEffect_Count] = {};
    float Total = 0.f;
    u32 Count = 0;
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        float Left = Player->StatusTimers[Effect];
        if (Left > 0.f)
        {
            snprintf(Labels[Effect], sizeof(Labels[Effect]), "%s %.1f",
                     StatusTable[Effect].Name, Left);
            Widths[Effect] = UITextWidth(Small, Labels[Effect]) +
                STATUS_STRIP_DOT + 3.f * STATUS_STRIP_PAD;
            Total += Widths[Effect];
            Count++;
        }
    }
    if (!Count)
    {
        return;
    }
    Total += (float)(Count - 1) * STATUS_STRIP_GAP;

    float X = 0.5f * ((float)WindowWidth - Total);
    float Y = AbilityBarPlateTop(WindowHeight) - STATUS_STRIP_LIFT - STATUS_STRIP_HEIGHT;
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        if (Widths[Effect] <= 0.f)
        {
            continue;
        }
        status_def *Def = &StatusTable[Effect];
        float Left = Player->StatusTimers[Effect];
        bool32 Blink = Left < 1.f && ((u32)(Left * 8.f) % 2) == 0;
        u32 Rim = IsStatusBoon(Def) ? UI_RGBA(110, 220, 130, 255) : UI_RGBA(230, 110, 100, 255);
        DrawUIPanel(RenderContext, X, Y, Widths[Effect], STATUS_STRIP_HEIGHT, Rim);
        // NOTE(zoubir): stunned and falling have no pip colour of their own
        u32 Dot = Def->Color ? Def->Color : UI_RGBA(250, 220, 90, 255);
        float DotY = Y + 0.5f * (STATUS_STRIP_HEIGHT - STATUS_STRIP_DOT);
        DrawFilledRectangle(RenderContext, X + STATUS_STRIP_PAD, DotY,
                            STATUS_STRIP_DOT, STATUS_STRIP_DOT, Dot, 0.f);
        u32 TextColor = Blink ? UI_RGBA(255, 255, 255, 120) : UI_RGBA(240, 240, 245, 255);
        UIText(RenderContext, Small, X + 2.f * STATUS_STRIP_PAD + STATUS_STRIP_DOT,
               Y + 0.5f * (STATUS_STRIP_HEIGHT - UILineHeight(Small)),
               Labels[Effect], TextColor);
        X += Widths[Effect] + STATUS_STRIP_GAP;
    }
}
