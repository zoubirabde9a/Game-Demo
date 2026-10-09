/* Starless effects: what the Starless Deep's new ability kinds look like
   (sim/monster_abilities/wells_brands_mirrors.cpp, docs/dungeon-starless.md).

   Gravity wells  through the windup the reach of the well is traced in
                  violet with chevrons pointing in, and its core in red;
                  then motes spiral in from the edge while the core
                  darkens, until it collapses.
   Void brands    a ring the size of the burst follows the branded,
                  closing in as the windup runs out; whoever carries it
                  sees a sigil over their own head, so they know to run.
   Mirrors        through the windup a silver shell closes round the
                  monster; while it holds, the shell shines and glints, so
                  anyone about to hit it knows to stop.
   Eclipses       each light is a pillar of gold on the floor, its edge
                  marked and a countdown closing round it; when the dark
                  falls the lights flash.

   Everything is worked out from what snapshots carry (ability phase,
   index, timer, aim and points), so it looks the same online. Entry
   point: DrawStarlessFx, once a frame from screen_pass.inc. */

#define STARLESS_VOID UI_RGBA(150, 90, 240, 220)
#define STARLESS_VOID_DEEP UI_RGBA(70, 30, 140, 200)
#define STARLESS_CORE UI_RGBA(255, 80, 110, 230)
#define STARLESS_GOLD UI_RGBA(255, 214, 110, 235)
#define STARLESS_GOLD_PALE UI_RGBA(255, 244, 200, 245)
#define STARLESS_MIRROR UI_RGBA(220, 228, 255, 235)
// NOTE(zoubir): how near the brand's point the local player must be to
// count as the one who carries it; snapshots trail the local player a
// little
#define STARLESS_BRAND_NEAR 48.f

internal void
DrawStarlessDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

// NOTE(zoubir): a dotted circle; Turn spins it, Share draws only that much
// of it, for a countdown
internal void
DrawStarlessRing(render_context *RenderContext, v2 Centre, float Radius, float Size,
                 u32 Color, float Turn, float Share)
{
    u32 Dots = (u32)Minimum(160.f, Maximum(12.f, 2.f * Pi32 * Radius / 8.f));
    u32 Shown = (u32)(Share * (float)Dots + 0.5f);
    for(u32 Dot = 0; Dot < Shown; Dot++)
    {
        float Angle = Turn + 2.f * Pi32 * (float)Dot / (float)Dots - 0.5f * Pi32;
        DrawStarlessDot(RenderContext, Centre + Radius * V2(Cos(Angle), Sin(Angle)), Size, Color);
    }
}

inline float
WindupProgress(world_entity *Monster, monster_ability *Ability)
{
    float Result = Ability->Windup > 0.f ? 1.f - Monster->AbilityTimer / Ability->Windup : 1.f;
    Result = Minimum(1.f, Maximum(0.f, Result));
    return Result;
}

internal void
DrawWellFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
           v3 CameraOffset, float Clock)
{
    if (!Monster->AbilityPointCount)
    {
        return;
    }
    v2 Well = Monster->AbilityPoints[0] - CameraOffset.XY;
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = WindupProgress(Monster, Ability);
        DrawStarlessRing(RenderContext, Well, Ability->Radius, 2.f,
                         WithAlpha(STARLESS_VOID, 0.3f + 0.5f * Progress), 0.1f * Clock, 1.f);
        DrawStarlessRing(RenderContext, Well, Ability->InnerRadius, 3.f,
                         WithAlpha(STARLESS_CORE, 0.4f + 0.5f * Progress), -0.4f * Clock, 1.f);
        // NOTE(zoubir): chevrons round the edge pointing in
        u32 Chevrons = 10;
        for(u32 Chevron = 0; Chevron < Chevrons; Chevron++)
        {
            float Angle = 2.f * Pi32 * (float)Chevron / (float)Chevrons + 0.2f * Clock;
            v2 Out = V2(Cos(Angle), Sin(Angle));
            v2 Side = V2(-Out.Y, Out.X);
            v2 Tip = Well + (Ability->Radius - 14.f - 6.f * Progress) * Out;
            for(i32 Step = -2; Step <= 2; Step++)
            {
                v2 P = Tip + (3.f * (float)Step) * Side + (2.f * Absolute((float)Step)) * Out;
                DrawStarlessDot(RenderContext, P, 3.f, WithAlpha(STARLESS_VOID, 0.8f));
            }
        }
        return;
    }
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    float Share = Ability->Active > 0.f ? Minimum(1.f, Elapsed / Ability->Active) : 1.f;
    // NOTE(zoubir): motes spiral in from the edge, faster as it nears
    // the collapse
    u32 Motes = 48;
    for(u32 Mote = 0; Mote < Motes; Mote++)
    {
        float Phase = fmodf((float)Mote / (float)Motes + (0.6f + Share) * Clock, 1.f);
        float Distance = Ability->Radius * (1.f - Phase);
        float Angle = 2.f * Pi32 * (float)((Mote * 37) % Motes) / (float)Motes + 3.f * Phase;
        u32 Color = (Mote % 3) ? STARLESS_VOID : STARLESS_GOLD;
        DrawStarlessDot(RenderContext, Well + Distance * V2(Cos(Angle), Sin(Angle)),
                        2.f + 2.f * Phase, WithAlpha(Color, 0.4f + 0.5f * Phase));
    }
    DrawStarlessRing(RenderContext, Well, Ability->Radius, 2.f,
                     WithAlpha(STARLESS_VOID_DEEP, 0.5f), 0.f, 1.f);
    // NOTE(zoubir): the core, the part that collapses, fills as time runs
    float Core = Ability->InnerRadius;
    DrawStarlessRing(RenderContext, Well, Core, 4.f, STARLESS_CORE, Clock, 1.f);
    DrawStarlessRing(RenderContext, Well, Core * (1.f - Share), 3.f,
                     WithAlpha(STARLESS_CORE, 0.7f), -Clock, 1.f);
    DrawStarlessDot(RenderContext, Well, 6.f + 6.f * Share, UI_RGBA(20, 6, 30, 240));
}

