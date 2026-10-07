/* Control scheme tests (client/control_scheme.cpp): in the mouse-moves
   scheme a left click walks the player to the spot and the spells sit on
   the keys movement leaves free, while a click that confirms an aim or
   lands on a screen's button walks nowhere. Esc closes what is open
   before it opens the options (ui/options_menu.cpp). The map vote line
   names every player's answer (ui/map_vote_view.cpp). Included by
   sim_tests.cpp, which calls RunControlSchemeTests; uses the helpers of
   cursor_tests.cpp. */

global_variable click_move TestClickMove;
global_variable talent_panel TestTalentPanel;

internal test_world
CreateMouseMovesTestWorld(cast_mode Mode = CastMode_Standard)
{
    test_world Result = CreateCursorTestWorld(Mode);
    TestClickMove = {};
    Result.AppState->ClickMove = &TestClickMove;
    GlobalControlScheme = ControlScheme_Mouse;
    GlobalMouseOnButton = false;
    return Result;
}

internal void
DestroyMouseMovesTestWorld(test_world *Test)
{
    GlobalControlScheme = ControlScheme_Keys;
    DestroyTestWorld(Test);
}

// NOTE(zoubir): a click walks toward the spot until the player is there,
// the eight ways the keys give, after the button is let go too
internal void
TestMouseMovesWalksToTheClick()
{
    test_world Test = CreateMouseMovesTestWorld();
    world_entity *Player = AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0,
                                           {300, 300, 0});
    app_input *Input = &Test.Input;
    PutCursorAt(&Test, 600, 300);
    PressKey(&Input->LeftButton);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    Check(Local.Move.X == 1.f && Local.Move.Y == 0.f);
    Check(MoveNetButtons(Local.Move) == NetButton_Right);

    NextInputFrame(Input);
    ReleaseKey(&Input->LeftButton);
    PutCursorAt(&Test, 100, 100);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Move.X == 1.f && Local.Move.Y == 0.f);

    Player->Position.X = 596.f;
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    Check(!TestClickMove.Walking);

    // NOTE(zoubir): down and to the right: both keys
    PutCursorAt(&Test, 800, 600);
    PressKey(&Input->LeftButton);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Move.X == 1.f && Local.Move.Y == 1.f);
    DestroyMouseMovesTestWorld(&Test);
}

// NOTE(zoubir): the movement keys are spells now: D throws the kunai and
// moves nothing, Z (W on QWERTY) is the fireball, and the labels follow
internal void
TestMouseMovesPutsSpellsOnTheMoveKeys()
{
    test_world Test = CreateMouseMovesTestWorld(CastMode_Quick);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    PressKey(&Input->ButtonZ);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Cast);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    NextInputFrame(Input);
    ReleaseKey(&Input->ButtonZ);
    PressKey(&Input->ButtonS);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Slam);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    Check(strcmp(ActionKeyLabel(PlayerButton_Kunai), "D") == 0);
    Check(strcmp(ActionKeyLabel(PlayerButton_Cast), "Z") == 0);
    GlobalKeyboardLayout = KeyboardLayout_Qwerty;
    Check(strcmp(ActionKeyLabel(PlayerButton_Cast), "W") == 0);
    Check(strcmp(ActionKeyLabel(PlayerButton_Shockwave), "A") == 0);
    GlobalKeyboardLayout = KeyboardLayout_Azerty;
    // NOTE(zoubir): online, S is the slam and not a step down
    app_input Sent = InputForServer(Input, Test.AppState);
    Check(Sent.ButtonS.EndedDown);
    u32 Held = NetButtonsFromKeyboard(&Sent);
    Check(!(Held & NetButton_Down));
    Check(Held & ((u32)PlayerButton_Slam << PLAYER_BUTTON_NET_SHIFT));
    DestroyMouseMovesTestWorld(&Test);
}

