/* Rift effects: what the Aurora Rift's new ability kinds look like
   (sim/monster_abilities/waves_beams_shards.cpp, docs/dungeon-rift.md).

   Frost waves  while the monster winds up, frost gathers in a ring at its
                feet and the reach of the wave is traced faintly round
                it; then each ring rolls out as a band of ice spikes. When
                a ring is about to reach the local player standing on the
                ground, a chevron over their head says to jump.
   Beams        through the windup a thin line shows where the beam
                starts and a dotted arc the way it will sweep; then the
                beam burns along the floor out to the first pillar, a
                fading trail behind it.
   Pylon wards  a boss with an Aurora Pylon standing wears a shell of
                light, and a stream of light runs from each pylon into it
                (sim/dungeon/boss_wards.cpp).

   Everything is worked out from what snapshots carry (ability phase,
   index, timer, aim and points; monster kinds), so it looks the same
   online. Entry point: DrawRiftFx, once a frame from screen_pass.inc. */

#define RIFT_FROST_COLOR UI_RGBA(200, 240, 255, 235)
#define RIFT_FROST_DEEP UI_RGBA(110, 190, 245, 200)
#define RIFT_BEAM_CORE UI_RGBA(240, 255, 250, 245)
#define RIFT_BEAM_GREEN UI_RGBA(110, 250, 180, 210)
#define RIFT_BEAM_VIOLET UI_RGBA(180, 130, 250, 170)
#define RIFT_WARD_COLOR UI_RGBA(190, 150, 255, 220)
#define RIFT_JUMP_COLOR UI_RGBA(255, 250, 200, 250)
// NOTE(zoubir): how soon before a ring reaches the local player the jump
// chevron shows: about how long a jump takes to clear the ground
#define RIFT_JUMP_WARNING 0.35f

internal void
DrawRiftDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

// NOTE(zoubir): a band of ice spikes round Centre at Radius; Seed keeps
// each spike's height steady from frame to frame
internal void
DrawFrostRing(render_context *RenderContext, v2 Centre, float Radius, float Alpha,
              u32 Seed)
{
    u32 Count = (u32)Minimum(180.f, Maximum(16.f, 2.f * Pi32 * Radius / 9.f));
    for(u32 Spike = 0; Spike < Count; Spike++)
    {
        float Angle = 2.f * Pi32 * (float)Spike / (float)Count;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        u32 Hash = (Spike * 2654435761u) ^ Seed;
        float Tall = 4.f + (float)((Hash >> 7) % 5);
        v2 Foot = Centre + Radius * Out;
        DrawRiftDot(RenderContext, Foot - 5.f * Out, 3.f, WithAlpha(RIFT_FROST_DEEP, 0.6f * Alpha));
        DrawRiftDot(RenderContext, Foot, 3.f, WithAlpha(RIFT_FROST_COLOR, Alpha));
        // NOTE(zoubir): the spike stands up out of the ice, up the screen
        DrawFilledRectangle(RenderContext, Foot.X - 1.f, Foot.Y - Tall, 2.f, Tall,
                            WithAlpha(RIFT_FROST_COLOR, 0.8f * Alpha), 0.f);
    }
}

internal void
DrawJumpChevron(render_context *RenderContext, world_entity *Player, v3 CameraOffset,
                float Clock)
{
    v2 Head = Player->Position.XY - CameraOffset.XY;
    Head.Y -= Player->Dimensions.Y + 10.f + 3.f * Absolute(Sin(10.f * Clock));
    for(u32 Row = 0; Row < 2; Row++)
    {
        float Y = Head.Y - 6.f * Row;
        for(i32 Step = -3; Step <= 3; Step++)
        {
            v2 P = V2(Head.X + 2.f * Step, Y + 1.5f * Absolute((float)Step));
            DrawRiftDot(RenderContext, P, 3.f, RIFT_JUMP_COLOR);
        }
    }
}

