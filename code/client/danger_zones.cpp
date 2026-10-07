/* Danger zones: where each monster attack in windup will land, painted on
   the ground under every unit (build/shaders/fx/danger_zone.frag), so a
   player standing in one sees it round their feet and not over their
   head. The area is the hit's true circle or lane: what is inside the
   stain is hit, what is outside is not. A brighter fill spreads over it
   as the windup runs; the hit lands when the fill reaches the rim.

   Read only from AbilityPhase, AbilityIndex, AbilityTimer, AbilityAim and
   AbilityPoints, which snapshots carry, so it looks the same online.
   art/monster_render.cpp keeps the thin rings over the units for the
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

inline u32
DangerZoneColor(float Progress, u32 Palette, bool32 Lane, float Strength)
{
    u32 Result = ((u32)(255.f * Clamp01(Strength)) << 24) |
        ((Lane ? 255u : 0u) << 16) | (Palette << 8) |
        (u32)(255.f * Clamp01(Progress));
    return Result;
}

internal void
DrawDangerDisc(render_context *RenderContext, world *World, v3 CameraOffset,
               v2 Center, float Radius, float Progress, u32 Palette)
{
    float GroundZ = TerrainHeightAt(World, Center.X, Center.Y);
    float Size = 2.f * Radius;
    BeginBatch(RenderContext, 0, DangerZoneSortKey(World, V3(Center.X, Center.Y, GroundZ)),
               RenderContext->Programs[Shader_DangerZone]);
    RenderQuadTexture(RenderContext, Center.X - Radius - CameraOffset.X,
                      Center.Y - GroundZ - Radius - CameraOffset.Y, Size, Size,
                      V4(0.f, 1.f, 1.f, 0.f),
                      DangerZoneColor(Progress, Palette, false, 1.f), 0.f);
    EndBatch(RenderContext);
}

// NOTE(zoubir): a strip from From along Direction, Length long
internal void
DrawDangerLane(render_context *RenderContext, world *World, v3 CameraOffset,
               v2 From, v2 Direction, float Length, float Width,
               float Progress, u32 Palette)
{
    float GroundZ = TerrainHeightAt(World, From.X, From.Y);
    v2 Middle = From + 0.5f * Length * Direction;
    float Angle = ATan2(Direction.Y, Direction.X);
    BeginBatch(RenderContext, 0, DangerZoneSortKey(World, V3(Middle.X, Middle.Y, GroundZ)),
               RenderContext->Programs[Shader_DangerZone]);
    RenderQuadTexture(RenderContext, Middle.X - 0.5f * Length - CameraOffset.X,
                      Middle.Y - GroundZ - 0.5f * Width - CameraOffset.Y,
                      Length, Width, V4(0.f, 1.f, 1.f, 0.f),
                      DangerZoneColor(Progress, Palette, true, 1.f), 0.f, Angle);
    EndBatch(RenderContext);
}

internal void
DrawDangerZones(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    if (!DangerZonesReady(RenderContext))
    {
        return;
    }
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            Entity->AbilityPhase != AbilityPhase_Windup)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        if (!Def || Entity->AbilityIndex >= Def->AbilityCount)
        {
            continue;
        }
        monster_ability *Ability = &Def->Abilities[Entity->AbilityIndex];
        float Progress = Ability->Windup > 0.f ?
            Clamp01(1.f - Entity->AbilityTimer / Ability->Windup) : 1.f;
        v2 Self = Entity->Position.XY;

        switch(Ability->Kind)
        {
            case MonsterAbility_Slam:
            {
                DrawDangerDisc(RenderContext, World, CameraOffset, Self,
                               Ability->Radius, Progress, DANGER_PALETTE_HARM);
            } break;

            case MonsterAbility_Charge:
            {
                DrawDangerLane(RenderContext, World, CameraOffset, Self,
                               Entity->AbilityAim, Ability->Speed * Ability->Active,
                               Ability->Radius, Progress, DANGER_PALETTE_HARM);
            } break;

            case MonsterAbility_Mortar:
            {
                for(u32 Point = 0; Point < Entity->AbilityPointCount; Point++)
                {
                    DrawDangerDisc(RenderContext, World, CameraOffset,
                                   Entity->AbilityPoints[Point],
                                   Ability->Radius, Progress, DANGER_PALETTE_HARM);
                }
            } break;

            case MonsterAbility_Blink:
            {
                DrawDangerDisc(RenderContext, World, CameraOffset,
                               Entity->AbilityPoints[0], Ability->Radius,
                               Progress, DANGER_PALETTE_SPIRIT);
            } break;

            case MonsterAbility_Volley:
            {
                v2 Directions[MAX_VOLLEY_SHOTS];
                u32 Count = GetVolleyDirections(Ability, Entity->AbilityAim,
                                                Directions, MAX_VOLLEY_SHOTS);
                for(u32 Shot = 0; Shot < Count; Shot++)
                {
                    DrawDangerLane(RenderContext, World, CameraOffset, Self,
                                   Directions[Shot], Ability->Speed * Ability->Active,
                                   DANGER_VOLLEY_WIDTH, Progress, DANGER_PALETTE_HARM);
                }
            } break;

            case MonsterAbility_Summon:
            {
                // NOTE(zoubir): graves that open when the windup ends; a
                // player standing on one keeps it shut
                for(u32 Point = 0; Point < Entity->AbilityPointCount; Point++)
                {
                    DrawDangerDisc(RenderContext, World, CameraOffset,
                                   Entity->AbilityPoints[Point],
                                   DANGER_GRAVE_RADIUS, Progress, DANGER_PALETTE_GRAVE);
                }
            } break;

            default:
            {
            } break;
        }
    }
}
