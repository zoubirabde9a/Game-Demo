/* Reads the manifest a release publishes (deploy/publish_client.sh
   writes it). One line per fact:

     game-demo-manifest 1
     version 20261006-153012-abc1234
     launcher <sha256> <size>
     file <sha256> <size> shaders/fx/glow.frag
     end

   "end" proves the download was not cut short. Paths must stay inside
   the version folder, so a bad manifest cannot write anywhere else. */

internal bool32
ManifestIsHash(char *Text)
{
    u32 Length = 0;
    for(; Text[Length]; ++Length)
    {
        char C = Text[Length];
        if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f')))
        {
            return false;
        }
    }
    return Length == 64;
}

// NOTE(zoubir): relative, plain ASCII, no "..", no drive or colon
internal bool32
ManifestIsSafePath(char *Path)
{
    if (!Path[0] || Path[0] == '/' || Path[0] == '\\' || strstr(Path, ".."))
    {
        return false;
    }
    for(char *C = Path; *C; ++C)
    {
        if (*C < 32 || *C > 126 || *C == ':' || *C == '\\')
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): the version name becomes a folder name
internal bool32
ManifestIsSafeName(char *Name)
{
    if (!Name[0])
    {
        return false;
    }
    for(char *C = Name; *C; ++C)
    {
        bool32 Ok = (*C >= '0' && *C <= '9') || (*C >= 'a' && *C <= 'z') ||
            (*C >= 'A' && *C <= 'Z') || *C == '-' || *C == '_' || *C == '.';
        if (!Ok)
        {
            return false;
        }
    }
    return strcmp(Name, ".") != 0 && strcmp(Name, "..") != 0;
}

// NOTE(zoubir): the next line of Text as words split on spaces; changes
// Text in place. Returns the word count, 0 at the end of Text
internal u32
ManifestNextLine(char **Text, char **Words, u32 MaxWords)
{
    char *At = *Text;
    while (*At == '\r' || *At == '\n')
    {
        ++At;
    }
    if (!*At)
    {
        *Text = At;
        return 0;
    }
    u32 Count = 0;
    while (*At && *At != '\n' && *At != '\r')
    {
        while (*At == ' ')
        {
            *At++ = 0;
        }
        if (*At && *At != '\n' && *At != '\r')
        {
            if (Count < MaxWords)
            {
                Words[Count] = At;
            }
            ++Count;
            while (*At && *At != ' ' && *At != '\n' && *At != '\r')
            {
                ++At;
            }
        }
    }
    if (*At)
    {
        *At++ = 0;
    }
    *Text = At;
    return Count;
}

internal void
ManifestCopy(char *To, char *From, u32 Size)
{
    strncpy_s(To, Size, From, _TRUNCATE);
}

// NOTE(zoubir): parses Text (changing it). False unless it is complete,
// names the game's exe, and every hash, size and path is well formed
internal bool32
ParseManifest(char *Text, manifest *Manifest)
{
    *Manifest = {};
    char *Words[5];
    if (ManifestNextLine(&Text, Words, 5) != 2 ||
        strcmp(Words[0], "game-demo-manifest") != 0 || strcmp(Words[1], "1") != 0)
    {
        return false;
    }

    bool32 Ended = false;
    bool32 HasGame = false;
    u32 Count;
    while (!Ended && (Count = ManifestNextLine(&Text, Words, 5)) != 0)
    {
        if (Count == 2 && strcmp(Words[0], "version") == 0 && ManifestIsSafeName(Words[1]))
        {
            ManifestCopy(Manifest->Version, Words[1], sizeof(Manifest->Version));
        }
        else if (Count == 3 && strcmp(Words[0], "launcher") == 0 && ManifestIsHash(Words[1]))
        {
            ManifestCopy(Manifest->LauncherHash, Words[1], sizeof(Manifest->LauncherHash));
            Manifest->LauncherSize = _strtoui64(Words[2], 0, 10);
        }
        else if (Count == 4 && strcmp(Words[0], "file") == 0 && ManifestIsHash(Words[1]) &&
                 ManifestIsSafePath(Words[3]) && Manifest->FileCount < LAUNCHER_MAX_FILES &&
                 strlen(Words[3]) < LAUNCHER_NAME_SIZE)
        {
            manifest_file *File = &Manifest->Files[Manifest->FileCount++];
            ManifestCopy(File->Hash, Words[1], sizeof(File->Hash));
            File->Size = _strtoui64(Words[2], 0, 10);
            ManifestCopy(File->Path, Words[3], sizeof(File->Path));
            if (strcmp(File->Path, "win32_app.exe") == 0)
            {
                HasGame = true;
            }
        }
        else if (Count == 1 && strcmp(Words[0], "end") == 0)
        {
            Ended = true;
        }
        else
        {
            return false;
        }
    }
    return Ended && HasGame && Manifest->Version[0] && Manifest->LauncherHash[0];
}

// NOTE(zoubir): the manifest's entry for Path, or 0
internal manifest_file *
FindManifestFile(manifest *Manifest, char *Path)
{
    for(u32 Index = 0; Index < Manifest->FileCount; ++Index)
    {
        if (strcmp(Manifest->Files[Index].Path, Path) == 0)
        {
            return &Manifest->Files[Index];
        }
    }
    return 0;
}

// NOTE(zoubir): reads a manifest file from disk (the copy each installed
// version keeps). Text must hold MaxSize bytes
internal bool32
ReadManifestFile(wchar_t *Path, char *Text, u32 MaxSize, manifest *Manifest)
{
    HANDLE File = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    DWORD Read = 0;
    bool32 Ok = ReadFile(File, Text, MaxSize - 1, &Read, 0);
    CloseHandle(File);
    if (!Ok)
    {
        return false;
    }
    Text[Read] = 0;
    return ParseManifest(Text, Manifest);
}
