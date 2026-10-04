/* Heads-up display: always-on screen-space widgets drawn on top of the
   world each frame: player health and ability cooldowns. */

// NOTE(zoubir): Fraction is 0..1, filled from the left
internal void
DrawHudBar(render_context *RenderContext, float X, float Y,
           float Width, float Height, float Fraction, u32 FillColor)
{
    Fraction = Minimum(1.f, Maximum(0.f, Fraction));
    DrawFilledRectangle(RenderContext, X, Y, Width, Height,
                        UI_COLOR_TRACK, 0.f);
    DrawFilledRectangle(RenderContext, X, Y, Fraction * Width, Height,
                        FillColor, 0.f);
    DrawRectangle(RenderContext, X, Y, Width, Height,
                  UI_COLOR_BORDER, 0.f);
}

// NOTE(zoubir): the name the player chose, or "Player N" without one
internal void
GetPlayerName(app_state *AppState, u32 SlotIndex, char *Out, u32 OutSize)
{
    char *Name = AppState->Players[SlotIndex].Name;
    if (Name[0])
    {
        snprintf(Out, OutSize, "%s", Name);
    }
    else
    {
        snprintf(Out, OutSize, "Player %u", SlotIndex + 1);
    }
}

// NOTE(zoubir): each other live player's name, just above the health bar
// DrawEntity puts over the sprite
internal void
DrawPlayerLabels(render_context *RenderContext, app_state *AppState,
                 v3 CameraOffset)
{
    font *Font = AppState->Fonts.Small;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (SlotIndex == AppState->LocalPlayerIndex || !Slot->Active ||
            !Player || !Player->IsPresent || IsDeadPlayer(Player))
        {
            continue;
        }
        zas_texture_info *Info =
            &GetAssetInfo(&AppState->Assets, Player->Texture)->Texture;
        float SpriteTop = Player->Position.Y - CameraOffset.Y -
            Player->Position.Z - Info->Origin.Y * Player->Dimensions.Y;
        char Text[24];
        GetPlayerName(AppState, SlotIndex, Text, sizeof(Text));
        UIText(RenderContext, Font, Player->Position.X - CameraOffset.X,
               SpriteTop - 14.f - UILineHeight(Font), Text, UI_COLOR_TEXT,
               UIAlign_Center);
    }
}

// NOTE(zoubir): a cooldown bar flashes and swells for HUD_PULSE_SECONDS
// the moment its ability is ready again
#define HUD_PULSE_SECONDS 0.3f

struct hud_pulses
{
    bool32 WasReady[PLAYER_COOLDOWN_COUNT];
    float Pulse[PLAYER_COOLDOWN_COUNT];
};

internal void
DrawHud(render_context *RenderContext, app_state *AppState,
        v3 CameraOffset, float DeltaTime)
{
    if (!AppState->HudPulses)
    {
        AppState->HudPulses = AllocateStruct(&AppState->MemoryArena, hud_pulses);
        *AppState->HudPulses = {};
        // NOTE(zoubir): bars that start ready do not flash at launch
        for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
        {
            AppState->HudPulses->WasReady[Index] = true;
        }
    }
    hud_pulses *Pulses = AppState->HudPulses;
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || Player->MaxHp <= 0.f)
    {
        return;
    }

    // NOTE(zoubir): laid out top to bottom with a running Y
    float X = UI_GAP_LARGE;
    float Y = UI_GAP_LARGE;
    float Width = 200.f;
    DrawHudBar(RenderContext, X, Y, Width, 14.f,
               Player->Hp / Player->MaxHp, UI_COLOR_HEALTH);
    Y += 14.f + UI_GAP_SMALL;

    DrawPlayerLabels(RenderContext, AppState, CameraOffset);

    // Ability bars under the health bar, one per cooldown in
    // sim/player_cooldowns.cpp that has one, each with its key below it
    // (client/action_keys.cpp). A bar fills back up while recharging and
    // turns the accent colour when ready.
    u32 Shown[PLAYER_COOLDOWN_COUNT];
    u32 AbilityCount = 0;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
    {
        float Full;
        if (PlayerCooldown(Player, Index, &Full) && Full > 0.f)
        {
            Shown[AbilityCount++] = Index;
        }
    }
    float BarGap = 10.f;
    float BarWidth = (Width - BarGap * (AbilityCount - 1)) / AbilityCount;
    font *Small = AppState->Fonts.Small;
    for(u32 AbilityIndex = 0; AbilityIndex < AbilityCount; AbilityIndex++)
    {
        u32 Index = Shown[AbilityIndex];
        float Full;
        float *Seconds = PlayerCooldown(Player, Index, &Full);
        float Charge = 1.f - *Seconds / Full;
        bool32 Ready = Charge >= 1.f;
        if (Ready && !Pulses->WasReady[Index])
        {
            Pulses->Pulse[Index] = 1.f;
        }
        Pulses->WasReady[Index] = Ready;
        float Pulse = Pulses->Pulse[Index];
        Pulses->Pulse[Index] = Maximum(0.f, Pulse - DeltaTime / HUD_PULSE_SECONDS);
        float BarX = X + AbilityIndex * (BarWidth + BarGap);
        // NOTE(zoubir): the flash: the bar swells and a white frame fades
        float Swell = 3.f * Pulse;
        DrawHudBar(RenderContext, BarX - Swell, Y - Swell, BarWidth + 2.f * Swell,
                   6.f + 2.f * Swell, Charge,
                   Ready ? UI_COLOR_ACCENT : UI_COLOR_DIM);
        if (Pulse > 0.f)
        {
            u32 Flash = ((u32)(255.f * Pulse) << 24) | 0x00FFFFFF;
            DrawRectangle(RenderContext, BarX - Swell - 1.f, Y - Swell - 1.f,
                          BarWidth + 2.f * Swell + 2.f, 8.f + 2.f * Swell,
                          Flash, 0.f);
        }
        UIText(RenderContext, Small, BarX + 0.5f * BarWidth, Y + 8.f,
               ActionKeyLabel(PlayerCooldownButton(Index)),
               Ready ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED, UIAlign_Center);
    }
    Y += 8.f + UILineHeight(Small) + UI_GAP_SMALL;

    font *Font = AppState->Fonts.Body;
    char Text[64];
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    snprintf(Text, sizeof(Text), "Kills %u   Deaths %u   Monsters %u",
             Slot->Kills, Slot->Deaths, Slot->MonsterKills);
    UIText(RenderContext, Font, X, Y, Text, UI_COLOR_TEXT);
    Y += UILineHeight(Font) + 4.f;

    DrawConnectionIndicator(RenderContext, AppState, X, Y);
}
