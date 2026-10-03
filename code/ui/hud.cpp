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

// NOTE(zoubir): expanding square outline around the player while the
// shockwave flash lasts, drawn in screen space on top of the world
internal void
DrawShockwaveRing(render_context *RenderContext, world_entity *Player,
                  v3 CameraOffset)
{
    if (Player->ShockwaveFlash <= 0.f)
    {
        return;
    }
    float Progress = 1.f - Player->ShockwaveFlash / SHOCKWAVE_FLASH_SECONDS;
    float Radius = SHOCKWAVE_RADIUS * (0.4f + 0.6f * Progress);
    v2 Center = Player->Position.XY - CameraOffset.XY;
    for(u32 RingIndex = 0; RingIndex < 3; RingIndex++)
    {
        float R = Radius - 3.f * RingIndex;
        DrawRectangle(RenderContext, Center.X - R, Center.Y - R,
                      2.f * R, 2.f * R, UI_COLOR_ACCENT, 0.f);
    }
}

// NOTE(zoubir): RenderText places the baseline at Y; this takes the top
internal void
DrawScreenText(render_context *RenderContext, font *Font, float X,
               float TopY, char *Text, u32 Color)
{
    v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
    RenderText(RenderContext, X, TopY + Font->UpperLimit, Font,
               RenderContext->TextureProgram, Text, Color,
               1.f, 1.f, NoClip, 0.f);
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
    font *Font = AppState->DefaultFont;
    if (!Font)
    {
        return;
    }
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
        float Width = GetTextWidth(Font, Text);
        float Height = Font->UpperLimit + Font->LowerLimit;
        DrawScreenText(RenderContext, Font,
                       Player->Position.X - CameraOffset.X - 0.5f * Width,
                       SpriteTop - 14.f - Height, Text, UI_COLOR_TEXT);
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

    DrawShockwaveRing(RenderContext, Player, CameraOffset);
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

    font *Font = AppState->DefaultFont;
    if (Font)
    {
        char Text[64];
        player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
        snprintf(Text, sizeof(Text), "Kills %u   Deaths %u   Monsters %u",
                 Slot->Kills, Slot->Deaths, Slot->MonsterKills);
        v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
        // NOTE(zoubir): Y is the baseline, so drop it by the font ascent
        RenderText(RenderContext, X, Y + 32.f + Font->UpperLimit, Font,
                   RenderContext->TextureProgram, Text, UI_COLOR_TEXT,
                   1.f, 1.f, NoClip, 0.f);

        GetOnlineStatusText(AppState->Online, Text, sizeof(Text));
        if (Text[0])
        {
            RenderText(RenderContext, X, Y + 60.f + Font->UpperLimit, Font,
                       RenderContext->TextureProgram, Text, UI_COLOR_TEXT,
                       1.f, 1.f, NoClip, 0.f);
        }
    }
}
