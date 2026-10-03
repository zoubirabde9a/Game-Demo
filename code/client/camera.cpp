/* Camera: the screen follows the local player, centred on them but never
   showing past the edge of the arena. The result is the top-left corner
   of the screen in world units, snapped to whole pixels. */

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
    if (Player)
    {
        float ArenaWidth = (float)(World->NumTilesX * World->TileWidth);
        float ArenaHeight = (float)(World->NumTilesY * World->TileHeight);
        AppState->TargetCamera = CenterCamera(Player->Position, 0.f, 0.f,
                                              ArenaWidth, ArenaHeight,
                                              Window->Width, Window->Height);
    }
    v3 CameraOffset = AppState->CameraOffset = AppState->TargetCamera;
    CameraOffset.X = (float)((u32)CameraOffset.X);
    CameraOffset.Y = (float)((u32)CameraOffset.Y);
    return CameraOffset;
}
