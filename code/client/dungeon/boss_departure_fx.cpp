/* Boss departure effects: what the party sees while a Starless Deep boss
   is away from its fight (sim/dungeon/boss_departures.cpp,
   docs/dungeon-starless.md). The boss itself and the hazards that fall
   meanwhile hang high over the floor and are not drawn
   (client/draw_entities.cpp); this draws them on the floor instead.

   Ommoroth gone     a pool of the dark where he sank, swirling.
   Nyxara gone       her shadow on the floor where she rose: a black disc
                     in a ring of gold fire, a shaft of light going up.
   Void Maw          the dark opens under the ring the slam draws, and
                     fangs close in on it as the windup runs out; then the
                     jaws snap shut.
   Falling Star      a star comes down out of the sky onto the ring, its
                     shadow on the floor growing under it; then it bursts.

   Everything is worked out from the monsters' heights, kinds and ability
   phases, which snapshots carry, so it looks the same online. Entry
   point: DrawBossDepartureFx, once a frame from screen_pass.inc. */

#define DEPARTURE_STAR_FALL_HEIGHT 380.f

// NOTE(zoubir): a disc of dots on the floor round Centre (screen space)
internal void
DrawDepartureDisc(render_context *RenderContext, v2 Centre, float Radius, u32 Color, float Turn)
{
    for(float Ring = 4.f; Ring <= Radius; Ring += 6.f)
    {
        DrawStarlessRing(RenderContext, Centre, Ring, 5.f, Color, Turn * (Ring / Radius), 1.f);
    }
}

internal void
DrawSunkenOmmoroth(render_context *RenderContext, v2 Ground, float Clock)
{
    float Pulse = 0.5f + 0.5f * Sin(2.f * Clock);
    DrawDepartureDisc(RenderContext, Ground, 54.f, UI_RGBA(12, 6, 22, 200), 0.f);
    DrawStarlessRing(RenderContext, Ground, 62.f + 4.f * Pulse, 3.f,
                     WithAlpha(STARLESS_VOID, 0.5f + 0.3f * Pulse), 0.6f * Clock, 1.f);
    DrawStarlessRing(RenderContext, Ground, 36.f, 3.f, WithAlpha(STARLESS_VOID_DEEP, 0.8f),
                     -0.9f * Clock, 1.f);
    // NOTE(zoubir): the dead star's light, deep down
    DrawStarlessDot(RenderContext, Ground, 4.f + 3.f * Pulse, WithAlpha(STARLESS_CORE, 0.6f));
}

internal void
DrawRisenNyxara(render_context *RenderContext, v2 Ground, float Clock)
{
    float Pulse = 0.5f + 0.5f * Sin(3.f * Clock);
    DrawDepartureDisc(RenderContext, Ground, 40.f, UI_RGBA(8, 4, 12, 170), 0.f);
    DrawStarlessRing(RenderContext, Ground, 48.f, 4.f, WithAlpha(STARLESS_GOLD, 0.6f + 0.3f * Pulse),
                     0.8f * Clock, 1.f);
    // NOTE(zoubir): a shaft of light up to where she went
    for(u32 Step = 0; Step < 24; Step++)
    {
        float Up = 12.f * (float)Step;
        float Fade = 1.f - (float)Step / 24.f;
        DrawStarlessDot(RenderContext, Ground - V2(0.f, Up), 3.f,
                        WithAlpha(STARLESS_GOLD_PALE, 0.5f * Fade));
    }
}

internal void
DrawVoidMawFx(render_context *RenderContext, world_entity *Maw, monster_ability *Ability,
              v2 Ground, float Clock)
{
    if (Maw->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = WindupProgress(Maw, Ability);
        DrawDepartureDisc(RenderContext, Ground, 8.f + 30.f * Progress,
                          UI_RGBA(14, 6, 26, 210), Clock);
        // NOTE(zoubir): fangs round the edge, closing in
        u32 Fangs = 8;
        for(u32 Fang = 0; Fang < Fangs; Fang++)
        {
            float Angle = 2.f * Pi32 * (float)Fang / (float)Fangs;
            v2 Out = V2(Cos(Angle), Sin(Angle));
            float Edge = Ability->Radius * (1.f - 0.45f * Progress);
            for(u32 Step = 0; Step < 3; Step++)
            {
                DrawStarlessDot(RenderContext, Ground + (Edge - 5.f * (float)Step) * Out,
                                4.f - (float)Step, WithAlpha(STARLESS_MIRROR, 0.4f + 0.5f * Progress));
            }
        }
        return;
    }
    // NOTE(zoubir): the jaws snap shut
    DrawDepartureDisc(RenderContext, Ground, Ability->Radius * 0.6f, UI_RGBA(30, 10, 50, 220), Clock);
    DrawStarlessRing(RenderContext, Ground, Ability->Radius, 4.f, STARLESS_VOID, Clock, 1.f);
}

internal void
DrawFallingStarFx(render_context *RenderContext, world_entity *Star, monster_ability *Ability,
                  v2 Ground, float Clock)
{
    if (Star->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = WindupProgress(Star, Ability);
        float Height = DEPARTURE_STAR_FALL_HEIGHT * Square(1.f - Progress);
        DrawStarlessDot(RenderContext, Ground, 6.f + 18.f * Progress, UI_RGBA(10, 6, 4, 140));
        // NOTE(zoubir): the star, and its tail up the way it came
        v2 Head = Ground - V2(0.f, Height);
        for(u32 Step = 1; Step < 8; Step++)
        {
            DrawStarlessDot(RenderContext, Head - V2(-2.f * (float)Step, 9.f * (float)Step),
                            7.f - 0.8f * (float)Step,
                            WithAlpha(STARLESS_GOLD, 0.8f - 0.1f * (float)Step));
        }
        DrawStarlessDot(RenderContext, Head, 9.f, STARLESS_GOLD_PALE);
        return;
    }
    // NOTE(zoubir): it bursts on the floor
    DrawStarlessRing(RenderContext, Ground, Ability->Radius, 5.f, STARLESS_GOLD, Clock, 1.f);
    DrawDepartureDisc(RenderContext, Ground, Ability->Radius * 0.5f, WithAlpha(STARLESS_GOLD_PALE, 0.7f), Clock);
}

internal void
DrawBossDepartureFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            Monster->Position.Z <= OUT_OF_SIGHT_HEIGHT)
        {
            continue;
        }
        v2 Ground = BurstToScreen(V3(Monster->Position.X, Monster->Position.Y,
                                     TerrainHeightAt(World, Monster->Position.X, Monster->Position.Y)),
                                  CameraOffset);
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        bool32 Striking = (Monster->AbilityPhase == AbilityPhase_Windup ||
                           Monster->AbilityPhase == AbilityPhase_Active) &&
            Monster->AbilityIndex < Def->AbilityCount;
        switch(Monster->MonsterKind)
        {
            case MonsterKind_Ommoroth: DrawSunkenOmmoroth(RenderContext, Ground, Clock); break;
            case MonsterKind_Nyxara: DrawRisenNyxara(RenderContext, Ground, Clock); break;
            case MonsterKind_VoidMaw:
            {
                if (Striking)
                {
                    DrawVoidMawFx(RenderContext, Monster, &Def->Abilities[Monster->AbilityIndex],
                                  Ground, Clock);
                }
            } break;
            case MonsterKind_FallingStar:
            {
                if (Striking)
                {
                    DrawFallingStarFx(RenderContext, Monster, &Def->Abilities[Monster->AbilityIndex],
                                      Ground, Clock);
                }
            } break;
            default: break;
        }
    }
}
