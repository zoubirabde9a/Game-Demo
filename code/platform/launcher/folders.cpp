/* The launcher's folders on disk:

     %LOCALAPPDATA%\GameDemo\GameDemo.exe     the launcher itself
     %LOCALAPPDATA%\GameDemo\current.txt      the version to run
     %LOCALAPPDATA%\GameDemo\versions\<v>\    one complete build each

   and the small file helpers the installer uses on them. Paths are wide
   strings so a user name with accents still works. */

global_variable char *LauncherArgs = "";   // --url/--dir, passed on when it restarts itself

internal void
WidePrint(wchar_t *To, u32 Count, wchar_t *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    _vsnwprintf_s(To, Count, _TRUNCATE, Format, Args);
    va_end(Args);
}

// NOTE(zoubir): Dir overrides the install folder and Url the server,
// both for testing (--dir, --url); 0 takes the defaults
internal void
LauncherFindPaths(launcher_paths *Paths, char *Dir, char *Url)
{
    *Paths = {};
    GetModuleFileNameW(0, Paths->Running, LAUNCHER_PATH_COUNT);
    if (Dir)
    {
        wchar_t WideDir[LAUNCHER_PATH_COUNT];
        MultiByteToWideChar(CP_UTF8, 0, Dir, -1, WideDir, LAUNCHER_PATH_COUNT);
        GetFullPathNameW(WideDir, LAUNCHER_PATH_COUNT, Paths->Root, 0);
    }
    else
    {
        wchar_t AppData[LAUNCHER_PATH_COUNT] = L".";
        GetEnvironmentVariableW(L"LOCALAPPDATA", AppData, LAUNCHER_PATH_COUNT);
        WidePrint(Paths->Root, LAUNCHER_PATH_COUNT, L"%s\\GameDemo", AppData);
    }
    WidePrint(Paths->Launcher, LAUNCHER_PATH_COUNT, L"%s\\%s", Paths->Root, LAUNCHER_EXE_NAME);

    strncpy_s(Paths->BaseUrl, sizeof(Paths->BaseUrl), Url ? Url : LAUNCHER_DEFAULT_URL, _TRUNCATE);
    size_t Length = strlen(Paths->BaseUrl);
    if (Length && Paths->BaseUrl[Length - 1] != '/' && Length + 1 < sizeof(Paths->BaseUrl))
    {
        Paths->BaseUrl[Length] = '/';
        Paths->BaseUrl[Length + 1] = 0;
    }
}

internal void
VersionFolder(launcher_paths *Paths, char *Version, wchar_t *Out)
{
    WidePrint(Out, LAUNCHER_PATH_COUNT, L"%s\\versions\\%S", Paths->Root, Version);
}

// NOTE(zoubir): Folder\Path, with the manifest's forward slashes turned
// into backslashes
internal void
JoinPath(wchar_t *Folder, char *Path, wchar_t *Out)
{
    WidePrint(Out, LAUNCHER_PATH_COUNT, L"%s\\%S", Folder, Path);
    for(wchar_t *C = Out; *C; ++C)
    {
        if (*C == L'/')
        {
            *C = L'\\';
        }
    }
}

// NOTE(zoubir): makes every folder above the file Path
internal void
MakeFoldersAbove(wchar_t *Path)
{
    wchar_t Copy[LAUNCHER_PATH_COUNT];
    wcsncpy_s(Copy, LAUNCHER_PATH_COUNT, Path, _TRUNCATE);
    for(wchar_t *C = Copy + 3; *C; ++C)
    {
        if (*C == L'\\')
        {
            *C = 0;
            CreateDirectoryW(Copy, 0);
            *C = L'\\';
        }
    }
}

