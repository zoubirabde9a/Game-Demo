/* Ability bar: the player's health and abilities, centred at the bottom of
   the screen. Each slot shows its icon (ability_icons/), its key, and
   while recharging a dark clock sweep with the seconds left. A slot that
   comes ready flashes a ring of light; pressing a key squeezes its slot,
   and pressing one still recharging shakes it red. Hovering a slot names
   the ability and its cooldown.

   The slots and their order are icon_list.inc. Cooldowns come from
   sim/player_cooldowns.cpp, which online play fills from the server, so
   the bar is the same offline and online. Looks are in the shaders the
   bar draws with (build/shaders/fx/slot_frame.frag, cooldown_sweep.frag,
   ring.frag, glow.frag, panel.frag) and the sizes below. */

#define ABILITY_SLOT_SIZE 50.f
#define ABILITY_SLOT_GAP 8.f
#define ABILITY_GROUP_GAP 22.f
#define ABILITY_BAR_BOTTOM 26.f
#define ABILITY_PLATE_PAD 12.f
// NOTE(zoubir): slot_frame.frag fills this share of its quad with the slot
// and leaves the rest for the glow
#define ABILITY_SLOT_BOX 0.78f
#define ABILITY_HEALTH_HEIGHT 18.f
#define ABILITY_PULSE_SECONDS 0.45f
#define ABILITY_PRESS_SECONDS 0.16f
#define ABILITY_DENIED_SECONDS 0.3f
// NOTE(zoubir): the red trail on the health bar catches up at this share
// of the gap per second
#define ABILITY_HEALTH_TRAIL_RATE 3.f

struct ability_bar
{
    u32 Atlas;
    bool32 WasReady[ABILITY_SLOT_DEF_COUNT];
    float Pulse[ABILITY_SLOT_DEF_COUNT];  // 1 when it came ready, fading to 0
    float Press[ABILITY_SLOT_DEF_COUNT];  // 1 when pressed, fading to 0
    float Denied[ABILITY_SLOT_DEF_COUNT]; // 1 when pressed while recharging
    float HealthTrail;                    // share of health the trail shows
};

// NOTE(zoubir): share of Button's cooldown still to run (0 = ready) and
// its seconds; when two cooldowns share a button the longer one counts.
// False when the button has no cooldown at all
internal bool32
AbilityCooldownLeft(world_entity *Player, u32 Button, float *Share, float *Seconds,
                    float *FullOut = 0)
{
    bool32 Result = false;
    *Share = 0.f;
    *Seconds = 0.f;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        float *Left = PlayerCooldown(Player, Index, &Full);
        if (Left && Full > 0.f && PlayerCooldownButton(Index) == Button)
        {
            Result = true;
            float IndexShare = Maximum(0.f, Minimum(1.f, *Left / Full));
            if (IndexShare >= *Share)
            {
                *Share = IndexShare;
                *Seconds = Maximum(0.f, *Left);
                if (FullOut)
                {
                    *FullOut = Full;
                }
            }
        }
    }
    return Result;
}

inline float
AbilityBarWidth()
{
    float Width = 0.f;
    bool32 First = true;
    for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
    {
        if (!AbilitySlotDefs[Index].Paint)
        {
            Width += ABILITY_GROUP_GAP - ABILITY_SLOT_GAP;
            continue;
        }
        Width += (First ? 0.f : ABILITY_SLOT_GAP) + ABILITY_SLOT_SIZE;
        First = false;
    }
    return Width;
}

// NOTE(zoubir): health above the slots: a red bar, a pale trail that
// drains after a hit so the size of the hit can be read, and the numbers
internal void
DrawAbilityBarHealth(render_context *RenderContext, app_state *AppState,
                     ability_bar *Bar, world_entity *Player,
                     float X, float Y, float Width, float DeltaTime)
{
    float Share = Player->MaxHp > 0.f ?
        Maximum(0.f, Minimum(1.f, Player->Hp / Player->MaxHp)) : 0.f;
    if (Share >= Bar->HealthTrail)
    {
        Bar->HealthTrail = Share;
    }
    else
    {
        Bar->HealthTrail = Maximum(Share, Bar->HealthTrail -
                                   ABILITY_HEALTH_TRAIL_RATE * DeltaTime *
                                   Maximum(0.08f, Bar->HealthTrail - Share));
    }
    float Height = ABILITY_HEALTH_HEIGHT;
    DrawFilledRectangle(RenderContext, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f,
                        UI_RGBA(6, 7, 10, 200), 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Width, Height, UI_RGBA(40, 18, 18, 230), 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Bar->HealthTrail * Width, Height,
                        UI_RGBA(255, 214, 190, 220), 0.f);
    u32 Fill = Share > 0.3f ? UI_COLOR_HEALTH : UI_RGBA(255, 70, 50, 255);
    DrawFilledRectangle(RenderContext, X, Y, Share * Width, Height, Fill, 0.f);
    // NOTE(zoubir): a lighter top half makes the bar look rounded
    DrawFilledRectangle(RenderContext, X, Y, Share * Width, 0.4f * Height,
                        UI_RGBA(255, 255, 255, 48), 0.f);
    char Text[32];
    snprintf(Text, sizeof(Text), "%d / %d", (int)(Player->Hp + 0.5f),
             (int)(Player->MaxHp + 0.5f));
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Small, X + 0.5f * Width,
           Y + 0.5f * (Height - UILineHeight(Small)) - 1.f, Text, UI_COLOR_TEXT,
           UIAlign_Center);
}

