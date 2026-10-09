/* Boss impacts (boss_fx.cpp): what a boss's blow does the moment it
   lands, on top of the danger zone's flash (danger_zones.cpp calls
   AddBossImpact for every windup that went on to its hit, a void
   brand's burst among them).

   A shockwave rolls out over the area it hit in the blow's colour, the
   ground under it flashes with light (world_lights.cpp, through
   GatherBossLights), and the camera shakes for whoever stands near. A
   Smite and a Falling Star also drop a shaft of light out of the sky onto
   where they land; a gravity well's collapse gets its own shock too
   (WatchBossCollapses), and so does an add erupting as it merges into
   its boss (WatchAddEruptions).

   Only the shown boss's casts, what falls while a Starless boss is away
   and Vaelith's frost count, so a room of packs stays calm; all of it is read from what
   snapshots carry, so online looks the same. */

#define BOSS_IMPACT_MAX 24
#define BOSS_IMPACT_SECONDS 0.7f
#define BOSS_SHAFT_SECONDS 0.45f
#define BOSS_SHAFT_HEIGHT 420.f
// NOTE(zoubir): how much camera shake a boss's blow adds, at most, for a
// player standing on it (fx_bursts.cpp fades it with distance)
#define BOSS_IMPACT_SHAKE 0.32f
#define BOSS_SMITE_SHOCK_RADIUS 70.f
// NOTE(zoubir): a Smite's caster that moved this far as it struck blinked
// to its victim (trigger.cpp); a puff marks where it left and arrived
#define BOSS_BLINK_STEP_MIN 40.f
#define BOSS_BLINK_PUFF_RADIUS 46.f
// NOTE(zoubir): an add's eruption hits everyone in the room, so its shock
// rolls out this far from the boss, and shakes every camera this much more
#define BOSS_ERUPTION_RADIUS 520.f
#define BOSS_ERUPTION_SHAKE 0.45f
#define BOSS_FX_TRACKED ArrayCount(((world *)0)->Entities)

#define BOSS_SHOCK_SPIRIT UI_RGBA(190, 120, 255, 255)
#define BOSS_SHOCK_VOID UI_RGBA(160, 90, 255, 255)
#define BOSS_SHOCK_GOLD UI_RGBA(255, 210, 110, 255)
#define BOSS_SHOCK_CRIMSON UI_RGBA(255, 50, 120, 255)
#define BOSS_SHOCK_FROST UI_RGBA(170, 225, 255, 255)

struct boss_impact
{
    v2 At;
    float Radius;
    u32 Color;
    float Born;
    u32 MapId;
    // NOTE(zoubir): a shaft of light comes down onto it, in this beam
    // palette (boss_beam.frag), or -1 for none
    float Shaft;
};

struct boss_fx
{
    boss_impact Impacts[BOSS_IMPACT_MAX];
    u32 ImpactCount;
    // NOTE(zoubir): per entity slot, the id of the monster whose well was
    // open last frame, so its collapse can be seen; 0 for none
    u32 WellOpen[BOSS_FX_TRACKED];
    v2 WellAt[BOSS_FX_TRACKED];
    // NOTE(zoubir): the add clock the HUD showed last frame
    u32 AddSeconds;
    bool32 AddBursts;
};

internal boss_fx *
GetBossFx(app_state *AppState)
{
    if (!AppState->BossFx)
    {
        AppState->BossFx = AllocateStruct(&AppState->MemoryArena, boss_fx);
        *AppState->BossFx = {};
    }
    return AppState->BossFx;
}

// NOTE(zoubir): the boss of the fight under way, what falls on the party
// while a Starless boss is away, and Vaelith's Frost Marks and Ice Tombs
internal bool32
IsBossCaster(app_state *AppState, u32 Kind)
{
    dungeon_run *Run = AppState->Dungeon;
    bool32 Result = (Run && Run->ShownBossKind == Kind) ||
        Kind == MonsterKind_VoidMaw || Kind == MonsterKind_FallingStar ||
        Kind == MonsterKind_FrostMark || Kind == MonsterKind_IceTomb;
    return Result;
}

internal void
PushBossImpact(app_state *AppState, v2 At, float Radius, u32 Color, float Shaft, float Now)
{
    boss_fx *Fx = GetBossFx(AppState);
    if (Fx->ImpactCount == BOSS_IMPACT_MAX)
    {
        for(u32 Index = 1; Index < Fx->ImpactCount; Index++)
        {
            Fx->Impacts[Index - 1] = Fx->Impacts[Index];
        }
        Fx->ImpactCount--;
    }
    boss_impact *Impact = &Fx->Impacts[Fx->ImpactCount++];
    Impact->At = At;
    Impact->Radius = Radius;
    Impact->Color = Color;
    Impact->Born = Now;
    Impact->MapId = AppState->World.MapId;
    Impact->Shaft = Shaft;

    world_entity *Local = GetLocalPlayer(AppState);
    if (Local && AppState->FxBursts)
    {
        float Distance = Maximum(0.f, Length(At - Local->Position.XY) - Radius);
        float Near = 1.f - Clamp01((Distance - SHAKE_NEAR) / (SHAKE_FAR - SHAKE_NEAR));
        float Size = Clamp01(Radius / 160.f);
        AppState->FxBursts->Trauma = Minimum(1.f, AppState->FxBursts->Trauma +
                                             Near * BOSS_IMPACT_SHAKE * (0.5f + 0.5f * Size));
    }
}

