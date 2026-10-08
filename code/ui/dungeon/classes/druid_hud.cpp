/* Druid HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on the
   party frames and the meter, and what the local Druid sees of its own
   class over the ability bar: Bloom as five flowers that open as it
   grows, all of them breathing light when full, and the number a heal
   would add from it now. Beside it, three small marks say whether a
   Moonfire is burning, a Rejuvenation is healing and roots are down.
   Everything reads ClassMeter and ClassFlags, which online play gets from
   the server. */

#define DRUID_HUD_WIDTH 210.f
#define DRUID_HUD_HEIGHT 18.f
// NOTE(zoubir): above the status strip (ui/status_strip.cpp), which sits
// just over the plate
#define DRUID_HUD_LIFT 46.f

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a leaf, tip up and
// right, its vein in the light colour
internal void
DrawDruidEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    v2 Dir = V2(0.7071f, -0.7071f);
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Tip = C + 1.15f * S * Dir;
    v2 Stem = C - 0.9f * S * Dir;
    v2 Mid = C + 0.1f * S * Dir;
    DrawPartyQuad(RenderContext, Stem, Mid + 0.62f * S * Side, Tip, Mid - 0.62f * S * Side, Fill);
    DrawPartyQuad(RenderContext, Stem - 0.35f * S * Dir + 0.06f * S * Side, Stem - 0.35f * S * Dir - 0.06f * S * Side,
                  Tip - 0.25f * S * Dir - 0.04f * S * Side, Tip - 0.25f * S * Dir + 0.04f * S * Side, Light);
}

