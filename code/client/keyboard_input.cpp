/* Local controls: turns this machine's keyboard and mouse into the
   player_input the simulation understands. ZQSD move on AZERTY and
   WASD on QWERTY (picked in the Esc menu, keyboard_layout.cpp),
   the mouse cursor aims, left click or X fireball, right click sword,
   Space jump, E shockwave, F blink, R push, A launch (the action keys
   are one table, action_keys.cpp, and the player can move any of them,
   the movement keys too, key_bindings.cpp). In the mouse-moves scheme
   (control_scheme.cpp) a left click walks instead (click_move.cpp) and
   the spells sit on the keys movement leaves free. In standard cast mode an area
   ability's key, and the kunai's (V), aims it first and a left click
   casts it (cast_targeting.cpp). Holding Tab shows the scoreboard
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
    Result.Target = UpdateCursorTarget(Input, AppState);
    if (!MouseMoves())
    {
        if (MoveKeyDown(Input, BINDING_MOVE_UP)) { Result.Move.Y = -1.f; }
        if (MoveKeyDown(Input, BINDING_MOVE_DOWN)) { Result.Move.Y = 1.f; }
        if (MoveKeyDown(Input, BINDING_MOVE_RIGHT)) { Result.Move.X = 1.f; }
        if (MoveKeyDown(Input, BINDING_MOVE_LEFT)) { Result.Move.X = -1.f; }
    }

    // NOTE(zoubir): standard cast holds an aimed ability back until it is
    // confirmed (cast_targeting.cpp); quick cast passes the keys through
    char *KunaiNo = LocalKunaiRefusal(AppState);
    u32 Refused = UpdateCastTargeting(Input, AppState, KunaiNo ? PlayerButton_Kunai : 0);
    if (Refused)
    {
        bool32 Kunai = (Refused & PlayerButton_Kunai) && KunaiNo;
        ShowTargetNotice(AppState, Input, Kunai ? KunaiNo : (char *)"Not ready");
    }
    Result.Pressed = FilterCastButtons(AppState, ActionButtonsFromKeys(Input, true));
    // NOTE(zoubir): the mouse-moves scheme walks to a free click
    if (MouseMoves())
    {
        Result.Move = UpdateClickMove(Input, AppState);
    }
    if (AppState->TileEditing)
    {
        Result.Pressed &= ~(u32)PlayerButton_Cast;
    }
    // NOTE(zoubir): a click on the talent panel is the panel's, one on
    // the party frames picks an ally (dungeon/role_targeting.cpp)
    if (TalentPanelHasMouse(AppState, Input) || PartyFramesHaveMouse(AppState, Input))
    {
        Result.Pressed &= ~(u32)PlayerButton_Attack;
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
        Slot->Level < TopLevel(AppState))
    {
        AwardXp(AppState, Slot, XpToReach(Slot->Level + 1) - Slot->Xp);
    }
#endif
    return Result;
}
