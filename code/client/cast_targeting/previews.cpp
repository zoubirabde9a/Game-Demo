/* Cast previews (cast_targeting.cpp): what the local player's ability
   would hit, drawn on the ground in world units so the shape on screen is
   the shape the simulation tests against.

   While standard mode aims an ability: its area at the cursor's angle (a
   disc, or a wedge for Push), a dotted line out to an area that lands
   away from the player, and for Blink the ring of its reach and the spot
   it would land on. For the kunai: the ring of its reach, a ring under
   every foe it could be thrown at (pulsing when in reach, faint red when
   not) and a crosshair at the cursor while it is on no foe; the foe it
   would hit is bracketed by targeting.cpp. Grey while the ability
   recharges, its icon's colour when it is ready.

   While the local player winds up an area ability, in either mode: the
   area's rim where it will land, filling from the middle as the wind-up
   runs, so a quick cast shows its reach too.

   The numbers come from the abilities' own tables (PlayerAreaAbilities,
   PlayerMovements, RewindAbilities), so a retuned radius shows as it is. */

#define CAST_PREVIEW_FILL_ALPHA 0.16f
#define CAST_PREVIEW_RIM_ALPHA 0.85f
#define CAST_PREVIEW_RIM 2.5f
#define CAST_PREVIEW_WAIT_RGB 0x00A0A0A0
#define CAST_PREVIEW_FADE_SECONDS 0.08f
#define CAST_PREVIEW_LINE_STEP 14.f
#define CAST_PREVIEW_BLINK_SPOT 14.f

// NOTE(zoubir): each targeted ability's colour, its ability bar icon's
// accent (ui/ability_icons/icon_list.inc), as 0x00BBGGRR
internal u32
CastPreviewRGB(u32 Button)
{
    u32 Result = 0x00FFFFFF;
    switch(Button)
    {
        case PlayerButton_Launch:       { Result = 0x003CC8FF; } break;
        case PlayerButton_Shockwave:    { Result = 0x00FFDCAA; } break;
        case PlayerButton_Push:         { Result = 0x00FFE6BE; } break;
        case PlayerButton_FrostNova:    { Result = 0x00FFE196; } break;
        case PlayerButton_GravityWell:  { Result = 0x00FF64B4; } break;
        case PlayerButton_Blink:        { Result = 0x00E66EF0; } break;
        case PlayerButton_RewindBubble: { Result = 0x00E6F06E; } break;
        case PlayerButton_Kunai:        { Result = 0x00FFF0D8; } break;
    }
    return Result;
}

// NOTE(zoubir): the area row Button casts, 0 for none
internal player_area_ability *
AreaAbilityForButton(u32 Button)
{
    player_area_ability *Result = 0;
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        if (PlayerAreaAbilities[Index].Button == Button)
        {
            Result = &PlayerAreaAbilities[Index];
        }
    }
    return Result;
}

// NOTE(zoubir): dots from From to To, every CAST_PREVIEW_LINE_STEP
internal void
DrawCastPreviewLine(render_context *RenderContext, v2 From, v2 To, u32 Color)
{
    v2 Way = To - From;
    float Distance = Length(Way);
    u32 Steps = (u32)(Distance / CAST_PREVIEW_LINE_STEP);
    for(u32 Step = 1; Step < Steps; Step++)
    {
        DrawFxDot(RenderContext, From + ((float)Step / (float)Steps) * Way, 3.f, Color);
    }
}

