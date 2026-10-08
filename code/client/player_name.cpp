/* Player names: what counts as a name worth joining a server with, and a
   ready-made one to offer a player who has none yet. The connect screen
   (ui/connect_screen.cpp, ui/name_prompt.cpp) checks a name with
   PlayerNameProblem before it connects; the server still accepts any
   name, and gives a taken one a " 2" (server/sim_game.cpp). */

#define PLAYER_NAME_MIN_LENGTH 2

// NOTE(zoubir): Out is In without spaces at either end and with runs of
// spaces inside cut to one, so "  Ash   Fox " and "Ash Fox" are one name
internal void
CleanPlayerName(char *Out, u32 OutSize, char *In)
{
    u32 Length = 0;
    bool32 Space = false;
    for(; *In && Length + 1 < OutSize; In++)
    {
        if (*In == ' ' || *In == '\t')
        {
            Space = (Length > 0);
            continue;
        }
        if (Space && Length + 2 < OutSize)
        {
            Out[Length++] = ' ';
        }
        Space = false;
        Out[Length++] = *In;
    }
    Out[Length] = 0;
}

// NOTE(zoubir): why Name (already cleaned) cannot be used, in a few words
// for the screen, or 0 when it can. "Player 3" is what players with no
// name are called, so a player taking it would be mistaken for one
internal char *
PlayerNameProblem(char *Name)
{
    char *Result = 0;
    u32 Length = (u32)strlen(Name);
    bool32 LooksNameless = (strncmp(Name, "Player ", 7) == 0 &&
                            Name[7] >= '0' && Name[7] <= '9');
    if (Length == 0)
    {
        Result = (char *)"Type a name to play online";
    }
    else if (Length < PLAYER_NAME_MIN_LENGTH)
    {
        Result = (char *)"At least 2 characters";
    }
    else if (LooksNameless)
    {
        Result = (char *)"Reserved for players with no name";
    }
    return Result;
}

// NOTE(zoubir): two lists whose longest pair still fits NET_NAME_SIZE
global_variable char *NameFirstWords[] =
{
    "Ash", "Ember", "Iron", "Grim", "Swift", "Storm", "Frost", "Shadow",
    "Crimson", "Silent", "Wild", "Bold", "Rune", "Stone", "Night", "Gold",
};
global_variable char *NameSecondWords[] =
{
    "Fox", "Wolf", "Raven", "Blade", "Hawk", "Bear", "Viper", "Lynx",
    "Owl", "Stag", "Moth", "Toad", "Crow", "Boar", "Pike", "Wyrm",
};

// NOTE(zoubir): a name like "Ember Lynx" picked by Seed, never Avoid (the
// one on screen now, so Shuffle always shows a new one)
internal void
SuggestPlayerName(char *Out, u32 OutSize, u32 Seed, char *Avoid = 0)
{
    for(u32 Try = 0; Try < 8; Try++)
    {
        u32 Mixed = (Seed + Try * 0x9E3779B9u) * 2654435761u;
        Mixed ^= Mixed >> 15;
        char *First = NameFirstWords[Mixed % ArrayCount(NameFirstWords)];
        char *Second = NameSecondWords[(Mixed >> 8) % ArrayCount(NameSecondWords)];
        snprintf(Out, OutSize, "%s %s", First, Second);
        if (!Avoid || strcmp(Out, Avoid) != 0)
        {
            break;
        }
    }
}
