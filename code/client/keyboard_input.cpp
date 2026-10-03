/* Local controls: turns this machine's keyboard and mouse into the
   player_input the simulation understands. ZQSD move (AZERTY layout),
   the mouse cursor aims, right click sword, left click fireball, Space
   jump, Alt dash, E shockwave. Holding Tab shows the scoreboard (read in app.cpp, it
   is not a player action). */

// NOTE(zoubir): unit vector from the local player to the cursor, in world
// units; the camera is last frame's, which is what is on screen. Zero when
// there is no local player or the cursor sits on it.
internal v2
AimFromCursor(app_input *Input, app_state *AppState)
{
    v2 Result = {};
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player)
    {
        v2 Cursor = V2((float)Input->MouseX + floorf(AppState->CameraOffset.X),
                       (float)Input->MouseY + floorf(AppState->CameraOffset.Y));
        v2 ToCursor = Cursor - Player->Position.XY;
        float LengthSquared = LengthSq(ToCursor);
        if (LengthSquared > 4.f)
        {
            Result = ToCursor * (1.f / SquareRoot(LengthSquared));
        }
    }
    return Result;
}

internal player_input
ReadKeyboardPlayerInput(app_input *Input, app_state *AppState)
{
    player_input Result = {};
    Result.Aim = AimFromCursor(Input, AppState);
    if (Input->ButtonZ.EndedDown) { Result.Move.Y = -1.f; }
    if (Input->ButtonS.EndedDown) { Result.Move.Y = 1.f; }
    if (Input->ButtonD.EndedDown) { Result.Move.X = 1.f; }
    if (Input->ButtonQ.EndedDown) { Result.Move.X = -1.f; }

    if (Input->RightButton.Pressed) { Result.Pressed |= PlayerButton_Attack; }
    if (Input->LeftButton.Pressed) { Result.Pressed |= PlayerButton_Cast; }
    if (Input->SpaceButton.Pressed) { Result.Pressed |= PlayerButton_Jump; }
    if (Input->AltButton.Pressed) { Result.Pressed |= PlayerButton_Dash; }
    if (Input->ButtonE.Pressed) { Result.Pressed |= PlayerButton_Shockwave; }
    return Result;
}
