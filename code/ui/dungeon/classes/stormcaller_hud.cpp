/* Stormcaller HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Stormcaller sees of its
   own class over the ability bar: the Charge bar.

   The bar fills smoothly toward the Charge the server sends (ClassMeter),
   pale gold rising to a hot yellow. A line marks STORM_SUPERCHARGED, and
   past it the bar crackles: Supercharged. Its last stretch is red, and it
   pulses harder the nearer the Charge gets to the overload. Under Eye of
   the Storm the red end turns blue and calm, since nothing can overload.
   Grounded after an overload, the bar goes grey and says so; a Thunderclap
   pressed short of Charge shakes it and lights the THUNDERCLAP_MIN_CHARGE
   notch. A small dome beside it lights while a Static Field is down.
   Everything reads ClassMeter and ClassFlags, which online play gets from
   the server. */

#define STORMCALLER_HUD_WIDTH 250.f
#define STORMCALLER_HUD_HEIGHT 12.f
// NOTE(zoubir): above the status strip (ui/status_strip.cpp), which sits
// just over the plate, as the Ranger's bar does
#define STORMCALLER_HUD_LIFT 46.f
// NOTE(zoubir): the red end of the bar starts at this much Charge, and
// pulses from STORMCALLER_HUD_PULSE on
#define STORMCALLER_HUD_RED 90.f
#define STORMCALLER_HUD_PULSE 80.f

internal float AbilityBarPlateTop(u32 WindowHeight);

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a lightning bolt
// striking down and to the left
internal void
DrawStormcallerEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    v2 TopLeft = C + S * V2(0.05f, -1.15f);
    v2 TopRight = C + S * V2(0.7f, -1.15f);
    v2 Waist = C + S * V2(0.22f, -0.2f);
    v2 Left = C + S * V2(-0.55f, 0.12f);
    v2 Right = C + S * V2(0.62f, -0.2f);
    v2 Under = C + S * V2(0.f, 0.15f);
    v2 Tip = C + S * V2(-0.4f, 1.2f);
    DrawPartyQuad(RenderContext, TopLeft, TopRight, Waist, Left, Fill);
    DrawPartyQuad(RenderContext, Left, Waist, Right, Under, Fill);
    DrawPartyQuad(RenderContext, Right, Under, Tip, Tip, Fill);
    // NOTE(zoubir): a pale edge down its front
    v2 Glint = 0.08f * S * V2(1.f, 0.f);
    DrawPartyQuad(RenderContext, TopRight - Glint, TopRight, Waist + Glint, Waist, Light);
    DrawPartyQuad(RenderContext, Right - Glint, Right, Tip, Tip, Light);
}

