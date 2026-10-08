/* Key binding tests (client/key_bindings.cpp): an action put on another
   key casts from that key and no longer from its old one, offline and in
   what goes to the server; the movement keys move too; a key another
   action had swaps with it; duels, dungeon runs and the two control
   schemes each keep their own keys; keys.txt reads back what it wrote and
   skips what it does not understand. Included by sim_tests.cpp, which
   calls RunKeyBindingTests; uses the helpers of cursor_tests.cpp. */

global_variable key_bindings TestKeyBindings;

internal test_world
CreateKeyBindingTestWorld()
{
    test_world Result = CreateCursorTestWorld(CastMode_Quick);
    TestKeyBindings = {};
    GlobalKeyBindings = &TestKeyBindings;
    return Result;
}

internal void
DestroyKeyBindingTestWorld(test_world *Test)
{
    GlobalKeyBindings = 0;
    GlobalControlScheme = ControlScheme_Keys;
    GlobalKeyboardLayout = KeyboardLayout_Azerty;
    DestroyTestWorld(Test);
}

// NOTE(zoubir): Launch on 1: 1 launches, A does nothing, the label and
// the server's bits follow
internal void
TestReboundKeyCasts()
{
    test_world Test = CreateKeyBindingTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    SetKeyBinding(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys, 4, KeyCode_0 + 1);
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "1") == 0);
    PressKey(&Input->ButtonA);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == 0);
    NextInputFrame(Input);
    ReleaseKey(&Input->ButtonA);
    PressKey(&Input->Button1);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Pressed == PlayerButton_Launch);
    app_input Sent = InputForServer(Input, Test.AppState);
    Check(NetButtonsFromKeyboard(&Sent) & ((u32)PlayerButton_Launch << PLAYER_BUTTON_NET_SHIFT));
    DestroyKeyBindingTestWorld(&Test);
}

// NOTE(zoubir): moving up on the up arrow: the arrow walks, Z does not,
// online too, and the hints name the keys
internal void
TestReboundMovement()
{
    test_world Test = CreateKeyBindingTestWorld();
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    app_input *Input = &Test.Input;
    SetKeyBinding(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys, BINDING_MOVE_UP,
                  KeyCode_Up);
    PressKey(&Input->ButtonZ);
    player_input Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Move.Y == 0.f);
    Check(!(NetButtonsFromKeyboard(Input) & NetButton_Up));
    ReleaseKey(&Input->ButtonZ);
    PressKey(&Input->ArrowUp);
    Local = ReadKeyboardPlayerInput(Input, Test.AppState);
    Check(Local.Move.Y == -1.f);
    Check(NetButtonsFromKeyboard(Input) & NetButton_Up);
    char Text[40];
    MoveKeysText(Text, sizeof(Text));
    Check(strcmp(Text, "Up Q S D") == 0);
    ResetKeyBinding(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys, BINDING_MOVE_UP);
    MoveKeysText(Text, sizeof(Text));
    Check(strcmp(Text, "ZQSD") == 0);
    DestroyKeyBindingTestWorld(&Test);
}

// NOTE(zoubir): Launch on X, the fireball's key: the fireball takes A
internal void
TestTakenKeySwaps()
{
    test_world Test = CreateKeyBindingTestWorld();
    u32 Swapped = SetKeyBinding(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys, 4,
                                LetterKeyCode('X'));
    Check(Swapped == 1);
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "X") == 0);
    Check(strcmp(ActionKeyLabel(PlayerButton_Cast), "A") == 0);
    for(u32 Row = 0; Row < BINDING_COUNT; Row++)
    {
        Check(!KeyBindingClashes(BindingMode_Duel, ControlScheme_Keys, Row));
    }
    ResetKeyBindingSet(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys);
    Check(!KeyBindingSetChanged(&TestKeyBindings, BindingMode_Duel, ControlScheme_Keys));
    Check(strcmp(ActionKeyLabel(PlayerButton_Cast), "X") == 0);
    DestroyKeyBindingTestWorld(&Test);
}

