/* Berserker HUD (ui/dungeon/classes/class_hud.cpp): the class's emblem on
   the party frames and the meter, and what the local Berserker sees of its own
   class over the ability bar: the Rage bar.

   The bar sits on the top edge of the ability bar's plate. Its fill eases
   toward the Rage the server sends (ClassMeter), dark blood at the empty
   end to a hot red at the full one; a gain flashes the part it added.
   Notches mark what Execute needs and what Whirlwind costs, and light up
   once there is that much. A full bar throbs, glowing. A spell pressed
   without its Rage (BERSERKER_FLAG_NO_RAGE) shakes the bar, flashes it and
   says so over it. Under Berserk the rim burns orange and the word beats
   over the bar. */

#define BERSERKER_BAR_WIDTH 260.f
#define BERSERKER_BAR_HEIGHT 12.f
// NOTE(zoubir): how fast the shown fill catches the real Rage, a share a
// second, and how long a gain's flash and a refusal's shake last
#define BERSERKER_BAR_EASE 10.f
#define BERSERKER_GAIN_SECONDS 0.35f

internal float AbilityBarPlateTop(u32 WindowHeight);

// NOTE(zoubir): the emblem's picture, centred on C, S about a third of
// its size; Fill in the class colour, Light paler: a great axe leaning
// across, its broad blade at the top right
internal void
DrawBerserkerEmblem(render_context *RenderContext, v2 C, float S, u32 Fill, u32 Light)
{
    v2 Butt = C + V2(-0.95f * S, 1.05f * S);
    v2 Head = C + V2(0.7f * S, -0.75f * S);
    v2 Along = Head - Butt;
    v2 Dir = NormalizeOr(Along, V2(1.f, -1.f));
    v2 Across = V2(-Dir.Y, Dir.X);
    float Half = 0.11f * S;
    DrawPartyQuad(RenderContext, Butt - Half * Across, Head + 0.3f * S * Dir - Half * Across,
                  Head + 0.3f * S * Dir + Half * Across, Butt + Half * Across, Light);
    // NOTE(zoubir): the blade, flaring away from the haft to a curved edge
    v2 Root0 = Head - 0.35f * S * Dir;
    v2 Root1 = Head + 0.25f * S * Dir;
    v2 Edge0 = Head - 0.75f * S * Dir - 0.85f * S * Across;
    v2 Edge1 = Head + 0.55f * S * Dir - 0.9f * S * Across;
    v2 EdgeMid = Head - 0.1f * S * Dir - 1.05f * S * Across;
    DrawPartyQuad(RenderContext, Root0, Root1, Edge1, EdgeMid, Fill);
    DrawPartyQuad(RenderContext, Root0, EdgeMid, Edge0, Root0 - 0.1f * S * Across, Fill);
    // NOTE(zoubir): the spike on the back of the head
    DrawPartyQuad(RenderContext, Head - 0.15f * S * Dir, Head + 0.15f * S * Dir,
                  Head + 0.5f * S * Across, Head + 0.5f * S * Across, Fill);
}

