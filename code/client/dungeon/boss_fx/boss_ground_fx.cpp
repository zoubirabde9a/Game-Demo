/* Boss ground effects (boss_fx.cpp): what the big ability kinds paint on
   the floor while they wind up and while they run, under the bodies.

   Gravity wells   a violet whirlpool over the well's reach, its arms
                   winding in faster as the collapse nears, the core dark
                   and fringed in red.
   Beams           a dashed warning line through the windup; then a beam
                   of light along the floor out to the first pillar, a
                   fading violet trail behind the way it sweeps.
   Eclipses        the dark rolls out from the caster as a front of
                   smoke; each light is a pool of gold with its countdown
                   round the rim.
   Waves           each ring rolls out as a band of light over the floor,
                   frost or, for Nyxara's Corona Flare, fire, under the
                   ice spikes rift_fx.cpp stands on it.
   Void brands     a violet ring the size of the burst under the branded,
                   a gold countdown closing round it.
   Smites          a crimson rune circle under the victim, its inner ring
                   closing as the blow gathers.
   Departures      the dark pool where Ommoroth sank, the black sun where
                   Nyxara rose, a Void Maw's pit opening with its fangs
                   closing in, and a Falling Star's mark with its shadow
                   growing.

   Drawn by DangerZones (danger_zones.cpp) from the ground pass, after the
   danger zones. Read only from what snapshots carry (ability phase,
   index, timer, aim and points; monster kinds and heights). */

#define BOSS_BEAM_GLOW_WIDTH 3.2f
#define BOSS_SUNKEN_RADIUS 72.f
#define BOSS_BLACK_SUN_RADIUS 64.f
#define BOSS_SMITE_MARK_MIN 44.f

internal void
DrawBossBeamGround(render_context *RenderContext, world *World, world_entity *Monster,
                   monster_ability *Ability, v3 CameraOffset)
{
    v2 From = Monster->Position.XY;
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = BossWindup(Monster, Ability);
        float Reach = BeamReach(World, From, Monster->AbilityAim, Ability);
        DrawBossBeam(RenderContext, World, CameraOffset, From, Monster->AbilityAim, Reach,
                     22.f, false, BOSS_BEAM_GREEN, 0.6f + 0.4f * Progress);
        return;
    }
    float Turn = BeamTurn(Monster);
    float Sweep = Ability->Spread * (Pi32 / 180.f);
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    float Share = Ability->Active > 0.f ? Minimum(1.f, Elapsed / Ability->Active) : 1.f;
    // NOTE(zoubir): the trail first, fainter the further behind
    for(u32 Ghost = 3; Ghost >= 1; Ghost--)
    {
        float Back = Maximum(0.f, Share - 0.035f * Ghost);
        v2 Way = RotateBy(Monster->AbilityAim, Turn * Back * Sweep);
        DrawBossBeam(RenderContext, World, CameraOffset, From, Way,
                     BeamReach(World, From, Way, Ability), 2.f * Ability->Radius, true,
                     BOSS_BEAM_VIOLET, 0.3f / (float)Ghost);
    }
    v2 Direction = BeamDirection(Monster, Ability);
    DrawBossBeam(RenderContext, World, CameraOffset, From, Direction,
                 BeamReach(World, From, Direction, Ability),
                 BOSS_BEAM_GLOW_WIDTH * Ability->Radius, true, BOSS_BEAM_GREEN, 1.f);
}

internal void
DrawBossWellGround(render_context *RenderContext, world *World, world_entity *Monster,
                   monster_ability *Ability, v3 CameraOffset)
{
    if (!Monster->AbilityPointCount || Ability->Radius <= 0.f)
    {
        return;
    }
    float Progress;
    float Strength;
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        float Windup = BossWindup(Monster, Ability);
        Progress = 0.3f * Windup;
        Strength = 0.35f + 0.5f * Windup;
    }
    else
    {
        float Elapsed = AbilityActiveElapsed(Monster, Ability);
        float Share = Ability->Active > 0.f ? Minimum(1.f, Elapsed / Ability->Active) : 1.f;
        Progress = 0.3f + 0.7f * Share;
        Strength = 1.f;
    }
    DrawBossMark(RenderContext, World, CameraOffset, Monster->AbilityPoints[0], Ability->Radius,
                 BossGround_Well, Progress, Ability->InnerRadius / Ability->Radius, Strength);
}

internal void
DrawBossEclipseGround(render_context *RenderContext, world *World, world_entity *Monster,
                      monster_ability *Ability, v3 CameraOffset)
{
    bool32 Falling = Monster->AbilityPhase == AbilityPhase_Active;
    float Progress = Falling ? 1.f : BossWindup(Monster, Ability);
    float Spread = 40.f + Ability->Spread;
    DrawBossMark(RenderContext, World, CameraOffset, Monster->Position.XY, Spread,
                 BossGround_EclipseDark, (40.f + Progress * Ability->Spread) / Spread, 0.f,
                 Falling ? 0.8f : 1.f);
    for(u32 Light = 0; Light < Monster->AbilityPointCount; Light++)
    {
        DrawBossMark(RenderContext, World, CameraOffset, Monster->AbilityPoints[Light],
                     Ability->Radius + 10.f, BossGround_EclipseLight, Progress,
                     Falling ? 0.f : 1.f - Progress, 1.f);
    }
}

