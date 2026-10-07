/* Cursor tests: a left click casts wherever it lands, and the tile
   editor's clicks paint without casting, here or online. Standard cast
   aims an area ability on its key and casts it on the next left click
   (client/cast_targeting.cpp); quick cast casts it on the key. The
   cursor picks the unit whose body it is on (client/targeting.cpp). On QWERTY
   the letter keys move to the same places (client/keyboard_layout.cpp).
   Included
   by sim_tests.cpp, which calls RunCursorTests. */

inline void
PressLeftButton(app_input *Input, i32 X, i32 Y)
{
    Input->MouseX = X;
    Input->MouseY = Y;
    Input->LeftButton.Pressed = true;
    Input->LeftButton.EndedDown = true;
}

// NOTE(zoubir): the test world's app_state has no MemoryArena to make
// the cast targeting state in, so it lives here
global_variable cast_targeting TestCastTargeting;
global_variable cursor_targeting TestCursorTargeting;

internal test_world
CreateCursorTestWorld(cast_mode Mode = CastMode_Standard)
{
    test_world Result = CreateTestWorld();
    TestCastTargeting = {};
    TestCastTargeting.Mode = Mode;
    Result.AppState->CastTargeting = &TestCastTargeting;
    TestCursorTargeting = {};
    Result.AppState->CursorTargeting = &TestCursorTargeting;
    return Result;
}

// NOTE(zoubir): the next frame: what was down stays down, nothing is new
inline void
NextInputFrame(app_input *Input)
{
    action_key Keys[ACTION_KEY_COUNT];
    GetActionKeys(Input, Keys);
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        Keys[Index].Key->Pressed = false;
    }
}

inline void
PressKey(app_button_state *Key)
{
    Key->Pressed = true;
    Key->EndedDown = true;
}

inline void
ReleaseKey(app_button_state *Key)
{
    Key->Pressed = false;
    Key->EndedDown = false;
}

// NOTE(zoubir): a click on bare ground used to walk there instead
internal void
TestLeftClickOnGroundCasts()
{
    test_world Test = CreateCursorTestWorld();
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
    test_world Test = CreateCursorTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    Test.AppState->TileEditing = true;
    PressLeftButton(&Test.Input, 450, 300);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check((Local.Pressed & PlayerButton_Cast) == 0);
    app_input Sent = InputForServer(&Test.Input, Test.AppState);
    Check(!Sent.LeftButton.EndedDown && !Sent.LeftButton.Pressed);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): standard cast: A aims Launch and sends nothing; the left
// click casts it, and is no fireball, here or online; holding the click
// on does not fire one either
internal void
TestStandardCastAimsThenClickCasts()
{
    test_world Test = CreateCursorTestWorld(CastMode_Standard);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    Input->MouseX = 600;
    Input->MouseY = 300;
    PressKey(&Input->ButtonA);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    Check(Test.AppState->CastTargeting->Aiming == PlayerButton_Launch);
    app_input Sent = InputForServer(Input, Test.AppState);
    Check(!Sent.ButtonA.EndedDown);

    NextInputFrame(Input);
    ReleaseKey(&Input->ButtonA);
    PressKey(&Input->LeftButton);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Launch);
    Check(Test.AppState->CastTargeting->Aiming == 0);
    Sent = InputForServer(Input, Test.AppState);
    Check(Sent.ButtonA.EndedDown && !Sent.LeftButton.EndedDown);

    NextInputFrame(Input);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    Sent = InputForServer(Input, Test.AppState);
    Check(!Sent.ButtonA.EndedDown && !Sent.LeftButton.EndedDown);

    // NOTE(zoubir): once released, the left button is a fireball again
    NextInputFrame(Input);
    ReleaseKey(&Input->LeftButton);
    ReadKeyboardPlayerInput(Input, Test.AppState);
    NextInputFrame(Input);
    PressKey(&Input->LeftButton);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Cast);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a right click while aiming cancels: no Launch, no sword
internal void
TestStandardCastRightClickCancels()
{
    test_world Test = CreateCursorTestWorld(CastMode_Standard);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    PressKey(&Input->ButtonA);
    ReadKeyboardPlayerInput(Input, Test.AppState);
    NextInputFrame(Input);
    ReleaseKey(&Input->ButtonA);
    PressKey(&Input->RightButton);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    Check(Test.AppState->CastTargeting->Aiming == 0);
    app_input Sent = InputForServer(Input, Test.AppState);
    Check(!Sent.RightButton.EndedDown && !Sent.ButtonA.EndedDown);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): quick cast: the key casts at once
