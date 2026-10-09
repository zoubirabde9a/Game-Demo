/* Shadowblade HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Shadowblade sees of its own
   class over the ability bar.

   Over the bar: five gem sockets on a dark plate, one lit per combo point
   (player_slot.ClassMeter). A point coming in pops its gem with a flash;
   at five the plate glows and pulses and says the finisher is ready; an
   Eviscerate or Deadly Throw empties them in a flash; pressing it with none shakes the
   plate red and says so; one whose foe got away in the wind-up keeps
   its points, and the plate says that too. Beside the plate, a ring that runs down while a
   critical strike waits (acid green) and while Shadow Dance lasts
   (violet), timed from when its flag goes up. It reads the slot's meter and flags and the class's bursts,
   which every client gets, so it is the same online. */

#define SHADOWBLADE_HUD_GEM 18.f
#define SHADOWBLADE_HUD_GAP 30.f
// NOTE(zoubir): from the ability bar's plate up to the gems' middle
#define SHADOWBLADE_HUD_LIFT 42.f

internal float AbilityBarPlateTop(u32 WindowHeight);

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: two daggers crossed
internal void
DrawShadowbladeEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    for(u32 Blade = 0; Blade < 2; Blade++)
    {
        float Way = Blade ? -1.f : 1.f;
        v2 Grip = C + V2(-Way * 0.9f * S, 0.9f * S);
        v2 Dir = V2(Way * 0.7071f, -0.7071f);
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Base = Grip + (0.55f * S) * Dir;
        v2 Tip = Grip + (2.3f * S) * Dir;
        DrawPartyQuad(RenderContext, Base - 0.26f * S * Side, Tip, Tip, Base + 0.26f * S * Side, Light);
        DrawPartyQuad(RenderContext, Base - 0.55f * S * Side, Base + 0.55f * S * Side,
                      Base + 0.55f * S * Side - 0.18f * S * Dir, Base - 0.55f * S * Side - 0.18f * S * Dir,
                      Fill);
        DrawPartyQuad(RenderContext, Grip - 0.12f * S * Side, Base - 0.12f * S * Side,
                      Base + 0.12f * S * Side, Grip + 0.12f * S * Side, Fill);
    }
}

// NOTE(zoubir): a ring at C running down from full to Left (0..1), with a
// glow, for a buff with a known length
internal void
DrawShadowbladeTimer(render_context *RenderContext, v2 C, float Radius, float Left, u32 RGB, float Alpha)
{
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 2.f * Radius, C.Y - 2.f * Radius, 4.f * Radius,
                   4.f * Radius, FxColor(0.45f * Alpha, RGB), RenderBlend_Additive);
    DrawArcBand(RenderContext, C, 0.f, 2.f * Pi32, 0.f, Radius + 2.f, FxColor(0.85f * Alpha, 0x00140A12),
                FxColor(0.85f * Alpha, 0x00140A12), RenderBlend_Alpha);
    float Start = -0.5f * Pi32;
    for(u32 Step = 0; Step < 24; Step++)
    {
        float A = Start + 2.f * Pi32 * Left * (float)Step / 24.f;
        float B = Start + 2.f * Pi32 * Left * (float)(Step + 1) / 24.f;
        v2 PA = C + Radius * V2(Cos(A), Sin(A));
        v2 PB = C + Radius * V2(Cos(B), Sin(B));
        DrawFxStroke(RenderContext, PA, PB, 3.f, 3.f, FxColor(Alpha, RGB), FxColor(Alpha, RGB),
                     RenderBlend_Alpha);
    }
}

