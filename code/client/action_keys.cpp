/* Action keys: which key or mouse button drives each player action, in
   one table. Offline play reads new presses from it (keyboard_input.cpp),
   online play sends the held keys as network buttons (online.cpp), which
   are the same bits moved up by PLAYER_BUTTON_NET_SHIFT. A new action is
   one row in each table. Letter keys named as on AZERTY go through LayoutKey
   (keyboard_layout.cpp) so they move on QWERTY. The mouse-moves scheme
   (control_scheme.cpp) has its own table, with the spells on the keys
   movement leaves free. Every ability has a key; one the duel rules leave out
   (GameRules, sim/player_stats.cpp) does nothing until the talent tree
   unlocks it (sim/progression/talents.cpp). */

struct action_key
{
    app_button_state *Key;
    u32 Button;
    // NOTE(zoubir): what the HUD writes under the button's cooldown bar
    char *Label;
};

#define ACTION_KEY_COUNT 12

internal void
GetActionKeys(app_input *Input, action_key *Keys)
{
    action_key Table[ACTION_KEY_COUNT] =
    {
        {&Input->SpaceButton, PlayerButton_Jump, "Space"},
        // NOTE(zoubir): the fireball, X on both layouts; the left click
        // only confirms an aimed ability (cast_targeting.cpp)
        {&Input->ButtonX, PlayerButton_Cast, "X"},
        {&Input->ButtonE, PlayerButton_Shield, "E"},
        {&Input->ButtonF, PlayerButton_Blink, "F"},
        {LayoutKey(Input, 'A'), PlayerButton_Launch, LayoutKeyName('A')},
        // NOTE(zoubir): abilities the talent tree unlocks
        // (sim/progression/talents.cpp); until then the key does nothing
        {&Input->RightButton, PlayerButton_Attack, "RMB"},
        {LayoutKey(Input, 'W'), PlayerButton_Shockwave, LayoutKeyName('W')},
        {&Input->ButtonR, PlayerButton_Push, "R"},
        {&Input->ButtonC, PlayerButton_Slam, "C"},
        {&Input->ButtonG, PlayerButton_FrostNova, "G"},
        {&Input->ButtonT, PlayerButton_GravityWell, "T"},
        // NOTE(zoubir): the kunai (sim/player_abilities/kunai.cpp), V on
        // both layouts
        {&Input->ButtonV, PlayerButton_Kunai, "V"},
    };
    // NOTE(zoubir): mouse moves: the spells on the keys under the left
    // hand, the top row first (A Z E R on AZERTY, Q W E R on QWERTY)
    action_key MouseTable[ACTION_KEY_COUNT] =
    {
        {&Input->SpaceButton, PlayerButton_Jump, "Space"},
        {LayoutKey(Input, 'Z'), PlayerButton_Cast, LayoutKeyName('Z')},
        {&Input->ButtonE, PlayerButton_Shield, "E"},
        {&Input->ButtonF, PlayerButton_Blink, "F"},
        {LayoutKey(Input, 'A'), PlayerButton_Launch, LayoutKeyName('A')},
        {&Input->RightButton, PlayerButton_Attack, "RMB"},
        {LayoutKey(Input, 'Q'), PlayerButton_Shockwave, LayoutKeyName('Q')},
        {&Input->ButtonR, PlayerButton_Push, "R"},
        {&Input->ButtonS, PlayerButton_Slam, "S"},
        {&Input->ButtonG, PlayerButton_FrostNova, "G"},
        {&Input->ButtonT, PlayerButton_GravityWell, "T"},
        {&Input->ButtonD, PlayerButton_Kunai, "D"},
    };
    action_key *From = MouseMoves() ? MouseTable : Table;
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        Keys[Index] = From[Index];
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