// NOTE(zoubir): called by danger_zones.cpp when a windup goes on to its
// hit, with the cast as it was locked in
internal void
AddBossImpact(app_state *AppState, danger_cast *Cast, float Now)
{
    u32 CasterKind = Cast->CasterKind;
    if (!IsBossCaster(AppState, CasterKind))
    {
        return;
    }
    monster_ability *Ability = Cast->Ability;
    u32 Color = CastTellColor(Ability->Kind, 255);
    switch(Ability->Kind)
    {
        case MonsterAbility_Slam:
        {
            if (CasterKind == MonsterKind_VoidMaw)
            {
                PushBossImpact(AppState, Cast->Self, Ability->Radius * 1.3f, BOSS_SHOCK_VOID, -1.f, Now);
            }
            else if (CasterKind == MonsterKind_FrostMark || CasterKind == MonsterKind_IceTomb)
            {
                // NOTE(zoubir): the ring closing and freezing who it held,
                // or a tomb shattering on who is inside
                PushBossImpact(AppState, Cast->Self, Ability->Radius * 1.3f, BOSS_SHOCK_FROST, -1.f, Now);
            }
            else if (CasterKind == MonsterKind_FallingStar)
            {
                PushBossImpact(AppState, Cast->Self, Ability->Radius * 1.5f, BOSS_SHOCK_GOLD,
                               BOSS_BEAM_GOLD, Now);
            }
            else
            {
                PushBossImpact(AppState, Cast->Self, Ability->Radius * 1.25f, Color, -1.f, Now);
            }
        } break;

        case MonsterAbility_Mortar:
        {
            for(u32 Point = 0; Point < Cast->PointCount; Point++)
            {
                PushBossImpact(AppState, Cast->Points[Point], Ability->Radius * 1.3f, Color, -1.f, Now);
            }
        } break;

        case MonsterAbility_Burrow:
        {
            // NOTE(zoubir): coming up through the floor where it was marked
            if (Cast->PointCount)
            {
                PushBossImpact(AppState, Cast->Points[0], Ability->Radius * 1.3f, Color, -1.f, Now);
            }
        } break;

        case MonsterAbility_Blink:
        {
            if (Cast->PointCount)
            {
                PushBossImpact(AppState, Cast->Points[0], Ability->Radius * 1.2f,
                               BOSS_SHOCK_SPIRIT, -1.f, Now);
            }
        } break;

        case MonsterAbility_Smite:
        {
            if (Cast->PointCount)
            {
                PushBossImpact(AppState, Cast->Points[0], BOSS_SMITE_SHOCK_RADIUS,
                               BOSS_SHOCK_CRIMSON, BOSS_BEAM_CRIMSON, Now);
            }
            if (Length(Cast->Landed - Cast->Self) > BOSS_BLINK_STEP_MIN)
            {
                PushBossImpact(AppState, Cast->Self, BOSS_BLINK_PUFF_RADIUS, BOSS_SHOCK_SPIRIT, -1.f, Now);
                PushBossImpact(AppState, Cast->Landed, BOSS_BLINK_PUFF_RADIUS, BOSS_SHOCK_SPIRIT, -1.f, Now);
            }
        } break;

        case MonsterAbility_Brand:
        {
            if (Cast->PointCount)
            {
                PushBossImpact(AppState, Cast->Points[0], Ability->Radius, BOSS_SHOCK_VOID, -1.f, Now);
            }
        } break;

        case MonsterAbility_Eclipse:
        {
            for(u32 Point = 0; Point < Cast->PointCount; Point++)
            {
                PushBossImpact(AppState, Cast->Points[Point], Ability->Radius * 1.4f,
                               BOSS_SHOCK_GOLD, -1.f, Now);
            }
        } break;

        default:
        {
        } break;
    }
}

