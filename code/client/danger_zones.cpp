/* Danger zones: where each monster attack in windup will land, painted on
   the ground under every unit (build/shaders/fx/danger_zone.frag), so a
   player standing in one sees it round their feet and not over their
   head. The area is the hit's true circle or lane: what is inside the
   stain is hit, what is outside is not. A brighter fill spreads over it
   as the windup runs; the hit lands when the fill reaches the rim. Then
   the area flashes full and fades, and a solid disc leaves broken
   ground behind.

   Read only from AbilityPhase, AbilityIndex, AbilityTimer, AbilityAim and
   AbilityPoints, which snapshots carry, so it looks the same online.
   art/monster_telegraphs.cpp keeps the thin rings over the units for the
   kinds this does not paint, and for every kind when the shader is
   missing. Drawn by DrawTileMap (draw_tilemap.cpp) after the cracks. */

#define DANGER_PALETTE_HARM 0
#define DANGER_PALETTE_SPIRIT 128
#define DANGER_PALETTE_GRAVE 255
// NOTE(zoubir): a volley's lane is as wide as a shot can graze a player
#define DANGER_VOLLEY_WIDTH 22.f
#define DANGER_GRAVE_RADIUS 18.f

inline bool32
DangerZonesReady(render_context *RenderContext)
{
    render_program Program = RenderContext->Programs[Shader_DangerZone];
    bool32 Result = Program.ID != RenderContext->TextureProgram.ID;
    return Result;
}

// NOTE(zoubir): the same sort key the cracks use, flat with the ground or
// just over a raised top
inline float
DangerZoneSortKey(world *World, v3 Position)
{
    float Result = FLAT_GROUND_SORT_KEY + 1.f;
    if (Position.Z > 0.f)
    {
        i32 TileY = FloorDiv((i32)floorf(Position.Y), (i32)World->TileHeight);
        Result = RaisedTopSortKey(TileY, (float)World->TileHeight, Position.Z) + 0.4f;
    }
    return Result;
}

// NOTE(zoubir): Shape is 0 for a disc, 255 for a lane, and between them
// a ring (DangerRingShape)
inline u32
DangerZoneColor(float Progress, u32 Palette, u32 Shape, float Strength)
{
    u32 Result = ((u32)(255.f * Clamp01(Strength)) << 24) |
        (Shape << 16) | (Palette << 8) |
        (u32)(255.f * Clamp01(Progress));
    return Result;
}

// NOTE(zoubir): a ring whose hole is Inner of its radius, as the shader
// reads it back: 64 for no hole up to 192 for none left
inline u32
DangerRingShape(float Inner)
{
    u32 Result = 64 + (u32)(128.f * Clamp01(Inner));
    return Result;
}

internal void
DrawDangerDisc(render_context *RenderContext, world *World, v3 CameraOffset,
               v2 Center, float Radius, float Progress, u32 Palette,
               u32 Shape, float Strength)
{
    float GroundZ = TerrainHeightAt(World, Center.X, Center.Y);
    float Size = 2.f * Radius;
    BeginBatch(RenderContext, 0, DangerZoneSortKey(World, V3(Center.X, Center.Y, GroundZ)),
               RenderContext->Programs[Shader_DangerZone]);
    RenderQuadTexture(RenderContext, Center.X - Radius - CameraOffset.X,
                      Center.Y - GroundZ - Radius - CameraOffset.Y, Size, Size,
                      V4(0.f, 1.f, 1.f, 0.f),
                      DangerZoneColor(Progress, Palette, Shape, Strength), 0.f);
    EndBatch(RenderContext);
}

// NOTE(zoubir): a strip from From along Direction, Length long
internal void
DrawDangerLane(render_context *RenderContext, world *World, v3 CameraOffset,
               v2 From, v2 Direction, float Length, float Width,
               float Progress, u32 Palette, float Strength)
{
    float GroundZ = TerrainHeightAt(World, From.X, From.Y);
    v2 Middle = From + 0.5f * Length * Direction;
    float Angle = ATan2(Direction.Y, Direction.X);
    BeginBatch(RenderContext, 0, DangerZoneSortKey(World, V3(Middle.X, Middle.Y, GroundZ)),
               RenderContext->Programs[Shader_DangerZone]);
    RenderQuadTexture(RenderContext, Middle.X - 0.5f * Length - CameraOffset.X,
                      Middle.Y - GroundZ - 0.5f * Width - CameraOffset.Y,
                      Length, Width, V4(0.f, 1.f, 1.f, 0.f),
                      DangerZoneColor(Progress, Palette, 255, Strength), 0.f, Angle);
    EndBatch(RenderContext);
}

