/* Ranger HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Ranger sees of its own
   class over the ability bar: Focus as five arrowhead segments that fill
   smoothly, each lighting as it completes, the whole bar breathing light
   when full ("Deadeye" when the next Piercing Shot will crit). While
   Piercing Shot draws, the bar says what the shot will spend. Beside it,
   two small marks say whether a Hunter's Mark is up and a snare is down.
   Everything reads ClassMeter and ClassFlags, which online play gets from
   the server. */

#define RANGER_HUD_WIDTH 250.f
#define RANGER_HUD_HEIGHT 12.f
// NOTE(zoubir): above the status strip (ui/status_strip.cpp), which sits
// just over the plate
#define RANGER_HUD_LIFT 46.f
#define RANGER_HUD_SEGMENTS 5

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a bow drawn round an
// arrow pointing up and right
internal void
DrawRangerEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    // NOTE(zoubir): the bow, an arc of short quads on the lower left
    v2 Last = {};
    for(u32 Step = 0; Step <= 6; Step++)
    {
        float A = 0.75f * Pi32 + 1.1f * Pi32 * ((float)Step / 6.f - 0.5f);
        v2 P = C + 1.05f * S * V2(Cos(A), Sin(A)) + V2(0.25f * S, -0.25f * S);
        if (Step)
        {
            v2 Along = P - Last;
            v2 Across = 0.16f * S * NormalizeOr(V2(-Along.Y, Along.X), V2(0.f, 1.f));
            DrawPartyQuad(RenderContext, Last - Across, P - Across, P + Across, Last + Across, Fill);
        }
        Last = P;
    }
    // NOTE(zoubir): the arrow across it, head up and to the right
    v2 Dir = V2(0.7071f, -0.7071f);
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Tail = C - 1.f * S * Dir;
    v2 Head = C + 1.05f * S * Dir;
    float Thin = 0.1f * S;
    DrawPartyQuad(RenderContext, Tail - Thin * Side, Head - Thin * Side, Head + Thin * Side,
                  Tail + Thin * Side, Light);
    v2 Base = Head - 0.45f * S * Dir;
    DrawPartyQuad(RenderContext, Head + 0.15f * S * Dir, Base + 0.32f * S * Side, Base,
                  Base - 0.32f * S * Side, Light);
    DrawPartyQuad(RenderContext, Tail + 0.35f * S * Dir, Tail + 0.1f * S * Dir + 0.28f * S * Side,
                  Tail - 0.15f * S * Dir + 0.28f * S * Side, Tail, Fill);
    DrawPartyQuad(RenderContext, Tail + 0.35f * S * Dir, Tail, Tail - 0.15f * S * Dir - 0.28f * S * Side,
                  Tail + 0.1f * S * Dir - 0.28f * S * Side, Fill);
}

// NOTE(zoubir): an arrowhead segment of the bar at X, Y, W wide: a body
// that ends in a point, notched at its back
internal void
DrawRangerSegment(render_context *RenderContext, float X, float Y, float W, float H, u32 Color)
{
    float Tip = 0.45f * H;
    v2 TopLeft = V2(X, Y);
    v2 TopRight = V2(X + W - Tip, Y);
    v2 Point = V2(X + W, Y + 0.5f * H);
    v2 BottomRight = V2(X + W - Tip, Y + H);
    v2 BottomLeft = V2(X, Y + H);
    v2 Notch = V2(X + Tip, Y + 0.5f * H);
    DrawFilledQuad(RenderContext, TopLeft, TopRight, Point, Notch, Color, Color, Color, Color,
                   RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Notch, Point, BottomRight, BottomLeft, Color, Color, Color, Color,
                   RenderBlend_Alpha);
}

// NOTE(zoubir): a small reticle at C, R across, for the mark indicator
internal void
DrawRangerHudReticle(render_context *RenderContext, v2 C, float R, u32 Color)
{
    for(u32 Quarter = 0; Quarter < 4; Quarter++)
    {
        float From = 0.5f * Pi32 * (float)Quarter + 0.25f;
        DrawArcBand(RenderContext, C, From, From + 0.5f * Pi32 - 0.5f, R - 1.6f, R, Color, Color,
                    RenderBlend_Alpha);
        float A = 0.5f * Pi32 * (float)Quarter;
        v2 Dir = V2(Cos(A), Sin(A));
        DrawFxStroke(RenderContext, C + (R + 3.f) * Dir, C + (R - 4.f) * Dir, 1.8f, 1.8f, Color, Color,
                     RenderBlend_Alpha);
    }
}

