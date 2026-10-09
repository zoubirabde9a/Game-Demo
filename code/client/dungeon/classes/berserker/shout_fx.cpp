/* Battle Shout (client/dungeon/classes/berserker.cpp): the cry's burst and
   the mark it leaves on everyone it lifts.
   - the burst (BerserkerBurst_Shout): a flash at the body, a gold-red
     ring racing out to BATTLE_SHOUT_REACH, where the shout reaches, and
     chevrons thrown out with it;
   - while the Berserker's BERSERKER_FLAG_SHOUT is up, the Berserker and
     every living ally within BATTLE_SHOUT_REACH of it (as the sim shares
     it, run_tree/run_effects.cpp) wear a small red chevron over the head
     and a faint red rim at the feet. The flag and the bodies reach every
     client, so it shows the same online. */

#define BERSERKER_GOLD_RGB 0x0040C0FF

internal void
DrawShoutBurst(render_context *RenderContext, app_state *AppState, role_burst *Burst, float T,
               v3 CameraOffset)
{
    world_entity *Caster = Burst->Slot < MAX_PLAYERS ? AppState->Players[Burst->Slot].Entity : 0;
    v2 Feet = BurstToScreen(Caster ? Caster->Position : Burst->Position, CameraOffset);
    v2 Body = Caster ? RoleLookPoint(Caster, 0.55f, CameraOffset) : Feet - V2(0.f, 24.f);
    float EaseOut = 1.f - Square(1.f - T);
    float Fade = 1.f - T * T;
    float Front = BATTLE_SHOUT_REACH * Minimum(1.f, 1.1f * EaseOut + 0.03f);
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, Front - 40.f * (1.f - T) - 4.f, Front,
                FxColor(0.f, BERSERKER_CRIMSON_RGB), FxColor(0.6f * Fade, BERSERKER_CRIMSON_RGB));
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, Front - 3.f, Front + 1.f,
                FxColor(0.9f * Fade, BERSERKER_GOLD_RGB), FxColor(0.9f * Fade, BERSERKER_GOLD_RGB));
    // NOTE(zoubir): a second, fainter ring behind it, and the reach left
    // marked a moment once the front gets there
    DrawBerserkerRing(RenderContext, Feet, 0.7f * Front, 14.f * (1.f - T) + 2.f, 0.5f * Fade);
    float Hold = Clamp01((T - 0.6f) / 0.4f);
    if (T > 0.6f)
    {
        DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, BATTLE_SHOUT_REACH - 2.f, BATTLE_SHOUT_REACH,
                    FxColor(0.5f * (1.f - Hold), BERSERKER_GOLD_RGB),
                    FxColor(0.5f * (1.f - Hold), BERSERKER_GOLD_RGB));
    }
    float Flash = Clamp01(1.f - 3.f * T);
    DrawShaderQuad(RenderContext, Shader_Glow, Body.X - 60.f, Body.Y - 70.f, 120.f, 120.f,
                   FxColor(Flash, BERSERKER_GOLD_RGB), RenderBlend_Additive);
    // NOTE(zoubir): the cry's chevrons, riding out with the ring
    for(u32 Mark = 0; Mark < 12; Mark++)
    {
        float A = 2.f * Pi32 * (float)Mark / 12.f;
        v2 Way = V2(Cos(A), Sin(A));
        v2 Side = V2(-Way.Y, Way.X);
        v2 Tip = Feet + (0.92f * Front) * Way;
        v2 Back = Tip - 12.f * Way;
        u32 Color = FxColor(Fade, BERSERKER_GOLD_RGB);
        u32 Clear = FxColor(0.f, BERSERKER_CRIMSON_RGB);
        DrawFxStroke(RenderContext, Back + 8.f * Side, Tip, 0.f, 4.f, Clear, Color);
        DrawFxStroke(RenderContext, Back - 8.f * Side, Tip, 0.f, 4.f, Clear, Color);
    }
}

// NOTE(zoubir): the shout's mark on one body: a chevron over the head,
// bobbing, and a faint red rim at the feet
internal void
DrawShoutMark(render_context *RenderContext, world_entity *Body, float Clock, v3 CameraOffset)
{
    v2 Feet = BurstToScreen(Body->Position, CameraOffset);
    v2 Over = RoleLookPoint(Body, 1.f, CameraOffset) - V2(0.f, 24.f + 2.f * Sin(4.f * Clock));
    float Beat = 0.75f + 0.25f * Sin(6.f * Clock);
    DrawArcBand(RenderContext, Feet, 0.f, 2.f * Pi32, 12.f, 22.f,
                FxColor(0.f, BERSERKER_CRIMSON_RGB), FxColor(0.3f * Beat, BERSERKER_CRIMSON_RGB));
    DrawShaderQuad(RenderContext, Shader_Glow, Over.X - 12.f, Over.Y - 10.f, 24.f, 20.f,
                   FxColor(0.45f * Beat, BERSERKER_CRIMSON_RGB), RenderBlend_Additive);
    for(u32 Arm = 0; Arm < 2; Arm++)
    {
        float Way = Arm ? 1.f : -1.f;
        v2 Tip = Over - V2(0.f, 4.f);
        v2 End = Over + V2(7.f * Way, 3.f);
        DrawFxStroke(RenderContext, End, Tip, 3.f, 3.f, FxColor(Beat, BERSERKER_CRIMSON_RGB),
                     FxColor(Beat, BERSERKER_GOLD_RGB), RenderBlend_Alpha);
    }
}

// NOTE(zoubir): every frame: the mark on each body a Battle Shout lifts,
// once however many shouts reach it
internal void
DrawBattleShoutMarks(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    u32 Lifted = 0;
    for(u32 ShouterIndex = 0; ShouterIndex < MAX_PLAYERS; ShouterIndex++)
    {
        player_slot *Shouter = &AppState->Players[ShouterIndex];
        world_entity *Source = Shouter->Entity;
        if (!Shouter->Active || Shouter->Role != PlayerRole_Berserker ||
            !(Shouter->ClassFlags & BERSERKER_FLAG_SHOUT) || !Source || !Source->IsPresent ||
            IsDeadPlayer(Source))
        {
            continue;
        }
        for(u32 OtherIndex = 0; OtherIndex < MAX_PLAYERS; OtherIndex++)
        {
            player_slot *Other = &AppState->Players[OtherIndex];
            world_entity *Body = Other->Entity;
            if (!Other->Active || !Body || !Body->IsPresent || IsDeadPlayer(Body) ||
                LengthSq(Body->Position.XY - Source->Position.XY) >
                BATTLE_SHOUT_REACH * BATTLE_SHOUT_REACH)
            {
                continue;
            }
            Lifted |= 1u << OtherIndex;
        }
    }
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < MAX_PLAYERS; Index++)
    {
        if (Lifted & (1u << Index))
        {
            DrawShoutMark(RenderContext, AppState->Players[Index].Entity, Clock, CameraOffset);
        }
    }
}