// NOTE(zoubir): deletes a folder and everything in it; false if anything
// is left (a file the game still has open)
internal bool32
DeleteFolder(wchar_t *Folder)
{
    wchar_t Pattern[LAUNCHER_PATH_COUNT];
    WidePrint(Pattern, LAUNCHER_PATH_COUNT, L"%s\\*", Folder);
    WIN32_FIND_DATAW Found;
    HANDLE Find = FindFirstFileW(Pattern, &Found);
    if (Find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (wcscmp(Found.cFileName, L".") == 0 || wcscmp(Found.cFileName, L"..") == 0)
            {
                continue;
            }
            wchar_t Child[LAUNCHER_PATH_COUNT];
            WidePrint(Child, LAUNCHER_PATH_COUNT, L"%s\\%s", Folder, Found.cFileName);
            if (Found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                DeleteFolder(Child);
            }
            else
            {
                SetFileAttributesW(Child, FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(Child);
            }
        } while (FindNextFileW(Find, &Found));
        FindClose(Find);
    }
    return RemoveDirectoryW(Folder) || GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_PATH_NOT_FOUND;
}

internal bool32
WriteWholeFile(wchar_t *Path, void *Data, u32 Size)
{
    HANDLE File = CreateFileW(Path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    DWORD Written = 0;
    bool32 Result = WriteFile(File, Data, Size, &Written, 0) && Written == Size;
    CloseHandle(File);
    return Result;
}

// NOTE(zoubir): the version current.txt names, "" when nothing is installed
internal void
ReadCurrentVersion(launcher_paths *Paths, char *Version, u32 Size)
{
    Version[0] = 0;
    wchar_t Path[LAUNCHER_PATH_COUNT];
    WidePrint(Path, LAUNCHER_PATH_COUNT, L"%s\\current.txt", Paths->Root);
    HANDLE File = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (File != INVALID_HANDLE_VALUE)
    {
        DWORD Read = 0;
        if (ReadFile(File, Version, Size - 1, &Read, 0))
        {
            Version[Read] = 0;
            for(char *C = Version; *C; ++C)
            {
                if (*C == '\r' || *C == '\n')
                {
                    *C = 0;
                    break;
                }
            }
        }
        CloseHandle(File);
    }
    if (!ManifestIsSafeName(Version))
    {
        Version[0] = 0;
    }
}

// NOTE(zoubir): written to a temporary file and moved over the old one,
// so a crash mid-write never leaves a half-written name
internal bool32
WriteCurrentVersion(launcher_paths *Paths, char *Version)
{
    wchar_t Path[LAUNCHER_PATH_COUNT];
    wchar_t Temp[LAUNCHER_PATH_COUNT];
    WidePrint(Path, LAUNCHER_PATH_COUNT, L"%s\\current.txt", Paths->Root);
    WidePrint(Temp, LAUNCHER_PATH_COUNT, L"%s\\current.txt.new", Paths->Root);
    return WriteWholeFile(Temp, Version, (u32)strlen(Version)) &&
        MoveFileExW(Temp, Path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

// NOTE(zoubir): deletes every version folder except Keep and Running,
// and any half-finished download. A folder still in use stays until the
// next try
internal void
RemoveOldVersions(launcher_paths *Paths, char *Keep, char *Running)
{
    wchar_t Pattern[LAUNCHER_PATH_COUNT];
    WidePrint(Pattern, LAUNCHER_PATH_COUNT, L"%s\\versions\\*", Paths->Root);
    WIN32_FIND_DATAW Found;
    HANDLE Find = FindFirstFileW(Pattern, &Found);
    if (Find == INVALID_HANDLE_VALUE)
    {
        return;
    }
    wchar_t KeepWide[LAUNCHER_NAME_SIZE];
    wchar_t RunningWide[LAUNCHER_NAME_SIZE];
    WidePrint(KeepWide, LAUNCHER_NAME_SIZE, L"%S", Keep);
    WidePrint(RunningWide, LAUNCHER_NAME_SIZE, L"%S", Running);
    do
    {
        if (!(Found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            wcscmp(Found.cFileName, L".") == 0 || wcscmp(Found.cFileName, L"..") == 0 ||
            wcscmp(Found.cFileName, KeepWide) == 0 || wcscmp(Found.cFileName, RunningWide) == 0)
        {
            continue;
        }
        wchar_t Folder[LAUNCHER_PATH_COUNT];
        WidePrint(Folder, LAUNCHER_PATH_COUNT, L"%s\\versions\\%s", Paths->Root, Found.cFileName);
        DeleteFolder(Folder);
    } while (FindNextFileW(Find, &Found));
    FindClose(Find);
}
