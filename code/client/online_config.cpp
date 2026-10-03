/* Online settings: the server address and player name, from the
   GAME_SERVER and GAME_NAME environment variables or else the first two
   lines of server.txt in the folder the game runs from. A connect from
   the connect screen writes server.txt so the next launch reuses it.
   With neither, the game joins ONLINE_DEFAULT_SERVER, the live server
   (deploy/README.md); GAME_SERVER=offline keeps it offline.
   online.cpp uses ReadOnlineConfig and SaveOnlineConfig. */

#define ONLINE_ADDRESS_FILE "server.txt"
#define ONLINE_ADDRESS_ENV "GAME_SERVER"
#define ONLINE_NAME_ENV "GAME_NAME"
#define ONLINE_DEFAULT_SERVER "152.53.147.77:27015"
#define ONLINE_OFFLINE_WORD "offline"

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
        fprintf(Handle, "%s\n%s\n", Address, Name);
        fclose(Handle);
    }
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#else
internal void SaveOnlineConfig(char *Address, char *Name) {}
#endif
