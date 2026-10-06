/* Keyboard layout: AZERTY or QWERTY, the player's pick in the Esc options
   menu (ui/options_menu.cpp), saved on the third line of server.txt
   (online_config.cpp). The game's keys are named as on AZERTY (ZQSD move,
   A launch, W shockwave). On QWERTY the keys at the same places are used
   instead, so A and Q swap and Z and W swap: WASD moves, Q launches, Z
   is the shockwave. Code reading a letter key goes through LayoutKey, and
   text naming one through LayoutKeyName. */

enum keyboard_layout
{
    KeyboardLayout_Azerty,
    KeyboardLayout_Qwerty,
};

// NOTE(zoubir): one setting for this machine, read once at startup; a
// global so the action key table (action_keys.cpp), which only sees the
// input, can follow it
global_variable keyboard_layout GlobalKeyboardLayout = KeyboardLayout_Azerty;

// NOTE(zoubir): the letter at the place AZERTY puts AzertyLetter, on the
// layout in use; any other character comes back as it is
internal char
LayoutLetter(char AzertyLetter)
{
    char Result = AzertyLetter;
    if (GlobalKeyboardLayout == KeyboardLayout_Qwerty)
    {
        switch (AzertyLetter)
        {
            case 'A': Result = 'Q'; break;
            case 'Q': Result = 'A'; break;
            case 'Z': Result = 'W'; break;
            case 'W': Result = 'Z'; break;
        }
    }
    return Result;
}

internal app_button_state *
LayoutKey(app_input *Input, char AzertyLetter)
{
    app_button_state *Result = &Input->AlphaButtons[LayoutLetter(AzertyLetter) - 'A'];
    return Result;
}

// NOTE(zoubir): the one-letter name of the key, as a string that lives
// for the whole run
internal char *
LayoutKeyName(char AzertyLetter)
{
    static char Names[26][2];
    char Letter = LayoutLetter(AzertyLetter);
    char *Result = Names[Letter - 'A'];
    Result[0] = Letter;
    Result[1] = 0;
    return Result;
}

// NOTE(zoubir): Text with each capital letter swapped for its key on the
// layout in use ("ZQSD" reads "WASD" on QWERTY)
internal void
LayoutKeyText(char *Out, u32 OutSize, char *Text)
{
    u32 Length = 0;
    for(; Text[Length] && Length + 1 < OutSize; Length++)
    {
        char Letter = Text[Length];
        Out[Length] = (Letter >= 'A' && Letter <= 'Z') ? LayoutLetter(Letter) : Letter;
    }
    Out[Length] = 0;
}

// NOTE(zoubir): the words server.txt stores
internal char *
KeyboardLayoutWord(keyboard_layout Layout)
{
    char *Result = (Layout == KeyboardLayout_Qwerty) ? (char *)"qwerty" : (char *)"azerty";
    return Result;
}