internal void
TestQuickCastCastsOnTheKey()
{
    test_world Test = CreateCursorTestWorld(CastMode_Quick);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    PressKey(&Test.Input.ButtonA);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Launch);
    Check(Test.AppState->CastTargeting->Aiming == 0);
    app_input Sent = InputForServer(&Test.Input, Test.AppState);
    Check(Sent.ButtonA.EndedDown);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): QWERTY: WASD moves and Q launches, where AZERTY has ZQSD
// and A; the key names follow
internal void
TestQwertyMovesTheLetterKeys()
{
    test_world Test = CreateCursorTestWorld(CastMode_Quick);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    GlobalKeyboardLayout = KeyboardLayout_Qwerty;
    PressKey(&Test.Input.ButtonW);
    PressKey(&Test.Input.ButtonA);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check(Local.Move.X == -1.f && Local.Move.Y == -1.f);
    Check(Local.Pressed == 0);
    ReleaseKey(&Test.Input.ButtonW);
    ReleaseKey(&Test.Input.ButtonA);
    PressKey(&Test.Input.ButtonQ);
    Local = ReadKeyboardPlayerInput(&Test.Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Launch);
    Check(Local.Move.X == 0.f && Local.Move.Y == 0.f);
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "Q") == 0);
    Check(strcmp(ActionKeyLabel(PlayerButton_Shockwave), "Z") == 0);
    char Keys[16];
    LayoutKeyText(Keys, sizeof(Keys), (char *)"ZQSD");
    Check(strcmp(Keys, "WASD") == 0);
    GlobalKeyboardLayout = KeyboardLayout_Azerty;
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "A") == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the cursor at a world point, the camera at the origin and
// one window pixel a world unit
inline void
PutCursorAt(test_world *Test, float X, float Y)
{
    Test->AppState->WorldZoom = 1.f;
    Test->AppState->CameraOffset = {};
    Test->Input.MouseX = (i32)X;
    Test->Input.MouseY = (i32)Y;
}

// NOTE(zoubir): the body on screen, above the feet, is what the cursor
// picks; a near miss snaps; overlapping bodies give the front one unless
// one was already picked; far off picks nothing
internal void
TestCursorPicksTheBodyOnScreen()
{
    test_world Test = CreateCursorTestWorld();
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {500, 300, 0});
    cursor_targeting *Targeting = AppState->CursorTargeting;

    // NOTE(zoubir): on the chest, 25 units above the feet
    PutCursorAt(&Test, 500, 275);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == A->ID + 1);

    // NOTE(zoubir): 10 units right of the body, fresh: snaps to it
    Targeting->Picked = 0;
    PutCursorAt(&Test, 525, 280);
    Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == A->ID + 1);

    // NOTE(zoubir): B stands just in front of A, the bodies overlap
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 2, {506, 312, 0});
    Targeting->Picked = 0;
    PutCursorAt(&Test, 503, 290);
    Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == B->ID + 1);
    Targeting->Picked = A->ID + 1;
    Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == A->ID + 1);

    // NOTE(zoubir): far off: nothing, even from a pick
    PutCursorAt(&Test, 700, 300);
    Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == 0);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): the local player is never its own target, and the kunai
// key with nothing picked says so by the cursor
internal void
TestCursorSkipsSelfAndSaysNoTarget()
{
    test_world Test = CreateCursorTestWorld(CastMode_Quick);
    app_state *AppState = Test.AppState;
    AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    PutCursorAt(&Test, 300, 280);
    PressKey(&Test.Input.ButtonV);
    player_input Local = ReadKeyboardPlayerInput(&Test.Input, AppState);
    Check(Local.Target == 0);
    Check(AppState->CursorTargeting->NoticeLeft > 0.f);
    DestroyTestWorld(&Test);
}

#define CURSOR_TEST(Test) printf("%s\n", #Test); Test()

internal void
RunCursorTests()
{
    CURSOR_TEST(TestLeftClickOnGroundCasts);
    CURSOR_TEST(TestTileEditorClicksDoNotCast);
    CURSOR_TEST(TestStandardCastAimsThenClickCasts);
    CURSOR_TEST(TestStandardCastRightClickCancels);
    CURSOR_TEST(TestQuickCastCastsOnTheKey);
    CURSOR_TEST(TestQwertyMovesTheLetterKeys);
    CURSOR_TEST(TestCursorPicksTheBodyOnScreen);
    CURSOR_TEST(TestCursorSkipsSelfAndSaysNoTarget);
}
