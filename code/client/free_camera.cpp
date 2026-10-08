/* Free camera: in a dungeon run a dead player lies downed until a healer
   revives them or the fight ends (sim/dungeon/revive.cpp). Meanwhile the
   camera comes loose from the body: the movement keys or the arrow keys
   fly it over the map at FREE_CAMERA_SPEED (Shift doubles it), and Space
   brings it back to the body. When the player stands again the camera
   goes back to them (camera.cpp cuts when that is far). Only this machine
   knows: the simulation never hears of it. */

// NOTE(zoubir): world units a second, about two runs
#define FREE_CAMERA_SPEED 420.f
#define FREE_CAMERA_FAST 2.f

// NOTE(zoubir): the movement keys and arrows as one direction, length 1
// or 0
internal v2
FreeCameraKeys(app_input *Input)
{
    v2 Result = {};
    // NOTE(zoubir): the keys-move scheme's movement keys either way: the
    // dead player casts nothing
    u32 Keys = ControlScheme_Keys;
    if (MoveKeyDown(Input, BINDING_MOVE_UP, Keys) || Input->ArrowUp.EndedDown) { Result.Y -= 1.f; }
    if (MoveKeyDown(Input, BINDING_MOVE_DOWN, Keys) || Input->ArrowDown.EndedDown) { Result.Y += 1.f; }
    if (MoveKeyDown(Input, BINDING_MOVE_RIGHT, Keys) || Input->ArrowRight.EndedDown) { Result.X += 1.f; }
    if (MoveKeyDown(Input, BINDING_MOVE_LEFT, Keys) || Input->ArrowLeft.EndedDown) { Result.X -= 1.f; }
    float Size = Length(Result);
    if (Size > 0.f)
    {
        Result = (1.f / Size) * Result;
    }
    return Result;
}

// NOTE(zoubir): where the camera centres while the local player is dead
// in a dungeon; false when it follows the player. Keys is false while a
// screen has the keyboard: the camera then holds still
internal bool32
FreeCameraFocus(app_state *AppState, app_input *Input, world_entity *Player,
                bool32 Keys, v2 *Focus)
{
    map_def *Map = GetMapDef((map_id)AppState->World.MapId);
    if (!Player || !IsDeadPlayer(Player) || !Map->Dungeon)
    {
        AppState->FreeCameraOn = false;
        return false;
    }
    if (!AppState->FreeCameraOn || (Keys && Input->SpaceButton.Pressed))
    {
        AppState->FreeCameraOn = true;
        AppState->FreeCamera = Player->Position.XY;
    }
    if (Keys)
    {
        float Speed = FREE_CAMERA_SPEED *
            (Input->ShiftButton.EndedDown ? FREE_CAMERA_FAST : 1.f);
        AppState->FreeCamera += (Speed * Input->DeltaTime) * FreeCameraKeys(Input);
    }
    world *World = &AppState->World;
    if (!World->Unbounded)
    {
        v2 *At = &AppState->FreeCamera;
        At->X = Maximum(0.f, Minimum(At->X, (float)(World->NumTilesX * World->TileWidth)));
        At->Y = Maximum(0.f, Minimum(At->Y, (float)(World->NumTilesY * World->TileHeight)));
    }
    *Focus = AppState->FreeCamera;
    return true;
}
