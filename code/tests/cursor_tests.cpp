/* Cursor tests: a left click casts wherever it lands, and the tile
   editor's clicks paint without casting, here or online. Included by
   sim_tests.cpp, which calls RunCursorTests. */

inline void
PressLeftButton(app_input *Input, i32 X, i32 Y)
{
    Input->MouseX = X;
    Input->MouseY = Y;
    Input->LeftButton.Pressed = true;
    Input->LeftButton.EndedDown = true;
}

// NOTE(zoubir): a click on bare ground used to walk there instead
internal void
TestLeftClickOnGroundCasts()
{
    test_world Test = CreateTestWorld();
    world_entity *Player = AddPlayerToSlot(Test.AppState, Test.World,
                                           &Test.Arena, 0, {300, 300, 0});
    PressLeftButton(&Test.Input, 600, 300);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check((Local.Pressed & PlayerButton_Cast) != 0);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    app_input Sent = InputForServer(&Test.Input, Test.AppState);
    Check(Sent.LeftButton.EndedDown);
    Check(Player->Position.X < 301.f);
    DestroyTestWorld(&Test);
}

internal void
TestTileEditorClicksDoNotCast()
{
    test_world Test = CreateTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    Test.AppState->TileEditing = true;
    PressLeftButton(&Test.Input, 450, 300);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check((Local.Pressed & PlayerButton_Cast) == 0);
    app_input Sent = InputForServer(&Test.Input, Test.AppState);
    Check(!Sent.LeftButton.EndedDown && !Sent.LeftButton.Pressed);
    DestroyTestWorld(&Test);
}

#define CURSOR_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunCursorTests()
{
    CURSOR_TEST(TestLeftClickOnGroundCasts);
    CURSOR_TEST(TestTileEditorClicksDoNotCast);
}
