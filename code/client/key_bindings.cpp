/* Key bindings: the player putting an action on another key. There are
   four sets of keys, one for each pair of game kind (duels, dungeon runs)
   and control scheme (keys move, mouse moves), each starting from the
   defaults in BindingDefs (action_keys.cpp). The screen that edits them is
   ui/key_bindings_menu.cpp; keys.txt keeps them for the next launch
   (key_bindings_file.cpp).

   A set never puts one key on two of its rows: picking a key another row
   has gives that row the key the first one had, so the two swap. A row
   can go back to its default (Backspace on the screen), and a whole set
   too. */

// NOTE(zoubir): whether Row means anything in Mode and Scheme: the
// movement keys only when keys move, and in a dungeon run only the
// abilities a class can have there (DUNGEON_SHARED_BUTTONS and RoleKeys,
// sim/dungeon/role_abilities.cpp)
internal bool32
BindingRowUsed(u32 Mode, u32 Scheme, u32 Row)
{
    bool32 Result = true;
    if (Row >= ACTION_KEY_COUNT)
    {
        Result = Scheme == ControlScheme_Keys;
    }
    else if (Mode == BindingMode_Dungeon)
    {
        u32 Button = BindingDefs[Row].Button;
        Result = (Button & DUNGEON_SHARED_BUTTONS) || RoleKeyForButton(Button) < ROLE_KEYS;
    }
    return Result;
}

// NOTE(zoubir): stores Code for Row, as "the default" when it is one
inline void
StoreKeyCode(key_bindings *Bindings, u32 Mode, u32 Scheme, u32 Row, u32 Code)
{
    Bindings->Keys[Mode][Scheme][Row] =
        (u8)((Code == DefaultKeyCode(Scheme, Row)) ? KeyCode_None : Code);
}

// NOTE(zoubir): puts Row on Code in that set. A row of the set that had
// Code takes Row's old key, rows the set does not use too, so a hidden
// row never keeps a key twice. Returns the row that swapped, or
// BINDING_COUNT for none
internal u32
SetKeyBinding(key_bindings *Bindings, u32 Mode, u32 Scheme, u32 Row, u32 Code)
{
    key_bindings *Saved = GlobalKeyBindings;
    GlobalKeyBindings = Bindings;
    u32 Old = BoundKeyCodeIn(Mode, Scheme, Row);
    u32 Result = BINDING_COUNT;
    for(u32 Other = 0; Other < BINDING_COUNT; Other++)
    {
        if (Other != Row && Old != Code && BoundKeyCodeIn(Mode, Scheme, Other) == Code)
        {
            StoreKeyCode(Bindings, Mode, Scheme, Other, Old);
            if (BindingRowUsed(Mode, Scheme, Other))
            {
                Result = Other;
            }
        }
    }
    StoreKeyCode(Bindings, Mode, Scheme, Row, Code);
    GlobalKeyBindings = Saved;
    return Result;
}

// NOTE(zoubir): Row back to its default key, swapping like SetKeyBinding
internal u32
ResetKeyBinding(key_bindings *Bindings, u32 Mode, u32 Scheme, u32 Row)
{
    u32 Result = SetKeyBinding(Bindings, Mode, Scheme, Row, DefaultKeyCode(Scheme, Row));
    return Result;
}

internal void
ResetKeyBindingSet(key_bindings *Bindings, u32 Mode, u32 Scheme)
{
    for(u32 Row = 0; Row < BINDING_COUNT; Row++)
    {
        Bindings->Keys[Mode][Scheme][Row] = KeyCode_None;
    }
}

// NOTE(zoubir): whether the set has any key the player picked
internal bool32
KeyBindingSetChanged(key_bindings *Bindings, u32 Mode, u32 Scheme)
{
    bool32 Result = false;
    for(u32 Row = 0; Row < BINDING_COUNT; Row++)
    {
        if (Bindings->Keys[Mode][Scheme][Row] != KeyCode_None)
        {
            Result = true;
        }
    }
    return Result;
}

// NOTE(zoubir): another row the set uses is on Row's key. Swaps keep a
// set clear, but switching the keyboard layout moves the default letters
// under the keys a player picked, so the screen marks what that broke
internal bool32
KeyBindingClashes(u32 Mode, u32 Scheme, u32 Row)
{
    u32 Code = BoundKeyCodeIn(Mode, Scheme, Row);
    bool32 Result = false;
    for(u32 Other = 0; Other < BINDING_COUNT; Other++)
    {
        if (Other != Row && BindingRowUsed(Mode, Scheme, Other) &&
            BoundKeyCodeIn(Mode, Scheme, Other) == Code)
        {
            Result = true;
        }
    }
    return Result;
}

// NOTE(zoubir): whether a row of the set in play now is on Code, so a
// screen that also reads the key (the map vote's 1 and 2) leaves it alone
internal bool32
KeyCodeBoundNow(u32 Code)
{
    bool32 Result = false;
    u32 Mode = BindingModeNow();
    for(u32 Row = 0; Row < BINDING_COUNT; Row++)
    {
        if (BindingRowUsed(Mode, GlobalControlScheme, Row) && BoundKeyCode(Row) == Code)
        {
            Result = true;
        }
    }
    return Result;
}

// NOTE(zoubir): the four movement keys for text, up, left, down, right:
// "ZQSD" when each is one letter, else their names with spaces
internal void
MoveKeysText(char *Out, u32 OutSize, u32 Scheme = ControlScheme_Keys)
{
    u32 Rows[] = {BINDING_MOVE_UP, BINDING_MOVE_LEFT, BINDING_MOVE_DOWN, BINDING_MOVE_RIGHT};
    bool32 Letters = true;
    for(u32 Index = 0; Index < ArrayCount(Rows); Index++)
    {
        Letters = Letters && KeyCodeName(BoundKeyCodeIn(BindingModeNow(), Scheme, Rows[Index]))[1] == 0;
    }
    Out[0] = 0;
    for(u32 Index = 0; Index < ArrayCount(Rows); Index++)
    {
        u32 Length = (u32)strlen(Out);
        snprintf(Out + Length, OutSize - Length, "%s%s", (Letters || !Length) ? "" : " ",
                 KeyCodeName(BoundKeyCodeIn(BindingModeNow(), Scheme, Rows[Index])));
    }
}

// NOTE(zoubir): each frame, from the options menu: the global points at
// AppState's keys again (it does not survive the game code reloading)
// and says whether a dungeon run is on
internal void
SyncKeyBindings(app_state *AppState)
{
    GlobalKeyBindings = AppState->KeyBindings;
    if (GlobalKeyBindings)
    {
        GlobalKeyBindings->Mode = IsDungeon(AppState) ? BindingMode_Dungeon : BindingMode_Duel;
    }
}