// NOTE(zoubir): a sigil of four points spinning over a player's head
internal void
DrawBrandSigil(render_context *RenderContext, v2 Head, float Clock, float Pulse)
{
    for(u32 Point = 0; Point < 4; Point++)
    {
        float Angle = 0.5f * Pi32 * (float)Point + 2.f * Clock;
        v2 Out = V2(Cos(Angle), 0.6f * Sin(Angle));
        for(u32 Step = 0; Step < 4; Step++)
        {
            DrawStarlessDot(RenderContext, Head + (3.f + 3.f * Step) * Out,
                            4.f - (float)Step * 0.6f,
                            WithAlpha(Step < 2 ? STARLESS_GOLD : STARLESS_VOID, Pulse));
        }
    }
    DrawStarlessDot(RenderContext, Head, 5.f, WithAlpha(STARLESS_GOLD_PALE, Pulse));
}

internal void
DrawBrandFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
            world_entity *Local, v3 CameraOffset, float Clock)
{
    if (!Monster->AbilityPointCount || Monster->AbilityPhase != AbilityPhase_Windup)
    {
        return;
    }
    v2 Mark = Monster->AbilityPoints[0];
    v2 At = Mark - CameraOffset.XY;
    float Progress = WindupProgress(Monster, Ability);
    float Pulse = 0.6f + 0.4f * Absolute(Sin((4.f + 10.f * Progress) * Clock));
    DrawStarlessRing(RenderContext, At, Ability->Radius, 3.f,
                     WithAlpha(STARLESS_VOID, Pulse), 0.3f * Clock, 1.f);
    // NOTE(zoubir): the countdown closes round the burst's edge
    DrawStarlessRing(RenderContext, At, Ability->Radius - 6.f, 2.f,
                     WithAlpha(STARLESS_GOLD, 0.9f), 0.f, 1.f - Progress);
    DrawStarlessRing(RenderContext, At, 18.f + 6.f * Progress, 3.f,
                     WithAlpha(STARLESS_GOLD, Pulse), -Clock, 1.f);
    bool32 Carried = Local && LengthSq(Local->Position.XY - Mark) <= Square(STARLESS_BRAND_NEAR);
    v2 Head = (Carried ? Local->Position.XY : Mark) - CameraOffset.XY;
    Head.Y -= (Carried ? Local->Dimensions.Y : 40.f) + 16.f;
    DrawBrandSigil(RenderContext, Head, Clock, Carried ? 1.f : 0.7f);
}

