/* Camera: the screen follows the local player, never showing past the
   edge of a bounded map (infinite maps have no edge; a map smaller than
   the screen sits in its middle). The result is the top-left corner of
   the screen in world units, snapped to whole window pixels.

   The world is drawn zoomed in: WorldZoom window pixels per world unit,
   chosen so the screen shows about WORLD_VIEW_HEIGHT units from top to
   bottom whatever the window size. GetWorldView gives the window measured
   in world units, which is what the camera, the ground and the world
   overlays work in; screens over the world stay in window pixels.

   The camera eases toward its target instead of jumping to it: each
   frame it closes the same share of the gap per second whatever the
   frame rate (CAMERA_FOLLOW_RATE), so uneven frames and the small
   corrections online play makes to the player do not shake the view. It
   also leans a little toward the mouse, so more of the screen lies the
   way the player aims, and runs a little ahead of a moving player, so a
   run shows more of what it heads into than what it left. A respawn or a jump across the map is a cut, not
   a long pan. Hits shake it (fx_bursts.cpp), and a solid hit the local
   player lands nudges it toward the blow (body_pose.cpp); both move only
   what is drawn.

   Called after the world has moved this frame, so the camera and the
   player it follows are drawn from the same positions. */

// NOTE(zoubir): world units shown from the top of the screen to the bottom
// (the window used to show its own height, 680 at the default size); the
// zoom never goes below 1 or above 3
#define WORLD_VIEW_HEIGHT 520.f
#define WORLD_ZOOM_MIN 1.f
#define WORLD_ZOOM_MAX 3.f
// NOTE(zoubir): share of the gap closed per second, as a rate: higher is
// tighter. 15 settles in under a quarter second, and a full run (260)
// trails the camera by about 17 units; at 10 it trailed by 26
#define CAMERA_FOLLOW_RATE 15.f
// NOTE(zoubir): the lean is this share of the cursor's distance from the
// screen centre, at most CAMERA_LEAN_MAX world units
#define CAMERA_LEAN_SHARE 0.12f
#define CAMERA_LEAN_MAX 50.f
// NOTE(zoubir): the camera aims this many seconds ahead of a moving
// player, at most CAMERA_LEAD_MAX units: a full run (260) leads by 31, so
// the easing's 17-unit trail turns into a small lead. Dashes hit the cap
#define CAMERA_LEAD_SECONDS 0.12f
#define CAMERA_LEAD_MAX 40.f
// NOTE(zoubir): a target farther than this many screen sizes away is cut to
#define CAMERA_CUT_SCREENS 0.75f

// NOTE(zoubir): sets AppState->WorldZoom for this frame and returns the
// window measured in world units
internal app_window
GetWorldView(app_state *AppState, app_window *Window)
{
    float Zoom = (float)Window->Height / WORLD_VIEW_HEIGHT;
    Zoom = Maximum(WORLD_ZOOM_MIN, Minimum(WORLD_ZOOM_MAX, Zoom));
    AppState->WorldZoom = Zoom;
    app_window Result = *Window;
    Result.Width = (u32)ceilf((float)Window->Width / Zoom);
    Result.Height = (u32)ceilf((float)Window->Height / Zoom);
    return Result;
}

// NOTE(zoubir): one axis of CenterCamera: centred on Position, kept inside
// Min..Max, or the whole map centred when the view is wider than it
inline float
CenterCameraAxis(float Position, float Min, float Max, float ViewSize)
{
    float Result = Position - 0.5f * ViewSize;
    if (ViewSize >= Max - Min)
    {
        Result = Min - 0.5f * (ViewSize - (Max - Min));
    }
    else
    {
        Result = Maximum(Min, Minimum(Result, Max - ViewSize));
    }
    return Result;
}

