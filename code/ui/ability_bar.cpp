/* Ability bar: the player's health, experience and abilities, centred at
   the bottom of the screen. Each slot shows its icon (ability_icons/), its
   key, its level as pips along its foot, and while recharging a dark
   clock sweep with the seconds left. Only abilities the player has show:
   one the talent tree unlocks (sim/progression/talents.cpp) appears with
   a flash. A slot that comes ready flashes a ring of light; pressing a
   key squeezes its slot, and pressing one still recharging shakes it red.
   A cooldown cut short (a kill's refund) pulls a ring in and flashes.
   Hovering a slot names the ability, its level and its cooldown.

   Health is a bar over the slots (ability_health.cpp). Experience runs
   as a thin strip between health and the slots, with the
   level badge left of the plate and the talent button right of it
   (talent_panel/xp_bar.cpp).

   The slots and their order are icon_list.inc. Cooldowns come from
   sim/player_cooldowns.cpp, which online play fills from the server, so
   the bar is the same offline and online. Looks are in the shaders the
   bar draws with (build/shaders/fx/slot_frame.frag, cooldown_sweep.frag,
   ring.frag, glow.frag, panel.frag) and the sizes below. */

#define ABILITY_SLOT_SIZE 48.f
#define ABILITY_SLOT_GAP 7.f
#define ABILITY_GROUP_GAP 18.f
#define ABILITY_BAR_BOTTOM 16.f
#define ABILITY_PLATE_PAD 9.f
// NOTE(zoubir): slot_frame.frag fills this share of its quad with the slot
// and leaves the rest for the glow
#define ABILITY_SLOT_BOX 0.78f
// NOTE(zoubir): from the health bar's foot to the slots' top, the XP
// strip in the middle
#define ABILITY_HEALTH_GAP 20.f
#define ABILITY_PULSE_SECONDS 0.45f
#define ABILITY_PRESS_SECONDS 0.16f
#define ABILITY_DENIED_SECONDS 0.3f
#define ABILITY_REFUND_SECONDS 0.5f
// NOTE(zoubir): a cooldown share dropping this much in one frame was cut
// short, not run down; a frame at 10 fps runs a 1 s cooldown down 0.1.
// One cut all the way to ready gets the ready ring instead
#define ABILITY_REFUND_DROP 0.12f

struct ability_bar
{
    u32 Atlas;
    bool32 WasReady[ABILITY_SLOT_DEF_COUNT];
    float Pulse[ABILITY_SLOT_DEF_COUNT];  // 1 when it came ready, fading to 0
    float Press[ABILITY_SLOT_DEF_COUNT];  // 1 when pressed, fading to 0
    float Denied[ABILITY_SLOT_DEF_COUNT]; // 1 when pressed while recharging
    float Refund[ABILITY_SLOT_DEF_COUNT]; // 1 when its cooldown was cut short
    float LastShare[ABILITY_SLOT_DEF_COUNT]; // cooldown share last frame
    health_bar Health;                    // the bar's trail and glide, ability_health.cpp
    bool32 WasShown[ABILITY_SLOT_DEF_COUNT]; // the player had it last frame
    bool32 ShownSeen; // WasShown holds a real frame, so a new slot is an unlock
};

