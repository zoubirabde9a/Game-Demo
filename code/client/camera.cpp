/* Camera: the screen follows the local player, never showing past the
   edge of a bounded map (infinite maps have no edge). The result is the
   top-left corner of the screen in world units, snapped to whole pixels.

   The camera eases toward its target instead of jumping to it: each
   frame it closes the same share of the gap per second whatever the
   frame rate (CAMERA_FOLLOW_RATE), so uneven frames and the small
   corrections online play makes to the player do not shake the view. It
   also leans a little toward the mouse, so more of the screen lies the
   way the player aims. A respawn or a jump across the map is a cut, not
   a long pan.

   Called after the world has moved this frame, so the camera and the
   player it follows are drawn from the same positions. */

// NOTE(zoubir): share of the gap closed per second, as a rate: higher is
// tighter. 10 settles in about a third of a second
#define CAMERA_FOLLOW_RATE 10.f
// NOTE(zoubir): the lean is this share of the cursor's distance from the
// screen centre, at most CAMERA_LEAN_MAX pixels
#define CAMERA_LEAN_SHARE 0.12f
#define CAMERA_LEAN_MAX 70.f
// NOTE(zoubir): a target farther than this many screen sizes away is cut to
#define CAMERA_CUT_SCREENS 0.75f

inline v3
CenterCamera(v3 Position, float MinX, float MinY,
             float MaxX, float MaxY, u32 WindowWidth,
             u32 WindowHeight)
{
    v3 Result = Position;

    Result.X -= (float)(WindowWidth / 2);
    Result.Y -= (float)(WindowHeight / 2);

    Result.X = Minimum(Result.X, MaxX - WindowWidth);
    Result.Y = Minimum(Result.Y, MaxY - WindowHeight);

    Result.X = Maximum(Result.X, MinX);
    Result.Y = Maximum(Result.Y, MinY);

    return Result;
}

// NOTE(zoubir): the push toward the cursor, in pixels
internal v2
CameraLean(app_input *Input, app_window *Window)
{
    v2 FromCentre = V2((float)Input->MouseX - 0.5f * (float)Window->Width,
                       (float)Input->MouseY - 0.5f * (float)Window->Height);
    // NOTE(zoubir): a cursor outside the window (or not moved yet) pulls nothing
    if (Input->MouseX < 0 || Input->MouseY < 0 ||
        Input->MouseX > (i32)Window->Width || Input->MouseY > (i32)Window->Height)
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
// Lean is off while a screen holds the mouse
internal v3
UpdateCamera(app_state *AppState, app_window *Window, app_input *Input,
             bool32 Lean)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player)
    {
        v3 Focus = Player->Position;
        if (Lean && !IsDeadPlayer(Player))
        {
            Focus.XY += CameraLean(Input, Window);
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
    CameraOffset.XY += GetCameraShake(AppState);
    // NOTE(zoubir): floor, not a cast, so negative positions snap the
    // same way as positive ones
    CameraOffset.X = floorf(CameraOffset.X);
    CameraOffset.Y = floorf(CameraOffset.Y);
    return CameraOffset;
}