// NOTE(zoubir): every frame while the local player is a Ranger
internal void
DrawRangerHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
           u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = Slot->Entity;
    if (!Player || IsDeadPlayer(Player))
    {
        return;
    }
    ranger_slot *Ranger = &Slot->Ranger;
    float Clock = GetFxClock(AppState);
    float DeltaTime = Clamp01(Clock - Ranger->HudClock);
    Ranger->HudClock = Clock;
    float Focus = (float)Slot->ClassMeter;
    bool32 WasFull = Ranger->HudFocus >= RANGER_FOCUS_MOST - 0.5f;
    // NOTE(zoubir): the fill eases toward Focus, quickly, and empties at
    // once when a Piercing Shot spends it
    if (Focus < Ranger->HudFocus - 30.f)
    {
        Ranger->HudFocus = Focus;
    }
    Ranger->HudFocus += (Focus - Ranger->HudFocus) * Minimum(1.f, 10.f * DeltaTime);
    bool32 Full = Focus >= RANGER_FOCUS_MOST;
    if (Full && !WasFull)
    {
        Ranger->HudFullAt = Clock;
    }

    font *Small = AppState->Fonts.Small;
    float Width = RANGER_HUD_WIDTH;
    float Height = RANGER_HUD_HEIGHT;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = AbilityBarPlateTop(WindowHeight) - RANGER_HUD_LIFT - Height;
    bool32 Deadeye = (Slot->ClassFlags & RANGER_FLAG_DEADEYE) != 0;
    u32 Teal = UI_RGBA(90, 210, 170, 255);
    u32 Pale = UI_RGBA(200, 255, 235, 255);
    u32 Gold = UI_RGBA(255, 220, 140, 255);
    u32 Lit = Deadeye ? Gold : Teal;

    // NOTE(zoubir): the backing, a dark glass strip a little wider than
    // the bar, with the label and the number
    float Pad = 6.f;
    float LabelWidth = 54.f;
    DrawRoundRect(RenderContext, X - Pad - LabelWidth, Y - Pad, Width + 2.f * Pad + 2.f * LabelWidth,
                  Height + 2.f * Pad, WithAlpha(UI_RGBA(10, 14, 18, 255), 0.72f));
    float Breath = Full ? 0.5f + 0.5f * Sin(5.f * Clock) : 0.f;
    float Burst = Clamp01(1.f - (Clock - Ranger->HudFullAt) / 0.6f);
    if (Full)
    {
        float G = 0.5f * Width + 40.f + 30.f * Burst;
        DrawShaderQuad(RenderContext, Shader_Glow, X + 0.5f * Width - G, Y + 0.5f * Height - 26.f - 10.f * Burst,
                       2.f * G, 52.f + 20.f * Burst, WithAlpha(Lit, 0.22f + 0.18f * Breath + 0.5f * Burst),
                       RenderBlend_Additive);
    }
    char *Label = Deadeye ? (char *)"Deadeye" : (char *)"Focus";
    float LineY = Y + 0.5f * Height - 0.5f * UILineHeight(Small);
    UIText(RenderContext, Small, X - 8.f, LineY, Label, Full ? Lit : UI_COLOR_TEXT_MUTED, UIAlign_Right);

    // NOTE(zoubir): the segments: a dark slot, then the fill, the newest
    // edge brightest
    float Gap = 4.f;
    float SegmentWidth = (Width - Gap * (float)(RANGER_HUD_SEGMENTS - 1)) / (float)RANGER_HUD_SEGMENTS;
    float Share = RANGER_FOCUS_MOST / (float)RANGER_HUD_SEGMENTS;
    for(u32 Segment = 0; Segment < RANGER_HUD_SEGMENTS; Segment++)
    {
        float SX = X + (float)Segment * (SegmentWidth + Gap);
        DrawRangerSegment(RenderContext, SX, Y, SegmentWidth, Height, UI_RGBA(36, 62, 58, 255));
        DrawRangerSegment(RenderContext, SX + 1.f, Y + 1.f, SegmentWidth - 2.f, Height - 2.f, UI_RGBA(18, 26, 28, 255));
        float Fill = Clamp01((Ranger->HudFocus - Share * (float)Segment) / Share);
        if (Fill <= 0.f)
        {
            continue;
        }
        u32 Color = Fill >= 1.f ? UIMixColor(Lit, Pale, 0.25f * Breath) : UIMixColor(UI_RGBA(40, 110, 95, 255), Teal, Fill);
        DrawRangerSegment(RenderContext, SX, Y + 1.f, Maximum(0.45f * Height + 1.f, Fill * SegmentWidth),
                          Height - 2.f, Color);
        // NOTE(zoubir): a light along the top edge of a filled segment
        if (Fill >= 1.f)
        {
            DrawFilledQuad(RenderContext, V2(SX + 0.45f * Height, Y + 1.f), V2(SX + SegmentWidth - 0.45f * Height, Y + 1.f),
                           V2(SX + SegmentWidth - 0.45f * Height, Y + 3.f), V2(SX + 0.45f * Height, Y + 3.f),
                           WithAlpha(Pale, 0.6f), WithAlpha(Pale, 0.6f), WithAlpha(Pale, 0.1f), WithAlpha(Pale, 0.1f),
                           RenderBlend_Alpha);
        }
    }

    // NOTE(zoubir): while Piercing Shot draws, what it will spend; else
    // the Focus number
    char Text[32];
    if (Player->CastSpell == PlayerSpell_RangerA)
    {
        if (Deadeye)
        {
            snprintf(Text, sizeof(Text), "crit");
        }
        else
        {
            snprintf(Text, sizeof(Text), "+%u%%", (u32)(100.f * PIERCE_PER_FOCUS * Focus / PIERCE_DAMAGE + 0.5f));
        }
    }
    else
    {
        snprintf(Text, sizeof(Text), "%u", (u32)Focus);
    }
    float Right = X + Width + 10.f;
    UIText(RenderContext, Small, Right, LineY, Text, Full ? Lit : UI_COLOR_TEXT, UIAlign_Left);

    // NOTE(zoubir): the mark and the snare, lit while they are up
    float IconX = X + Width + LabelWidth + Pad + 16.f;
    v2 MarkAt = V2(IconX, Y + 0.5f * Height);
    bool32 Marked = (Slot->ClassFlags & RANGER_FLAG_MARK) != 0;
    DrawRangerHudReticle(RenderContext, MarkAt, 7.f, Marked ? Teal : WithAlpha(UI_RGBA(120, 130, 135, 255), 0.5f));
    if (Marked)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, MarkAt.X - 14.f, MarkAt.Y - 14.f, 28.f, 28.f,
                       WithAlpha(Teal, 0.4f), RenderBlend_Additive);
    }
    if (RoleSpellLearned(Slot, 2))
    {
        v2 TrapAt = MarkAt + V2(24.f, 0.f);
        bool32 Down = (Slot->ClassFlags & RANGER_FLAG_TRAP) != 0;
        u32 Steel = Down ? UI_RGBA(210, 215, 220, 255) : WithAlpha(UI_RGBA(120, 130, 135, 255), 0.5f);
        DrawArcBand(RenderContext, TrapAt, 0.f, 2.f * Pi32, 5.f, 7.f, Steel, Steel, RenderBlend_Alpha);
        for(u32 Tooth = 0; Tooth < 6; Tooth++)
        {
            float A = 2.f * Pi32 * (float)Tooth / 6.f;
            DrawFxStroke(RenderContext, TrapAt + 5.f * V2(Cos(A), Sin(A)), TrapAt + 2.f * V2(Cos(A), Sin(A)),
                         2.f, 0.5f, Steel, Steel, RenderBlend_Alpha);
        }
        if (Down)
        {
            DrawFxDot(RenderContext, TrapAt, 3.f, Teal);
        }
    }
}
