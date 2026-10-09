/* Boss lights (client/dungeon/boss_fx/): the light boss abilities throw on
   the floor and the bodies near them, added to the frame's list by
   GatherWorldLights (world_lights.cpp), which the world grade shader
   reads. A beam lights the floor along its length, an eclipse light
   glows gold, a well's core and a smite's mark glow while they gather,
   a falling star brightens as it comes down, and every boss impact
   flashes once (boss_impacts.cpp). */

global_variable world_light_look BossBeamLight = {110.f, 0.9f, {0.45f, 1.0f, 0.7f}, 0.3f};
global_variable world_light_look BossGoldLight = {150.f, 0.9f, {1.0f, 0.8f, 0.4f}, 0.2f};
global_variable world_light_look BossVoidLight = {120.f, 0.7f, {0.6f, 0.3f, 1.0f}, 0.2f};
global_variable world_light_look BossSmiteLight = {90.f, 0.8f, {1.0f, 0.2f, 0.45f}, 0.f};
// NOTE(zoubir): an impact lights this many times its own radius
#define BOSS_IMPACT_LIGHT_REACH 1.6f
#define BOSS_BEAM_LIGHTS 3

internal void
GatherBossLights(app_state *AppState, world_lights *Lights, v3 CameraOffset, float Zoom,
                 float WindowHeight)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster)
        {
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
        bool32 Active = Monster->AbilityPhase == AbilityPhase_Active;
        float Progress = Active ? 1.f : BossWindup(Monster, Ability);
        bool32 Pointed = Monster->AbilityPointCount > 0;
        switch(Ability->Kind)
        {
            case MonsterAbility_Beam:
            {
                if (Active)
                {
                    v2 From = Monster->Position.XY;
                    v2 Direction = BeamDirection(Monster, Ability);
                    float Reach = BeamReach(World, From, Direction, Ability);
                    for(u32 Light = 1; Light <= BOSS_BEAM_LIGHTS; Light++)
                    {
                        v2 P = From + (Reach * (float)Light / (float)(BOSS_BEAM_LIGHTS + 1)) * Direction;
                        AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, P, 0.f,
                                      BossBeamLight, 1.f);
                    }
                }
            } break;
            case MonsterAbility_Eclipse:
            {
                for(u32 Light = 0; Light < Monster->AbilityPointCount; Light++)
                {
                    world_light_look Look = BossGoldLight;
                    Look.Radius = 1.5f * Ability->Radius;
                    AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight,
                                  Monster->AbilityPoints[Light], 0.f, Look, 0.5f + 0.5f * Progress);
                }
            } break;
            case MonsterAbility_Pull:
            {
                if (Pointed)
                {
                    AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight,
                                  Monster->AbilityPoints[0], 0.f, BossVoidLight, 0.3f + 0.7f * Progress);
                }
            } break;
            case MonsterAbility_Smite:
            {
                if (Pointed && !Active)
                {
                    AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight,
                                  Monster->AbilityPoints[0], 0.f, BossSmiteLight, Progress);
                }
            } break;
            case MonsterAbility_Slam:
            {
                if (Monster->MonsterKind == MonsterKind_FallingStar && !Active)
                {
                    AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, Monster->Position.XY,
                                  0.f, BossGoldLight, Progress * Progress);
                }
            } break;
            default: break;
        }
    }

    boss_fx *Fx = AppState->BossFx;
    float Now = GetFxClock(AppState);
    for(u32 Index = 0; Fx && Index < Fx->ImpactCount; Index++)
    {
        boss_impact *Impact = &Fx->Impacts[Index];
        float Left = 1.f - (Now - Impact->Born) / BOSS_IMPACT_SECONDS;
        if (Left <= 0.f || Left > 1.f || Impact->MapId != World->MapId)
        {
            continue;
        }
        u32 C = Impact->Color;
        v3 Color = V3((float)(C & 0xFF), (float)((C >> 8) & 0xFF), (float)((C >> 16) & 0xFF));
        Color *= 1.f / Maximum(1.f, Maximum(Color.X, Maximum(Color.Y, Color.Z)));
        world_light_look Look = {BOSS_IMPACT_LIGHT_REACH * Impact->Radius, 1.2f, Color, 0.f};
        AddWorldLight(Lights, CameraOffset, Zoom, WindowHeight, Impact->At, 0.f, Look,
                      Left * Left);
    }
}
