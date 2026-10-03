/* Landmark pointer: on infinite maps, a chevron at the edge of the screen
   points toward the nearest landmark this player has not yet reached,
   with its name and distance in tiles. A landmark counts as reached once
   the local player has been within LANDMARK_REACHED_TILES of its middle;
   the memory is this machine's only, so it works the same offline and
   online. The landmarks themselves come from the map (sim/terrain/
   landmarks.cpp), so the client finds them without asking the server. */

#define LANDMARK_SEARCH_REGIONS 3
#define LANDMARK_REACHED_TILES 12
#define LANDMARK_POINTER_COLOR 0xFF40D0FF
#define LANDMARK_POINTER_SHADOW 0xC0101010

struct landmark_memory
{
    u32 MapId;
    i32 ReachedX[128];
    i32 ReachedY[128];
    u32 ReachedCount;
    u32 ReachedNext;
};

// NOTE(zoubir): client-only memory; resets if app.dll is hot reloaded,
// which only means old landmarks are pointed at again
global_variable landmark_memory LandmarkMemory;

inline bool32
HasReachedLandmark(landmark_memory *Memory, landmark_spot *Spot)
{
    for(u32 Index = 0; Index < Memory->ReachedCount; Index++)
    {
        if (Memory->ReachedX[Index] == Spot->RegionX &&
            Memory->ReachedY[Index] == Spot->RegionY)
        {
            return true;
        }
    }
    return false;
}

inline void
RememberLandmark(landmark_memory *Memory, landmark_spot *Spot)
{
    u32 Slot = Memory->ReachedNext;
    Memory->ReachedX[Slot] = Spot->RegionX;
    Memory->ReachedY[Slot] = Spot->RegionY;
    Memory->ReachedNext = (Slot + 1) % ArrayCount(Memory->ReachedX);
    if (Memory->ReachedCount < ArrayCount(Memory->ReachedX))
    {
        Memory->ReachedCount++;
    }
}

// NOTE(zoubir): the nearest landmark not yet reached around Position, after
// marking any the player now stands at as reached. False when none is near
internal bool32
FindLandmarkToPoint(world *World, v3 Position, landmark_memory *Memory,
                    landmark_spot *Out, v3 *OutCenter)
{
    if (Memory->MapId != World->MapId)
    {
        *Memory = {};
        Memory->MapId = World->MapId;
    }
    map_def *Map = GetMapDef((map_id)World->MapId);
    i32 Tile = (i32)World->TileWidth;
    i32 RegionSize = LANDMARK_REGION_TILES * Tile;
    i32 HomeX = FloorDiv((i32)floorf(Position.X), RegionSize);
    i32 HomeY = FloorDiv((i32)floorf(Position.Y), RegionSize);
    float Reached = (float)(LANDMARK_REACHED_TILES * Tile);
    float Best = 0.f;
    bool32 Found = false;
    for(i32 DY = -LANDMARK_SEARCH_REGIONS; DY <= LANDMARK_SEARCH_REGIONS; DY++)
    {
        for(i32 DX = -LANDMARK_SEARCH_REGIONS; DX <= LANDMARK_SEARCH_REGIONS; DX++)
        {
            landmark_spot Spot = GetRegionLandmark(Map, HomeX + DX, HomeY + DY);
            if (!Spot.Present || HasReachedLandmark(Memory, &Spot))
            {
                continue;
            }
            v3 Center = GetLandmarkCenter(&Spot, Tile);
            float Distance = Length(Center.XY - Position.XY);
            if (Distance < Reached)
            {
                RememberLandmark(Memory, &Spot);
                continue;
            }
            if (!Found || Distance < Best)
            {
                Found = true;
                Best = Distance;
                *Out = Spot;
                *OutCenter = Center;
            }
        }
    }
    return Found;
}

inline void
DrawPointerDot(render_context *RenderContext, v2 P, float Size, u32 Color)
{
    DrawFilledRectangle(RenderContext, P.X - 0.5f * Size, P.Y - 0.5f * Size,
                        Size, Size, Color, 0.f);
}

internal void
DrawLandmarkPointer(render_context *RenderContext, app_state *AppState,
                    v3 CameraOffset, app_window *Window)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    if (!World->Unbounded || !Player)
    {
        return;
    }
    landmark_spot Spot;
    v3 Center;
    if (!FindLandmarkToPoint(World, Player->Position, &LandmarkMemory, &Spot, &Center))
    {
        return;
    }

    v2 Screen = V2((float)Window->Width, (float)Window->Height);
    v2 PlayerOnScreen = Player->Position.XY - CameraOffset.XY;
    v2 TargetOnScreen = Center.XY - CameraOffset.XY;
    v2 Toward = TargetOnScreen - PlayerOnScreen;
    float Distance = Length(Toward);
    if (Distance <= 0.f)
    {
        return;
    }
    Toward *= 1.f / Distance;

    // NOTE(zoubir): on screen already: point at it from just above it;
    // otherwise sit on an ellipse inside the screen edge
    v2 Anchor;
    bool32 Visible = TargetOnScreen.X > 40.f && TargetOnScreen.Y > 40.f &&
        TargetOnScreen.X < Screen.X - 40.f && TargetOnScreen.Y < Screen.Y - 40.f;
    if (Visible)
    {
        Anchor = TargetOnScreen - V2(0.f, 40.f);
        Toward = V2(0.f, 1.f);
    }
    else
    {
        v2 Middle = 0.5f * Screen;
        Anchor = Middle + V2(Toward.X * (0.5f * Screen.X - 60.f),
                             Toward.Y * (0.5f * Screen.Y - 60.f));
    }

    // NOTE(zoubir): a chevron of dots, its point toward the landmark,
    // gently pulsing
    float Pulse = 1.f + 0.15f * Sin(0.1f * (float)AppState->UpdateID);
    v2 Side = V2(-Toward.Y, Toward.X);
    for(i32 Arm = -4; Arm <= 4; Arm++)
    {
        float Back = (float)(Arm < 0 ? -Arm : Arm) * 4.f * Pulse;
        v2 P = Anchor + 4.f * Pulse * (float)Arm * Side - Back * Toward;
        DrawPointerDot(RenderContext, P + V2(1.f, 1.f), 5.f, LANDMARK_POINTER_SHADOW);
        DrawPointerDot(RenderContext, P, 4.f, LANDMARK_POINTER_COLOR);
    }

    font *Font = AppState->DefaultFont;
    if (Font)
    {
        landmark_def *Def = &LandmarkTable[Spot.Landmark];
        char Label[96];
        u32 Tiles = (u32)(Length(Center.XY - Player->Position.XY) / (float)World->TileWidth);
        snprintf(Label, sizeof(Label), "%s  %u", Def->Name, Tiles);
        v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
        // NOTE(zoubir): the label sits on the screen side of the chevron
        float LabelX = Anchor.X - (Toward.X > 0.3f ? 210.f : -14.f);
        float LabelY = Anchor.Y - 10.f * Toward.Y + Font->UpperLimit * 0.5f;
        RenderText(RenderContext, LabelX + 1.f, LabelY + 1.f, Font,
                   RenderContext->TextureProgram, Label, LANDMARK_POINTER_SHADOW,
                   0.8f, 0.8f, NoClip, 0.f);
        RenderText(RenderContext, LabelX, LabelY, Font,
                   RenderContext->TextureProgram, Label, LANDMARK_POINTER_COLOR,
                   0.8f, 0.8f, NoClip, 0.f);
    }
}
