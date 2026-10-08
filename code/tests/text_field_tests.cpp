/* Text field tests: keys reach an edit box in the order they were typed
   (engine/ui/edit_box.cpp), so a fast typist's backspaces, a word erase
   and a paste all land where they should. Included by sim_tests.cpp,
   which calls RunTextFieldTests. */

internal void
TypeKeys(ui_state *Box, char *Keys, u32 MaxLength = 15)
{
    TypeIntoEditBox(Box, Keys, (u32)strlen(Keys), MaxLength);
}

internal void
TestTypingKeepsKeyOrder()
{
    ui_state Box = {};
    char Keys[16] = {'a', 'b', TEXT_KEY_ERASE, 'c', 0};
    TypeKeys(&Box, Keys);
    Check(strcmp(Box.Text, "ac") == 0);

    // NOTE(zoubir): three backspaces in one frame take off three letters
    char Erase3[4] = {TEXT_KEY_ERASE, TEXT_KEY_ERASE, TEXT_KEY_ERASE, 0};
    TypeKeys(&Box, (char *)"defg");
    TypeKeys(&Box, Erase3);
    Check(strcmp(Box.Text, "acd") == 0);
    TypeKeys(&Box, Erase3);
    TypeKeys(&Box, Erase3);
    Check(Box.TextCount == 0 && Box.Text[0] == 0);
}

internal void
TestWordEraseAndLimits()
{
    ui_state Box = {};
    char Word[2] = {TEXT_KEY_ERASE_WORD, 0};
    TypeKeys(&Box, (char *)"Iron Owl  ");
    TypeKeys(&Box, Word);
    Check(strcmp(Box.Text, "Iron ") == 0);
    TypeKeys(&Box, Word);
    Check(Box.TextCount == 0);

    // NOTE(zoubir): a paste longer than the field keeps what fits
    TypeKeys(&Box, (char *)"192.168.100.200:27015", 15);
    Check(strcmp(Box.Text, "192.168.100.200") == 0);
}

internal void
TestSelectedTextIsReplaced()
{
    ui_state Box = {};
    TypeKeys(&Box, (char *)"Ember Lynx");
    Box.AllSelected = true;
    TypeKeys(&Box, (char *)"M");
    Check(strcmp(Box.Text, "M") == 0);
    Check(!Box.AllSelected);

    // NOTE(zoubir): Backspace on a selection clears it, and only it
    TypeKeys(&Box, (char *)"ahdi");
    Box.AllSelected = true;
    char Erase[2] = {TEXT_KEY_ERASE, 0};
    TypeKeys(&Box, Erase);
    Check(Box.TextCount == 0);
}

internal void
RunTextFieldTests()
{
    printf("TestTypingKeepsKeyOrder\n");
    TestTypingKeepsKeyOrder();
    printf("TestWordEraseAndLimits\n");
    TestWordEraseAndLimits();
    printf("TestSelectedTextIsReplaced\n");
    TestSelectedTextIsReplaced();
}
