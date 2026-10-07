/* Control scheme: how the player moves, the player's pick in the Esc
   options menu (ui/options_menu.cpp), saved on the fourth line of
   server.txt (online_config.cpp).

   Keys move (the default): ZQSD on AZERTY, WASD on QWERTY. A left click
   throws the fireball at the cursor and the right click swings the sword,
   so the hand on the mouse attacks while the other moves and casts.

   Mouse moves: a left click on the ground walks there, and holding it
   keeps walking toward the cursor (click_move.cpp). The movement keys are
   free then, so the spells move onto them: the row under the left hand
   (A Z E R on AZERTY, Q W E R on QWERTY) and the one below it. The action
   key table (action_keys.cpp) reads the scheme from the global, like the
   layout. */

enum control_scheme
{
    ControlScheme_Keys,
    ControlScheme_Mouse,
};

// NOTE(zoubir): one setting for this machine, read once at startup; a
// global for the same reason as GlobalKeyboardLayout
global_variable control_scheme GlobalControlScheme = ControlScheme_Keys;

// NOTE(zoubir): the words server.txt stores
internal char *
ControlSchemeWord(control_scheme Scheme)
{
    char *Result = (Scheme == ControlScheme_Mouse) ? (char *)"mouse" : (char *)"keys";
    return Result;
}

inline bool32
MouseMoves()
{
    bool32 Result = GlobalControlScheme == ControlScheme_Mouse;
    return Result;
}