// NOTE(zoubir): share of Button's cooldown still to run (0 = ready) and
// its seconds; when two cooldowns share a button the longer one counts.
// False when the button has no cooldown at all
internal bool32
AbilityCooldownLeft(app_state *AppState, world_entity *Player, u32 Button,
                    float *Share, float *Seconds,
                    float *FullOut = 0)
{
    bool32 Result = false;
    *Share = 0.f;
    *Seconds = 0.f;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        float *Left = PlayerCooldown(AppState, Player, Index, &Full);
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

// NOTE(zoubir): the room before the next shown slot: none for the first,
// a group's gap after a gap entry, the slot gap otherwise. First and Gap
// carry from one call to the next across the list
inline float
AbilitySlotSpacing(bool32 *First, bool32 *Gap)
{
    float Result = *First ? 0.f : (*Gap ? ABILITY_GROUP_GAP : ABILITY_SLOT_GAP);
    *First = false;
    *Gap = false;
    return Result;
}

inline float
AbilityBarWidth(app_state *AppState, player_slot *Slot)
{
    float Width = 0.f;
    bool32 First = true;
    bool32 Gap = false;
    for(u32 Position = 0; Position < AbilitySlotCount(AppState); Position++)
    {
        ability_slot_def *Def = &AbilitySlotDefs[AbilitySlotIndex(AppState, Position)];
        if (!Def->Paint)
        {
            Gap = true;
            continue;
        }
        if (IsAbilitySlotShown(AppState, Slot, Def))
        {
            Width += AbilitySlotSpacing(&First, &Gap) + ABILITY_SLOT_SIZE;
        }
    }
    return Width;
}

// NOTE(zoubir): the top of the glass plate behind health and slots; other
// panels stay above it (controls_panel.cpp)
internal float
AbilityBarPlateTop(u32 WindowHeight)
{
    float SlotTop = (float)WindowHeight - ABILITY_BAR_BOTTOM - ABILITY_SLOT_SIZE;
    float HealthY = SlotTop - ABILITY_HEALTH_GAP - ABILITY_HEALTH_HEIGHT;
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
        New->Health = {1.f, 1.f};
        for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
        {
            New->WasReady[Index] = true;
        }
        AppState->AbilityBar = New;
    }
    ability_bar *Bar = AppState->AbilityBar;
    player_slot *LocalSlot = &AppState->Players[AppState->LocalPlayerIndex];
    xp_bar *Xp = GetXpBar(AppState);
    TrackExperience(AppState, Xp, Input->DeltaTime);
    float DeltaTime = Input->DeltaTime;
    float Time = RenderContext->Time;
    bool32 Dead = IsDeadPlayer(Player);
    // NOTE(zoubir): the frame's presses as the player made them, after the
    // tile editor's left button stopped being a cast (keyboard_input.cpp)
    u32 Pressed = Dead ? 0 : AppState->Players[AppState->LocalPlayerIndex].Input.Pressed;

    // NOTE(zoubir): never narrower than this, so health and experience
    // stay readable with few abilities
    float Width = Maximum(AbilityBarWidth(AppState, LocalSlot), 5.f * ABILITY_SLOT_SIZE);
    float Left = 0.5f * ((float)WindowWidth - Width);
    float SlotTop = (float)WindowHeight - ABILITY_BAR_BOTTOM - ABILITY_SLOT_SIZE;
    float HealthY = SlotTop - ABILITY_HEALTH_GAP - ABILITY_HEALTH_HEIGHT;

    // NOTE(zoubir): one glass plate behind health and slots
    float PlatePad = ABILITY_PLATE_PAD;
    float PlateTop = AbilityBarPlateTop(WindowHeight);
    float PlateHeight = (float)WindowHeight - ABILITY_BAR_BOTTOM + PlatePad - PlateTop;
    float PlateWidth = Width + 2.f * PlatePad;
    DrawUIPanel(RenderContext, Left - PlatePad, PlateTop, PlateWidth, PlateHeight);
    DrawHealthBar(RenderContext, AppState, &Bar->Health, Player, Left, HealthY, Width,
                  DeltaTime);
    // NOTE(zoubir): the strip hugs the health bar's foot, the rest of the
    // gap goes above the slots' key tabs
    DrawXpStrip(RenderContext, AppState, Left, HealthY + ABILITY_HEALTH_HEIGHT + 3.f, Width);
    Xp->PlateX = Left - PlatePad;
    Xp->PlateY = PlateTop;
    Xp->PlateWidth = PlateWidth;
    Xp->PlateHeight = PlateHeight;
    float PlateMiddle = PlateTop + 0.5f * PlateHeight;
    DrawLevelBadge(RenderContext, AppState, Left - PlatePad - 12.f - 0.5f * XP_BADGE_SIZE,
                   PlateMiddle);
    talent_panel *TalentPanel = GetTalentPanel(AppState);
    if (DrawTalentPointsButton(RenderContext, AppState, Input,
                               Left + Width + PlatePad + 12.f, PlateMiddle,
                               TalentPanel->Open))
    {
        ToggleTalentPanel(AppState);
    }

    font *Small = AppState->Fonts.Small;
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    float QuadSize = ABILITY_SLOT_SIZE / ABILITY_SLOT_BOX;
    float X = Left + 0.5f * (Width - AbilityBarWidth(AppState, LocalSlot));
    bool32 First = true;
    bool32 Gap = false;
    i32 Hovered = -1;
    float HoveredX = 0.f;
    for(u32 Position = 0; Position < AbilitySlotCount(AppState); Position++)
    {
        u32 Index = AbilitySlotIndex(AppState, Position);
        ability_slot_def *Def = &AbilitySlotDefs[Index];
        if (!Def->Paint)
        {
            Gap = true;
            continue;
        }
        bool32 Shown = IsAbilitySlotShown(AppState, LocalSlot, Def);
        if (Shown && !Bar->WasShown[Index])
        {
            // NOTE(zoubir): just unlocked: it arrives with the ready flash
            // and a word over the bar
            Bar->Pulse[Index] = 1.f;
            if (Bar->ShownSeen)
            {
                ShowUnlockToast(AppState, Def->Name, ActionKeyLabel(Def->Button), Def->Accent);
            }
        }
        Bar->WasShown[Index] = Shown;
        if (!Shown)
        {
            continue;
        }
        X += AbilitySlotSpacing(&First, &Gap);
        float SlotX = X;
        X += ABILITY_SLOT_SIZE;

        float Share, Seconds;
        bool32 HasCooldown = AbilityCooldownLeft(AppState, Player, Def->Button, &Share, &Seconds);
        bool32 Ready = Share <= 0.f;
        if (Ready && !Bar->WasReady[Index])
        {
            Bar->Pulse[Index] = 1.f;
        }
        Bar->WasReady[Index] = Ready;
        if (!Ready && Bar->LastShare[Index] - Share >= ABILITY_REFUND_DROP)
        {
            Bar->Refund[Index] = 1.f;
        }
        Bar->LastShare[Index] = Share;
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
        float Refund = Bar->Refund[Index];
        Bar->Refund[Index] = Maximum(0.f, Refund - DeltaTime / ABILITY_REFUND_SECONDS);

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
        // NOTE(zoubir): a dungeon role's spell on the key shows its own
        // icon (ui/dungeon/role_icons.cpp)
        u32 Cell = Index;
        if (RoleSpellOnButton(AppState, Player, Def->Button))
        {
            u32 RoleIcon = RoleIconIndex(LocalSlot->Role, RoleKeyForButton(Def->Button));
            Cell = RoleIcon < ROLE_ICON_COUNT ? ABILITY_SLOT_DEF_COUNT + RoleIcon : Index;
        }
        DrawTexturedQuad(RenderContext, Bar->Atlas, CentreX - 0.5f * Icon,
                         CentreY - 0.5f * Icon, Icon, Icon, AbilityIconUvs(Cell), IconTint);
        if (Press > 0.f)
        {
            DrawShaderQuad(RenderContext, Shader_Glow, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(Def->Accent, 0.5f * Press), RenderBlend_Additive);
        }
        // NOTE(zoubir): the ability standard cast is aiming
        // (client/cast_targeting.cpp) glows until it is cast or cancelled
        if (AppState->CastTargeting && AppState->CastTargeting->Aiming == Def->Button)
        {
            DrawShaderQuad(RenderContext, Shader_Glow, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(Def->Accent, 0.4f + 0.15f * Sin(Time * 8.f)),
                           RenderBlend_Additive);
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
        if (Refund > 0.f)
        {
            // NOTE(zoubir): cut short: a pale ring closes in on the slot,
            // the way the ready ring flies out
            float Ring = ABILITY_SLOT_SIZE * (1.f + 0.9f * Refund);
            u32 Pale = UI_RGBA(210, 255, 220, 255);
            DrawShaderQuad(RenderContext, Shader_Ring, CentreX - 0.5f * Ring,
                           CentreY - 0.5f * Ring, Ring, Ring,
                           WithAlpha(Pale, Refund), RenderBlend_Additive);
            DrawShaderQuad(RenderContext, Shader_Glow, CentreX - 0.5f * Quad,
                           CentreY - 0.5f * Quad, Quad, Quad,
                           WithAlpha(Pale, 0.5f * Refund), RenderBlend_Additive);
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
        DrawRoundRect(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                      UI_RGBA(12, 13, 20, 235));
        DrawRoundOutline(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                         WithAlpha(Def->Accent, Ready ? 0.7f : 0.25f));
        UIText(RenderContext, Small, KeyX + 4.f, KeyY, Key,
               Ready ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);

        // NOTE(zoubir): the level as diamonds along the slot's foot, gold
        // for each level held (sim/progression/talents.cpp)
        u32 Talent = TalentForButton(Def->Button);
        if (Talent < Talent_Count)
        {
            u32 MaxLevel = TalentDefs[Talent].MaxLevel;
            u32 Level = TalentLevel(LocalSlot, Talent);
            float Pip = 6.f;
            float PipGap = 3.f;
            float PipsWidth = (float)MaxLevel * Pip + (float)(MaxLevel - 1) * PipGap;
            float PipY = CentreY + 0.5f * Slot - 1.f;
            u32 Under = UI_RGBA(6, 7, 10, 230);
            for(u32 Rank = 0; Rank < MaxLevel; Rank++)
            {
                v2 P = V2(CentreX - 0.5f * PipsWidth + (float)Rank * (Pip + PipGap) + 0.5f * Pip,
                          PipY);
                u32 Color = Rank < Level ? UI_RGBA(255, 206, 90, 255) : UI_RGBA(20, 22, 30, 240);
                float H = 0.5f * Pip + 1.f;
                DrawFilledQuad(RenderContext, P - V2(0.f, H), P + V2(H, 0.f), P + V2(0.f, H),
                               P - V2(H, 0.f), Under, Under, Under, Under, RenderBlend_Alpha);
                H = 0.5f * Pip;
                DrawFilledQuad(RenderContext, P - V2(0.f, H), P + V2(H, 0.f), P + V2(0.f, H),
                               P - V2(H, 0.f), Color, Color, Color, Color, RenderBlend_Alpha);
            }
        }

        // NOTE(zoubir): a point can go into it: a gold plus on its
        // top-right corner, and a click opens the tree on it. Not on a
        // key a dungeon role has taken: its talent is the game's ability's
        bool32 SlotHot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, SlotX, SlotTop,
                                            ABILITY_SLOT_SIZE, ABILITY_SLOT_SIZE);
        if (Talent < Talent_Count && CanLearnTalent(LocalSlot, Talent) == TalentRefusal_None &&
            !RoleSpellOnButton(AppState, Player, Def->Button))
        {
            float Beat = 0.5f + 0.5f * Sin(5.f * Time + (float)Index);
            float BadgeX = CentreX + 0.5f * Slot - 4.f;
            float BadgeY = CentreY - 0.5f * Slot + 4.f;
            float Glow = 26.f;
            DrawShaderQuad(RenderContext, Shader_Glow, BadgeX - 0.5f * Glow, BadgeY - 0.5f * Glow,
                           Glow, Glow, WithAlpha(XP_COLOR, 0.5f + 0.4f * Beat),
                           RenderBlend_Additive);
            DrawFilledRectangle(RenderContext, BadgeX - 7.f, BadgeY - 7.f, 14.f, 14.f,
                                UI_RGBA(40, 28, 6, 240), 0.f);
            DrawRectangle(RenderContext, BadgeX - 7.f, BadgeY - 7.f, 14.f, 14.f, XP_COLOR, 0.f);
            DrawFilledRectangle(RenderContext, BadgeX - 4.f, BadgeY - 1.f, 8.f, 2.f,
                                UI_RGBA(255, 230, 150, 255), 0.f);
            DrawFilledRectangle(RenderContext, BadgeX - 1.f, BadgeY - 4.f, 2.f, 8.f,
                                UI_RGBA(255, 230, 150, 255), 0.f);
            if (SlotHot && Input->LeftButton.Pressed)
            {
                ShowTalentInPanel(AppState, Talent);
            }
        }
        if (SlotHot)
        {
            Hovered = (i32)Index;
            HoveredX = CentreX;
        }
    }
    Bar->ShownSeen = true;
    // NOTE(zoubir): clicks on the plate are the bar's, not a cast
    // (client/talent_requests.cpp)
    talent_requests *Requests = GetTalentRequests(AppState);
    Requests->BarX = Left - PlatePad - 12.f - XP_BADGE_SIZE;
    Requests->BarY = PlateTop;
    Requests->BarWidth = PlateWidth + 2.f * (12.f + XP_BADGE_SIZE);
    Requests->BarHeight = PlateHeight;

    if (Hovered >= 0)
    {
        ability_slot_def *Def = &AbilitySlotDefs[Hovered];
        float Share, Seconds, Full = 0.f;
        char Text[96];
        char Level[24] = "";
        u32 Talent = TalentForButton(Def->Button);
        if (Talent < Talent_Count)
        {
            snprintf(Level, sizeof(Level), "  level %u/%u", TalentLevel(LocalSlot, Talent),
                     TalentDefs[Talent].MaxLevel);
        }
        // NOTE(zoubir): a dungeon role's spell on the key (sim/dungeon/
        // role_abilities.cpp) goes by its own name
        role_spell *RoleSpell = RoleSpellOnButton(AppState, Player, Def->Button);
        char *Name = RoleSpell ? RoleSpell->Name : Def->Name;
        if (RoleSpell)
        {
            Level[0] = 0;
        }
        if (AbilityCooldownLeft(AppState, Player, Def->Button, &Share, &Seconds, &Full))
        {
            snprintf(Text, sizeof(Text), "%s  (%s)%s  %.1f s cooldown", Name,
                     ActionKeyLabel(Def->Button), Level, Full);
        }
        else
        {
            snprintf(Text, sizeof(Text), "%s  (%s)%s", Name, ActionKeyLabel(Def->Button),
                     Level);
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