internal void
DrawBossWaveGround(render_context *RenderContext, world *World, world_entity *Monster,
                   monster_ability *Ability, v3 CameraOffset)
{
    if (Monster->AbilityPhase != AbilityPhase_Active || Ability->Radius <= 0.f)
    {
        return;
    }
    float Fire = Monster->MonsterKind == MonsterKind_Nyxara ? 1.f : 0.f;
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    u32 Rings = Maximum(1u, Ability->Count);
    for(u32 Ring = 0; Ring < Rings; Ring++)
    {
        float Front = WaveFront(Ability, Elapsed, Ring);
        if (Front > 0.f && Front < Ability->Radius)
        {
            DrawBossMark(RenderContext, World, CameraOffset, Monster->Position.XY, Ability->Radius,
                         BossGround_Wave, Front / Ability->Radius, Fire,
                         1.f - 0.4f * Front / Ability->Radius);
        }
    }
}

internal void
DrawBossBrandGround(render_context *RenderContext, world *World, world_entity *Monster,
                    monster_ability *Ability, v3 CameraOffset)
{
    if (!Monster->AbilityPointCount || Monster->AbilityPhase != AbilityPhase_Windup)
    {
        return;
    }
    DrawBossMark(RenderContext, World, CameraOffset, Monster->AbilityPoints[0], Ability->Radius,
                 BossGround_Brand, BossWindup(Monster, Ability), 0.f, 1.f);
}

internal void
DrawBossSmiteGround(render_context *RenderContext, world *World, world_entity *Monster,
                    monster_ability *Ability, v3 CameraOffset)
{
    if (!Monster->AbilityPointCount || Monster->AbilityPhase != AbilityPhase_Windup)
    {
        return;
    }
    float Progress = BossWindup(Monster, Ability);
    float Radius = Maximum(BOSS_SMITE_MARK_MIN, 1.5f * Ability->Radius);
    DrawBossMark(RenderContext, World, CameraOffset, Monster->AbilityPoints[0], Radius,
                 BossGround_Smite, Progress, 0.f, 0.7f + 0.3f * Progress);
}

// NOTE(zoubir): a Starless boss gone from its fight and what falls in the
// meantime hang out of sight (boss_departures.cpp); their marks go on the
// floor under them
internal void
DrawBossDepartureGround(render_context *RenderContext, world *World, world_entity *Monster,
                        v3 CameraOffset)
{
    monster_def *Def = GetMonsterDef(Monster->MonsterKind);
    monster_ability *Ability = Monster->AbilityIndex < Def->AbilityCount ?
        &Def->Abilities[Monster->AbilityIndex] : 0;
    bool32 Striking = Ability && Monster->AbilityPhase == AbilityPhase_Windup;
    v2 At = Monster->Position.XY;
    switch(Monster->MonsterKind)
    {
        case MonsterKind_Ommoroth:
        {
            DrawBossMark(RenderContext, World, CameraOffset, At, BOSS_SUNKEN_RADIUS,
                         BossGround_SunkenDark, 0.f, 0.f, 1.f);
        } break;
        case MonsterKind_Nyxara:
        {
            DrawBossMark(RenderContext, World, CameraOffset, At, BOSS_BLACK_SUN_RADIUS,
                         BossGround_BlackSun, 0.f, 0.f, 1.f);
        } break;
        case MonsterKind_VoidMaw:
        {
            if (Striking)
            {
                DrawBossMark(RenderContext, World, CameraOffset, At, Ability->Radius,
                             BossGround_VoidMaw, BossWindup(Monster, Ability), 0.f, 1.f);
            }
        } break;
        case MonsterKind_FallingStar:
        {
            if (Striking)
            {
                DrawBossMark(RenderContext, World, CameraOffset, At, Ability->Radius,
                             BossGround_StarMark, BossWindup(Monster, Ability), 0.f, 1.f);
            }
        } break;
        default: break;
    }
}

internal void
DrawBossGroundFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Now = GetFxClock(AppState);
    WatchBossCollapses(AppState, Now);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster)
        {
            continue;
        }
        if (Monster->Position.Z > OUT_OF_SIGHT_HEIGHT)
        {
            DrawBossDepartureGround(RenderContext, World, Monster, CameraOffset);
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount ||
            (Monster->AbilityPhase != AbilityPhase_Windup &&
             Monster->AbilityPhase != AbilityPhase_Active))
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        switch(Ability->Kind)
        {
            case MonsterAbility_Beam: DrawBossBeamGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            case MonsterAbility_Pull: DrawBossWellGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            case MonsterAbility_Eclipse: DrawBossEclipseGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            case MonsterAbility_Wave: DrawBossWaveGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            case MonsterAbility_Brand: DrawBossBrandGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            case MonsterAbility_Smite: DrawBossSmiteGround(RenderContext, World, Monster, Ability, CameraOffset); break;
            default: break;
        }
    }
    DrawBossImpacts(RenderContext, AppState, CameraOffset, Now);
}
