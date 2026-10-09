/* Boss effect drawing (boss_fx.cpp): one quad each through the three boss
   shaders, flat on the ground in the world pass so bodies stand on them
   and the grade and the bloom reach them.

   DrawBossMark     a circle on the floor (build/shaders/fx/boss_ground.frag),
                    in one of the BossGround_* looks
   DrawBossBeam     a beam of light along the floor, or up into the sky
                    (boss_beam.frag)
   DrawBossShock    the shockwave of a blow landing (boss_shock.frag)

   Every one is skipped when its shader did not build, so the dotted
   effects drawn over the world (rift_fx.cpp, starless_fx.cpp,
   boss_departure_fx.cpp) stay the fallback; BossFxReady says which. */

enum boss_ground_look
{
    BossGround_Well,
    BossGround_EclipseLight,
    BossGround_VoidMaw,
    BossGround_StarMark,
    BossGround_SunkenDark,
    BossGround_BlackSun,
    BossGround_Smite,
    BossGround_EclipseDark,
    BossGround_Wave,
    BossGround_Brand,
    BossGround_Mirror,
};

// NOTE(zoubir): the beam palettes, as boss_beam.frag reads colour b
#define BOSS_BEAM_GREEN 0.f
#define BOSS_BEAM_VIOLET 0.33f
#define BOSS_BEAM_CRIMSON 0.66f
#define BOSS_BEAM_GOLD 1.f

inline bool32
BossFxReady(render_context *RenderContext, u32 Shader)
{
    bool32 Result = RenderContext->Programs[Shader].ID != RenderContext->TextureProgram.ID;
    return Result;
}

// NOTE(zoubir): four numbers 0 to 1 in the vertex colour, r in the low
// byte, as the shaders read them back
inline u32
BossFxColor(float R, float G, float B, float A)
{
    u32 Result = ((u32)(255.f * Clamp01(A)) << 24) | ((u32)(255.f * Clamp01(B)) << 16) |
        ((u32)(255.f * Clamp01(G)) << 8) | (u32)(255.f * Clamp01(R));
    return Result;
}

inline float
BossWindup(world_entity *Monster, monster_ability *Ability)
{
    float Result = Ability->Windup > 0.f ?
        Clamp01(1.f - Monster->AbilityTimer / Ability->Windup) : 1.f;
    return Result;
}

internal void
DrawBossQuad(render_context *RenderContext, world *World, v3 CameraOffset, u32 Shader,
             u32 Blend, v2 Centre, float Width, float Height, float Angle, u32 Color,
             float SortLift)
{
    float GroundZ = TerrainHeightAt(World, Centre.X, Centre.Y);
    // NOTE(zoubir): flat ground's key is so big only whole steps change
    // it; on a raised top a whole step would pass the bodies near it
    float Key = DangerZoneSortKey(World, V3(Centre.X, Centre.Y, GroundZ)) +
        (GroundZ > 0.f ? 0.05f * SortLift : SortLift);
    BeginBatch(RenderContext, 0, Key, RenderContext->Programs[Shader]);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = Blend;
    RenderQuadTexture(RenderContext, Centre.X - 0.5f * Width - CameraOffset.X,
                      Centre.Y - GroundZ - 0.5f * Height - CameraOffset.Y,
                      Width, Height, V4(0.f, 1.f, 1.f, 0.f), Color, 0.f, Angle);
    EndBatch(RenderContext);
}

// NOTE(zoubir): Progress and Param as boss_ground.frag reads them for Look
internal void
DrawBossMark(render_context *RenderContext, world *World, v3 CameraOffset, v2 Centre,
             float Radius, boss_ground_look Look, float Progress, float Param, float Strength)
{
    if (!BossFxReady(RenderContext, Shader_BossGround) || Radius <= 0.f)
    {
        return;
    }
    u32 Color = BossFxColor(Progress, (float)Look * (16.f / 255.f), Param, Strength);
    // NOTE(zoubir): a step over the danger zones, which still show through
    DrawBossQuad(RenderContext, World, CameraOffset, Shader_BossGround, RenderBlend_Alpha,
                 Centre, 2.f * Radius, 2.f * Radius, 0.f, Color, 1.f);
}

// NOTE(zoubir): a BossGround_* look drawn over the bodies (screen_pass.inc)
// instead of on the floor, round Centre in screen space
internal void
DrawBossShell(render_context *RenderContext, v2 Centre, float Radius, boss_ground_look Look,
              float Progress, float Param, float Strength)
{
    if (!BossFxReady(RenderContext, Shader_BossGround) || Radius <= 0.f)
    {
        return;
    }
    DrawShaderQuad(RenderContext, Shader_BossGround, Centre.X - Radius, Centre.Y - Radius,
                   2.f * Radius, 2.f * Radius,
                   BossFxColor(Progress, (float)Look * (16.f / 255.f), Param, Strength));
}

// NOTE(zoubir): a beam from From, Length along Direction, Width across;
// Live false draws the thin warning line
internal void
DrawBossBeam(render_context *RenderContext, world *World, v3 CameraOffset, v2 From,
             v2 Direction, float Length, float Width, bool32 Live, float Palette,
             float Strength)
{
    if (!BossFxReady(RenderContext, Shader_BossBeam) || Length <= 0.f)
    {
        return;
    }
    v2 Middle = From + 0.5f * Length * Direction;
    u32 Color = BossFxColor(Strength, Live ? 1.f : 0.f, Palette, 1.f);
    // NOTE(zoubir): two whole steps over the cracks and the danger zones,
    // over the boss marks too, so the light is on top of them
    DrawBossQuad(RenderContext, World, CameraOffset, Shader_BossBeam, RenderBlend_Additive,
                 Middle, Length, Width, ATan2(Direction.Y, Direction.X), Color, 2.f);
}

// NOTE(zoubir): a shaft of light standing up from Foot, Height tall, drawn
// over the world (screen_pass.inc) since it stands in front of whatever
// is behind it; a Steady one is a calm pillar, else it crackles like the
// live beams
internal void
DrawBossShaft(render_context *RenderContext, v2 Foot, float Height, float Width,
              float Palette, float Strength, bool32 Steady = false)
{
    if (!BossFxReady(RenderContext, Shader_BossBeam) || Height <= 0.f)
    {
        return;
    }
    BeginBatch(RenderContext, 0, 0.f, RenderContext->Programs[Shader_BossBeam]);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
    v2 Middle = Foot - V2(0.f, 0.5f * Height);
    RenderQuadTexture(RenderContext, Middle.X - 0.5f * Height, Middle.Y - 0.5f * Width,
                      Height, Width, V4(0.f, 1.f, 1.f, 0.f),
                      BossFxColor(Strength, Steady ? 0.5f : 1.f, Palette, 1.f), 0.f, -0.5f * Pi32);
    EndBatch(RenderContext);
}

// NOTE(zoubir): Age 0 the moment it lands, 1 when it is gone
internal void
DrawBossShock(render_context *RenderContext, world *World, v3 CameraOffset, v2 Centre,
              float Radius, u32 Color, float Age)
{
    if (!BossFxReady(RenderContext, Shader_BossShock) || Radius <= 0.f)
    {
        return;
    }
    DrawBossQuad(RenderContext, World, CameraOffset, Shader_BossShock, RenderBlend_Additive,
                 Centre, 2.f * Radius, 2.f * Radius, 0.f, WithAlpha(Color, Age), 2.f);
}