// NOTE(zoubir): every frame while the local player is a Stormcaller
internal void
DrawStormcallerHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                   u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = Slot->Entity;
    if (!Player || IsDeadPlayer(Player))
    {
        return;
    }
    stormcaller_slot *Storm = &Slot->Stormcaller;
    float Clock = GetFxClock(AppState);
    float DeltaTime = Clamp01(Clock - Storm->HudClock);
    Storm->HudClock = Clock;
    float Charge = (float)Slot->ClassMeter;
    // NOTE(zoubir): the fill eases toward the Charge, quickly, and empties
    // at once when a Thunderclap or an overload spends it
    if (Charge < Storm->HudCharge - 15.f)
    {
        Storm->HudCharge = Charge;
    }
    Storm->HudCharge += (Charge - Storm->HudCharge) * Minimum(1.f, 10.f * DeltaTime);
    if (Absolute(Charge - Storm->HudCharge) < 0.5f)
    {
        Storm->HudCharge = Charge;
    }
    u32 Flags = Slot->ClassFlags;
    bool32 Super = (Flags & STORMCALLER_FLAG_SUPERCHARGED) != 0;
    bool32 Eye = (Flags & STORMCALLER_FLAG_EYE) != 0;
    bool32 Grounded = (Flags & STORMCALLER_FLAG_GROUNDED) != 0;
    bool32 Empty = (Flags & STORMCALLER_FLAG_EMPTY) != 0;

    font *Small = AppState->Fonts.Small;
    float Width = STORMCALLER_HUD_WIDTH;
    float Height = STORMCALLER_HUD_HEIGHT;
    float Shake = Empty ? 3.f * Sin(60.f * Clock) : 0.f;
    float X = 0.5f * ((float)WindowWidth - Width) + Shake;
    float Y = AbilityBarPlateTop(WindowHeight) - STORMCALLER_HUD_LIFT - Height;
    u32 Gold = UI_RGBA(250, 220, 80, 255);
    u32 Pale = UI_RGBA(255, 250, 210, 255);
    u32 Dim = UI_RGBA(170, 125, 30, 255);
    u32 Red = UI_RGBA(255, 70, 50, 255);
    u32 Blue = UI_RGBA(140, 200, 255, 255);
    u32 Grey = UI_RGBA(130, 130, 136, 255);
    float Danger = Eye ? 0.f : Clamp01((Charge - STORMCALLER_HUD_PULSE) / (STORM_CHARGE_MOST - STORMCALLER_HUD_PULSE));
    float Pulse = Danger * (0.5f + 0.5f * Sin((8.f + 10.f * Danger) * Clock));

    // NOTE(zoubir): the backing, a dark glass strip a little wider than
    // the bar, with the label and the number
    float Pad = 6.f;
    float LabelWidth = 84.f;
    DrawRoundRect(RenderContext, X - Pad - LabelWidth, Y - Pad, Width + 2.f * Pad + 2.f * LabelWidth,
                  Height + 2.f * Pad, WithAlpha(UI_RGBA(10, 12, 18, 255), 0.72f));
    if (Danger > 0.f)
    {
        float G = 60.f + 40.f * Pulse;
        DrawShaderQuad(RenderContext, Shader_Glow, X + Width - G, Y + 0.5f * Height - 0.5f * G, 2.f * G, G,
                       WithAlpha(Red, 0.25f + 0.45f * Pulse), RenderBlend_Additive);
    }
    char *Label = (char *)"Charge";
    u32 LabelColor = UI_COLOR_TEXT_MUTED;
    if (Grounded)
    {
        Label = (char *)"Grounded";
        LabelColor = Grey;
    }
    else if (Eye)
    {
        Label = (char *)"Eye of the Storm";
        LabelColor = Blue;
    }
    else if (Super)
    {
        Label = (char *)"Supercharged";
        LabelColor = UIMixColor(Gold, Pale, 0.5f + 0.5f * Sin(9.f * Clock));
    }
    float LineY = Y + 0.5f * Height - 0.5f * UILineHeight(Small);
    UIText(RenderContext, Small, X - 8.f, LineY, Label, LabelColor, UIAlign_Right);

    // NOTE(zoubir): the slot, its red end, the fill
    float Mark = X + Width * STORM_SUPERCHARGED / STORM_CHARGE_MOST;
    float RedFrom = X + Width * STORMCALLER_HUD_RED / STORM_CHARGE_MOST;
    DrawFilledRectangle(RenderContext, X - 1.f, Y - 1.f, Width + 2.f, Height + 2.f, UI_RGBA(60, 54, 30, 255), 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Width, Height, UI_RGBA(20, 20, 24, 255), 0.f);
    u32 EndColor = Eye ? Blue : Red;
    DrawFilledRectangle(RenderContext, RedFrom, Y, X + Width - RedFrom, Height,
                        WithAlpha(EndColor, Eye ? 0.35f : 0.3f + 0.5f * Pulse), 0.f);
    float FillWidth = Width * Clamp01(Storm->HudCharge / STORM_CHARGE_MOST);
    if (FillWidth > 0.f)
    {
        u32 From = Grounded ? Grey : Dim;
        u32 To = Grounded ? Grey : (Super ? UI_RGBA(255, 240, 140, 255) : Gold);
        u32 Head = Danger > 0.f ? UIMixColor(Gold, Red, 0.3f + 0.7f * Pulse) : To;
        DrawFilledQuad(RenderContext, V2(X, Y + 1.f), V2(X + FillWidth, Y + 1.f), V2(X + FillWidth, Y + Height - 1.f),
                       V2(X, Y + Height - 1.f), From, Head, Head, From, RenderBlend_Alpha);
        // NOTE(zoubir): a light along the top of the fill
        DrawFilledQuad(RenderContext, V2(X, Y + 1.f), V2(X + FillWidth, Y + 1.f), V2(X + FillWidth, Y + 3.f),
                       V2(X, Y + 3.f), WithAlpha(Pale, 0.1f), WithAlpha(Pale, 0.6f), WithAlpha(Pale, 0.1f),
                       WithAlpha(Pale, 0.05f), RenderBlend_Alpha);
        // NOTE(zoubir): Supercharged, sparks crackle along the fill past
        // the mark
        if (Super && !Grounded)
        {
            u32 Seed = (u32)(Clock / 0.06f);
            for(u32 Spark = 0; Spark < 5; Spark++)
            {
                float At = Mark + (X + FillWidth - Mark) * BurstJitter(Spark, Seed);
                float Up = (BurstJitter(Spark, Seed + 9) - 0.5f) * 10.f;
                DrawFxStroke(RenderContext, V2(At - 3.f, Y + 0.5f * Height), V2(At + 3.f, Y + 0.5f * Height + Up),
                             1.5f, 0.5f, WithAlpha(Pale, 0.9f), WithAlpha(Gold, 0.f));
            }
        }
    }
    // NOTE(zoubir): the red end shows through the fill too, beating
    if (!Eye && FillWidth > RedFrom - X)
    {
        DrawFilledRectangle(RenderContext, RedFrom, Y + 1.f, X + FillWidth - RedFrom, Height - 2.f,
                            WithAlpha(Red, 0.35f + 0.5f * Pulse), 0.f);
    }
    // NOTE(zoubir): the Supercharged line, and the notch Thunderclap needs
    u32 MarkColor = Charge >= STORM_SUPERCHARGED ? Pale : UI_RGBA(200, 180, 110, 255);
    DrawFilledRectangle(RenderContext, Mark - 1.f, Y - 4.f, 2.f, Height + 8.f, MarkColor, 0.f);
    float Need = X + Width * THUNDERCLAP_MIN_CHARGE / STORM_CHARGE_MOST;
    u32 NeedColor = Empty ? Red : (Charge >= THUNDERCLAP_MIN_CHARGE ? WithAlpha(Pale, 0.7f) : WithAlpha(Grey, 0.7f));
    DrawFilledRectangle(RenderContext, Need - 1.f, Y + Height - 3.f, 2.f, 6.f, NeedColor, 0.f);
    if (Empty)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Need - 14.f, Y + Height - 14.f, 28.f, 28.f,
                       WithAlpha(Red, 0.6f), RenderBlend_Additive);
    }

    char Text[32];
    snprintf(Text, sizeof(Text), "%u", (u32)Charge);
    float Right = X + Width + 10.f;
    UIText(RenderContext, Small, Right, LineY, Text,
           Danger > 0.f ? UIMixColor(Gold, Red, Pulse) : (Super ? Gold : UI_COLOR_TEXT), UIAlign_Left);

    // NOTE(zoubir): the Static Field, lit while one is down
    v2 FieldAt = V2(X + Width + LabelWidth - 10.f, Y + 0.5f * Height + 3.f);
    bool32 Field = (Flags & STORMCALLER_FLAG_FIELD) != 0;
    u32 Violet = UI_RGBA(190, 140, 255, 255);
    u32 FieldColor = Field ? Violet : WithAlpha(Grey, 0.5f);
    DrawArcBand(RenderContext, FieldAt, Pi32, 2.f * Pi32, 6.f, 8.f, FieldColor, FieldColor, RenderBlend_Alpha);
    DrawFilledRectangle(RenderContext, FieldAt.X - 9.f, FieldAt.Y, 18.f, 2.f, FieldColor, 0.f);
    if (Field)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, FieldAt.X - 14.f, FieldAt.Y - 16.f, 28.f, 28.f,
                       WithAlpha(Violet, 0.4f), RenderBlend_Additive);
    }
}
