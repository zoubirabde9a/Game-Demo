/* Camera: the screen follows the local player, centred on them but never
   showing past the edge of a bounded map (infinite maps have no edge). The
   result is the top-left corner of the screen in world units, snapped to
   whole pixels. */

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

// NOTE(zoubir): keeps the previous position when there is no local player
internal v3
UpdateCamera(app_state *AppState, app_window *Window)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player && World->Unbounded)
    {
        AppState->TargetCamera = Player->Position;
        AppState->TargetCamera.X -= (float)(Window->Width / 2);
        AppState->TargetCamera.Y -= (float)(Window->Height / 2);
    }
    else if (Player)
    {
        float ArenaWidth = (float)(World->NumTilesX * World->TileWidth);
        float ArenaHeight = (float)(World->NumTilesY * World->TileHeight);
        AppState->TargetCamera = CenterCamera(Player->Position, 0.f, 0.f,
                                              ArenaWidth, ArenaHeight,
                                              Window->Width, Window->Height);
    }
    v3 CameraOffset = AppState->CameraOffset = AppState->TargetCamera;
    // NOTE(zoubir): hits shake the screen (fx_bursts.cpp); added only to
    // what is drawn, never to where the camera is heading
    CameraOffset.XY += GetCameraShake(AppState);
    // NOTE(zoubir): floor, not a cast, so negative positions snap the
    // same way as positive ones
    CameraOffset.X = floorf(CameraOffset.X);
    CameraOffset.Y = floorf(CameraOffset.Y);
    return CameraOffset;
}