// NOTE(zoubir): a dungeon key does not move the duel's, nor the other
// scheme's; the dungeon tab has no talent abilities
internal void
TestEachSetKeepsItsKeys()
{
    test_world Test = CreateKeyBindingTestWorld();
    SetKeyBinding(&TestKeyBindings, BindingMode_Dungeon, ControlScheme_Keys, 4, KeyCode_0 + 1);
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "A") == 0);
    TestKeyBindings.Mode = BindingMode_Dungeon;
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "1") == 0);
    Check(KeyCodeBoundNow(KeyCode_0 + 1));
    GlobalControlScheme = ControlScheme_Mouse;
    Check(strcmp(ActionKeyLabel(PlayerButton_Launch), "A") == 0);
    Check(!KeyCodeBoundNow(KeyCode_0 + 1));
    Check(!BindingRowUsed(BindingMode_Duel, ControlScheme_Mouse, BINDING_MOVE_UP));
    Check(BindingRowUsed(BindingMode_Duel, ControlScheme_Keys, 9));
    Check(!BindingRowUsed(BindingMode_Dungeon, ControlScheme_Keys, 9));
    Check(BindingRowUsed(BindingMode_Dungeon, ControlScheme_Keys, 4));
    // NOTE(zoubir): a picked key is the key itself; a default letter
    // follows the layout
    SetKeyBinding(&TestKeyBindings, BindingMode_Dungeon, ControlScheme_Mouse, 7,
                  LetterKeyCode('Y'));
    GlobalKeyboardLayout = KeyboardLayout_Qwerty;
    Check(strcmp(ActionKeyLabel(PlayerButton_Push), "Y") == 0);
    Check(strcmp(ActionKeyLabel(PlayerButton_Cast), "W") == 0);
    DestroyKeyBindingTestWorld(&Test);
}

// NOTE(zoubir): keys.txt back into the same keys; lines it cannot use,
// and reserved keys, are skipped
internal void
TestKeyBindingsFileRoundTrip()
{
    key_bindings Written = {};
    Written.Keys[BindingMode_Dungeon][ControlScheme_Keys][4] = KeyCode_0 + 1;
    Written.Keys[BindingMode_Duel][ControlScheme_Mouse][5] = KeyCode_Mouse4;
    Written.Keys[BindingMode_Duel][ControlScheme_Keys][BINDING_MOVE_LEFT] = KeyCode_Left;
    char Text[KEY_BINDINGS_FILE_SIZE];
    FormatKeyBindings(&Written, Text, sizeof(Text));
    Check(strstr(Text, "dungeon keys launch 1\n") != 0);
    Check(strstr(Text, "duel mouse sword Mouse4\n") != 0);
    key_bindings Read = {};
    ParseKeyBindings(&Read, Text);
    Check(memcmp(&Read.Keys, &Written.Keys, sizeof(Read.Keys)) == 0);

    key_bindings Junk = {};
    ParseKeyBindings(&Junk, (char *)"duel keys launch\r\nduel keys nothing Q\n"
                     "arena keys jump Q\nduel keys jump H\n  DUEL  KEYS  Blink  shift\r\n");
    Check(Junk.Keys[BindingMode_Duel][ControlScheme_Keys][3] == KeyCode_Shift);
    Junk.Keys[BindingMode_Duel][ControlScheme_Keys][3] = KeyCode_None;
    key_bindings Empty = {};
    Check(memcmp(&Junk, &Empty, sizeof(Junk)) == 0);
}

internal void
RunKeyBindingTests()
{
    CURSOR_TEST(TestReboundKeyCasts);
    CURSOR_TEST(TestReboundMovement);
    CURSOR_TEST(TestTakenKeySwaps);
    CURSOR_TEST(TestEachSetKeepsItsKeys);
    CURSOR_TEST(TestKeyBindingsFileRoundTrip);
}
