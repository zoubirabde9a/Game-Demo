/* Key codes: one small number for every key or mouse button a player
   action can be put on (key_bindings.cpp), with its name for the screen
   and for keys.txt. A letter is the letter printed on the key, as the
   platform reports it, so a key a player picked stays that key on either
   keyboard layout. The left mouse button is never one: it walks, confirms
   an aim and clicks the screens' buttons. Keys the game keeps for itself
   (Esc, Tab, the F keys, H for the controls, N for the talents, J for the
   battle theme) are not codes, or are refused (KeyCodeReserved). */

enum key_code
{
    KeyCode_None,
    // NOTE(zoubir): KeyCode_A + 0 .. 25, then the digits KeyCode_0 + 0 .. 9
    KeyCode_A,
    KeyCode_0 = KeyCode_A + 26,
    KeyCode_Space = KeyCode_0 + 10,
    KeyCode_Shift,
    KeyCode_Alt,
    KeyCode_Caps,
    KeyCode_RightMouse,
    KeyCode_MiddleMouse,
    KeyCode_Mouse4,
    KeyCode_Mouse5,
    KeyCode_Up,
    KeyCode_Down,
    KeyCode_Left,
    KeyCode_Right,
    KeyCode_Count,
};

inline key_code
LetterKeyCode(char Letter)
{
    key_code Result = (key_code)(KeyCode_A + (Letter - 'A'));
    return Result;
}

// NOTE(zoubir): the code of the key at the place AZERTY puts AzertyLetter,
// on the layout in use (keyboard_layout.cpp)
inline key_code
LayoutKeyCode(char AzertyLetter)
{
    key_code Result = LetterKeyCode(LayoutLetter(AzertyLetter));
    return Result;
}

// NOTE(zoubir): Code's state in Input; a code with no key gets a button
// that is never down
internal app_button_state *
KeyCodeState(app_input *Input, u32 Code)
{
    static app_button_state Never;
    Never = {};
    app_button_state *Result = &Never;
    if (Code >= KeyCode_A && Code < KeyCode_0)
    {
        Result = &Input->AlphaButtons[Code - KeyCode_A];
    }
    else if (Code >= KeyCode_0 && Code < KeyCode_Space)
    {
        Result = &Input->NumbersButtons[Code - KeyCode_0];
    }
    else
    {
        switch(Code)
        {
            case KeyCode_Space: Result = &Input->SpaceButton; break;
            case KeyCode_Shift: Result = &Input->ShiftButton; break;
            case KeyCode_Alt: Result = &Input->AltButton; break;
            case KeyCode_Caps: Result = &Input->CapsButton; break;
            case KeyCode_RightMouse: Result = &Input->RightButton; break;
            case KeyCode_MiddleMouse: Result = &Input->MidButton; break;
            case KeyCode_Mouse4: Result = &Input->xButton1; break;
            case KeyCode_Mouse5: Result = &Input->xButton2; break;
            case KeyCode_Up: Result = &Input->ArrowUp; break;
            case KeyCode_Down: Result = &Input->ArrowDown; break;
            case KeyCode_Left: Result = &Input->ArrowLeft; break;
            case KeyCode_Right: Result = &Input->ArrowRight; break;
        }
    }
    return Result;
}

// NOTE(zoubir): the names after the letters and digits, in key_code order
global_variable char *KeyCodeNames[] =
{
    "Space", "Shift", "Alt", "Caps", "RMB", "MMB", "Mouse4", "Mouse5",
    "Up", "Down", "Left", "Right",
};

// NOTE(zoubir): what the HUD and keys.txt call Code, a string that lives
// for the whole run; "" for none
internal char *
KeyCodeName(u32 Code)
{
    static char Short[36][2];
    char *Result = (char *)"";
    if (Code >= KeyCode_A && Code < KeyCode_Space)
    {
        u32 Index = Code - KeyCode_A;
        Short[Index][0] = (Code < KeyCode_0) ? (char)('A' + Index) : (char)('0' + (Code - KeyCode_0));
        Short[Index][1] = 0;
        Result = Short[Index];
    }
    else if (Code >= KeyCode_Space && Code < KeyCode_Count)
    {
        Result = KeyCodeNames[Code - KeyCode_Space];
    }
    return Result;
}

// NOTE(zoubir): the code KeyCodeName gives Name, ignoring case;
// KeyCode_None for a name it never gives
internal key_code
KeyCodeFromName(char *Name)
{
    key_code Result = KeyCode_None;
    for(u32 Code = KeyCode_A; Code < KeyCode_Count; Code++)
    {
        if (Name[0] && StringsMatchIgnoringCase(Name, KeyCodeName(Code)))
        {
            Result = (key_code)Code;
        }
    }
    return Result;
}

// NOTE(zoubir): the keys the game reads for itself, which an action
// cannot take: H holds the controls panel, N opens the talents, J plays
// the battle theme
inline bool32
KeyCodeReserved(u32 Code)
{
    bool32 Result = Code == (u32)LetterKeyCode('H') || Code == (u32)LetterKeyCode('N') ||
        Code == (u32)LetterKeyCode('J');
    return Result;
}

// NOTE(zoubir): the first key pressed this frame that an action can be
// put on, reserved ones too (the caller says why it refuses them);
// KeyCode_None when there is none
internal key_code
FirstPressedKeyCode(app_input *Input)
{
    key_code Result = KeyCode_None;
    for(u32 Code = KeyCode_A; Code < KeyCode_Count && !Result; Code++)
    {
        if (KeyCodeState(Input, Code)->Pressed)
        {
            Result = (key_code)Code;
        }
    }
    return Result;
}
