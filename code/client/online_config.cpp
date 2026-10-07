/* Online settings: the server address and player name, from the
   GAME_SERVER and GAME_NAME environment variables or else the first two
   lines of server.txt in the folder the game runs from. A connect from
   the connect screen writes server.txt so the next launch reuses it.
   Its third line is the keyboard layout (keyboard_layout.cpp), the fourth
   the control scheme (control_scheme.cpp) and the fifth the window,
   "fullscreen" or "window"; they are kept here because the launcher
   carries server.txt over to each new version.
   With neither, the game joins ONLINE_DEFAULT_SERVER, the first server in
   server_list.cpp; GAME_SERVER=offline keeps it offline.
   online.cpp uses ReadOnlineConfig and SaveOnlineConfig. */

#define ONLINE_ADDRESS_FILE "server.txt"
#define ONLINE_ADDRESS_ENV "GAME_SERVER"
#define ONLINE_NAME_ENV "GAME_NAME"
#define ONLINE_DEFAULT_SERVER (ServerList[0].Address)
#define ONLINE_OFFLINE_WORD "offline"

// NOTE(zoubir): the window setting, full screen or not, for the next
// launch (ui/options_menu.cpp); a global like the layout so a save from
// the connect screen keeps it
global_variable bool32 GlobalFullscreen = true;

// NOTE(zoubir): A and B hold the same letters, ignoring case
internal bool32
StringsMatchIgnoringCase(char *A, char *B)
{
    for(; *A && *B; A++, B++)
    {
        char CA = (*A >= 'A' && *A <= 'Z') ? (char)(*A + 32) : *A;
        char CB = (*B >= 'A' && *B <= 'Z') ? (char)(*B + 32) : *B;
        if (CA != CB)
        {
            return false;
        }
    }
    return *A == *B;
}

// NOTE(zoubir): copies the first line of Text, trimmed of spaces
internal void
CopyFirstLine(char *Out, u32 OutSize, char *Text)
{
    while (*Text == ' ' || *Text == '\t')
    {
        Text++;
    }
    u32 Length = 0;
    while (Text[Length] && Text[Length] != '\r' && Text[Length] != '\n' &&
           Length + 1 < OutSize)
    {
        Out[Length] = Text[Length];
        Length++;
    }
    while (Length > 0 && (Out[Length - 1] == ' ' || Out[Length - 1] == '\t'))
    {
        Length--;
    }
    Out[Length] = 0;
}

// NOTE(zoubir): environment first, then the file; false when neither is set.
// getenv and fopen are standard and portable; MSVC's "unsafe" warning on
// them does not apply to reading one short line.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
// NOTE(zoubir): the text after LineIndex line breaks, or "" past the end
internal char *
SkipLines(char *Text, u32 LineIndex)
{
    for(; LineIndex > 0 && *Text; Text++)
    {
        if (*Text == '\n')
        {
            LineIndex--;
        }
    }
    return Text;
}

// NOTE(zoubir): each setting from its environment variable, else from its
// line of server.txt. Returns false when there is no address at all.
internal bool32
ReadOnlineConfig(char *Address, u32 AddressSize, char *Name, u32 NameSize)
{
    char File[256] = {};
    FILE *Handle = fopen(ONLINE_ADDRESS_FILE, "rb");
    if (Handle)
    {
        fread(File, 1, sizeof(File) - 1, Handle);
        fclose(Handle);
    }

    char *FromEnv = getenv(ONLINE_ADDRESS_ENV);
    CopyFirstLine(Address, AddressSize,
                  (FromEnv && FromEnv[0]) ? FromEnv : File);
    FromEnv = getenv(ONLINE_NAME_ENV);
    CopyFirstLine(Name, NameSize,
                  (FromEnv && FromEnv[0]) ? FromEnv : SkipLines(File, 1));
    return Address[0] != 0;
}

// NOTE(zoubir): server.txt's line LineIndex (from 0) into Word, "" when
// there is none
internal void
ReadSavedSetting(u32 LineIndex, char *Word, u32 WordSize)
{
    char File[256] = {};
    FILE *Handle = fopen(ONLINE_ADDRESS_FILE, "rb");
    if (Handle)
    {
        fread(File, 1, sizeof(File) - 1, Handle);
        fclose(Handle);
    }
    CopyFirstLine(Word, WordSize, SkipLines(File, LineIndex));
}

// NOTE(zoubir): the layout server.txt's third line names; AZERTY when it
// names none
internal keyboard_layout
ReadSavedKeyboardLayout()
{
    char Word[16];
    ReadSavedSetting(2, Word, sizeof(Word));
    keyboard_layout Result = StringsMatchIgnoringCase(Word, (char *)"qwerty") ?
        KeyboardLayout_Qwerty : KeyboardLayout_Azerty;
    return Result;
}

// NOTE(zoubir): the scheme the fourth line names; keys move when it names
// none
internal control_scheme
ReadSavedControlScheme()
{
    char Word[16];
    ReadSavedSetting(3, Word, sizeof(Word));
    control_scheme Result = StringsMatchIgnoringCase(Word, ControlSchemeWord(ControlScheme_Mouse)) ?
        ControlScheme_Mouse : ControlScheme_Keys;
    return Result;
}

// NOTE(zoubir): the fifth line; full screen unless it says "window"
internal bool32
ReadSavedFullscreen()
{
    char Word[16];
    ReadSavedSetting(4, Word, sizeof(Word));
    bool32 Result = !StringsMatchIgnoringCase(Word, (char *)"window");
    return Result;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if !COMPILER_EMSCRIPTEN
// NOTE(zoubir): fopen and fprintf are portable; see the warning note above
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
internal void
SaveOnlineConfig(char *Address, char *Name)
{
    FILE *Handle = fopen(ONLINE_ADDRESS_FILE, "wb");
    if (Handle)
    {
        fprintf(Handle, "%s\n%s\n%s\n%s\n%s\n", Address, Name,
                KeyboardLayoutWord(GlobalKeyboardLayout),
                ControlSchemeWord(GlobalControlScheme),
                GlobalFullscreen ? "fullscreen" : "window");
        fclose(Handle);
    }
}

// NOTE(zoubir): rewrites server.txt with the layout, scheme and window in
// use, keeping the address and name lines as they were in the file
internal void
SaveLocalSettings()
{
    char File[256] = {};
    FILE *Handle = fopen(ONLINE_ADDRESS_FILE, "rb");
    if (Handle)
    {
        fread(File, 1, sizeof(File) - 1, Handle);
        fclose(Handle);
    }
    char Address[128], Name[64];
    CopyFirstLine(Address, sizeof(Address), File);
    CopyFirstLine(Name, sizeof(Name), SkipLines(File, 1));
    SaveOnlineConfig(Address, Name);
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#else
internal void SaveOnlineConfig(char *Address, char *Name) {}
internal void SaveLocalSettings() {}
#endif