// NOTE(zoubir): where one cast of an ability lands, as it was locked in
// during its windup; kept after the windup ends for the impact
struct danger_cast
{
    monster_ability *Ability;
    v2 Self;
    v2 Aim;
    v2 Points[MAX_ABILITY_POINTS];
    u32 PointCount;
};

inline danger_cast
GetDangerCast(world_entity *Entity, monster_ability *Ability)
{
    danger_cast Result = {};
    Result.Ability = Ability;
    Result.Self = Entity->Position.XY;
    Result.Aim = Entity->AbilityAim;
    Result.PointCount = Minimum(Entity->AbilityPointCount, (u32)MAX_ABILITY_POINTS);
    for(u32 Point = 0; Point < Result.PointCount; Point++)
    {
        Result.Points[Point] = Entity->AbilityPoints[Point];
    }
    return Result;
}

inline u32
DangerShape(monster_ability *Ability)
{
    u32 Result = Ability->InnerRadius > 0.f ?
        DangerRingShape(Ability->InnerRadius / Ability->Radius) : 0;
    return Result;
}

// NOTE(zoubir): the area one cast covers; Progress 1 is the moment it
// lands, Strength fades an impact out
internal void
DrawDangerCast(render_context *RenderContext, world *World, v3 CameraOffset,
               danger_cast *Cast, float Progress, float Strength)
{
    monster_ability *Ability = Cast->Ability;
    switch(Ability->Kind)
    {
        case MonsterAbility_Slam:
        {
            DrawDangerDisc(RenderContext, World, CameraOffset, Cast->Self,
                           Ability->Radius, Progress, DANGER_PALETTE_HARM,
                           DangerShape(Ability), Strength);
        } break;

        case MonsterAbility_Charge:
        {
            DrawDangerLane(RenderContext, World, CameraOffset, Cast->Self, Cast->Aim,
                           Ability->Speed * Ability->Active, Ability->Radius,
                           Progress, DANGER_PALETTE_HARM, Strength);
        } break;

        case MonsterAbility_Mortar:
        {
            for(u32 Point = 0; Point < Cast->PointCount; Point++)
            {
                DrawDangerDisc(RenderContext, World, CameraOffset, Cast->Points[Point],
                               Ability->Radius, Progress, DANGER_PALETTE_HARM,
                               DangerShape(Ability), Strength);
            }
        } break;

        case MonsterAbility_Blink:
        {
            DrawDangerDisc(RenderContext, World, CameraOffset, Cast->Points[0],
                           Ability->Radius, Progress, DANGER_PALETTE_SPIRIT, 0, Strength);
        } break;

        case MonsterAbility_Volley:
        {
            v2 Directions[MAX_VOLLEY_SHOTS];
            u32 Count = GetVolleyDirections(Ability, Cast->Aim, Directions,
                                            MAX_VOLLEY_SHOTS);
            for(u32 Shot = 0; Shot < Count; Shot++)
            {
                DrawDangerLane(RenderContext, World, CameraOffset, Cast->Self,
                               Directions[Shot], Ability->Speed * Ability->Active,
                               DANGER_VOLLEY_WIDTH, Progress, DANGER_PALETTE_HARM,
                               Strength);
            }
        } break;

        case MonsterAbility_Summon:
        {
            // NOTE(zoubir): graves that open when the windup ends; a
            // player standing on one keeps it shut
            for(u32 Point = 0; Point < Cast->PointCount; Point++)
            {
                DrawDangerDisc(RenderContext, World, CameraOffset, Cast->Points[Point],
                               DANGER_GRAVE_RADIUS, Progress, DANGER_PALETTE_GRAVE, 0,
                               Strength);
            }
        } break;

        default:
        {
        } break;
    }
}

// NOTE(zoubir): when a windup ends, its area flashes full for
// DANGER_IMPACT_SECONDS and fades, so a player sees where it really hit;
// solid discs also break the ground (ground_cracks.cpp)
#define DANGER_IMPACT_SECONDS 0.45f
#define DANGER_IMPACT_MAX 16
#define DANGER_CRACK_MOST_RADIUS 140.f
#define DANGER_TRACKED ArrayCount(((world *)0)->Entities)

