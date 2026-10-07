/* Ground marks: what walking leaves on soft or wet ground. A player or
   monster wading through water leaves rings spreading out behind it; one
   crossing snow leaves footprints, left and right, that fade after a few
   seconds. Client only and read from positions, so they show the same
   offline and online, for every unit on screen.

   Each frame UpdateGroundMarks adds up how far every unit on the ground
   has moved since the last frame; past a stride on water or snow (read
   from the terrain cache) it puts down a mark. Marks live in a ring of
   GROUND_MARKS_MAX, oldest replaced first, and are drawn flat on the
   ground right after the ground cracks (DrawTileMap, draw_tilemap.cpp):
   ripples with the ring shader added as light, footprints with the glow
   shader in a dark, cold colour. Only flat ground gets marks. */

#define GROUND_MARKS_MAX 192
#define GROUND_MARK_UNITS 4096
// NOTE(zoubir): world units walked between two marks
#define RIPPLE_STRIDE 26.f
#define FOOTPRINT_STRIDE 13.f
#define RIPPLE_SECONDS 1.3f
#define FOOTPRINT_SECONDS 7.f
#define DROP_SECONDS 0.5f
// NOTE(zoubir): a jump or a flyer this far above the ground leaves nothing
#define GROUND_MARK_MAX_HEIGHT 2.f

enum ground_mark_kind
{
    GroundMark_None,
    GroundMark_Ripple,
    GroundMark_Footprint,
    GroundMark_Drop,     // a raindrop landing on water (weather.cpp)
};

struct ground_mark
{
    v2 Position;
    v2 Direction;
    float Born;
    u32 Kind;
};

struct ground_marks
{
    ground_mark Marks[GROUND_MARKS_MAX];
    u32 Next;
    u32 MapId;
    // NOTE(zoubir): per entity slot: who was there, where it stood last
    // frame, how far it has walked since its last mark, which foot is next
    u32 Who[GROUND_MARK_UNITS];
    v2 Last[GROUND_MARK_UNITS];
    float Walked[GROUND_MARK_UNITS];
    u8 Foot[GROUND_MARK_UNITS];
};

internal void
AddGroundMark(ground_marks *Marks, u32 Kind, v2 Position, v2 Direction, float Now)
{
    ground_mark *Mark = &Marks->Marks[Marks->Next];
    Marks->Next = (Marks->Next + 1) % GROUND_MARKS_MAX;
    Mark->Kind = Kind;
    Mark->Position = Position;
    Mark->Direction = Direction;
    Mark->Born = Now;
}

internal void
UpdateGroundMarks(app_state *AppState, float Now)
{
    if (!AppState->GroundMarks)
    {
        AppState->GroundMarks = AllocateStruct(&AppState->MemoryArena, ground_marks);
        ZeroSize(AppState->GroundMarks, sizeof(ground_marks));
    }
    ground_marks *Marks = AppState->GroundMarks;
    world *World = &AppState->World;
    if (Marks->MapId != World->MapId)
    {
        ZeroSize(Marks, sizeof(*Marks));
        Marks->MapId = World->MapId;
    }
    if (!World->TileWidth || World->TileMap.Texture.Type != AssetType_TerrainAtlas)
    {
        return;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    u32 Count = Minimum(World->EntityCount, (u32)GROUND_MARK_UNITS);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        bool32 Walker = Entity->IsPresent && !IsDeadPlayer(Entity) &&
            (Entity->Type == EntityType_Player || Entity->Type == EntityType_Monster);
        if (!Walker)
        {
            Marks->Who[Index] = 0;
            continue;
        }
        v2 P = Entity->Position.XY;
        // NOTE(zoubir): a new body in this slot starts from where it is
        if (Marks->Who[Index] != Entity->ID + 1)
        {
            Marks->Who[Index] = Entity->ID + 1;
            Marks->Last[Index] = P;
            Marks->Walked[Index] = 0.f;
            continue;
        }
        v2 Step = P - Marks->Last[Index];
        Marks->Last[Index] = P;
        float Moved = Length(Step);
        // NOTE(zoubir): a blink or a respawn is not a walk
        if (Moved <= 0.01f || Moved > 40.f)
        {
            continue;
        }
        i32 TileX = FloorDiv((i32)floorf(P.X), (i32)World->TileWidth);
        i32 TileY = FloorDiv((i32)floorf(P.Y), (i32)World->TileHeight);
        terrain_cache_tile *Tile = CachedTile(AppState, Map, TileX, TileY);
        float Ground = (float)Tile->Steps * ELEVATION_STEP_HEIGHT;
        u32 Kind = GroundMark_None;
        if (Tile->Steps == 0 && Entity->Position.Z <= Ground + GROUND_MARK_MAX_HEIGHT)
        {
            if (Tile->Kind == TerrainKind_ShallowWater || Tile->Kind == TerrainKind_DeepWater)
            {
                Kind = GroundMark_Ripple;
            }
            else if (Tile->Kind == TerrainKind_Snow)
            {
                Kind = GroundMark_Footprint;
            }
        }
        if (!Kind)
        {
            Marks->Walked[Index] = 0.f;
            continue;
        }
        Marks->Walked[Index] += Moved;
        float Stride = Kind == GroundMark_Ripple ? RIPPLE_STRIDE : FOOTPRINT_STRIDE;
        if (Marks->Walked[Index] >= Stride)
        {
            Marks->Walked[Index] = 0.f;
            v2 Direction = Step * (1.f / Moved);
            v2 At = P;
            if (Kind == GroundMark_Footprint)
            {
                // NOTE(zoubir): left and right of the line walked
                float Side = Marks->Foot[Index] ? 3.f : -3.f;
                Marks->Foot[Index] ^= 1;
                At += V2(-Direction.Y, Direction.X) * Side;
            }
            AddGroundMark(Marks, Kind, At, Direction, Now);
        }
    }
}

