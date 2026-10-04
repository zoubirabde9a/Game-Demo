/* Action keys: which key or mouse button drives each player action, in
   one table. Offline play reads new presses from it (keyboard_input.cpp),
   online play sends the held keys as network buttons (online.cpp), which
   are the same bits moved up by PLAYER_BUTTON_NET_SHIFT. A new action is
   one row here. */

struct action_key
{
    app_button_state *Key;
    u32 Button;
};

#define ACTION_KEY_COUNT 8

internal void
GetActionKeys(app_input *Input, action_key *Keys)
{
    action_key Table[ACTION_KEY_COUNT] =
    {
        {&Input->SpaceButton, PlayerButton_Jump},
        {&Input->AltButton, PlayerButton_Dash},
        {&Input->LeftButton, PlayerButton_Cast},
        {&Input->RightButton, PlayerButton_Attack},
        {&Input->ButtonE, PlayerButton_Shockwave},
        {&Input->ButtonF, PlayerButton_Blink},
        {&Input->ButtonR, PlayerButton_Push},
        {&Input->ButtonA, PlayerButton_Launch},
    };
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        Keys[Index] = Table[Index];
    }
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
