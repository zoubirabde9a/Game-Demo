/* Local controls: turns this machine's keyboard and mouse into the
   player_input the simulation understands. ZQSD move (AZERTY layout),
   right click sword, left click fireball, Space jump, Alt dash,
   E shockwave. Holding Tab shows the scoreboard (read in app.cpp, it
   is not a player action). */

internal player_input
ReadKeyboardPlayerInput(app_input *Input)
{
    player_input Result = {};
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
