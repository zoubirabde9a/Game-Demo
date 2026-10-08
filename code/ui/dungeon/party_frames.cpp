/* Party frames (dungeon_hud.cpp): one plate per player of a dungeon run,
   stacked up the left edge from above the controls hint, the local
   player at the bottom. Each shows:

   - the role's emblem (a shield, a cross, a flame) in its colour and
     the name;
   - health as a bar and as numbers, green to amber to red as it drops;
     a healer's ward on top of it as a pale band past the health, and a
     tank's Shield Wall or rally as a steel rim round the bar;
   - a red pulse round the plate and a count while monsters are after
     that player (a tank's count is drawn calm, holding them is its job);
   - a downed player greyed out, with the revive filling across the bar;
   - an ally out of a healer's reach dimmed.

   The plates are buttons: the mouse on one makes it the target of the
   local tank's or healer's ally spells, a click picks it until clicked
   again (client/dungeon/role_targeting.cpp keeps both), and the plate
   those spells would land on wears a gold rim. Each plate's place goes
   to party_pick each frame, which the input reads the next frame. */

#define PARTY_FRAME_WIDTH 224.f
#define PARTY_FRAME_HEIGHT 46.f
#define PARTY_FRAME_COMPACT 34.f
#define PARTY_FRAME_GAP 5.f
#define PARTY_FRAME_MARGIN 14.f
// NOTE(zoubir): room left under the frames for the controls hint
#define PARTY_FRAME_BOTTOM 44.f
#define PARTY_BAR_HEIGHT 12.f
#define PARTY_EMBLEM 26.f

// NOTE(zoubir): each class's colour on the frames (roles.cpp)
inline u32
RoleUIColor(u32 Role)
{
    u8 *C = GetRoleDef(Role)->Color;
    u32 Result = UI_RGBA(C[0], C[1], C[2], 255);
    return Result;
}

internal void DrawClassEmblem(render_context *RenderContext, u32 Role, v2 C, float S, u32 Fill,
                              u32 Light);

// NOTE(zoubir): health's colour at Share of the most
internal u32
PartyHealthColor(float Share)
{
    u32 Full = UI_RGBA(84, 200, 110, 255);
    u32 Half = UI_RGBA(230, 190, 60, 255);
    u32 Low = UI_RGBA(220, 60, 50, 255);
    u32 Result = Share > 0.5f ? UIMixColor(Half, Full, (Share - 0.5f) * 2.f) :
        UIMixColor(Low, Half, Share * 2.f);
    return Result;
}

// NOTE(zoubir): a filled four-cornered shape, one colour
inline void
DrawPartyQuad(render_context *RenderContext, v2 A, v2 B, v2 C, v2 D, u32 Color)
{
    DrawFilledQuad(RenderContext, A, B, C, D, Color, Color, Color, Color, RenderBlend_Alpha);
}

// NOTE(zoubir): the role's emblem in a Size box at X, Y: a heater shield
// for the tank, a cross for the healer, a flame for the damage role
internal void
DrawRoleEmblem(render_context *RenderContext, u32 Role, float X, float Y, float Size,
               u32 Color, float Alpha)
{
    DrawRoundRect(RenderContext, X, Y, Size, Size, WithAlpha(UI_RGBA(12, 13, 20, 255), 0.9f * Alpha));
    DrawRoundOutline(RenderContext, X, Y, Size, Size, WithAlpha(Color, 0.8f * Alpha));
    v2 C = V2(X + 0.5f * Size, Y + 0.5f * Size);
    float S = 0.32f * Size;
    u32 Fill = WithAlpha(Color, Alpha);
    u32 Light = WithAlpha(UIMixColor(Color, UI_RGBA(255, 255, 255, 255), 0.55f), Alpha);
    if (Role == PlayerRole_Tank)
    {
        DrawPartyQuad(RenderContext, C + V2(-S, -S), C + V2(S, -S), C + V2(S, 0.15f * S),
                      C + V2(-S, 0.15f * S), Fill);
        DrawPartyQuad(RenderContext, C + V2(-S, 0.15f * S), C + V2(S, 0.15f * S),
                      C + V2(0.f, 1.15f * S), C + V2(0.f, 1.15f * S), Fill);
        DrawPartyQuad(RenderContext, C + V2(-0.12f * S, -0.8f * S), C + V2(0.12f * S, -0.8f * S),
                      C + V2(0.12f * S, 0.85f * S), C + V2(-0.12f * S, 0.85f * S), Light);
    }
    else if (Role == PlayerRole_Healer)
    {
        float W = 0.32f * S;
        DrawPartyQuad(RenderContext, C + V2(-W, -S), C + V2(W, -S), C + V2(W, S), C + V2(-W, S), Fill);
        DrawPartyQuad(RenderContext, C + V2(-S, -W), C + V2(S, -W), C + V2(S, W), C + V2(-S, W), Fill);
        DrawPartyQuad(RenderContext, C + V2(-0.4f * W, -0.7f * S), C + V2(0.4f * W, -0.7f * S),
                      C + V2(0.4f * W, 0.7f * S), C + V2(-0.4f * W, 0.7f * S), Light);
    }
    else if (Role != PlayerRole_Damage)
    {
        DrawClassEmblem(RenderContext, Role, C, S, Fill, Light);
    }
    else
    {
        // NOTE(zoubir): a flame: a round base under a point, twice over
        DrawPartyQuad(RenderContext, C + V2(0.f, -1.2f * S), C + V2(0.75f * S, 0.1f * S),
                      C + V2(0.f, 1.f * S), C + V2(-0.75f * S, 0.1f * S), Fill);
        DrawPartyQuad(RenderContext, C + V2(0.15f * S, -0.5f * S), C + V2(0.42f * S, 0.35f * S),
                      C + V2(0.f, 0.85f * S), C + V2(-0.42f * S, 0.35f * S), Light);
    }
}

