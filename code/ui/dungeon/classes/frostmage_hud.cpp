/* Frost Mage HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, a snowflake, and what the local Frost
   Mage sees of its own class over the ability bar: its Icicles as five
   shards of ice that fill one at a time, the whole bar shining when all
   five are up ("Freeze": the next Glacial Spike freezes its foe). While
   Glacial Spike casts, the bar says what the spike will deal. Beside it,
   two small marks say whether Ice Barrier holds and whether the next
   Frostbolt shatters (Fingers of Frost). Everything reads ClassMeter and
   ClassFlags, which online play gets from the server. */

#define FROSTMAGE_HUD_WIDTH 250.f
#define FROSTMAGE_HUD_HEIGHT 12.f
// NOTE(zoubir): above the status strip (ui/status_strip.cpp), which sits
// just over the plate
#define FROSTMAGE_HUD_LIFT 46.f

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a snowflake
internal void
DrawFrostMageEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    for(u32 Arm = 0; Arm < 6; Arm++)
    {
        float A = Pi32 * (float)Arm / 3.f + 0.5f * Pi32;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 End = C + 1.15f * S * Dir;
        DrawPartyQuad(RenderContext, C + 0.12f * S * Side, End + 0.06f * S * Side, End - 0.06f * S * Side,
                      C - 0.12f * S * Side, Fill);
        v2 Fork = C + 0.65f * S * Dir;
        for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
        {
            float B = A + Sign * 0.75f;
            v2 Twig = Fork + 0.38f * S * V2(Cos(B), Sin(B));
            DrawPartyQuad(RenderContext, Fork + 0.06f * S * Side, Twig, Twig, Fork - 0.06f * S * Side, Light);
        }
    }
    DrawPartyQuad(RenderContext, C + V2(0.f, -0.22f * S), C + V2(0.22f * S, 0.f), C + V2(0.f, 0.22f * S),
                  C + V2(-0.22f * S, 0.f), Light);
}

// NOTE(zoubir): one shard of the bar at X, Y, W wide: a long diamond
// pointing right
internal void
DrawFrostMageShard(render_context *RenderContext, float X, float Y, float W, float H, u32 Color)
{
    float Tip = 0.5f * H;
    v2 Left = V2(X, Y + 0.5f * H);
    v2 Top = V2(X + Tip, Y);
    v2 TopRight = V2(X + W - Tip, Y);
    v2 Right = V2(X + W, Y + 0.5f * H);
    v2 BottomRight = V2(X + W - Tip, Y + H);
    v2 Bottom = V2(X + Tip, Y + H);
    DrawFilledQuad(RenderContext, Left, Top, TopRight, Right, Color, Color, Color, Color, RenderBlend_Alpha);
    DrawFilledQuad(RenderContext, Left, Right, BottomRight, Bottom, Color, Color, Color, Color,
                   RenderBlend_Alpha);
}

