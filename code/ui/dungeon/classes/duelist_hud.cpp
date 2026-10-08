/* Duelist HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Duelist sees of its own
   class over the ability bar.

   Over the bar: five thin diamonds on a dark plate, one filled per Tempo
   stack (player_slot.ClassMeter). A stack coming in pops its diamond with
   a flash; two lost to a hit crack down the middle and fall away in halves
   (DuelistBurst_Break), and the plate shakes red; at five the plate glows
   and says so. Beside the plate, a ring runs down while the guard is up
   (pale, gold once it has parried, which the plate also says) and one
   while Perfect Form lasts. It reads the slot's meter and flags and the
   class's bursts, which every client gets, so it is the same online. */

#define DUELIST_HUD_DIAMOND 26.f
#define DUELIST_HUD_GAP 26.f
// NOTE(zoubir): from the ability bar's plate up to the diamonds' middle
#define DUELIST_HUD_LIFT 42.f

internal float AbilityBarPlateTop(u32 WindowHeight);

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a rapier point up
// through a heart
internal void
DrawDuelistEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    v2 L = C + V2(-0.45f * S, 0.15f * S);
    v2 R = C + V2(0.45f * S, 0.15f * S);
    for(u32 Lobe = 0; Lobe < 2; Lobe++)
    {
        v2 P = Lobe ? R : L;
        DrawPartyQuad(RenderContext, P + V2(0.f, -0.55f * S), P + V2(0.55f * S, 0.f), P + V2(0.f, 0.55f * S),
                      P + V2(-0.55f * S, 0.f), Fill);
    }
    DrawPartyQuad(RenderContext, C + V2(-0.98f * S, 0.12f * S), C + V2(0.98f * S, 0.12f * S),
                  C + V2(0.f, 1.2f * S), C + V2(0.f, 1.2f * S), Fill);
    // NOTE(zoubir): the blade up through it, a crossguard under the heart
    DrawPartyQuad(RenderContext, C + V2(-0.13f * S, 1.6f * S), C + V2(0.13f * S, 1.6f * S),
                  C + V2(0.03f * S, -1.7f * S), C + V2(-0.03f * S, -1.7f * S), Light);
    DrawPartyQuad(RenderContext, C + V2(-0.6f * S, 1.25f * S), C + V2(0.6f * S, 1.25f * S),
                  C + V2(0.6f * S, 1.45f * S), C + V2(-0.6f * S, 1.45f * S), Light);
}

// NOTE(zoubir): a ring at C running down from full to Left (0..1), for a
// guard or a buff with a known length
internal void
DrawDuelistTimer(render_context *RenderContext, v2 C, float Radius, float Left, u32 RGB)
{
    DrawShaderQuad(RenderContext, Shader_Glow, C.X - 2.f * Radius, C.Y - 2.f * Radius, 4.f * Radius,
                   4.f * Radius, FxColor(0.4f, RGB), RenderBlend_Additive);
    DrawDuelistDisc(RenderContext, C, Radius + 2.f, FxColor(0.85f, 0x00140A14));
    float Start = -0.5f * Pi32;
    for(u32 Step = 0; Step < 24; Step++)
    {
        float A = Start + 2.f * Pi32 * Left * (float)Step / 24.f;
        float B = Start + 2.f * Pi32 * Left * (float)(Step + 1) / 24.f;
        DrawFxStroke(RenderContext, C + Radius * V2(Cos(A), Sin(A)), C + Radius * V2(Cos(B), Sin(B)),
                     3.f, 3.f, FxColor(1.f, RGB), FxColor(1.f, RGB), RenderBlend_Alpha);
    }
}

