/* Monster cast tells: what a monster is casting, drawn on the monster
   itself so a player looking at it can read it without finding the
   warning on the ground.

   - while an ability winds up, its name shows over the monster's head
     above a bar that fills until the hit (cast_bars.cpp);
   - motes in the ability's color circle the body and close in on it as
     the windup runs, so a monster gathering power looks it;
   - when the windup ends, a ring bursts out of the body and the name
     holds for a moment, so the cast reads as let go.

   Read only from AbilityPhase, AbilityIndex and AbilityTimer, which
   snapshots already carry (client/replicas/apply.cpp), so it looks the
   same online and offline. The ground warnings stay in
   art/monster_render.cpp.

   Entry point: DrawMonsterCastTells, once a frame from screen_pass.inc. */

#define CAST_TELL_RELEASE_SECONDS 0.3f
#define CAST_TELL_MOTES 8
#define CAST_TELL_TRACKED ArrayCount(((world *)0)->Entities)

struct monster_cast_tells
{
    // NOTE(zoubir): per entity slot, whether it was winding up last
    // frame, and what it released, for the release burst
    u8 WasWindingUp[CAST_TELL_TRACKED];
    u8 ReleasedAbility[CAST_TELL_TRACKED];
    float ReleaseLeft[CAST_TELL_TRACKED];
};

// NOTE(zoubir): one color per ability kind, so the same color means the
// same kind of danger on every monster
inline u32
CastTellColor(monster_ability_kind Kind, u32 Alpha)
{
    u32 Result;
    switch(Kind)
    {
        case MonsterAbility_Slam:   Result = UI_RGBA(255, 140,  50, Alpha); break;
        case MonsterAbility_Charge: Result = UI_RGBA(255,  70,  60, Alpha); break;
        case MonsterAbility_Mortar: Result = UI_RGBA(255, 190,  60, Alpha); break;
        case MonsterAbility_Blink:  Result = UI_RGBA(190, 110, 255, Alpha); break;
        case MonsterAbility_Volley: Result = UI_RGBA(255, 230,  90, Alpha); break;
        case MonsterAbility_Summon: Result = UI_RGBA(120, 255, 150, Alpha); break;
        case MonsterAbility_Mend:   Result = UI_RGBA(140, 255, 210, Alpha); break;
        case MonsterAbility_Burrow: Result = UI_RGBA(220, 170, 110, Alpha); break;
        default:                    Result = UI_RGBA(255, 255, 255, Alpha); break;
    }
    return Result;
}

inline monster_ability *
GetCastTellAbility(world_entity *Entity, u32 AbilityIndex)
{
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    monster_ability *Result = (Def && AbilityIndex < Def->AbilityCount) ?
        &Def->Abilities[AbilityIndex] : 0;
    return Result;
}

internal void
DrawCastTellWindup(render_context *RenderContext, app_state *AppState,
                   world_entity *Entity, monster_ability *Ability,
                   v2 Feet, float Clock)
{
    float Progress = Ability->Windup > 0.f ?
        1.f - Entity->AbilityTimer / Ability->Windup : 1.f;
    Progress = ArtClamp01(Progress);
    bool32 Blink = IsCastBarBlinking(Progress, Clock);
    u32 Color = CastTellColor(Ability->Kind, 255);

    // NOTE(zoubir): motes start wide and slow, end tight and fast, and
    // grow as they close in
    float Height = Entity->Dimensions.Y * CAST_BAR_SPRITE_TOP;
    v2 Middle = Feet - V2(0.f, 0.45f * Height);
    float Wide = 0.75f * Maximum(Entity->Dimensions.X, Height);
    float Radius = Wide * (1.f - 0.65f * Progress);
    float Spin = Clock * (2.f + 6.f * Progress);
    float Size = 2.f + 3.f * Progress;
    u32 MoteColor = CastTellColor(Ability->Kind, (u32)(110.f + 145.f * Progress));
    for(u32 Mote = 0; Mote < CAST_TELL_MOTES; Mote++)
    {
        float Angle = Spin + 2.f * Pi32 * (float)Mote / (float)CAST_TELL_MOTES;
        // NOTE(zoubir): flattened, as the ground is seen from above
        v2 P = Middle + Radius * V2(Cos(Angle), 0.55f * Sin(Angle));
        DrawFxDot(RenderContext, P, Size, Blink ? UI_RGBA(255, 255, 255, 255) : MoteColor);
    }
    DrawCastBar(RenderContext, AppState, Entity, Feet, Progress, Clock,
                Ability->Name, Color);
}

