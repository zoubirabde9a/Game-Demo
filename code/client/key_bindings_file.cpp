/* Key bindings file: keys.txt, in the folder the game runs from, keeps the
   keys the player picked (key_bindings.cpp) for the next launch, and the
   launcher carries it over to each new version like server.txt. One line
   for each key that is not the default, four words:

       dungeon keys launch 1
       duel mouse sword Mouse4

   the game kind (duel or dungeon), the control scheme (keys or mouse,
   ControlSchemeWord), the row's word (BindingDefs) and the key's name
   (KeyCodeName). A line the game does not understand is skipped. The
   browser build keeps no file. */

#define KEY_BINDINGS_FILE "keys.txt"

global_variable char *BindingModeWords[BindingMode_Count] = {"duel", "dungeon"};

// NOTE(zoubir): the next space-separated word of *Text into Word, moving
// *Text past it; stops at the end of the line
internal void
NextWord(char **Text, char *Word, u32 WordSize)
{
    char *At = *Text;
    while (*At == ' ' || *At == '\t')
    {
        At++;
    }
    u32 Length = 0;
    while (*At && *At != ' ' && *At != '\t' && *At != '\r' && *At != '\n')
    {
        if (Length + 1 < WordSize)
        {
            Word[Length++] = *At;
        }
        At++;
    }
    Word[Length] = 0;
    *Text = At;
}

// NOTE(zoubir): Text is keys.txt's contents; each line it understands goes
// into Bindings, the rest stay at their defaults
internal void
ParseKeyBindings(key_bindings *Bindings, char *Text)
{
    while (*Text)
    {
        char Words[4][24];
        char *At = Text;
        for(u32 Index = 0; Index < ArrayCount(Words); Index++)
        {
            NextWord(&At, Words[Index], sizeof(Words[Index]));
        }
        u32 Mode = BindingMode_Count;
        for(u32 Each = 0; Each < BindingMode_Count; Each++)
        {
            if (StringsMatchIgnoringCase(Words[0], BindingModeWords[Each]))
            {
                Mode = Each;
            }
        }
        u32 Scheme = 2;
        if (StringsMatchIgnoringCase(Words[1], ControlSchemeWord(ControlScheme_Keys)))
        {
            Scheme = ControlScheme_Keys;
        }
        else if (StringsMatchIgnoringCase(Words[1], ControlSchemeWord(ControlScheme_Mouse)))
        {
            Scheme = ControlScheme_Mouse;
        }
        u32 Row = BINDING_COUNT;
        for(u32 Each = 0; Each < BINDING_COUNT; Each++)
        {
            if (StringsMatchIgnoringCase(Words[2], BindingDefs[Each].Word))
            {
                Row = Each;
            }
        }
        key_code Code = KeyCodeFromName(Words[3]);
        if (Mode < BindingMode_Count && Scheme < 2 && Row < BINDING_COUNT && Code &&
            !KeyCodeReserved(Code))
        {
            Bindings->Keys[Mode][Scheme][Row] = (u8)Code;
        }
        while (*At && *At != '\n')
        {
            At++;
        }
        Text = *At ? At + 1 : At;
    }
}

// NOTE(zoubir): what keys.txt holds for Bindings, into Out
internal void
FormatKeyBindings(key_bindings *Bindings, char *Out, u32 OutSize)
{
    Out[0] = 0;
    for(u32 Mode = 0; Mode < BindingMode_Count; Mode++)
    {
        for(u32 Scheme = 0; Scheme < 2; Scheme++)
        {
            for(u32 Row = 0; Row < BINDING_COUNT; Row++)
            {
                u32 Code = Bindings->Keys[Mode][Scheme][Row];
                if (Code != KeyCode_None)
                {
                    u32 Length = (u32)strlen(Out);
                    snprintf(Out + Length, OutSize - Length, "%s %s %s %s\n",
                             BindingModeWords[Mode], ControlSchemeWord((control_scheme)Scheme),
                             BindingDefs[Row].Word, KeyCodeName(Code));
                }
            }
        }
    }
}

// NOTE(zoubir): room for every row of every set on its own line
#define KEY_BINDINGS_FILE_SIZE (BindingMode_Count * 2 * BINDING_COUNT * 48)

#if !COMPILER_EMSCRIPTEN
// NOTE(zoubir): fopen is portable; MSVC's "unsafe" warning does not apply
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
internal void
LoadKeyBindings(key_bindings *Bindings)
{
    *Bindings = {};
    char File[KEY_BINDINGS_FILE_SIZE] = {};
    FILE *Handle = fopen(KEY_BINDINGS_FILE, "rb");
    if (Handle)
    {
        fread(File, 1, sizeof(File) - 1, Handle);
        fclose(Handle);
    }
    ParseKeyBindings(Bindings, File);
}

internal void
SaveKeyBindings(key_bindings *Bindings)
{
    char Text[KEY_BINDINGS_FILE_SIZE];
    FormatKeyBindings(Bindings, Text, sizeof(Text));
    FILE *Handle = fopen(KEY_BINDINGS_FILE, "wb");
    if (Handle)
    {
        fwrite(Text, 1, strlen(Text), Handle);
        fclose(Handle);
    }
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#else
internal void LoadKeyBindings(key_bindings *Bindings) { *Bindings = {}; }
internal void SaveKeyBindings(key_bindings *Bindings) {}
#endif