internal void
DrawMirrorFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
             v3 CameraOffset, float Clock)
{
    v2 Centre = Monster->Position.XY - CameraOffset.XY;
    Centre.Y -= 0.5f * Monster->Dimensions.Y;
    float Shell = Ability->Radius;
    bool32 Raised = Monster->AbilityPhase == AbilityPhase_Active;
    float Progress = Raised ? 1.f : WindupProgress(Monster, Ability);
    u32 Facets = 6;
    for(u32 Facet = 0; Facet < Facets; Facet++)
    {
        float A0 = 2.f * Pi32 * ((float)Facet / (float)Facets) - 0.2f * Clock;
        float A1 = A0 + 2.f * Pi32 / (float)Facets;
        // NOTE(zoubir): the facets swing in from far out as it rises
        float Reach = Shell * (1.f + 1.5f * (1.f - Progress));
        v2 P0 = Centre + Reach * V2(Cos(A0), Sin(A0));
        v2 P1 = Centre + Reach * V2(Cos(A1), Sin(A1));
        for(u32 Step = 0; Step < 8; Step++)
        {
            DrawStarlessDot(RenderContext, Lerp2(P0, (float)Step / 8.f, P1), Raised ? 4.f : 3.f,
                            WithAlpha(STARLESS_MIRROR, Raised ? 0.95f : 0.35f + 0.5f * Progress));
        }
    }
    if (Raised)
    {
        // NOTE(zoubir): a glint runs round the shell, and the time left
        // shows as a gold arc
        float Angle = 3.f * Clock;
        v2 Glint = Centre + Shell * V2(Cos(Angle), Sin(Angle));
        DrawStarlessDot(RenderContext, Glint, 7.f, STARLESS_GOLD_PALE);
        DrawStarlessDot(RenderContext, Centre + Shell * V2(Cos(Angle + Pi32), Sin(Angle + Pi32)),
                        5.f, STARLESS_MIRROR);
        float Left = Ability->Active > 0.f ? Monster->AbilityTimer / Ability->Active : 0.f;
        DrawStarlessRing(RenderContext, Centre, Shell + 7.f, 2.f, STARLESS_GOLD, 0.f,
                         Minimum(1.f, Maximum(0.f, Left)));
    }
}

internal void
DrawEclipseFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
              v3 CameraOffset, float Clock)
{
    bool32 Falling = Monster->AbilityPhase == AbilityPhase_Active;
    float Progress = Falling ? 1.f : WindupProgress(Monster, Ability);
    // NOTE(zoubir): a dark ring spreads from her as the light goes out
    v2 From = Monster->Position.XY - CameraOffset.XY;
    DrawStarlessRing(RenderContext, From, 40.f + Progress * Ability->Spread, 4.f,
                     WithAlpha(STARLESS_VOID_DEEP, 0.6f * (1.f - 0.5f * Progress)), 0.f, 1.f);
    for(u32 Light = 0; Light < Monster->AbilityPointCount; Light++)
    {
        v2 At = Monster->AbilityPoints[Light] - CameraOffset.XY;
        float Radius = Ability->Radius;
        // NOTE(zoubir): the floor of the light, a dotted disc
        for(float R = 10.f; R < Radius; R += 12.f)
        {
            DrawStarlessRing(RenderContext, At, R, 2.f,
                             WithAlpha(STARLESS_GOLD, Falling ? 0.7f : 0.2f + 0.2f * Progress),
                             0.2f * Clock + R, 1.f);
        }
        DrawStarlessRing(RenderContext, At, Radius, 4.f,
                         Falling ? STARLESS_GOLD_PALE : WithAlpha(STARLESS_GOLD, 0.9f), 0.f, 1.f);
        if (!Falling)
        {
            DrawStarlessRing(RenderContext, At, Radius + 8.f, 3.f, STARLESS_GOLD_PALE, 0.f,
                             1.f - Progress);
        }
        // NOTE(zoubir): a shaft of light standing up out of it
        for(u32 Ray = 0; Ray < 12; Ray++)
        {
            float X = At.X - 0.6f * Radius + 1.2f * Radius * (float)Ray / 11.f;
            float Tall = 30.f + 20.f * Absolute(Sin(1.3f * Clock + (float)Ray));
            DrawFilledRectangle(RenderContext, X - 1.f, At.Y - Tall, 2.f, Tall,
                                WithAlpha(STARLESS_GOLD_PALE, 0.25f + 0.3f * Progress), 0.f);
        }
    }
}

internal void
DrawStarlessFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && Local->Hp <= 0.f)
    {
        Local = 0;
    }
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Monster = &World->Entities[Index];
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            (Monster->AbilityPhase != AbilityPhase_Windup &&
             Monster->AbilityPhase != AbilityPhase_Active))
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Monster->MonsterKind);
        if (Monster->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Monster->AbilityIndex];
        switch(Ability->Kind)
        {
            case MonsterAbility_Pull: DrawWellFx(RenderContext, Monster, Ability, CameraOffset, Clock); break;
            case MonsterAbility_Brand: DrawBrandFx(RenderContext, Monster, Ability, Local, CameraOffset, Clock); break;
            case MonsterAbility_Reflect: DrawMirrorFx(RenderContext, Monster, Ability, CameraOffset, Clock); break;
            case MonsterAbility_Eclipse: DrawEclipseFx(RenderContext, Monster, Ability, CameraOffset, Clock); break;
            default: break;
        }
    }
}