// NOTE(zoubir): the frame of slot SlotIndex with its top-left at X, Y
internal void
DrawPartyFrame(render_context *RenderContext, app_state *AppState, app_input *Input,
               u32 SlotIndex, float X, float Y, float Height)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    world_entity *Player = Slot->Entity;
    world_entity *Local = GetLocalPlayer(AppState);
    party_pick *Pick = GetPartyPick(AppState);
    font *Small = AppState->Fonts.Small;
    float Clock = GetFxClock(AppState);
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    u32 RoleColor = RoleUIColor(Role);
    bool32 Down = IsDeadPlayer(Player);
    bool32 Hot = PartyFrameUnderMouse(AppState, Input) == SlotIndex + 1;
    bool32 Focused = Pick->Focus == SlotIndex + 1;
    bool32 Targeted = LocalPicksAllies(AppState) && Pick->Target == SlotIndex + 1;
    // NOTE(zoubir): a healer's spells do not reach past MENDING_BOLT_RANGE
    bool32 Far = Local && Local != Player && LocalPicksAllies(AppState) && !Down &&
        Length(Player->Position.XY - Local->Position.XY) > MENDING_BOLT_RANGE;
    float Alpha = Far ? 0.5f : 1.f;
    float Width = PARTY_FRAME_WIDTH;

    Pick->FrameX[SlotIndex] = X;
    Pick->FrameY[SlotIndex] = Y;
    Pick->FrameWidth[SlotIndex] = Width;
    Pick->FrameHeight[SlotIndex] = Height;

    u32 Rim = Targeted ? UI_RGBA(255, 214, 110, 255) : (Hot ? UI_RGBA(220, 225, 240, 255) : RoleColor);
    DrawUIPanel(RenderContext, X, Y, Width, Height, Rim, Down ? 0.6f : 0.92f * Alpha + 0.08f);
    if (Slot->Aggro > 0 && !Down)
    {
        float Beat = 0.5f + 0.5f * Sin(7.f * Clock);
        float Glow = Role == PlayerRole_Tank ? 0.25f : 0.45f + 0.35f * Beat;
        DrawRoundOutline(RenderContext, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f,
                         WithAlpha(UI_RGBA(255, 70, 50, 255), Glow));
    }
    if (Focused)
    {
        DrawRoundOutline(RenderContext, X - 1.f, Y - 1.f, Width + 2.f, Height + 2.f,
                         UI_RGBA(255, 214, 110, 255));
    }

    float Pad = 7.f;
    float Emblem = Minimum(PARTY_EMBLEM, Height - 2.f * Pad);
    DrawRoleEmblem(RenderContext, Role, X + Pad, Y + 0.5f * (Height - Emblem), Emblem,
                   Down ? UI_COLOR_DIM : RoleColor, Alpha);
    float Left = X + Pad + Emblem + Pad;
    float Right = X + Width - Pad;

    char Name[32];
    GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
    char Line[64];
    snprintf(Line, sizeof(Line), "%s", Name);
    u32 NameColor = SlotIndex == AppState->LocalPlayerIndex ? UI_COLOR_ACCENT : UI_COLOR_TEXT;
    bool32 Compact = Height < PARTY_FRAME_HEIGHT;
    float BarHeight = Compact ? 8.f : PARTY_BAR_HEIGHT;
    float BarY = Y + Height - Pad - BarHeight;
    float TextY = Y + Pad - 3.f;
    if (!Compact)
    {
        UIText(RenderContext, Small, Left, TextY, Line, WithAlpha(NameColor, Alpha));
    }

    float BarWidth = Right - Left;
    float Share = (Player->MaxHp > 0.f && !Down) ? Clamp01(Player->Hp / Player->MaxHp) : 0.f;
    DrawRoundRect(RenderContext, Left, BarY, BarWidth, BarHeight,
                  WithAlpha(Down ? UI_COLOR_DIM : UI_COLOR_TRACK, Alpha));
    if (Share > 0.f)
    {
        DrawRoundRect(RenderContext, Left, BarY, Maximum(BarHeight, Share * BarWidth), BarHeight,
                      WithAlpha(PartyHealthColor(Share), Alpha));
    }
    // NOTE(zoubir): a ward is health on top: a pale band after the bar
    if (Slot->WardAbsorb > 0.f && !Down && Player->MaxHp > 0.f)
    {
        float Ward = Minimum(1.f - Share, Slot->WardAbsorb / Player->MaxHp);
        float WardX = Left + Share * BarWidth;
        if (Ward > 0.01f)
        {
            DrawRoundRect(RenderContext, WardX, BarY, Maximum(4.f, Ward * BarWidth), BarHeight,
                          WithAlpha(UI_RGBA(140, 210, 255, 255), 0.75f * Alpha));
        }
        else
        {
            DrawRoundOutline(RenderContext, Left, BarY, BarWidth, BarHeight,
                             WithAlpha(UI_RGBA(140, 210, 255, 255), Alpha));
        }
    }
    if ((Slot->ShieldWallSeconds > 0.f || Slot->RallySeconds > 0.f) && !Down)
    {
        DrawRoundOutline(RenderContext, Left - 2.f, BarY - 2.f, BarWidth + 4.f, BarHeight + 4.f,
                         WithAlpha(UI_RGBA(235, 225, 205, 255), 0.9f * Alpha));
    }
    if (Down)
    {
        float Revive = Clamp01(Slot->ReviveSeconds / REVIVE_SECONDS);
        if (Revive > 0.f)
        {
            DrawRoundRect(RenderContext, Left, BarY, Maximum(BarHeight, Revive * BarWidth),
                          BarHeight, UI_RGBA(112, 240, 144, 255));
        }
        if (!Compact)
        {
            UIText(RenderContext, Small, Right, TextY,
                   Revive > 0.f ? (char *)"reviving" : (char *)"down",
                   Revive > 0.f ? UI_RGBA(112, 240, 144, 255) : UI_COLOR_TEXT_MUTED,
                   UIAlign_Right);
        }
    }
    else if (!Compact)
    {
        snprintf(Line, sizeof(Line), "%.0f / %.0f", Maximum(0.f, Player->Hp), Player->MaxHp);
        UIText(RenderContext, Small, Right, TextY, Line,
               WithAlpha(PartyHealthColor(Share), Alpha), UIAlign_Right);
    }
    if (Slot->RenewSeconds > 0.f && !Down)
    {
        // NOTE(zoubir): Renewal: a green leaf of light at the bar's end
        float Beat = 0.6f + 0.4f * Sin(5.f * Clock);
        DrawShaderQuad(RenderContext, Shader_Glow, Right - 12.f, BarY - 6.f, 20.f, 20.f,
                       WithAlpha(UI_RGBA(120, 255, 150, 255), Beat * Alpha), RenderBlend_Additive);
    }
    if (Slot->Aggro > 0 && !Down)
    {
        snprintf(Line, sizeof(Line), "%u", Slot->Aggro);
        float ChipX = X + Width + 4.f;
        float ChipY = Y + 0.5f * Height - 0.5f * UILineHeight(Small);
        float ChipWidth = UITextWidth(Small, Line) + 10.f;
        u32 Chip = Role == PlayerRole_Tank ? UI_RGBA(90, 60, 50, 230) : UI_RGBA(200, 50, 40, 240);
        DrawRoundRect(RenderContext, ChipX, ChipY, ChipWidth, UILineHeight(Small), Chip);
        UIText(RenderContext, Small, ChipX + 5.f, ChipY, Line, UI_COLOR_TEXT);
    }
    if (Hot && Input->LeftButton.Pressed && !Down && !TalentPanelHasMouse(AppState, Input))
    {
        ClickPartyFrame(AppState, SlotIndex);
    }
}