// NOTE(zoubir): every frame while the local player is a Duelist
internal void
DrawDuelistHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
               u32 WindowWidth, u32 WindowHeight)
{
    if (IsDeadPlayer(Slot->Entity))
    {
        return;
    }
    duelist_slot *Duel = &Slot->Duelist;
    u32 SlotIndex = AppState->LocalPlayerIndex;
    float Clock = GetFxClock(AppState);
    u32 Tempo = Minimum((u32)Slot->ClassMeter, (u32)DUELIST_MOST_TEMPO);
    bool32 Full = Tempo >= DUELIST_MOST_TEMPO;
    v2 Centre = V2(0.5f * (float)WindowWidth, AbilityBarPlateTop(WindowHeight) - DUELIST_HUD_LIFT);

    // NOTE(zoubir): when the stacks last rose, for the newest one's pop;
    // drops come with a Break burst when a hit took them, which carries
    // the Tempo before
    if (Tempo > Duel->ShownTempo)
    {
        Duel->GainClock = Clock;
    }
    Duel->ShownTempo = Tempo;
    u32 Rose = Slot->ClassFlags & ~Duel->ShownFlags;
    Duel->GuardClock = (Rose & DUELIST_FLAG_GUARD) ? Clock : Duel->GuardClock;
    Duel->FormClock = (Rose & DUELIST_FLAG_FORM) ? Clock : Duel->FormClock;
    Duel->ShownFlags = Slot->ClassFlags;
    float Gained = Clock - Duel->GainClock;
    role_burst *Break = DuelistNewestBurst(AppState, SlotIndex, DuelistBurst_Break);
    float Broke = Break ? Clock - Break->Start : 100.f;
    u32 Before = Break ? Minimum(DuelistBurstVariant(Break->Position), (u32)DUELIST_MOST_TEMPO) : 0;
    float Shake = Broke < 0.35f ? 6.f * Sin(70.f * Broke) * (1.f - Broke / 0.35f) : 0.f;
    Centre.X += Shake;

    // NOTE(zoubir): the plate, glowing and pulsing at five
    float Width = DUELIST_HUD_GAP * (float)(DUELIST_MOST_TEMPO - 1) + DUELIST_HUD_DIAMOND + 16.f;
    float Height = DUELIST_HUD_DIAMOND + 12.f;
    float Pulse = Full ? 0.5f + 0.5f * Sin(6.f * Clock) : 0.f;
    if (Full)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.8f * Width, Centre.Y - 1.5f * Height,
                       1.6f * Width, 3.f * Height, FxColor(0.2f + 0.25f * Pulse, DUELIST_RGB),
                       RenderBlend_Additive);
    }
    DrawRoundRect(RenderContext, Centre.X - 0.5f * Width, Centre.Y - 0.5f * Height, Width, Height,
                  UI_RGBA(20, 10, 18, 200));
    u32 Rim = Broke < 0.5f ? UIMixColor(UI_RGBA(255, 70, 80, 255), UI_RGBA(170, 80, 130, 160), Broke / 0.5f) :
        (Full ? UIMixColor(UI_RGBA(220, 100, 160, 200), UI_RGBA(255, 220, 240, 255), Pulse) :
         UI_RGBA(150, 70, 120, 150));
    DrawRoundOutline(RenderContext, Centre.X - 0.5f * Width, Centre.Y - 0.5f * Height, Width, Height, Rim);

    float Left = -0.5f * DUELIST_HUD_GAP * (float)(DUELIST_MOST_TEMPO - 1);
    for(u32 Stack = 0; Stack < DUELIST_MOST_TEMPO; Stack++)
    {
        v2 P = Centre + V2(Left + DUELIST_HUD_GAP * (float)Stack, 0.f);
        bool32 Lit = Stack < Tempo;
        float Size = DUELIST_HUD_DIAMOND;
        float Flash = Full ? 0.4f * Pulse : 0.f;
        // NOTE(zoubir): the newest stack pops in over a quarter second,
        // filling from the bottom
        float Fill = Lit ? 1.f : 0.f;
        if (Lit && Stack + 1 == Tempo && Gained < 0.25f)
        {
            float T = Gained / 0.25f;
            Size *= 1.f + 0.35f * (1.f - T) * (1.f - T);
            Fill = DuelistEase(T * 1.6f);
            Flash = 1.f - T;
        }
        bool32 Cracking = !Lit && Stack < Before && Broke < 0.8f;
        if (Cracking)
        {
            if (Broke < 0.08f)
            {
                DrawDuelistDiamond(RenderContext, P, Size, 1.f, 1.f, 1.f);
                DrawFxStroke(RenderContext, P + V2(0.f, -0.45f * Size), P + V2(-3.f, 0.05f * Size), 2.f, 2.f,
                             FxColor(1.f, 0x00FFFFFF), FxColor(1.f, 0x00FFFFFF), RenderBlend_Alpha);
                DrawFxStroke(RenderContext, P + V2(-3.f, 0.05f * Size), P + V2(1.f, 0.45f * Size), 2.f, 2.f,
                             FxColor(1.f, 0x00FFFFFF), FxColor(1.f, 0x00FFFFFF), RenderBlend_Alpha);
            }
            else
            {
                DrawDuelistDiamond(RenderContext, P, Size, 0.f, 0.f, 1.f);
                float Fade = 1.f - DuelistEase((Broke - 0.3f) / 0.5f);
                DrawDuelistShard(RenderContext, P, Size, false, Broke - 0.08f, Fade);
                DrawDuelistShard(RenderContext, P, Size, true, Broke - 0.08f, Fade);
            }
            continue;
        }
        DrawDuelistDiamond(RenderContext, P, Size, Fill, Flash, 1.f);
    }

    // NOTE(zoubir): a line over the plate when it says something
    font *Small = AppState->Fonts.Small;
    float LineY = Centre.Y - 0.5f * Height - UILineHeight(Small) - 2.f;
    float Parried = DuelistBurstAge(AppState, SlotIndex, DuelistBurst_Parry);
    if (Parried < 0.9f)
    {
        float Fade = 1.f - Clamp01((Parried - 0.6f) / 0.3f);
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"Parried! Riposte back in 2 s",
               WithAlpha(UI_RGBA(255, 215, 110, 255), Fade), UIAlign_Center);
    }
    else if (Broke < 0.9f && Before > Tempo)
    {
        float Fade = 1.f - Clamp01((Broke - 0.6f) / 0.3f);
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"Hit: Tempo lost",
               WithAlpha(UI_RGBA(255, 110, 110, 255), Fade), UIAlign_Center);
    }
    else if (Full)
    {
        UIText(RenderContext, Small, Centre.X, LineY, (char *)"Full Tempo",
               UIMixColor(UI_RGBA(240, 150, 200, 230), UI_RGBA(255, 240, 250, 255), Pulse),
               UIAlign_Center);
    }

    // NOTE(zoubir): the timers, left the guard, right Perfect Form
    float Side = 0.5f * Width + 22.f;
    if (Slot->ClassFlags & DUELIST_FLAG_GUARD)
    {
        float Length = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Bait) ? BAIT_GUARD_SECONDS :
            RIPOSTE_GUARD_SECONDS;
        float Up = Clock - Duel->GuardClock;
        u32 RGB = (Slot->ClassFlags & DUELIST_FLAG_PARRIED) ? DUELIST_GOLD_RGB : DUELIST_PALE_RGB;
        v2 C = Centre - V2(Side, 0.f);
        DrawDuelistTimer(RenderContext, C, 11.f, Clamp01(1.f - Up / Length), RGB);
        DrawDuelistRapier(RenderContext, C + V2(-5.f, 7.f), DuelistNormal(V2(0.6f, -1.f)), 16.f, 0.f, 1.f);
    }
    float Form = Clock - Duel->FormClock;
    if ((Slot->ClassFlags & DUELIST_FLAG_FORM) && Form < FORM_SECONDS)
    {
        v2 C = Centre + V2(Side, 0.f);
        DrawDuelistTimer(RenderContext, C, 11.f, 1.f - Form / FORM_SECONDS, DUELIST_RGB);
        DrawDuelistPetal(RenderContext, C, 9.f, 0.8f, 1.f);
    }
}
