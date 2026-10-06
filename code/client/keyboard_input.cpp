/* Local controls: turns this machine's keyboard and mouse into the
   player_input the simulation understands. ZQSD move on AZERTY and
   WASD on QWERTY (picked in the Esc menu, keyboard_layout.cpp),
   the mouse cursor aims, right click sword, left click fireball, Space
   jump, Alt dash, E shockwave, F blink, R push, A launch (the action keys
   are one table, action_keys.cpp). In standard cast mode an area
   ability's key aims it first and a left click casts it
   (cast_targeting.cpp). Holding Tab shows the scoreboard
   (read in app.cpp, it is not a player action). While the tile editor
   (F3) is open the left button paints tiles and does not cast. */

// NOTE(zoubir): from the local player to the cursor, in world units, over
// PLAYER_AIM_REACH and capped at length 1; the camera is last frame's,
// which is what is on screen. Zero when there is no local player or the
// cursor sits on it.
internal v2
AimFromCursor(app_input *Input, app_state *AppState)
{
    v2 Result = {};
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player)
    {
        v2 Cursor = CursorInWorld(Input, AppState);
        v2 ToCursor = Cursor - Player->Position.XY;
        float LengthSquared = LengthSq(ToCursor);
        if (LengthSquared > 4.f)
        {
            float Length = SquareRoot(LengthSquared);
            Result = ToCursor * (Minimum(Length, PLAYER_AIM_REACH) /
                                 (Length * PLAYER_AIM_REACH));
        }
    }
    return Result;
}

internal player_input
ReadKeyboardPlayerInput(app_input *Input, app_state *AppState)
{
    player_input Result = {};
    Result.Aim = AimFromCursor(Input, AppState);
    if (LayoutKey(Input, 'Z')->EndedDown) { Result.Move.Y = -1.f; }
    if (Input->ButtonS.EndedDown) { Result.Move.Y = 1.f; }
    if (Input->ButtonD.EndedDown) { Result.Move.X = 1.f; }
    if (LayoutKey(Input, 'Q')->EndedDown) { Result.Move.X = -1.f; }

    // NOTE(zoubir): standard cast holds an aimed ability back until it is
    // confirmed (cast_targeting.cpp); quick cast passes the keys through
    UpdateCastTargeting(Input, AppState);
    Result.Pressed = FilterCastButtons(AppState, ActionButtonsFromKeys(Input, true));
    if (AppState->TileEditing)
    {
        Result.Pressed &= ~(u32)PlayerButton_Cast;
    }
    // NOTE(zoubir): a click on the talent panel is the panel's
    if (TalentPanelHasMouse(AppState, Input))
    {
        Result.Pressed &= ~(u32)(PlayerButton_Cast | PlayerButton_Attack);
    }
    // NOTE(zoubir): offline the local simulation spends the points the
    // panel asked for; online they go in the held buttons (online.cpp)
    if (!IsOnline(AppState->Online))
    {
        Result.Learn = TakeOfflineTalentRequest(AppState);
    }
#if APP_DEV
    // NOTE(zoubir): F6, offline in developer builds: the experience to the
    // next level, to try the talent tree without playing for it
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (Input->ButtonF6.Pressed && !IsOnline(AppState->Online) && Slot->Active &&
        Slot->Level < PLAYER_MAX_LEVEL)
    {
        AwardXp(AppState, Slot, XpToReach(Slot->Level + 1) - Slot->Xp);
    }
#endif
    return Result;
}