// NOTE(zoubir): a disc (Half >= Pi32) or a wedge of half-angle Half round
// Angle: a faint fill out to Fill (a share of Radius) and a rim at Radius
internal void
DrawCastPreviewArea(render_context *RenderContext, v2 Centre, float Radius,
                    float Angle, float Half, float Fill, float Alpha, u32 RGB)
{
    float From = Half >= Pi32 ? 0.f : Angle - Half;
    float To = Half >= Pi32 ? 2.f * Pi32 : Angle + Half;
    float FillRadius = Fill * Radius;
    DrawArcBand(RenderContext, Centre, From, To, 0.f, FillRadius,
                FxColor(0.4f * CAST_PREVIEW_FILL_ALPHA * Alpha, RGB),
                FxColor(CAST_PREVIEW_FILL_ALPHA * Alpha, RGB), RenderBlend_Alpha);
    // NOTE(zoubir): a dark edge under the rim, so it reads on sand and snow
    DrawArcBand(RenderContext, Centre, From, To, Radius - CAST_PREVIEW_RIM - 1.f,
                Radius + 1.f, FxColor(0.45f * Alpha, 0), FxColor(0.45f * Alpha, 0),
                RenderBlend_Alpha);
    DrawArcBand(RenderContext, Centre, From, To, Radius - CAST_PREVIEW_RIM, Radius,
                FxColor(CAST_PREVIEW_RIM_ALPHA * Alpha, RGB),
                FxColor(CAST_PREVIEW_RIM_ALPHA * Alpha, RGB), RenderBlend_Alpha);
    if (Half < Pi32)
    {
        u32 Edge = FxColor(CAST_PREVIEW_RIM_ALPHA * Alpha, RGB);
        DrawCastPreviewLine(RenderContext, Centre, Centre + GroundCircle(From, Radius), Edge);
        DrawCastPreviewLine(RenderContext, Centre, Centre + GroundCircle(To, Radius), Edge);
    }
}

// NOTE(zoubir): an area row's shape for Player along Aim; Fill as above
internal void
DrawAreaAbilityPreview(render_context *RenderContext, world_entity *Player,
                       player_area_ability *Ability, v2 Aim, v3 CameraOffset,
                       float Fill, float Alpha, u32 RGB)
{
    v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y,
                               Player->GroundZ), CameraOffset);
    v2 Centre = BurstToScreen(AreaCentre(Player, Ability, Aim), CameraOffset);
    float Angle = ATan2(Aim.Y, Aim.X);
    float Half = Ability->ConeCos > -1.f ? acosf(Ability->ConeCos) : Pi32;
    if (Ability->Reach > 0.f)
    {
        DrawCastPreviewLine(RenderContext, Feet, Centre,
                            FxColor(0.7f * Alpha, RGB));
    }
    DrawCastPreviewArea(RenderContext, Centre, Ability->Radius, Angle, Half,
                        Fill, Alpha, RGB);
}

// NOTE(zoubir): Blink: the ring of its reach round the player, and the
// spot it would land on (FindBlinkLanding, sim/player_abilities/blink_landing.cpp)
internal void
DrawBlinkAimPreview(render_context *RenderContext, app_state *AppState,
                    world_entity *Player, v3 CameraOffset, float Alpha, u32 RGB)
{
    player_movement_ability *Blink = &PlayerMovements[PlayerMove_Blink];
    float Range = Blink->Power * PlayerPowerScale(AppState, Player, Blink->Button);
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    v2 Target = Player->Position.XY + (Reach * Range) * GetPlayerAim(Player);
    v2 Landing = FindBlinkLanding(AppState, Player, Target);
    v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y,
                               Player->GroundZ), CameraOffset);
    v2 Spot = BurstToScreen(V3(Landing.X, Landing.Y, Player->GroundZ), CameraOffset);
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, Range - 2.f, Range,
                FxColor(0.5f * Alpha, RGB), FxColor(0.5f * Alpha, RGB),
                RenderBlend_Alpha);
    DrawCastPreviewLine(RenderContext, Feet, Spot, FxColor(0.7f * Alpha, RGB));
    DrawCastPreviewArea(RenderContext, Spot, CAST_PREVIEW_BLINK_SPOT, 0.f, Pi32,
                        1.f, Alpha, RGB);
}

// NOTE(zoubir): a crosshair of four arms round P, a gap in the middle
internal void
DrawCastCrosshair(render_context *RenderContext, v2 P, u32 Color)
{
    float Gap = 5.f;
    float Arm = 8.f;
    float W = 1.f;
    for(u32 Side = 0; Side < 4; Side++)
    {
        v2 Dir = (Side == 0) ? V2(1.f, 0.f) : (Side == 1) ? V2(-1.f, 0.f) :
            (Side == 2) ? V2(0.f, 1.f) : V2(0.f, -1.f);
        v2 Across = V2(-Dir.Y, Dir.X) * W;
        v2 From = P + Gap * Dir;
        v2 To = P + (Gap + Arm) * Dir;
        DrawFilledQuad(RenderContext, From - Across, To - Across, To + Across,
                       From + Across, Color, Color, Color, Color);
    }
}