// NOTE(zoubir): once a frame: a well collapses at the end of its Active
// phase, which danger_zones.cpp does not watch, so its slot is watched
// here for leaving Active. Offline it goes on to Recover; online Recover
// is not sent, so the snapshot shows it Ready
internal void
WatchBossCollapses(app_state *AppState, float Now)
{
    boss_fx *Fx = GetBossFx(AppState);
    world *World = &AppState->World;
    for(u32 Slot = 0; Slot < BOSS_FX_TRACKED; Slot++)
    {
        world_entity *Monster = &World->Entities[Slot];
        bool32 Open = false;
        monster_ability *Ability = 0;
        if (Slot < World->EntityCount && Monster->IsPresent &&
            Monster->Type == EntityType_Monster && IsBossCaster(AppState, Monster->MonsterKind))
        {
            monster_def *Def = GetMonsterDef(Monster->MonsterKind);
            if (Monster->AbilityIndex < Def->AbilityCount)
            {
                Ability = &Def->Abilities[Monster->AbilityIndex];
                Open = Monster->AbilityPhase == AbilityPhase_Active &&
                    Ability->Kind == MonsterAbility_Pull && Monster->AbilityPointCount;
            }
        }
        if (Open)
        {
            Fx->WellOpen[Slot] = Monster->ID;
            Fx->WellAt[Slot] = Monster->AbilityPoints[0];
            continue;
        }
        if (Fx->WellOpen[Slot] && Slot < World->EntityCount && Monster->IsPresent &&
            Monster->ID == Fx->WellOpen[Slot] && Monster->Hp > 0.f &&
            (Monster->AbilityPhase == AbilityPhase_Recover ||
             Monster->AbilityPhase == AbilityPhase_Ready))
        {
            monster_def *Def = GetMonsterDef(Monster->MonsterKind);
            float Core = 0.f;
            for(u32 Index = 0; Index < Def->AbilityCount; Index++)
            {
                if (Def->Abilities[Index].Kind == MonsterAbility_Pull)
                {
                    Core = Maximum(Core, Def->Abilities[Index].InnerRadius);
                }
            }
            PushBossImpact(AppState, Fx->WellAt[Slot], 1.6f * Maximum(Core, 40.f),
                           BOSS_SHOCK_VOID, -1.f, Now);
        }
        Fx->WellOpen[Slot] = 0;
    }
}

// NOTE(zoubir): once a frame: an add that erupts as it merges (a Magma
// Champion, sim/dungeon/boss_clock.cpp) hits the whole room for a share of
// everyone's health. The HUD's add clock (online from the snapshot) puts
// one that erupts first; when it was in its last second and then is gone,
// or the clock jumps to the next one, it went off
internal void
WatchAddEruptions(app_state *AppState, float Now)
{
    boss_fx *Fx = GetBossFx(AppState);
    dungeon_run *Run = AppState->Dungeon;
    u32 Seconds = Run ? Run->Clock.ShownAddSeconds : 0;
    bool32 Bursts = Run && Run->Clock.ShownAddBursts && Seconds;
    bool32 WentOff = Fx->AddBursts && Fx->AddSeconds <= 1 &&
        (!Bursts || Seconds > Fx->AddSeconds);
    Fx->AddSeconds = Seconds;
    Fx->AddBursts = Bursts;
    world_entity *Boss = 0;
    world *World = &AppState->World;
    for(u32 Index = 0; WentOff && Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster && Entity->Hp > 0.f &&
            (u32)Entity->MonsterKind == Run->ShownBossKind)
        {
            Boss = Entity;
            break;
        }
    }
    if (!Boss)
    {
        return;
    }
    PushBossImpact(AppState, Boss->Position.XY, BOSS_ERUPTION_RADIUS,
                   CastTellColor(MonsterAbility_Slam, 255), -1.f, Now);
    if (AppState->FxBursts)
    {
        AppState->FxBursts->Trauma = Minimum(1.f, AppState->FxBursts->Trauma + BOSS_ERUPTION_SHAKE);
    }
}

internal void
DrawBossImpacts(render_context *RenderContext, app_state *AppState, v3 CameraOffset, float Now)
{
    boss_fx *Fx = GetBossFx(AppState);
    world *World = &AppState->World;
    for(u32 Index = 0; Index < Fx->ImpactCount;)
    {
        boss_impact *Impact = &Fx->Impacts[Index];
        float Age = (Now - Impact->Born) / BOSS_IMPACT_SECONDS;
        if (Age >= 1.f || Age < 0.f || Impact->MapId != World->MapId)
        {
            Fx->Impacts[Index] = Fx->Impacts[--Fx->ImpactCount];
            continue;
        }
        Index++;
        DrawBossShock(RenderContext, World, CameraOffset, Impact->At, Impact->Radius,
                      Impact->Color, Age);
    }
}

// NOTE(zoubir): the shafts, over the world (screen_pass.inc), narrowing
// as they fade
internal void
DrawBossImpactShafts(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                     float Now)
{
    boss_fx *Fx = GetBossFx(AppState);
    world *World = &AppState->World;
    for(u32 Index = 0; Index < Fx->ImpactCount; Index++)
    {
        boss_impact *Impact = &Fx->Impacts[Index];
        float Age = (Now - Impact->Born) / BOSS_SHAFT_SECONDS;
        if (Impact->Shaft < 0.f || Age >= 1.f || Age < 0.f || Impact->MapId != World->MapId)
        {
            continue;
        }
        float Left = 1.f - Age;
        v2 Foot = Impact->At - CameraOffset.XY;
        Foot.Y -= TerrainHeightAt(World, Impact->At.X, Impact->At.Y);
        DrawBossShaft(RenderContext, Foot, BOSS_SHAFT_HEIGHT, 70.f * Left + 16.f,
                      Impact->Shaft, Left);
    }
}
