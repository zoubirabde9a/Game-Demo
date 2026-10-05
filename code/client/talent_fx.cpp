/* Talent effects drawn over the world: the Ward talent's shell, a slow
   golden hexagon round every player whose ward is up, so an attacker can
   see the next hit will only break it (read from the player slots, which
   online play fills from the scores); and Frost Nova's frost on whoever
   it slowed, and ice round whoever it still holds (read from the status
   timers). */

#define WARD_SHELL_RADIUS 24.f
#define WARD_SHELL_RGB 0x006ED7FF

internal void
DrawWardShells(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    float Clock = GetFxClock(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = Slot->Entity;
        if (!Slot->Active || !Slot->WardReady || !Player || !Player->IsPresent ||
            IsDeadPlayer(Player))
        {
            continue;
        }
        v3 Chest = Player->Position;
        Chest.Z += 18.f;
        v2 Centre = BurstToScreen(Chest, CameraOffset);
        float Breath = 0.75f + 0.25f * Sin(3.f * Clock + (float)SlotIndex);
        float Turn = 0.4f * Clock;
        for(u32 Side = 0; Side < 6; Side++)
        {
            float A0 = Turn + 2.f * Pi32 * (float)Side / 6.f;
            float A1 = Turn + 2.f * Pi32 * (float)(Side + 1) / 6.f;
            v2 P0 = Centre + WARD_SHELL_RADIUS * V2(Cos(A0), 0.9f * Sin(A0));
            v2 P1 = Centre + WARD_SHELL_RADIUS * V2(Cos(A1), 0.9f * Sin(A1));
            // NOTE(zoubir): the front edges brighter than the back ones
            float Front = 0.55f + 0.45f * Sin(0.5f * (A0 + A1));
            DrawFxStreak(RenderContext, P0, P1, 3.f,
                         FxColor(0.8f * Breath * Front, WARD_SHELL_RGB),
                         FxColor(0.8f * Breath * Front, WARD_SHELL_RGB));
            DrawFxDot(RenderContext, P0, 3.f, FxColor(0.8f * Breath, 0x00FFFFFF));
        }
    }
}

#define FROST_RGB 0x00FFE696
#define ICE_RGB 0x00FFD9A8

// NOTE(zoubir): Frost Nova's marks on whoever it caught, from the status
// timers, so they show online as offline: a slowed unit walks on a ring
// of frost with flakes rising off it; one still frozen (stunned and
// slowed) stands in a block of ice
internal void
DrawFrostOnUnits(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Hp <= 0.f || !Entity->Collision ||
            !HasStatus(Entity, StatusEffect_Slowed))
        {
            continue;
        }
        v3 HalfDims = Entity->Collision->TotalVolume.HalfDims;
        float Radius = Maximum(14.f, 1.2f * HalfDims.X);
        v3 Feet = Entity->Position;
        Feet.Z = Entity->GroundZ;
        v2 Ground = BurstToScreen(Feet, CameraOffset);
        float Turn = 0.6f * Clock + (float)EntityIndex;
        DrawArcBand(RenderContext, Ground, 0.f, 2.f * Pi32, 0.f, Radius,
                    FxColor(0.05f, FROST_RGB), FxColor(0.35f, FROST_RGB), RenderBlend_Alpha);
        for(u32 Shard = 0; Shard < 8; Shard++)
        {
            float Angle = Turn + 2.f * Pi32 * (float)Shard / 8.f;
            v2 P = Ground + GroundCircle(Angle, Radius);
            DrawFxStreak(RenderContext, P, P + 5.f * V2(Cos(Angle), Sin(Angle)) - V2(0.f, 4.f),
                         2.f, FxColor(0.8f, FROST_RGB), FxColor(0.9f, 0x00FFFFFF));
        }
        // NOTE(zoubir): flakes rising and fading, each on its own clock
        for(u32 Flake = 0; Flake < 6; Flake++)
        {
            float Life = fmodf(0.7f * Clock + BurstJitter(Flake, EntityIndex), 1.f);
            float Angle = 2.f * Pi32 * BurstJitter(Flake, EntityIndex + 7);
            v2 P = Ground + GroundCircle(Angle, Radius * 0.8f) -
                V2(0.f, 34.f * Life);
            DrawFxDot(RenderContext, P, 2.f + 1.f * (1.f - Life),
                      FxColor(0.9f * (1.f - Life), 0x00FFFFFF));
        }

        if (!HasStatus(Entity, StatusEffect_Stunned))
        {
            continue;
        }
        // NOTE(zoubir): the block: a front face and a lighter top, both
        // see-through, edged in white, with a glint sliding down it
        // NOTE(zoubir): round the body where it is, in the air too
        v2 Body = BurstToScreen(Entity->Position, CameraOffset);
        float Height = 2.f * HalfDims.Z + 10.f;
        float Half = Radius;
        v2 BottomLeft = Body + V2(-Half, 4.f);
        v2 BottomRight = Body + V2(Half, 4.f);
        v2 TopLeft = BottomLeft - V2(0.f, Height);
        v2 TopRight = BottomRight - V2(0.f, Height);
        v2 Back = V2(0.f, -0.45f * Half);
        DrawFilledQuad(RenderContext, BottomLeft, BottomRight, TopRight, TopLeft,
                       FxColor(0.42f, ICE_RGB), FxColor(0.42f, ICE_RGB),
                       FxColor(0.22f, ICE_RGB), FxColor(0.22f, ICE_RGB), RenderBlend_Alpha);
        DrawFilledQuad(RenderContext, TopLeft, TopRight, TopRight + Back, TopLeft + Back,
                       FxColor(0.5f, 0x00FFFFFF), FxColor(0.5f, 0x00FFFFFF),
                       FxColor(0.35f, ICE_RGB), FxColor(0.35f, ICE_RGB), RenderBlend_Alpha);
        u32 Edge = FxColor(0.85f, 0x00FFFFFF);
        DrawFxStreak(RenderContext, BottomLeft, TopLeft, 1.5f, Edge, Edge, RenderBlend_Alpha);
        DrawFxStreak(RenderContext, BottomRight, TopRight, 1.5f, Edge, Edge, RenderBlend_Alpha);
        DrawFxStreak(RenderContext, TopLeft, TopRight, 1.5f, Edge, Edge, RenderBlend_Alpha);
        DrawFxStreak(RenderContext, TopLeft + Back, TopRight + Back, 1.f,
                     FxColor(0.5f, 0x00FFFFFF), FxColor(0.5f, 0x00FFFFFF), RenderBlend_Alpha);
        float Glint = fmodf(0.8f * Clock + 0.3f * (float)EntityIndex, 1.4f);
        if (Glint < 1.f)
        {
            v2 A = TopLeft + Glint * (BottomLeft - TopLeft) + V2(4.f, 0.f);
            v2 B = A + V2(0.6f * Half, -0.6f * Half);
            DrawFxStreak(RenderContext, A, B, 3.f, FxColor(0.f, 0x00FFFFFF),
                         FxColor(0.7f * Sin(Pi32 * Glint), 0x00FFFFFF));
        }
    }
}

// NOTE(zoubir): every talent effect over the world, one call for the
// screen pass
internal void
DrawTalentFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    DrawFrostOnUnits(RenderContext, AppState, CameraOffset);
    DrawWardShells(RenderContext, AppState, CameraOffset);
}
