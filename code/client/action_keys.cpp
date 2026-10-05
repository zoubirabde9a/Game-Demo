/* Action keys: which key or mouse button drives each player action, in
   one table. Offline play reads new presses from it (keyboard_input.cpp),
   online play sends the held keys as network buttons (online.cpp), which
   are the same bits moved up by PLAYER_BUTTON_NET_SHIFT. A new action is
   one row here. Only the abilities the duel rules play have a key
   (GameRules, sim/player_stats.cpp). */

struct action_key
{
    app_button_state *Key;
    u32 Button;
    // NOTE(zoubir): what the HUD writes under the button's cooldown bar
    char *Label;
};

#define ACTION_KEY_COUNT 7

internal void
GetActionKeys(app_input *Input, action_key *Keys)
{
    action_key Table[ACTION_KEY_COUNT] =
    {
        {&Input->SpaceButton, PlayerButton_Jump, "Space"},
        {&Input->AltButton, PlayerButton_Dash, "Alt"},
        {&Input->LeftButton, PlayerButton_Cast, "LMB"},
        {&Input->ButtonE, PlayerButton_Shield, "E"},
        {&Input->ButtonF, PlayerButton_Blink, "F"},
        {&Input->ButtonA, PlayerButton_Launch, "A"},
        {&Input->ButtonV, PlayerButton_RewindWorld, "V"},
    };
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        Keys[Index] = Table[Index];
    }
}

// NOTE(zoubir): the key's name for a player_button, "" for none
internal char *
ActionKeyLabel(u32 Button)
{
    static app_input NoInput;
    action_key Keys[ACTION_KEY_COUNT];
    GetActionKeys(&NoInput, Keys);
    char *Result = (char *)"";
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        if (Keys[Index].Button == Button)
        {
            Result = Keys[Index].Label;
        }
    }
    return Result;
}

// NOTE(zoubir): the player_button bits of the keys pressed this frame
// (Pressed) or held down
internal u32
ActionButtonsFromKeys(app_input *Input, bool32 Pressed)
{
    action_key Keys[ACTION_KEY_COUNT];
    GetActionKeys(Input, Keys);
    u32 Result = 0;
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        if (Pressed ? Keys[Index].Key->Pressed : Keys[Index].Key->EndedDown)
        {
            Result |= Keys[Index].Button;
        }
    }
    return Result;
}