internal void
DrawGroundMarks(render_context *RenderContext, app_state *AppState, v3 CameraOffset,
                float Now)
{
    ground_marks *Marks = AppState->GroundMarks;
    render_program Ring = RenderContext->Programs[Shader_Ring];
    render_program Glow = RenderContext->Programs[Shader_Glow];
    if (!Marks || Ring.ID == RenderContext->TextureProgram.ID ||
        Glow.ID == RenderContext->TextureProgram.ID)
    {
        return;
    }
    // NOTE(zoubir): +1, the smallest step a float keeps this far out
    float SortKey = FLAT_GROUND_SORT_KEY + 1.f;
    BeginBatch(RenderContext, 0, SortKey, Glow);
    for(u32 Index = 0; Index < GROUND_MARKS_MAX; Index++)
    {
        ground_mark *Mark = &Marks->Marks[Index];
        float Age = Now - Mark->Born;
        if (Mark->Kind != GroundMark_Footprint || Age < 0.f || Age > FOOTPRINT_SECONDS)
        {
            continue;
        }
        float Fade = 1.f - Age / FOOTPRINT_SECONDS;
        u32 Alpha = (u32)(150.f * Fade * Fade);
        // NOTE(zoubir): an oval along the way it walked, drawn three times
        // its size since the glow fades to nothing at the quad's edge
        float Long = 7.f * 2.2f;
        float Wide = 4.f * 2.2f;
        float Angle = atan2f(Mark->Direction.Y, Mark->Direction.X) + 1.5708f;
        RenderQuadTexture(RenderContext, Mark->Position.X - 0.5f * Wide - CameraOffset.X,
                          Mark->Position.Y - 0.5f * Long - CameraOffset.Y, Wide, Long,
                          V4(0.f, 1.f, 1.f, 0.f), (Alpha << 24) | 0x00705A48, 0.f, Angle);
    }
    EndBatch(RenderContext);

    BeginBatch(RenderContext, 0, SortKey, Ring);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Additive;
    for(u32 Index = 0; Index < GROUND_MARKS_MAX; Index++)
    {
        ground_mark *Mark = &Marks->Marks[Index];
        float Age = Now - Mark->Born;
        bool32 Drop = Mark->Kind == GroundMark_Drop;
        float Life = Drop ? DROP_SECONDS : RIPPLE_SECONDS;
        if ((Mark->Kind != GroundMark_Ripple && !Drop) || Age < 0.f || Age > Life)
        {
            continue;
        }
        float T = Age / Life;
        float Width = Drop ? 3.f + 11.f * T : 10.f + 40.f * T;
        // NOTE(zoubir): seen from above and a little in front, a ring on
        // the water is an ellipse
        float Height = 0.55f * Width;
        u32 Alpha = (u32)((Drop ? 170.f : 110.f) * (1.f - T) * (1.f - T));
        RenderQuadTexture(RenderContext, Mark->Position.X - 0.5f * Width - CameraOffset.X,
                          Mark->Position.Y - 0.5f * Height - CameraOffset.Y, Width, Height,
                          V4(0.f, 1.f, 1.f, 0.f), (Alpha << 24) | 0x00FFEED0, 0.f);
    }
    EndBatch(RenderContext);
}