inline v3
CenterCamera(v3 Position, float MinX, float MinY,
             float MaxX, float MaxY, u32 WindowWidth,
             u32 WindowHeight)
{
    v3 Result = Position;
    Result.X = CenterCameraAxis(Position.X, MinX, MaxX, (float)WindowWidth);
    Result.Y = CenterCameraAxis(Position.Y, MinY, MaxY, (float)WindowHeight);
    return Result;
}

// NOTE(zoubir): the push toward the cursor, in world units. View is the
// window in world units; the mouse is in window pixels
internal v2
CameraLean(app_input *Input, app_window *View, float Zoom)
{
    v2 Mouse = V2((float)Input->MouseX / Zoom, (float)Input->MouseY / Zoom);
    v2 FromCentre = V2(Mouse.X - 0.5f * (float)View->Width,
                       Mouse.Y - 0.5f * (float)View->Height);
    // NOTE(zoubir): a cursor outside the window (or not moved yet) pulls nothing
    if (Mouse.X < 0.f || Mouse.Y < 0.f ||
        Mouse.X > (float)View->Width || Mouse.Y > (float)View->Height)
    {
        FromCentre = V2(0.f, 0.f);
    }
    v2 Result = CAMERA_LEAN_SHARE * FromCentre;
    float Size = Length(Result);
    if (Size > CAMERA_LEAN_MAX)
    {
        Result = (CAMERA_LEAN_MAX / Size) * Result;
    }
    return Result;
}

// NOTE(zoubir): keeps the previous position when there is no local player.
// Window is the view in world units (GetWorldView). Lean is off while a
// screen holds the mouse
internal v3
UpdateCamera(app_state *AppState, app_window *Window, app_input *Input,
             bool32 Lean)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
    if (Player)
    {
        v3 Focus = Player->Position;
        if (Lean && !IsDeadPlayer(Player))
        {
            Focus.XY += CameraLean(Input, Window, Zoom);
        }
        if (!IsDeadPlayer(Player))
        {
            v2 Lead = CAMERA_LEAD_SECONDS * Player->Velocity.XY;
            float LeadSize = Length(Lead);
            if (LeadSize > CAMERA_LEAD_MAX)
            {
                Lead = (CAMERA_LEAD_MAX / LeadSize) * Lead;
            }
            Focus.XY += Lead;
        }
        if (World->Unbounded)
        {
            AppState->TargetCamera = Focus;
            AppState->TargetCamera.X -= (float)(Window->Width / 2);
            AppState->TargetCamera.Y -= (float)(Window->Height / 2);
        }
        else
        {
            float ArenaWidth = (float)(World->NumTilesX * World->TileWidth);
            float ArenaHeight = (float)(World->NumTilesY * World->TileHeight);
            AppState->TargetCamera = CenterCamera(Focus, 0.f, 0.f,
                                                  ArenaWidth, ArenaHeight,
                                                  Window->Width, Window->Height);
        }
    }

    v3 *Camera = &AppState->CameraOffset;
    v3 Gap = AppState->TargetCamera - *Camera;
    float CutDistance = CAMERA_CUT_SCREENS * (float)Maximum(Window->Width, Window->Height);
    if (!AppState->CameraPlaced || Length(Gap.XY) > CutDistance)
    {
        *Camera = AppState->TargetCamera;
        AppState->CameraPlaced = true;
    }
    else
    {
        float Share = 1.f - expf(-CAMERA_FOLLOW_RATE * Input->DeltaTime);
        *Camera += Share * Gap;
    }

    v3 CameraOffset = *Camera;
    // NOTE(zoubir): hits shake the screen (fx_bursts.cpp); added only to
    // what is drawn, never to where the camera is heading
    CameraOffset.XY += GetCameraShake(AppState) + GetHitNudge(AppState);
    // NOTE(zoubir): to whole window pixels, rounding down so negative
    // positions snap the same way as positive ones
    CameraOffset.X = SnapToScreenPixel(CameraOffset.X, Zoom);
    CameraOffset.Y = SnapToScreenPixel(CameraOffset.Y, Zoom);
    return CameraOffset;
}