internal void
DrawWaveFx(render_context *RenderContext, world_entity *Monster, monster_ability *Ability,
           world_entity *Local, v3 CameraOffset, float Clock, bool32 *WarnJump)
{
    v2 Centre = Monster->Position.XY - CameraOffset.XY;
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = Ability->Windup > 0.f ?
            1.f - Monster->AbilityTimer / Ability->Windup : 1.f;
        Progress = Minimum(1.f, Maximum(0.f, Progress));
        DrawFrostRing(RenderContext, Centre, 20.f + 25.f * Progress, 0.4f + 0.6f * Progress,
                      Monster->ID);
        // NOTE(zoubir): the reach, faint, so the party sees how far to run
        // or that running will not do
        u32 Dots = 64;
        for(u32 Dot = 0; Dot < Dots; Dot++)
        {
            float Angle = 2.f * Pi32 * ((float)Dot / (float)Dots + 0.02f * Clock);
            DrawRiftDot(RenderContext, Centre + Ability->Radius * V2(Cos(Angle), Sin(Angle)),
                        2.f, WithAlpha(RIFT_FROST_DEEP, 0.25f + 0.3f * Progress));
        }
        return;
    }
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    u32 Rings = Maximum(1u, Ability->Count);
    float LocalDistance = Local ? Length(Local->Position.XY - Monster->Position.XY) : 0.f;
    for(u32 Ring = 0; Ring < Rings; Ring++)
    {
        float Front = WaveFront(Ability, Elapsed, Ring);
        if (Front > 0.f && Front < Ability->Radius)
        {
            float Fade = 1.f - 0.5f * Front / Ability->Radius;
            DrawFrostRing(RenderContext, Centre, Front, Fade, Monster->ID + Ring * 977u);
        }
        // NOTE(zoubir): a ring the local player will meet in the next
        // moment, while it is still in reach
        float Gap = LocalDistance - Front;
        if (Local && LocalDistance <= Ability->Radius && Gap > -4.f &&
            Gap < RIFT_JUMP_WARNING * Ability->Speed)
        {
            *WarnJump = true;
        }
    }
}

internal void
DrawBeamLine(render_context *RenderContext, world *World, v2 From, v2 Direction,
             monster_ability *Ability, v3 CameraOffset, float Width, u32 Color, float Spacing)
{
    float Reach = BeamReach(World, From, Direction, Ability);
    v2 Side = V2(-Direction.Y, Direction.X);
    for(float Along = 10.f; Along < Reach; Along += Spacing)
    {
        v2 P = From + Along * Direction - CameraOffset.XY;
        for(float Across = -0.5f * Width; Across <= 0.5f * Width; Across += 3.f)
        {
            DrawRiftDot(RenderContext, P + Across * Side, 3.f, Color);
        }
    }
}

internal void
DrawBeamFx(render_context *RenderContext, world *World, world_entity *Monster,
           monster_ability *Ability, v3 CameraOffset, float Clock)
{
    v2 From = Monster->Position.XY;
    float Turn = BeamTurn(Monster);
    float Sweep = Ability->Spread * (Pi32 / 180.f);
    if (Monster->AbilityPhase == AbilityPhase_Windup)
    {
        float Progress = Ability->Windup > 0.f ?
            1.f - Monster->AbilityTimer / Ability->Windup : 1.f;
        bool32 Flash = Progress > 0.7f && ((u32)(Progress * 20.f) % 2) == 0;
        DrawBeamLine(RenderContext, World, From, Monster->AbilityAim, Ability, CameraOffset,
                     2.f, Flash ? RIFT_BEAM_CORE : WithAlpha(RIFT_BEAM_GREEN, 0.4f + 0.5f * Progress), 7.f);
        // NOTE(zoubir): the arc it will sweep, filling the way it turns
        u32 Dots = 24;
        for(u32 Dot = 0; Dot <= Dots; Dot++)
        {
            float Share = (float)Dot / (float)Dots;
            v2 Way = RotateBy(Monster->AbilityAim, Turn * Share * Sweep);
            u32 Color = Share <= Progress ? RIFT_BEAM_GREEN : RIFT_BEAM_VIOLET;
            DrawRiftDot(RenderContext, From + 70.f * Way - CameraOffset.XY, Share <= Progress ? 3.f : 2.f,
                        WithAlpha(Color, 0.7f));
        }
        return;
    }
    // NOTE(zoubir): the trail first, then the beam itself on top
    float Elapsed = AbilityActiveElapsed(Monster, Ability);
    float Share = Ability->Active > 0.f ? Minimum(1.f, Elapsed / Ability->Active) : 1.f;
    for(u32 Ghost = 3; Ghost >= 1; Ghost--)
    {
        float Back = Maximum(0.f, Share - 0.04f * Ghost);
        v2 Way = RotateBy(Monster->AbilityAim, Turn * Back * Sweep);
        DrawBeamLine(RenderContext, World, From, Way, Ability, CameraOffset,
                     Ability->Radius, WithAlpha(RIFT_BEAM_VIOLET, 0.35f / (float)Ghost), 9.f);
    }
    v2 Direction = BeamDirection(Monster, Ability);
    float Shimmer = 0.85f + 0.15f * Sin(30.f * Clock);
    DrawBeamLine(RenderContext, World, From, Direction, Ability, CameraOffset,
                 2.f * Ability->Radius, WithAlpha(RIFT_BEAM_GREEN, Shimmer), 4.f);
    DrawBeamLine(RenderContext, World, From, Direction, Ability, CameraOffset,
                 0.6f * Ability->Radius, RIFT_BEAM_CORE, 3.f);
}

