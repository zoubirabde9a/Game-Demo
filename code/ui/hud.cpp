/* Heads-up display: always-on screen-space widgets drawn on top of the
   world each frame: other players' names, the score and the connection.
   Health and cooldowns are the ability bar (ability_bar.cpp). */

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
        // NOTE(zoubir): the world is drawn zoomed; the name is placed in
        // window pixels so its text stays sharp
        float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
        float SpriteTop = Player->Position.Y - CameraOffset.Y -
            Player->Position.Z - Info->Origin.Y * Player->Dimensions.Y;
        char Text[24];
        GetPlayerName(AppState, SlotIndex, Text, sizeof(Text));
        UIText(RenderContext, Font, Zoom * (Player->Position.X - CameraOffset.X),
               Zoom * (SpriteTop - 14.f) - UILineHeight(Font), Text, UI_COLOR_TEXT,
               UIAlign_Center);
    }
}

internal void
DrawHud(render_context *RenderContext, app_state *AppState,
        v3 CameraOffset, float DeltaTime)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || Player->MaxHp <= 0.f)
    {
        return;
    }
    DrawPlayerLabels(RenderContext, AppState, CameraOffset);

    // NOTE(zoubir): health and abilities are the ability bar at the bottom
    // (ability_bar.cpp); the top left has the score and the connection
    float X = UI_GAP_LARGE;
    float Y = UI_GAP_LARGE;
    font *Font = AppState->Fonts.Body;
    char Text[64];
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    snprintf(Text, sizeof(Text), "Kills %u   Deaths %u   Monsters %u",
             Slot->Kills, Slot->Deaths, Slot->MonsterKills);
    UIText(RenderContext, Font, X, Y, Text, UI_COLOR_TEXT);
    Y += UILineHeight(Font) + 4.f;

    DrawConnectionIndicator(RenderContext, AppState, X, Y);
}