// NOTE(zoubir): every frame while the local player is a Shadowblade
internal void
DrawShadowbladeHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
           u32 WindowWidth, u32 WindowHeight)
{
    if (IsDeadPlayer(Slot->Entity))
    {
        return;
    }
    u32 SlotIndex = AppState->LocalPlayerIndex;
    float Clock = GetFxClock(AppState);
    u32 Points = Minimum((u32)Slot->ClassMeter, (u32)SHADOWBLADE_MOST_POINTS);
    bool32 Full = Points >= SHADOWBLADE_MOST_POINTS;
    v2 Centre = V2(0.5f * (float)WindowWidth, AbilityBarPlateTop(WindowHeight) - SHADOWBLADE_HUD_LIFT);

    // NOTE(zoubir): the newest point coming in, the points spent, and a
    // finisher pressed with none
    float Gained = 100.f;
    u32 Builders[3] = {ShadowbladeBurst_TwinStrike, ShadowbladeBurst_Step, ShadowbladeBurst_Fan};
    for(u32 Index = 0; Index < ArrayCount(Builders); Index++)
    {
        Gained = Minimum(Gained, ShadowbladeBurstAge(AppState, SlotIndex, Builders[Index]));
    }
    // NOTE(zoubir): an Eviscerate whose foe got away carries no points
    // (shadowblade_defs.cpp): nothing was spent, and the plate says why
    role_burst *Evis = ShadowbladeNewestBurst(AppState, SlotIndex, ShadowbladeBurst_Eviscerate);
    float EvisAge = Evis ? Clock - Evis->Start : 100.f;
    bool32 Whiff = Evis && ShadowbladeBurstVariant(Evis->Position) == 0;
    float Spent = Minimum(Whiff ? 100.f : EvisAge,
                          ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Throw));
    float Missed = Whiff ? EvisAge : 100.f;
    float Empty = ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Empty);
    float Shake = Empty < 0.4f ? 6.f * Sin(70.f * Empty) * (1.f - Empty / 0.4f) : 0.f;
    Centre.X += Shake;

    // NOTE(zoubir): the plate, glowing and pulsing at five
    float Width = SHADOWBLADE_HUD_GAP * (float)(SHADOWBLADE_MOST_POINTS - 1) + 2.f * SHADOWBLADE_HUD_GEM + 8.f;
    float Height = SHADOWBLADE_HUD_GEM + 16.f;
    float Pulse = Full ? 0.5f + 0.5f * Sin(6.f * Clock) : 0.f;
    if (Full)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.8f * Width, Centre.Y - 1.6f * Height,
                       1.6f * Width, 3.2f * Height, FxColor(0.25f + 0.25f * Pulse, 0x00FF6EAA),
                       RenderBlend_Additive);
    }
    DrawRoundRect(RenderContext, Centre.X - 0.5f * Width, Centre.Y - 0.5f * Height, Width, Height,
                  UI_RGBA(14, 10, 22, 200));
    u32 Rim = Empty < 0.5f ? UIMixColor(UI_RGBA(255, 70, 80, 255), UI_RGBA(120, 80, 170, 160), Empty / 0.5f) :
        (Full ? UIMixColor(UI_RGBA(150, 100, 230, 200), UI_RGBA(230, 200, 255, 255), Pulse) :
         UI_RGBA(110, 70, 160, 150));
    DrawRoundOutline(RenderContext, Centre.X - 0.5f * Width, Centre.Y - 0.5f * Height, Width, Height, Rim);

    float Left = -0.5f * SHADOWBLADE_HUD_GAP * (float)(SHADOWBLADE_MOST_POINTS - 1);
    for(u32 Gem = 0; Gem < SHADOWBLADE_MOST_POINTS; Gem++)
    {
        v2 P = Centre + V2(Left + SHADOWBLADE_HUD_GAP * (float)Gem, 0.f);
        bool32 Lit = Gem < Points;
        float Size = SHADOWBLADE_HUD_GEM;
        float Flash = 0.f;
        // NOTE(zoubir): the newest point pops in over a quarter second
        if (Lit && Gem + 1 == Points && Gained < 0.25f)
        {
            float T = Gained / 0.25f;
            Size *= 1.f + 0.45f * (1.f - T) * (1.f - T);
            Flash = 1.f - T;
        }
        DrawShadowbladeGem(RenderContext, P, Size, Lit ? 1.f : 0.f, Flash, Full ? Pulse : 0.f, 1.f);
        // NOTE(zoubir): spent points burst out as they empty
        if (!Lit && Spent < 0.35f)
        {
            float T = Spent / 0.35f;
            DrawShadowbladeGem(RenderContext, P - V2(0.f, 12.f * T), SHADOWBLADE_HUD_GEM * (1.f + 0.6f * T),
                               1.f, 1.f, 0.f, 1.f - T);
        }
        if (Empty < 0.5f)
        {
            DrawShaderQuad(RenderContext, Shader_Glow, P.X - Size, P.Y - Size, 2.f * Size, 2.f * Size,
                           FxColor(0.6f * (1.f - Empty / 0.5f), 0x002030FF), RenderBlend_Additive);
        }
    }

    // NOTE(zoubir): a line over the plate when it says something
    font *Small = AppState->Fonts.Small;
    float LineY = Centre.Y - 0.5f * Height - UILineHeight(Small) - 2.f;
    if (Empty < 0.9f)
    {
        float Fade = 1.f - Clamp01((Empty - 0.6f) / 0.3f);
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"No combo points",
               WithAlpha(UI_RGBA(255, 110, 110, 255), Fade), UIAlign_Center);
    }
    else if (Missed < 0.9f)
    {
        float Fade = 1.f - Clamp01((Missed - 0.6f) / 0.3f);
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"Out of reach: points kept",
               WithAlpha(UI_RGBA(255, 200, 120, 255), Fade), UIAlign_Center);
    }
    else if (Full)
    {
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"Finisher ready",
               UIMixColor(UI_RGBA(200, 160, 255, 230), UI_RGBA(255, 240, 255, 255), Pulse),
               UIAlign_Center);
    }

    // NOTE(zoubir): the timers, left the critical strike, right the dance,
    // timed from when each flag went up; a Shadowstep or Dance burst newer
    // than that restarted a window that never closed
    shadowblade_slot *Blade = &Slot->Shadowblade;
    u32 Rose = Slot->ClassFlags & ~Blade->ShownFlags;
    Blade->CritClock = (Rose & SHADOWBLADE_FLAG_CRIT) ? Clock : Blade->CritClock;
    Blade->DanceClock = (Rose & SHADOWBLADE_FLAG_DANCE) ? Clock : Blade->DanceClock;
    Blade->ShownFlags = Slot->ClassFlags;
    Blade->CritClock = Maximum(Blade->CritClock,
                               Clock - ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Step));
    Blade->DanceClock = Maximum(Blade->DanceClock,
                                Clock - ShadowbladeBurstAge(AppState, SlotIndex, ShadowbladeBurst_Dance));
    float Side = 0.5f * Width + 22.f;
    float Step = Clock - Blade->CritClock;
    if ((Slot->ClassFlags & SHADOWBLADE_FLAG_CRIT) && Step < SHADOWBLADE_CRIT_SECONDS)
    {
        v2 C = Centre - V2(Side, 0.f);
        DrawShadowbladeTimer(RenderContext, C, 11.f, 1.f - Step / SHADOWBLADE_CRIT_SECONDS, 0x0046FF96, 1.f);
        DrawShadowbladeDagger(RenderContext, C + V2(-4.f, 5.f), ShadowbladeNormal(V2(0.6f, -1.f)), 13.f, 1.f, 1.f);
    }
    float Dance = Clock - Blade->DanceClock;
    if ((Slot->ClassFlags & SHADOWBLADE_FLAG_DANCE) && Dance < DANCE_SECONDS)
    {
        v2 C = Centre + V2(Side, 0.f);
        DrawShadowbladeTimer(RenderContext, C, 11.f, 1.f - Dance / DANCE_SECONDS, 0x00FF6EAA, 1.f);
        DrawShadowbladeSilhouette(RenderContext, C + V2(0.f, 9.f), 12.f, 18.f, 1.f);
    }
}
