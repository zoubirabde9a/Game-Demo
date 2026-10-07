/* The mouse cursor on the ground: where it points in the world, for the
   aim (keyboard_input.cpp), and the input the online session sends. A
   left click always casts; while the tile editor (F3) is open the left
   button paints tiles instead, so it is never sent as a cast. */

// NOTE(zoubir): a world coordinate rounded down to a whole window pixel
// at Zoom pixels per unit; the camera snaps this way (camera.cpp) so pixel
// art does not shimmer
inline float
SnapToScreenPixel(float World, float Zoom)
{
    float Result = floorf(World * Zoom) / Zoom;
    return Result;
}

// NOTE(zoubir): the cursor on the ground, in world units; the camera is
// last frame's, which is what is on screen
inline v2
CursorInWorld(app_input *Input, app_state *AppState)
{
    float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
    v2 Result = V2((float)Input->MouseX / Zoom + SnapToScreenPixel(AppState->CameraOffset.X, Zoom),
                   (float)Input->MouseY / Zoom + SnapToScreenPixel(AppState->CameraOffset.Y, Zoom));
    return Result;
}

// NOTE(zoubir): the input the online session sends: the tile editor's
// left button paints, so the server must not see it held
internal app_input
InputForServer(app_input *Input, app_state *AppState)
{
    app_input Result = *Input;
    if (AppState->TileEditing)
    {
        Result.LeftButton = {};
    }
    if (TalentPanelHasMouse(AppState, Input) || PartyFramesHaveMouse(AppState, Input))
    {
        Result.LeftButton = {};
        Result.RightButton = {};
    }
    FilterCastKeys(&Result, AppState);
    return Result;
}