// NOTE(zoubir): a ring thrown out of the body, and the name rising and
// fading, for CAST_TELL_RELEASE_SECONDS after the windup ends
internal void
DrawCastTellRelease(render_context *RenderContext, app_state *AppState,
                    world_entity *Entity, monster_ability *Ability,
                    v2 Feet, float Left)
{
    float T = 1.f - Left / CAST_TELL_RELEASE_SECONDS;
    u32 Alpha = (u32)(255.f * (1.f - T));
    u32 Color = CastTellColor(Ability->Kind, Alpha);
    float Height = Entity->Dimensions.Y * CAST_BAR_SPRITE_TOP;
    v2 Middle = Feet - V2(0.f, 0.45f * Height);
    float Radius = (0.3f + 0.9f * T) * Maximum(Entity->Dimensions.X, Height);
    u32 Dots = 16;
    for(u32 Dot = 0; Dot < Dots; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / (float)Dots;
        v2 P = Middle + Radius * V2(Cos(Angle), 0.55f * Sin(Angle));
        DrawFxDot(RenderContext, P, 5.f - 3.f * T, Color);
    }

    font *Font = AppState->Fonts.Small;
    float Top = CastBarTop(Entity, Feet) - 10.f * T;
    DrawCastName(RenderContext, Font, V2(Feet.X, Top - UILineHeight(Font) - 1.f),
                 Ability->Name, UI_RGBA(255, 255, 255, Alpha), Alpha);
}

internal void
DrawMonsterCastTells(render_context *RenderContext, app_state *AppState,
                     v3 CameraOffset, float DeltaTime)
{
    if (!AppState->CastTells)
    {
        AppState->CastTells = AllocateStruct(&AppState->MemoryArena, monster_cast_tells);
        *AppState->CastTells = {};
    }
    monster_cast_tells *Tells = AppState->CastTells;
    world *World = &AppState->World;
    float Clock = GetFxClock(AppState);

    u32 Count = Minimum(World->EntityCount, (u32)CAST_TELL_TRACKED);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        bool32 IsMonster = Entity->IsPresent && Entity->Type == EntityType_Monster;
        bool32 WindingUp = IsMonster && Entity->AbilityPhase == AbilityPhase_Windup;

        if (Tells->WasWindingUp[Index] && !WindingUp && IsMonster)
        {
            Tells->ReleaseLeft[Index] = CAST_TELL_RELEASE_SECONDS;
        }
        else if (!IsMonster)
        {
            Tells->ReleaseLeft[Index] = 0.f;
        }
        Tells->WasWindingUp[Index] = (u8)(WindingUp != 0);
        if (WindingUp)
        {
            Tells->ReleasedAbility[Index] = (u8)Entity->AbilityIndex;
        }
        if (!IsMonster || Entity->Burrowed)
        {
            continue;
        }

        v2 Feet = Entity->Position.XY - CameraOffset.XY - V2(0.f, Entity->Position.Z);
        if (WindingUp)
        {
            monster_ability *Ability = GetCastTellAbility(Entity, Entity->AbilityIndex);
            if (Ability)
            {
                DrawCastTellWindup(RenderContext, AppState, Entity, Ability, Feet, Clock);
            }
        }
        else if (Tells->ReleaseLeft[Index] > 0.f)
        {
            monster_ability *Ability =
                GetCastTellAbility(Entity, Tells->ReleasedAbility[Index]);
            if (Ability)
            {
                DrawCastTellRelease(RenderContext, AppState, Entity, Ability,
                                    Feet, Tells->ReleaseLeft[Index]);
            }
            Tells->ReleaseLeft[Index] -= DeltaTime;
        }
    }
    for(u32 Index = Count; Index < CAST_TELL_TRACKED; Index++)
    {
        Tells->WasWindingUp[Index] = 0;
        Tells->ReleaseLeft[Index] = 0.f;
    }
}