// NOTE(zoubir): the top of the glass plate behind health and slots; other
// panels stay above it (controls_panel.cpp)
internal float
AbilityBarPlateTop(u32 WindowHeight)
{
    float SlotTop = (float)WindowHeight - ABILITY_BAR_BOTTOM - ABILITY_SLOT_SIZE;
    float HealthY = SlotTop - 18.f - ABILITY_HEALTH_HEIGHT;
    return HealthY - ABILITY_PLATE_PAD;
}

internal void
DrawAbilityBar(render_context *RenderContext, app_state *AppState, app_input *Input,
               u32 WindowWidth, u32 WindowHeight)
{
    world_entity *Player = GetLocalPlayer(AppState);
    // NOTE(zoubir): the connect screen covers the middle of the screen
    if (!Player || Player->MaxHp <= 0.f || ConnectScreenTakesInput(AppState))
    {
        return;
    }
    if (!AppState->AbilityBar)
    {
        ability_bar *New = AllocateStruct(&AppState->MemoryArena, ability_bar);
        *New = {};
        New->Atlas = BuildAbilityIconAtlas(RenderContext->OpenGL, RenderContext->Arena);
        New->HealthTrail = 1.f;
        for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
        {
            New->WasReady[Index] = true;
        }
        AppState->AbilityBar = New;
    }
    ability_bar *Bar = AppState->AbilityBar;
    float DeltaTime = Input->DeltaTime;
    float Time = RenderContext->Time;
    bool32 Dead = IsDeadPlayer(Player);
    // NOTE(zoubir): the frame's presses as the player made them, after the
    // tile editor's left button stopped being a cast (keyboard_input.cpp)
    u32 Pressed = Dead ? 0 : AppState->Players[AppState->LocalPlayerIndex].Input.Pressed;

    float Width = AbilityBarWidth();
    float Left = 0.5f * ((float)WindowWidth - Width);
    float SlotTop = (float)WindowHeight - ABILITY_BAR_BOTTOM - ABILITY_SLOT_SIZE;
    float HealthY = SlotTop - 18.f - ABILITY_HEALTH_HEIGHT;

    // NOTE(zoubir): one glass plate behind health and slots
    float PlatePad = ABILITY_PLATE_PAD;
    float PlateTop = AbilityBarPlateTop(WindowHeight);
    float PlateHeight = (float)WindowHeight - ABILITY_BAR_BOTTOM + PlatePad - PlateTop;
    float PlateWidth = Width + 2.f * PlatePad;
    DrawUIPanel(RenderContext, Left - PlatePad, PlateTop, PlateWidth, PlateHeight);
    DrawAbilityBarHealth(RenderContext, AppState, Bar, Player, Left, HealthY, Width,
                         DeltaTime);

    font *Small = AppState->Fonts.Small;
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    float QuadSize = ABILITY_SLOT_SIZE / ABILITY_SLOT_BOX;
    float X = Left;
    bool32 First = true;
    i32 Hovered = -1;
    float HoveredX = 0.f;
    for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
    {
        ability_slot_def *Def = &AbilitySlotDefs[Index];
        if (!Def->Paint)
        {
            X += ABILITY_GROUP_GAP - ABILITY_SLOT_GAP;
            continue;
        }
        X += First ? 0.f : ABILITY_SLOT_GAP;
        First = false;
        float SlotX = X;
        X += ABILITY_SLOT_SIZE;

        float Share, Seconds;
        bool32 HasCooldown = AbilityCooldownLeft(Player, Def->Button, &Share, &Seconds);
        bool32 Ready = Share <= 0.f;
        if (Ready && !Bar->WasReady[Index])
        {
            Bar->Pulse[Index] = 1.f;
        }
        Bar->WasReady[Index] = Ready;
        if (Pressed & Def->Button)
        {
            if (Ready)
            {
                Bar->Press[Index] = 1.f;
            }
            else
            {
                Bar->Denied[Index] = 1.f;
            }
        }
        float Pulse = Bar->Pulse[Index];
        float Press = Bar->Press[Index];
        float Denied = Bar->Denied[Index];
        Bar->Pulse[Index] = Maximum(0.f, Pulse - DeltaTime / ABILITY_PULSE_SECONDS);
        Bar->Press[Index] = Maximum(0.f, Press - DeltaTime / ABILITY_PRESS_SECONDS);
        Bar->Denied[Index] = Maximum(0.f, Denied - DeltaTime / ABILITY_DENIED_SECONDS);

        // NOTE(zoubir): squeeze on a press, shake when refused
        float Scale = 1.f - 0.1f * Press;
        float Shake = 4.f * Denied * Sin(Time * 70.f);
        float CentreX = SlotX + 0.5f * ABILITY_SLOT_SIZE + Shake;
        float CentreY = SlotTop + 0.5f * ABILITY_SLOT_SIZE;
        float Quad = QuadSize * Scale;
        float Slot = ABILITY_SLOT_SIZE * Scale;

        // NOTE(zoubir): abilities with no cooldown keep a quiet frame; the
        // glow is the signal that something came back
        float Readiness = Dead ? 0.f : (HasCooldown ? (Ready ? 1.f : 0.f) : 0.3f);
        u32 Accent = Denied > 0.f ? UI_RGBA(255, 60, 50, 255) : Def->Accent;
        DrawShaderQuad(RenderContext, Shader_SlotFrame, CentreX - 0.5f * Quad,
                       CentreY - 0.5f * Quad, Quad, Quad, WithAlpha(Accent, Readiness));

        float Icon = 0.8f * Slot;
        u32 IconTint = (Ready && !Dead) ? 0xFFFFFFFF : UI_RGBA(150, 150, 160, 255);
        DrawTexturedQuad(RenderContext, Bar->Atlas, CentreX - 0.5f * Icon,
                         CentreY - 0.5f * Icon, Icon, Icon, AbilityIconUvs(Index), IconTint);
        if (Press > 0.f)
        {
            DrawShaderQuad(RenderContext, Shader_Glow, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(Def->Accent, 0.5f * Press), RenderBlend_Additive);
        }
        if (!Ready)
        {
            DrawShaderQuad(RenderContext, Shader_CooldownSweep, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(UI_RGBA(255, 244, 210, 255), Share));
            char Text[16];
            if (Seconds < 1.f)
            {
                snprintf(Text, sizeof(Text), "%.1f", Seconds);
            }
            else
            {
                snprintf(Text, sizeof(Text), "%d", (int)(Seconds + 0.95f));
            }
            UIText(RenderContext, Strong, CentreX,
                   CentreY - 0.5f * UILineHeight(Strong), Text, UI_COLOR_TEXT,
                   UIAlign_Center);
        }
        if (Pulse > 0.f)
        {
            // NOTE(zoubir): came ready: a ring flies out and the slot flares
            float Ring = ABILITY_SLOT_SIZE * (1.f + 0.9f * (1.f - Pulse));
            DrawShaderQuad(RenderContext, Shader_Ring, CentreX - 0.5f * Ring,
                           CentreY - 0.5f * Ring, Ring, Ring,
                           WithAlpha(Def->Accent, Pulse), RenderBlend_Additive);
            DrawShaderQuad(RenderContext, Shader_Glow, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(Def->Accent, 0.7f * Pulse), RenderBlend_Additive);
        }

        // NOTE(zoubir): the key on a dark tab over the slot's top-left
        // corner, so it never sits on the icon's colours
        char *Key = ActionKeyLabel(Def->Button);
        float KeyWidth = UITextWidth(Small, Key) + 8.f;
        float KeyHeight = UILineHeight(Small);
        float KeyX = CentreX - 0.5f * Slot - 3.f;
        float KeyY = CentreY - 0.5f * Slot - 0.45f * KeyHeight;
        DrawFilledRectangle(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                            UI_RGBA(8, 9, 14, 220), 0.f);
        DrawRectangle(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                      WithAlpha(Def->Accent, Ready ? 0.7f : 0.25f), 0.f);
        UIText(RenderContext, Small, KeyX + 4.f, KeyY, Key,
               Ready ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);

        if (IsMouseOnRectangle(Input->MouseX, Input->MouseY, SlotX, SlotTop,
                               ABILITY_SLOT_SIZE, ABILITY_SLOT_SIZE))
        {
            Hovered = (i32)Index;
            HoveredX = CentreX;
        }
    }

    if (Hovered >= 0)
    {
        ability_slot_def *Def = &AbilitySlotDefs[Hovered];
        float Share, Seconds, Full = 0.f;
        char Text[64];
        if (AbilityCooldownLeft(Player, Def->Button, &Share, &Seconds, &Full))
        {
            snprintf(Text, sizeof(Text), "%s  (%s)  %.1f s cooldown", Def->Name,
                     ActionKeyLabel(Def->Button), Full);
        }
        else
        {
            snprintf(Text, sizeof(Text), "%s  (%s)", Def->Name, ActionKeyLabel(Def->Button));
        }
        font *Body = AppState->Fonts.Body;
        float TipWidth = UITextWidth(Body, Text) + 24.f;
        float TipHeight = UILineHeight(Body) + 12.f;
        float TipX = Maximum(8.f, HoveredX - 0.5f * TipWidth);
        float TipY = PlateTop - TipHeight - 8.f;
        DrawUIPanel(RenderContext, TipX, TipY, TipWidth, TipHeight, Def->Accent);
        UIText(RenderContext, Body, TipX + 12.f, TipY + 6.f, Text, UI_COLOR_TEXT);
    }
}