// NOTE(zoubir): a click that confirms an aimed spell casts it and walks
// nowhere; one on a screen's button does neither
internal void
TestMouseMovesClickOnAimOrButtonDoesNotWalk()
{
    test_world Test = CreateMouseMovesTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    PutCursorAt(&Test, 600, 300);
    PressKey(&Input->ButtonA);
    ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Test.AppState->CastTargeting->Aiming == PlayerButton_Launch);
    NextInputFrame(Input);
    ReleaseKey(&Input->ButtonA);
    PressKey(&Input->LeftButton);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Launch);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);

    NextInputFrame(Input);
    ReleaseKey(&Input->LeftButton);
    ReadKeyboardPlayerInput(Input, Test.AppState);
    GlobalMouseOnButton = true;
    PressKey(&Input->LeftButton);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    GlobalMouseOnButton = false;
    DestroyMouseMovesTestWorld(&Test);
}

// NOTE(zoubir): keys move: a click on a screen's button throws nothing
internal void
TestKeysMoveClickOnButtonThrowsNothing()
{
    test_world Test = CreateCursorTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    GlobalMouseOnButton = true;
    PressLeftButton(&Test.Input, 600, 300);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check(Local.Pressed == 0);
    GlobalMouseOnButton = false;
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): Esc closes the aim, then the talent panel, then the tile
// editor, one a press; with nothing open it has nothing to close
internal void
TestEscClosesTheTopScreenFirst()
{
    test_world Test = CreateCursorTestWorld();
    app_state *AppState = Test.AppState;
    TestTalentPanel = {};
    AppState->TalentPanel = &TestTalentPanel;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    PressKey(&Test.Input.ButtonA);
    ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(AppState->CastTargeting->Aiming == PlayerButton_Launch);
    TestTalentPanel.Open = true;
    AppState->TileEditing = true;

    Check(CloseTopScreen(AppState));
    Check(AppState->CastTargeting->Aiming == 0 && TestTalentPanel.Open);
    Check(CloseTopScreen(AppState));
    Check(!TestTalentPanel.Open && AppState->TileEditing);
    Check(CloseTopScreen(AppState));
    Check(!AppState->TileEditing);
    Check(!CloseTopScreen(AppState));
    AppState->TalentPanel = 0;
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the vote line names every player's answer
internal void
TestVoteAnswersNameEveryPlayer()
{
    test_world Test = CreateCursorTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {500, 300, 0});
    snprintf(AppState->Players[0].Name, sizeof(AppState->Players[0].Name), "Gary");
    snprintf(AppState->Players[1].Name, sizeof(AppState->Players[1].Name), "Mira");
    AppState->Votes[0] = MapVote_Yes;
    AppState->Votes[1] = MapVote_None;
    char Text[48];
    Check(VoteAnswerText(AppState, 0, Text, sizeof(Text)) == UI_COLOR_GOOD);
    Check(strcmp(Text, "Gary: yes") == 0);
    Check(VoteAnswerText(AppState, 1, Text, sizeof(Text)) == UI_COLOR_TEXT_MUTED);
    Check(strcmp(Text, "Mira: ...") == 0);
    AppState->Votes[1] = MapVote_No;
    Check(VoteAnswerText(AppState, 1, Text, sizeof(Text)) == UI_COLOR_HEALTH);
    AppState->Votes[0] = AppState->Votes[1] = MapVote_None;
    DestroyTestWorld(&Test);
}

internal void
RunControlSchemeTests()
{
    CURSOR_TEST(TestMouseMovesWalksToTheClick);
    CURSOR_TEST(TestMouseMovesPutsSpellsOnTheMoveKeys);
    CURSOR_TEST(TestMouseMovesClickOnAimOrButtonDoesNotWalk);
    CURSOR_TEST(TestKeysMoveClickOnButtonThrowsNothing);
    CURSOR_TEST(TestEscClosesTheTopScreenFirst);
    CURSOR_TEST(TestVoteAnswersNameEveryPlayer);
}