// NOTE(zoubir): every frame while the local player is a Frost Mage
internal void
DrawFrostMageHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
                 u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = Slot->Entity;
    if (!Player || IsDeadPlayer(Player))
    {
        return;
    }
    frostmage_slot *Mage = &Slot->FrostMage;
    float Clock = GetFxClock(AppState);
    float DeltaTime = Clamp01(Clock - Mage->HudClock);
    Mage->HudClock = Clock;
    u32 Icicles = Minimum((u32)Slot->ClassMeter, (u32)FROSTMAGE_ICICLES_MOST);
    // NOTE(zoubir): a new Icicle eases in; spent ones go at once
    if ((float)Icicles < Mage->HudIcicles)
    {
        Mage->HudIcicles = (float)Icicles;
    }
    Mage->HudIcicles += ((float)Icicles - Mage->HudIcicles) * Minimum(1.f, 12.f * DeltaTime);
    bool32 Full = Icicles >= FROSTMAGE_ICICLES_MOST;

    font *Small = AppState->Fonts.Small;
    float Width = FROSTMAGE_HUD_WIDTH;
    float Height = FROSTMAGE_HUD_HEIGHT;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = AbilityBarPlateTop(WindowHeight) - FROSTMAGE_HUD_LIFT - Height;
    u32 Ice = UI_RGBA(150, 215, 255, 255);
    u32 Pale = UI_RGBA(230, 245, 255, 255);

    float Pad = 6.f;
    float LabelWidth = 54.f;
    DrawRoundRect(RenderContext, X - Pad - LabelWidth, Y - Pad, Width + 2.f * Pad + 2.f * LabelWidth,
                  Height + 2.f * Pad, WithAlpha(UI_RGBA(10, 14, 22, 255), 0.72f));
    float Breath = Full ? 0.5f + 0.5f * Sin(5.f * Clock) : 0.f;
    if (Full)
    {
        float G = 0.5f * Width + 40.f;
        DrawShaderQuad(RenderContext, Shader_Glow, X + 0.5f * Width - G, Y + 0.5f * Height - 26.f, 2.f * G, 52.f,
                       WithAlpha(Ice, 0.22f + 0.18f * Breath), RenderBlend_Additive);
    }
    char *Label = Full ? (char *)"Freeze" : (char *)"Icicles";
    float LineY = Y + 0.5f * Height - 0.5f * UILineHeight(Small);
    UIText(RenderContext, Small, X - 8.f, LineY, Label, Full ? Ice : UI_COLOR_TEXT_MUTED, UIAlign_Right);

    float Gap = 4.f;
    float ShardWidth = (Width - Gap * (float)(FROSTMAGE_ICICLES_MOST - 1)) / (float)FROSTMAGE_ICICLES_MOST;
    for(u32 Shard = 0; Shard < FROSTMAGE_ICICLES_MOST; Shard++)
    {
        float SX = X + (float)Shard * (ShardWidth + Gap);
        DrawFrostMageShard(RenderContext, SX, Y, ShardWidth, Height, UI_RGBA(40, 58, 78, 255));
        DrawFrostMageShard(RenderContext, SX + 1.f, Y + 1.f, ShardWidth - 2.f, Height - 2.f, UI_RGBA(16, 22, 32, 255));
        float Fill = Clamp01(Mage->HudIcicles - (float)Shard);
        if (Fill <= 0.f)
        {
            continue;
        }
        u32 Color = Fill >= 1.f ? UIMixColor(Ice, Pale, 0.3f * Breath) : UIMixColor(UI_RGBA(50, 90, 140, 255), Ice, Fill);
        float W = Fill * (ShardWidth - 2.f);
        DrawFrostMageShard(RenderContext, SX + 1.f + 0.5f * (ShardWidth - 2.f - W), Y + 1.f, W, Height - 2.f, Color);
        if (Fill >= 1.f)
        {
            DrawFilledQuad(RenderContext, V2(SX + 0.5f * Height, Y + 2.f), V2(SX + ShardWidth - 0.5f * Height, Y + 2.f),
                           V2(SX + ShardWidth - 0.5f * Height, Y + 4.f), V2(SX + 0.5f * Height, Y + 4.f),
                           WithAlpha(Pale, 0.7f), WithAlpha(Pale, 0.7f), WithAlpha(Pale, 0.1f), WithAlpha(Pale, 0.1f),
                           RenderBlend_Alpha);
        }
    }

    // NOTE(zoubir): while Glacial Spike casts, what it will deal; else the
    // count
    char Text[32];
    if (Player->CastSpell == PlayerSpell_FrostMageA)
    {
        float Spike = GLACIAL_SPIKE_DAMAGE + GLACIAL_SPIKE_PER_ICICLE * (float)Icicles;
        snprintf(Text, sizeof(Text), "%u%s", (u32)(Spike * GetRoleDef(PlayerRole_FrostMage)->DamageDealt + 0.5f),
                 Full ? " + freeze" : "");
    }
    else
    {
        snprintf(Text, sizeof(Text), "%u", Icicles);
    }
    float Right = X + Width + 10.f;
    UIText(RenderContext, Small, Right, LineY, Text, Full ? Ice : UI_COLOR_TEXT, UIAlign_Left);

    // NOTE(zoubir): the barrier and Fingers of Frost, small, past the
    // number: a shield of ice and a white snowflake
    float MarkX = Right + 34.f + (Player->CastSpell == PlayerSpell_FrostMageA ? 70.f : 0.f);
    v2 Mark = V2(MarkX, Y + 0.5f * Height);
    if (Slot->ClassFlags & FROSTMAGE_FLAG_BARRIER)
    {
        DrawArcBand(RenderContext, Mark, 0.f, 2.f * Pi32, 5.f, 8.f, WithAlpha(Ice, 0.5f), Ice, RenderBlend_Alpha);
        Mark.X += 22.f;
    }
    if (Slot->ClassFlags & FROSTMAGE_FLAG_FINGERS)
    {
        DrawFrostMageEmblem(RenderContext, Mark, 7.f, Pale, Ice);
    }
}