// NOTE(zoubir): the shell of light round a warded boss and a stream from
// each pylon into it
internal void
DrawPylonWards(render_context *RenderContext, app_state *AppState, v3 CameraOffset, float Clock)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    if (!Run || Run->ShownBossKind >= MonsterKind_Count)
    {
        return;
    }
    world_entity *Boss = 0;
    bool32 AnyPylon = false;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster || Entity->Hp <= 0.f)
        {
            continue;
        }
        AnyPylon |= Entity->MonsterKind == MonsterKind_AuroraPylon;
        if ((u32)Entity->MonsterKind == Run->ShownBossKind)
        {
            Boss = Entity;
        }
    }
    if (!Boss || !AnyPylon)
    {
        return;
    }
    v2 BossAt = Boss->Position.XY - CameraOffset.XY;
    BossAt.Y -= 0.5f * Boss->Dimensions.Y;
    float Shell = 0.45f * Boss->Dimensions.X + 2.f * Sin(4.f * Clock);
    u32 Facets = 6;
    for(u32 Facet = 0; Facet < Facets; Facet++)
    {
        float A0 = 2.f * Pi32 * ((float)Facet / (float)Facets) + 0.3f * Clock;
        float A1 = A0 + 2.f * Pi32 / (float)Facets;
        v2 P0 = BossAt + Shell * V2(Cos(A0), Sin(A0));
        v2 P1 = BossAt + Shell * V2(Cos(A1), Sin(A1));
        for(u32 Step = 0; Step < 8; Step++)
        {
            DrawRiftDot(RenderContext, Lerp2(P0, (float)Step / 8.f, P1), 3.f, RIFT_WARD_COLOR);
        }
    }
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Pylon = &World->Entities[Index];
        if (!Pylon->IsPresent || Pylon->Type != EntityType_Monster || Pylon->Hp <= 0.f ||
            Pylon->MonsterKind != MonsterKind_AuroraPylon)
        {
            continue;
        }
        v2 Top = Pylon->Position.XY - CameraOffset.XY;
        Top.Y -= 0.75f * Pylon->Dimensions.Y;
        for(u32 Mote = 0; Mote < 14; Mote++)
        {
            float T = fmodf((float)Mote / 14.f + 0.8f * Clock, 1.f);
            v2 P = Lerp2(Top, T, BossAt);
            P.Y -= 26.f * Sin(Pi32 * T);
            DrawRiftDot(RenderContext, P, 2.f + 2.f * Sin(Pi32 * T),
                        WithAlpha((Mote % 2) ? RIFT_BEAM_GREEN : RIFT_WARD_COLOR, 0.9f));
        }
    }
}

internal void
DrawRiftFx(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);
    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && (Local->Hp <= 0.f || IsClearOfGround(Local)))
    {
        Local = 0;
    }
    bool32 WarnJump = false;
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
        if (Ability->Kind == MonsterAbility_Wave)
        {
            DrawWaveFx(RenderContext, Monster, Ability, Local, CameraOffset, Clock, &WarnJump);
        }
        else if (Ability->Kind == MonsterAbility_Beam)
        {
            DrawBeamFx(RenderContext, World, Monster, Ability, CameraOffset, Clock);
        }
    }
    if (WarnJump && Local)
    {
        DrawJumpChevron(RenderContext, Local, CameraOffset, Clock);
    }
    DrawPylonWards(RenderContext, AppState, CameraOffset, Clock);
}