// NOTE(zoubir): every frame while the local player is a Berserker
internal void
DrawBerserkerHud(render_context *RenderContext, app_state *AppState, player_slot *Slot,
           u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = Slot->Entity;
    if (IsDeadPlayer(Player))
    {
        return;
    }
    berserker_slot *State = &Slot->Berserker;
    float Clock = GetFxClock(AppState);
    float DeltaTime = Clamp01(Clock - State->HudClock);
    State->HudClock = Clock;
    float Rage = (float)Slot->ClassMeter / (float)BERSERKER_RAGE_MAX;
    if (Rage > State->ShownRage + 0.005f)
    {
        State->GainFrom = Minimum(State->GainFrom, State->ShownRage);
        State->GainSeconds = BERSERKER_GAIN_SECONDS;
    }
    State->ShownRage += (Rage - State->ShownRage) * Clamp01(BERSERKER_BAR_EASE * DeltaTime);
    State->GainSeconds = Maximum(0.f, State->GainSeconds - DeltaTime);
    if (State->GainSeconds <= 0.f)
    {
        State->GainFrom = State->ShownRage;
    }
    bool32 Lacking = (Slot->ClassFlags & BERSERKER_FLAG_NO_RAGE) != 0;
    State->Refused = Lacking ? 1.f : Maximum(0.f, State->Refused - 3.f * DeltaTime);
    bool32 Berserk = (Slot->ClassFlags & BERSERKER_FLAG_BERSERK) != 0;
    bool32 Full = Slot->ClassMeter >= BERSERKER_RAGE_MAX;

    float Beat = 0.5f + 0.5f * Sin((Berserk ? 9.f : 6.f) * Clock);
    float Throb = Full ? 1.f + 0.12f * Beat : 1.f;
    float Width = BERSERKER_BAR_WIDTH;
    float Height = BERSERKER_BAR_HEIGHT * Throb;
    float Shake = 4.f * State->Refused * Sin(70.f * Clock);
    float X = 0.5f * ((float)WindowWidth - Width) + Shake;
    float Middle = AbilityBarPlateTop(WindowHeight) - 4.f;
    float Y = Middle - 0.5f * Height;

    // NOTE(zoubir): the glow behind it, beating when full or under Berserk
    float Glow = Full || Berserk ? 0.35f + 0.35f * Beat : 0.12f * Rage;
    if (Glow > 0.f)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, X - 30.f, Y - 22.f, Width + 60.f, Height + 44.f,
                       FxColor(Glow, Berserk ? BERSERKER_HOT_RGB : BERSERKER_CRIMSON_RGB),
                       RenderBlend_Additive);
    }
    DrawRoundRect(RenderContext, X - 3.f, Y - 3.f, Width + 6.f, Height + 6.f, UI_RGBA(16, 8, 10, 235));

    // NOTE(zoubir): the fill, dark blood to hot red, its head bright
    float Inner = Width - 4.f;
    float FillX = X + 2.f;
    float FillY = Y + 2.f;
    float FillH = Height - 4.f;
    float Shown = Clamp01(State->ShownRage);
    if (Shown > 0.f)
    {
        float Right = FillX + Shown * Inner;
        u32 Dark = FxColor(1.f, BERSERKER_DARK_RGB);
        u32 Hot = FxColor(1.f, Shown > 0.8f ? BERSERKER_HOT_RGB : BERSERKER_CRIMSON_RGB);
        DrawFilledQuad(RenderContext, V2(FillX, FillY), V2(Right, FillY), V2(Right, FillY + FillH),
                       V2(FillX, FillY + FillH), Dark, Hot, Hot, Dark, RenderBlend_Alpha);
        DrawFilledQuad(RenderContext, V2(FillX, FillY), V2(Right, FillY), V2(Right, FillY + 0.4f * FillH),
                       V2(FillX, FillY + 0.4f * FillH), FxColor(0.f, 0x00FFFFFF),
                       FxColor(0.35f, BERSERKER_PALE_RGB), FxColor(0.f, BERSERKER_PALE_RGB),
                       FxColor(0.f, 0x00FFFFFF), RenderBlend_Additive);
        DrawFilledRectangle(RenderContext, Right - 2.f, FillY - 1.f, 2.f, FillH + 2.f,
                            UI_RGBA(255, 220, 190, 255), 0.f);
        // NOTE(zoubir): a gain flashes the part it added
        if (State->GainSeconds > 0.f && State->GainFrom < Shown)
        {
            float From = FillX + Clamp01(State->GainFrom) * Inner;
            u32 Flash = FxColor(State->GainSeconds / BERSERKER_GAIN_SECONDS, BERSERKER_PALE_RGB);
            DrawFilledQuad(RenderContext, V2(From, FillY), V2(Right, FillY), V2(Right, FillY + FillH),
                           V2(From, FillY + FillH), Flash, Flash, Flash, Flash, RenderBlend_Additive);
        }
    }
    // NOTE(zoubir): the notches: what Execute needs, what Whirlwind costs
    u32 Marks[2] = {EXECUTE_MIN_RAGE, WHIRLWIND_RAGE};
    for(u32 Mark = 0; Mark < 2; Mark++)
    {
        float At = FillX + Inner * (float)Marks[Mark] / (float)BERSERKER_RAGE_MAX;
        bool32 Lit = Slot->ClassMeter >= Marks[Mark];
        DrawFilledRectangle(RenderContext, At - 1.f, Y - 3.f, 2.f, Height + 6.f,
                            Lit ? UI_RGBA(255, 214, 120, 255) : UI_RGBA(90, 60, 60, 255), 0.f);
    }
    u32 Rim = Berserk ? UIMixColor(UI_RGBA(255, 140, 60, 255), UI_RGBA(255, 230, 160, 255), Beat) :
        UI_RGBA(225, 50, 50, 255);
    DrawRoundOutline(RenderContext, X - 3.f, Y - 3.f, Width + 6.f, Height + 6.f, Rim);
    if (State->Refused > 0.f)
    {
        DrawRoundRect(RenderContext, X - 3.f, Y - 3.f, Width + 6.f, Height + 6.f,
                      UI_RGBA(255, 60, 40, (u32)(150.f * State->Refused)));
    }

    // NOTE(zoubir): the words: the bar's name and Rage on its sides, and
    // over it a refusal or Berserk
    font *Small = AppState->Fonts.Small;
    if (!Small)
    {
        return;
    }
    float Line = UILineHeight(Small);
    float TextY = Middle - 0.5f * Line - 1.f;
    UIText(RenderContext, Small, X - 10.f, TextY, (char *)"RAGE", UI_RGBA(235, 90, 80, 255), UIAlign_Right);
    char Text[16];
    snprintf(Text, sizeof(Text), "%u", (u32)Slot->ClassMeter);
    UIText(RenderContext, Small, X + Width + 10.f, TextY, Text,
           Full ? UI_RGBA(255, 220, 170, 255) : UI_RGBA(235, 210, 205, 255));
    float Over = Y - 6.f - Line;
    if (State->Refused > 0.f)
    {
        UIText(RenderContext, Small, 0.5f * (float)WindowWidth, Over - 6.f * (1.f - State->Refused),
               (char *)"Not enough Rage", UI_RGBA(255, 120, 100, (u32)(255.f * State->Refused)),
               UIAlign_Center);
    }
    else if (Berserk)
    {
        UIText(RenderContext, Small, 0.5f * (float)WindowWidth, Over, (char *)"BERSERK",
               UIMixColor(UI_RGBA(255, 120, 60, 255), UI_RGBA(255, 230, 170, 255), Beat), UIAlign_Center);
    }
}
