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

internal void
DrawHud(render_context *RenderContext, app_state *AppState,
        v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || Player->MaxHp <= 0.f)
    {
        return;
    }

    float X = 20.f;
    float Y = 20.f;
    DrawHudBar(RenderContext, X, Y, 200.f, 14.f,
               Player->Hp / Player->MaxHp, UI_COLOR_HEALTH);

    DrawPlayerLabels(RenderContext, AppState, CameraOffset);

    // Ability bars, left to right: dash (Alt), shockwave (E). Each fills
    // back up while recharging and turns yellow when ready.
    float Cooldowns[] =
        {
            Player->DashCooldown / PLAYER_DASH_COOLDOWN,
            Player->ShockwaveCooldown / PLAYER_SHOCKWAVE_COOLDOWN,
        };
    for(u32 AbilityIndex = 0;
        AbilityIndex < ArrayCount(Cooldowns);
        AbilityIndex++)
    {
        float Charge = 1.f - Cooldowns[AbilityIndex];
        DrawHudBar(RenderContext, X + AbilityIndex * 70.f, Y + 20.f,
                   60.f, 6.f, Charge,
                   Charge >= 1.f ?
                   UI_COLOR_ACCENT : UI_COLOR_DIM);
    }

    font *Font = AppState->Fonts.Body;
    char Text[64];
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    snprintf(Text, sizeof(Text), "Kills %u   Deaths %u   Monsters %u",
             Slot->Kills, Slot->Deaths, Slot->MonsterKills);
    UIText(RenderContext, Font, X, Y + 32.f, Text, UI_COLOR_TEXT);

    GetOnlineStatusText(AppState->Online, Text, sizeof(Text));
    if (Text[0])
    {
        UIText(RenderContext, Font, X, Y + 32.f + UILineHeight(Font) + 4.f,
               Text, UI_COLOR_TEXT_MUTED);
    }
}