// NOTE(zoubir): the kunai being aimed: its reach, the foes it could hit,
// and the cursor as a crosshair until it is on one (the brackets then)
internal void
DrawKunaiAimPreview(render_context *RenderContext, app_state *AppState,
                    world_entity *Player, v3 CameraOffset, float Seconds,
                    float Alpha, u32 RGB)
{
    v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y,
                               Player->GroundZ), CameraOffset);
    DrawCastPreviewArea(RenderContext, Feet, PlayerStats.KunaiRange, 0.f, Pi32,
                        0.f, Alpha, RGB);
    float Beat = 0.5f + 0.5f * Sin(6.f * Seconds);
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (!IsKunaiTarget(Unit, Player))
        {
            continue;
        }
        bool32 InReach = Length(Unit->Position.XY - Player->Position.XY) <=
            PlayerStats.KunaiRange;
        v2 UnitFeet = BurstToScreen(V3(Unit->Position.X, Unit->Position.Y,
                                       Unit->GroundZ), CameraOffset);
        float Radius = Maximum(12.f, 0.45f * Unit->Dimensions.X);
        if (InReach)
        {
            DrawCastPreviewArea(RenderContext, UnitFeet, Radius + 3.f * Beat, 0.f, Pi32,
                                0.6f, Alpha, RGB);
        }
        else
        {
            DrawCastPreviewArea(RenderContext, UnitFeet, Radius, 0.f, Pi32,
                                0.f, 0.5f * Alpha, 0x004040F0);
        }
    }
    cursor_targeting *Cursor = GetCursorTargeting(AppState);
    if (!PickedUnit(AppState, Player, Cursor->Picked))
    {
        DrawCastCrosshair(RenderContext, Cursor->CursorAt - CameraOffset.XY,
                          FxColor(0.9f * Alpha, RGB));
    }
}

// NOTE(zoubir): once a frame, in the world pass's projection
internal void
DrawCastPreview(render_context *RenderContext, app_state *AppState,
                v3 CameraOffset)
{
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        return;
    }
    cast_targeting *Targeting = GetCastTargeting(AppState);

    // NOTE(zoubir): the wind-up of an area ability, in either mode
    if (IsPlayerCasting(Player))
    {
        for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
        {
            player_area_ability *Ability = &PlayerAreaAbilities[Index];
            if (Ability->Spell != PlayerSpell_None && (u32)Ability->Spell == Player->CastSpell)
            {
                float Progress = Clamp01(PlayerCastProgress(Player));
                DrawAreaAbilityPreview(RenderContext, Player, Ability,
                                       NormalizeOr(Player->CastingDirection,
                                                   GetPlayerAim(Player)),
                                       CameraOffset, Progress, 1.f,
                                       CastPreviewRGB(Ability->Button));
            }
        }
    }

    u32 Button = Targeting->Aiming;
    if (!Button)
    {
        return;
    }
    float Alpha = Clamp01(Targeting->AimSeconds / CAST_PREVIEW_FADE_SECONDS);
    u32 RGB = IsCastReady(AppState, Player, Button) ?
        CastPreviewRGB(Button) : CAST_PREVIEW_WAIT_RGB;
    v2 Aim = GetPlayerAim(Player);
    player_area_ability *Area = AreaAbilityForButton(Button);
    if (Area)
    {
        DrawAreaAbilityPreview(RenderContext, Player, Area, Aim, CameraOffset,
                               1.f, Alpha, RGB);
    }
    else if (Button == PlayerButton_Blink)
    {
        DrawBlinkAimPreview(RenderContext, AppState, Player, CameraOffset, Alpha, RGB);
    }
    else if (Button == PlayerButton_RewindBubble)
    {
        v2 Feet = BurstToScreen(V3(Player->Position.X, Player->Position.Y,
                                   Player->GroundZ), CameraOffset);
        DrawCastPreviewArea(RenderContext, Feet,
                            RewindAbilities[RewindKind_Bubble].Radius, 0.f, Pi32,
                            1.f, Alpha, RGB);
    }
    else if (Button == PlayerButton_Kunai)
    {
        DrawKunaiAimPreview(RenderContext, AppState, Player, CameraOffset,
                            Targeting->AimSeconds, Alpha, RGB);
    }
}