struct danger_impact
{
    danger_cast Cast;
    float Born;
    u32 MapId;
};

struct danger_zones
{
    // NOTE(zoubir): per entity slot, the cast it was winding up last
    // frame (Ability 0 when none) and which entity was in the slot
    danger_cast Winding[DANGER_TRACKED];
    u32 WindingId[DANGER_TRACKED];
    danger_impact Impacts[DANGER_IMPACT_MAX];
    u32 ImpactCount;
};

internal void
AddDangerImpact(app_state *AppState, danger_zones *Zones, danger_cast *Cast, float Now)
{
    if (Zones->ImpactCount == DANGER_IMPACT_MAX)
    {
        for(u32 Index = 1; Index < Zones->ImpactCount; Index++)
        {
            Zones->Impacts[Index - 1] = Zones->Impacts[Index];
        }
        Zones->ImpactCount--;
    }
    danger_impact *Impact = &Zones->Impacts[Zones->ImpactCount++];
    Impact->Cast = *Cast;
    Impact->Born = Now;
    Impact->MapId = AppState->World.MapId;

    monster_ability *Ability = Cast->Ability;
    bool32 Solid = Ability->InnerRadius <= 0.f &&
        Ability->Radius <= DANGER_CRACK_MOST_RADIUS;
    if (Solid && Ability->Kind == MonsterAbility_Slam)
    {
        AddGroundCrack(AppState, V3(Cast->Self.X, Cast->Self.Y, 0.f), Ability->Radius, Now);
    }
    if (Solid && Ability->Kind == MonsterAbility_Mortar)
    {
        for(u32 Point = 0; Point < Cast->PointCount; Point++)
        {
            AddGroundCrack(AppState, V3(Cast->Points[Point].X, Cast->Points[Point].Y, 0.f),
                           Ability->Radius, Now);
        }
    }
}

internal void
DrawDangerZones(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    if (!DangerZonesReady(RenderContext))
    {
        return;
    }
    if (!AppState->DangerZones)
    {
        AppState->DangerZones = AllocateStruct(&AppState->MemoryArena, danger_zones);
        *AppState->DangerZones = {};
    }
    danger_zones *Zones = AppState->DangerZones;
    world *World = &AppState->World;
    float Now = GetFxClock(AppState);

    for(u32 EntityIndex = 0; EntityIndex < DANGER_TRACKED; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        bool32 Live = EntityIndex < World->EntityCount && Entity->IsPresent &&
            Entity->Type == EntityType_Monster;
        monster_def *Def = Live ? GetMonsterDef(Entity->MonsterKind) : 0;
        bool32 WindingUp = Def && Entity->AbilityPhase == AbilityPhase_Windup &&
            Entity->AbilityIndex < Def->AbilityCount;
        danger_cast *Last = &Zones->Winding[EntityIndex];
        if (!WindingUp)
        {
            // NOTE(zoubir): only a windup that went on to its hit, not one
            // cut short by the monster dying or leaving the slot; online a
            // short Active can fall between two snapshots, so Recover counts
            if (Last->Ability && Live && Zones->WindingId[EntityIndex] == Entity->ID &&
                (Entity->AbilityPhase == AbilityPhase_Active ||
                 Entity->AbilityPhase == AbilityPhase_Recover))
            {
                AddDangerImpact(AppState, Zones, Last, Now);
            }
            Last->Ability = 0;
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
        float Progress = Ability->Windup > 0.f ?
            Clamp01(1.f - Entity->AbilityTimer / Ability->Windup) : 1.f;
        *Last = GetDangerCast(Entity, Ability);
        Zones->WindingId[EntityIndex] = Entity->ID;
        DrawDangerCast(RenderContext, World, CameraOffset, Last, Progress, 1.f);
    }

    for(u32 Index = 0; Index < Zones->ImpactCount;)
    {
        danger_impact *Impact = &Zones->Impacts[Index];
        float Age = Now - Impact->Born;
        if (Age >= DANGER_IMPACT_SECONDS || Age < 0.f || Impact->MapId != World->MapId)
        {
            Zones->Impacts[Index] = Zones->Impacts[--Zones->ImpactCount];
            continue;
        }
        Index++;
        float Strength = 1.f - Age / DANGER_IMPACT_SECONDS;
        DrawDangerCast(RenderContext, World, CameraOffset, &Impact->Cast, 1.f,
                       Strength * Strength);
    }
}