// NOTE(zoubir): every party member, the local player at the bottom and
// the others stacked above in slot order
internal void
DrawDungeonParty(render_context *RenderContext, app_state *AppState, app_input *Input,
                 u32 WindowHeight)
{
    party_pick *Pick = GetPartyPick(AppState);
    u32 Count = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        Pick->FrameWidth[SlotIndex] = 0.f;
        player_slot *Slot = &AppState->Players[SlotIndex];
        Count += (Slot->Active && Slot->Entity) ? 1 : 0;
    }
    float Room = 0.6f * (float)WindowHeight;
    float Height = ((float)Count * (PARTY_FRAME_HEIGHT + PARTY_FRAME_GAP) > Room) ?
        PARTY_FRAME_COMPACT : PARTY_FRAME_HEIGHT;
    float X = PARTY_FRAME_MARGIN;
    float Y = (float)WindowHeight - PARTY_FRAME_BOTTOM - Height;
    for(u32 Pass = 0; Pass < 2; Pass++)
    {
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            bool32 Local = SlotIndex == AppState->LocalPlayerIndex;
            if (!Slot->Active || !Slot->Entity || Local != (Pass == 0))
            {
                continue;
            }
            DrawPartyFrame(RenderContext, AppState, Input, SlotIndex, X, Y, Height);
            Y -= Height + PARTY_FRAME_GAP;
        }
    }
}