// NOTE(zoubir): every frame while the local player is a Druid
internal void
DrawDruidHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
             u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = Slot->Entity;
    if (!Player || IsDeadPlayer(Player))
    {
        return;
    }
    druid_slot *Druid = &Slot->Druid;
    float Clock = GetFxClock(AppState);
    float DeltaTime = Clamp01(Clock - Druid->HudClock);
    Druid->HudClock = Clock;
    float Bloom = (float)Slot->ClassMeter;
    bool32 WasFull = Druid->HudBloom >= (float)DRUID_BLOOM_MOST - 0.05f;
    // NOTE(zoubir): the flowers ease open, and close at once when a heal
    // spends them
    if (Bloom < Druid->HudBloom)
    {
        Druid->HudBloom = Bloom;
    }
    Druid->HudBloom += (Bloom - Druid->HudBloom) * Minimum(1.f, 10.f * DeltaTime);
    bool32 Full = Bloom >= (float)DRUID_BLOOM_MOST;
    if (Full && !WasFull && Druid->HudBloom >= (float)DRUID_BLOOM_MOST - 0.05f)
    {
        Druid->HudFullAt = Clock;
    }

    font *Small = AppState->Fonts.Small;
    float Width = DRUID_HUD_WIDTH;
    float Height = DRUID_HUD_HEIGHT;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = AbilityBarPlateTop(WindowHeight) - DRUID_HUD_LIFT - Height;
    u32 Leaf = UI_RGBA(165, 200, 60, 255);
    u32 Petal = UI_RGBA(255, 150, 190, 255);

    float Pad = 6.f;
    float LabelWidth = 54.f;
    DrawRoundRect(RenderContext, X - Pad - LabelWidth, Y - Pad, Width + 2.f * Pad + 2.f * LabelWidth,
                  Height + 2.f * Pad, WithAlpha(UI_RGBA(12, 16, 10, 255), 0.72f));
    float Breath = Full ? 0.5f + 0.5f * Sin(4.f * Clock) : 0.f;
    float Flash = Clamp01(1.f - (Clock - Druid->HudFullAt) / 0.6f);
    if (Full)
    {
        float G = 0.5f * Width + 40.f + 30.f * Flash;
        DrawShaderQuad(RenderContext, Shader_Glow, X + 0.5f * Width - G, Y + 0.5f * Height - 26.f - 10.f * Flash,
                       2.f * G, 52.f + 20.f * Flash, WithAlpha(Petal, 0.18f + 0.15f * Breath + 0.5f * Flash),
                       RenderBlend_Additive);
    }
    float LineY = Y + 0.5f * Height - 0.5f * UILineHeight(Small);
    UIText(RenderContext, Small, X - 8.f, LineY, "Bloom", Full ? Petal : UI_COLOR_TEXT_MUTED, UIAlign_Right);

    // NOTE(zoubir): five flowers on a vine, each opening as its Bloom grows
    float Step = Width / (float)DRUID_BLOOM_MOST;
    float VineY = Y + 0.5f * Height;
    DrawFxStroke(RenderContext, V2(X, VineY), V2(X + Width, VineY), 2.f, 2.f, UI_RGBA(60, 90, 30, 255),
                 UI_RGBA(60, 90, 30, 255), RenderBlend_Alpha);
    for(u32 Flower = 0; Flower < DRUID_BLOOM_MOST; Flower++)
    {
        v2 C = V2(X + Step * ((float)Flower + 0.5f), VineY);
        float Open = Clamp01(Druid->HudBloom - (float)Flower);
        DrawDruidLeaf(RenderContext, C + V2(-0.32f * Step, 3.f), V2(-0.8f, 0.6f), 9.f, 0.9f, DRUID_FX_DEEP_RGB);
        if (Open <= 0.f)
        {
            DrawDruidBud(RenderContext, C, 0.5f * Height, 1.f);
            continue;
        }
        float Sway = Full ? 0.15f * Sin(3.f * Clock + (float)Flower) : 0.f;
        DrawDruidFlower(RenderContext, C, 0.55f * Height * (0.6f + 0.4f * Open), Open, Sway - 0.5f * Pi32,
                        0.4f + 0.6f * Open, DRUID_FX_PETAL_RGB);
    }

    // NOTE(zoubir): what the Bloom adds to the next Regrowth now
    char Text[32];
    snprintf(Text, sizeof(Text), Bloom > 0.f ? "+%u" : "%u", (u32)(REGROWTH_PER_BLOOM * Bloom + 0.5f));
    UIText(RenderContext, Small, X + Width + 10.f, LineY, Text, Full ? Petal : UI_COLOR_TEXT, UIAlign_Left);

    // NOTE(zoubir): the moon, the leaf and the roots, lit while they are up
    float IconX = X + Width + LabelWidth + Pad + 16.f;
    u32 Off = WithAlpha(UI_RGBA(120, 130, 120, 255), 0.5f);
    v2 MoonAt = V2(IconX, VineY);
    bool32 Moon = (Slot->ClassFlags & DRUID_FLAG_MOONFIRE) != 0;
    DrawArcBand(RenderContext, MoonAt, 0.6f * Pi32, 1.9f * Pi32, 3.5f, 7.f, Moon ? UI_RGBA(210, 225, 255, 255) : Off,
                Moon ? UI_RGBA(170, 200, 255, 255) : Off, RenderBlend_Alpha);
    v2 LeafAt = MoonAt + V2(22.f, 0.f);
    bool32 Rejuv = (Slot->ClassFlags & DRUID_FLAG_REJUVENATION) != 0;
    DrawDruidLeaf(RenderContext, LeafAt, V2(0.7071f, -0.7071f), 14.f, Rejuv ? 1.f : 0.4f,
                  Rejuv ? DRUID_FX_LEAF_RGB : 0x00787878);
    if (RoleSpellLearned(Slot, 2))
    {
        v2 RootsAt = LeafAt + V2(22.f, 0.f);
        bool32 Roots = (Slot->ClassFlags & DRUID_FLAG_ROOTS) != 0;
        u32 Vine = Roots ? Leaf : Off;
        for(u32 Strand = 0; Strand < 3; Strand++)
        {
            float SX = RootsAt.X + 4.f * ((float)Strand - 1.f);
            DrawFxStroke(RenderContext, V2(SX, RootsAt.Y + 7.f), V2(SX + 2.f * Sin(3.f * Clock + (float)Strand),
                         RootsAt.Y - 6.f), 2.5f, 1.f, Vine, Vine, RenderBlend_Alpha);
        }
    }
}
