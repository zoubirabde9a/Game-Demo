/* Action keys: which key or mouse button drives each player action, in
   one table. Offline play reads new presses from it (keyboard_input.cpp),
   online play sends the held keys as network buttons (online.cpp), which
   are the same bits moved up by PLAYER_BUTTON_NET_SHIFT. A new action is
   one row in BindingDefs, before the four movement rows.

   Each row has a default key for each control scheme (control_scheme.cpp):
   the mouse-moves scheme puts the spells on the keys movement leaves free.
   Default letters are named as on AZERTY and go through LayoutLetter
   (keyboard_layout.cpp), so they move on QWERTY. A player can put any
   action on another key, separately for duels and dungeon runs and for
   each scheme (key_bindings.cpp, the screen in ui/key_bindings_menu.cpp);
   a key picked that way stays the same key on either layout. In a dungeon
   run the class's spells are on the same buttons (RoleKeys,
   sim/dungeon/role_abilities.cpp), so their keys move the same way. Every
   ability has a key; one the duel rules leave out (GameRules,
   sim/player_stats.cpp) does nothing until the talent tree unlocks it
   (sim/progression/talents.cpp). */

struct action_key
{
    app_button_state *Key;
    u32 Button;
    // NOTE(zoubir): what the HUD writes under the button's cooldown bar
    char *Label;
};

struct binding_def
{
    // NOTE(zoubir): the player_button, 0 for the movement rows
    u32 Button;
    u8 KeysMove;
    u8 MouseMoves;
    // NOTE(zoubir): the row's word in keys.txt and its name on the screen
    char *Word;
    char *Name;
};

#define ACTION_KEY_COUNT 12
// NOTE(zoubir): the movement rows come after the actions, in this order
#define BINDING_MOVE_UP (ACTION_KEY_COUNT + 0)
#define BINDING_MOVE_DOWN (ACTION_KEY_COUNT + 1)
#define BINDING_MOVE_LEFT (ACTION_KEY_COUNT + 2)
#define BINDING_MOVE_RIGHT (ACTION_KEY_COUNT + 3)
#define BINDING_COUNT (ACTION_KEY_COUNT + 4)
#define KEY_LETTER(Letter) (u8)(KeyCode_A + ((Letter) - 'A'))

global_variable binding_def BindingDefs[BINDING_COUNT] =
{
    {PlayerButton_Jump, KeyCode_Space, KeyCode_Space, "jump", "Jump"},
    // NOTE(zoubir): the fireball, X on both layouts; the left click only
    // confirms an aimed ability (cast_targeting.cpp)
    {PlayerButton_Cast, KEY_LETTER('X'), KEY_LETTER('Z'), "fireball", "Fireball"},
    {PlayerButton_Shield, KEY_LETTER('E'), KEY_LETTER('E'), "shield", "Shield"},
    {PlayerButton_Blink, KEY_LETTER('F'), KEY_LETTER('F'), "blink", "Blink"},
    {PlayerButton_Launch, KEY_LETTER('A'), KEY_LETTER('A'), "launch", "Launch"},
    // NOTE(zoubir): abilities the talent tree unlocks
    // (sim/progression/talents.cpp); until then the key does nothing
    {PlayerButton_Attack, KeyCode_RightMouse, KeyCode_RightMouse, "sword", "Sword"},
    {PlayerButton_Shockwave, KEY_LETTER('W'), KEY_LETTER('Q'), "shockwave", "Shockwave"},
    {PlayerButton_Push, KEY_LETTER('R'), KEY_LETTER('R'), "push", "Push"},
    {PlayerButton_Slam, KEY_LETTER('C'), KEY_LETTER('S'), "slam", "Slam"},
    {PlayerButton_FrostNova, KEY_LETTER('G'), KEY_LETTER('G'), "frostnova", "Frost Nova"},
    {PlayerButton_GravityWell, KEY_LETTER('T'), KEY_LETTER('T'), "gravitywell", "Gravity Well"},
    // NOTE(zoubir): the kunai (sim/player_abilities/kunai.cpp), V on both
    // layouts
    {PlayerButton_Kunai, KEY_LETTER('V'), KEY_LETTER('D'), "kunai", "Kunai"},
    // NOTE(zoubir): ZQSD on AZERTY, WASD on QWERTY; none when the mouse
    // moves (click_move.cpp)
    {0, KEY_LETTER('Z'), KeyCode_None, "up", "Move up"},
    {0, KEY_LETTER('S'), KeyCode_None, "down", "Move down"},
    {0, KEY_LETTER('Q'), KeyCode_None, "left", "Move left"},
    {0, KEY_LETTER('D'), KeyCode_None, "right", "Move right"},
};

// NOTE(zoubir): which set of keys is in use: a dungeon run has its own,
// so a player can put the class spells on other keys than the duel's
enum binding_mode
{
    BindingMode_Duel,
    BindingMode_Dungeon,
    BindingMode_Count,
};

// NOTE(zoubir): the keys the player picked, KeyCode_None where a row
// keeps its default; indexed by binding_mode, control_scheme and row
struct key_bindings
{
    u8 Keys[BindingMode_Count][2][BINDING_COUNT];
    // NOTE(zoubir): the binding_mode being played now
    u32 Mode;
};

// NOTE(zoubir): the player's keys, which live in AppState (KeyBindings)
// and are pointed at again each frame, like GlobalKeyboardLayout; null
// (the tests) plays every default
global_variable key_bindings *GlobalKeyBindings;

// NOTE(zoubir): Row's key when the player picked none
internal u32
DefaultKeyCode(u32 Scheme, u32 Row)
{
    u32 Result = (Scheme == ControlScheme_Mouse) ? BindingDefs[Row].MouseMoves :
        BindingDefs[Row].KeysMove;
    if (Result >= KeyCode_A && Result < KeyCode_0)
    {
        Result = LayoutKeyCode((char)('A' + (Result - KeyCode_A)));
    }
    return Result;
}

// NOTE(zoubir): Row's key in Mode and Scheme
internal u32
BoundKeyCodeIn(u32 Mode, u32 Scheme, u32 Row)
{
    u32 Result = GlobalKeyBindings ? GlobalKeyBindings->Keys[Mode][Scheme][Row] : 0;
    if (Result == KeyCode_None)
    {
        Result = DefaultKeyCode(Scheme, Row);
    }
    return Result;
}

inline u32
BindingModeNow()
{
    u32 Result = GlobalKeyBindings ? GlobalKeyBindings->Mode : BindingMode_Duel;
    return Result;
}

// NOTE(zoubir): Row's key now
internal u32
BoundKeyCode(u32 Row)
{
    u32 Result = BoundKeyCodeIn(BindingModeNow(), GlobalControlScheme, Row);
    return Result;
}

// NOTE(zoubir): whether a movement row's key is down; never when the
// mouse moves. Scheme picks which scheme's keys (the free camera uses the
// keys-move ones either way, free_camera.cpp)
internal bool32
MoveKeyDown(app_input *Input, u32 Row, u32 Scheme = GlobalControlScheme)
{
    u32 Code = BoundKeyCodeIn(BindingModeNow(), Scheme, Row);
    bool32 Result = Code != KeyCode_None && KeyCodeState(Input, Code)->EndedDown;
    return Result;
}

internal void
GetActionKeys(app_input *Input, action_key *Keys)
{
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        u32 Code = BoundKeyCode(Index);
        Keys[Index].Key = KeyCodeState(Input, Code);
        Keys[Index].Button = BindingDefs[Index].Button;
        Keys[Index].Label = KeyCodeName(Code);
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
