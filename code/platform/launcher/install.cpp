/* Installing a published build. Each version gets its own folder, filled
   in versions\<v>.partial and renamed when every file has been checked,
   so the game in the old folder keeps running while the new one
   downloads, and a cut connection never leaves a broken build. Files
   that did not change are copied from the installed version instead of
   downloaded again. */

global_variable char ManifestTextScratch[64*1024];
global_variable manifest InstalledScratch;

internal void
LauncherSetStatus(launcher_status *Status, wchar_t *Text, u64 Total, bool32 Show)
{
    EnterCriticalSection(&Status->Lock);
    wcsncpy_s(Status->Text, ArrayCount(Status->Text), Text, _TRUNCATE);
    Status->Done = 0;
    Status->Total = Total;
    Status->ShowWindow = Show;
    LeaveCriticalSection(&Status->Lock);
}

// NOTE(zoubir): whether versions\<v> holds a complete install of Version
internal bool32
IsVersionInstalled(launcher_paths *Paths, char *Version)
{
    wchar_t Folder[LAUNCHER_PATH_COUNT];
    wchar_t Path[LAUNCHER_PATH_COUNT];
    VersionFolder(Paths, Version, Folder);
    JoinPath(Folder, "manifest.txt", Path);
    return ReadManifestFile(Path, ManifestTextScratch, sizeof(ManifestTextScratch),
                            &InstalledScratch) &&
        strcmp(InstalledScratch.Version, Version) == 0;
}

// NOTE(zoubir): puts one file of the new version at Target, from the old
// version's folder when it has the same bytes, else from the server
internal bool32
InstallOneFile(launcher_paths *Paths, manifest_file *File, wchar_t *Target,
               manifest *Old, wchar_t *OldFolder, launcher_status *Status)
{
    manifest_file *Same = Old ? FindManifestFile(Old, File->Path) : 0;
    if (Same && strcmp(Same->Hash, File->Hash) == 0)
    {
        wchar_t Source[LAUNCHER_PATH_COUNT];
        JoinPath(OldFolder, File->Path, Source);
        char Hash[LAUNCHER_HASH_SIZE];
        u64 Size = 0;
        if (CopyFileW(Source, Target, FALSE) && Sha256File(Target, Hash, &Size) &&
            strcmp(Hash, File->Hash) == 0)
        {
            LauncherAddProgress(Status, File->Size);
            return true;
        }
    }
    for(u32 Try = 0; Try < 3; ++Try)
    {
        EnterCriticalSection(&Status->Lock);
        u64 DoneBefore = Status->Done;
        LeaveCriticalSection(&Status->Lock);
        if (HttpDownloadFile(Paths, File->Hash, File->Size, Target, Status))
        {
            return true;
        }
        // NOTE(zoubir): take back the progress of the failed try
        EnterCriticalSection(&Status->Lock);
        Status->Done = DoneBefore;
        LeaveCriticalSection(&Status->Lock);
        Sleep(1000);
    }
    return false;
}

// NOTE(zoubir): installs New (whose text is ManifestText) next to the
// installed version OldVersion ("" for none) and makes it current.
// Shows progress when Show is set. False leaves the old version current
internal bool32
InstallVersion(launcher_paths *Paths, manifest *New, char *ManifestText,
               char *OldVersion, launcher_status *Status, bool32 Show)
{
    wchar_t Final[LAUNCHER_PATH_COUNT];
    VersionFolder(Paths, New->Version, Final);
    if (IsVersionInstalled(Paths, New->Version))
    {
        return WriteCurrentVersion(Paths, New->Version);
    }

    u64 Total = 0;
    for(u32 Index = 0; Index < New->FileCount; ++Index)
    {
        Total += New->Files[Index].Size;
    }
    LauncherSetStatus(Status, OldVersion[0] ? L"Downloading the new version" : L"Downloading the game",
                      Total, Show);

    manifest *Old = 0;
    wchar_t OldFolder[LAUNCHER_PATH_COUNT] = {};
    if (OldVersion[0] && IsVersionInstalled(Paths, OldVersion))
    {
        Old = &InstalledScratch;
        VersionFolder(Paths, OldVersion, OldFolder);
    }

    wchar_t Partial[LAUNCHER_PATH_COUNT];
    WidePrint(Partial, LAUNCHER_PATH_COUNT, L"%s.partial", Final);
    DeleteFolder(Partial);
    wchar_t Versions[LAUNCHER_PATH_COUNT];
    WidePrint(Versions, LAUNCHER_PATH_COUNT, L"%s\\versions", Paths->Root);
    CreateDirectoryW(Paths->Root, 0);
    CreateDirectoryW(Versions, 0);
    CreateDirectoryW(Partial, 0);

    bool32 Result = true;
    for(u32 Index = 0; Index < New->FileCount && Result; ++Index)
    {
        wchar_t Target[LAUNCHER_PATH_COUNT];
        JoinPath(Partial, New->Files[Index].Path, Target);
        MakeFoldersAbove(Target);
        Result = InstallOneFile(Paths, &New->Files[Index], Target, Old, OldFolder, Status);
    }

    if (Result && Old)
    {
        // NOTE(zoubir): the server the player last picked (online_config.cpp)
        wchar_t From[LAUNCHER_PATH_COUNT];
        wchar_t To[LAUNCHER_PATH_COUNT];
        JoinPath(OldFolder, "server.txt", From);
        JoinPath(Partial, "server.txt", To);
        CopyFileW(From, To, FALSE);
        // NOTE(zoubir): and the keys they picked (client/key_bindings_file.cpp)
        JoinPath(OldFolder, "keys.txt", From);
        JoinPath(Partial, "keys.txt", To);
        CopyFileW(From, To, FALSE);
    }

    wchar_t ManifestPath[LAUNCHER_PATH_COUNT];
    JoinPath(Partial, "manifest.txt", ManifestPath);
    Result = Result && WriteWholeFile(ManifestPath, ManifestText, (u32)strlen(ManifestText));
    if (Result)
    {
        DeleteFolder(Final);
        Result = MoveFileW(Partial, Final) && WriteCurrentVersion(Paths, New->Version);
    }
    if (!Result)
    {
        DeleteFolder(Partial);
    }
    return Result;
}

// NOTE(zoubir): starts Exe with LauncherArgs and Extra after it
internal bool32
StartLauncher(wchar_t *Exe, char *Extra)
{
    wchar_t CommandLine[LAUNCHER_PATH_COUNT*2];
    WidePrint(CommandLine, ArrayCount(CommandLine), L"\"%s\" %S %S", Exe, LauncherArgs, Extra);
    STARTUPINFOW Startup = {};
    Startup.cb = sizeof(Startup);
    PROCESS_INFORMATION Process = {};
    if (!CreateProcessW(Exe, CommandLine, 0, 0, FALSE, 0, 0, 0, &Startup, &Process))
    {
        return false;
    }
    CloseHandle(Process.hThread);
    CloseHandle(Process.hProcess);
    return true;
}

// NOTE(zoubir): replaces the launcher with the published one when they
// differ and starts it. True means the new launcher is running and this
// one should quit
internal bool32
UpdateLauncher(launcher_paths *Paths, manifest *Manifest, launcher_status *Status)
{
    if (_wcsicmp(Paths->Running, Paths->Launcher) != 0)
    {
        // NOTE(zoubir): running from outside the install folder; the
        // installed copy is the one that updates itself
        return false;
    }
    char Hash[LAUNCHER_HASH_SIZE];
    u64 Size;
    if (!Sha256File(Paths->Running, Hash, &Size) || strcmp(Hash, Manifest->LauncherHash) == 0)
    {
        return false;
    }

    LauncherSetStatus(Status, L"Updating the launcher", Manifest->LauncherSize, true);
    wchar_t New[LAUNCHER_PATH_COUNT];
    wchar_t Old[LAUNCHER_PATH_COUNT];
    WidePrint(New, LAUNCHER_PATH_COUNT, L"%s\\GameDemo.new.exe", Paths->Root);
    WidePrint(Old, LAUNCHER_PATH_COUNT, L"%s\\GameDemo.old.exe", Paths->Root);
    if (!HttpDownloadFile(Paths, Manifest->LauncherHash, Manifest->LauncherSize, New, Status))
    {
        return false;
    }
    // NOTE(zoubir): Windows lets a running exe be renamed, not replaced
    if (!MoveFileExW(Paths->Launcher, Old, MOVEFILE_REPLACE_EXISTING))
    {
        DeleteFileW(New);
        return false;
    }
    if (!MoveFileExW(New, Paths->Launcher, MOVEFILE_REPLACE_EXISTING))
    {
        MoveFileExW(Old, Paths->Launcher, MOVEFILE_REPLACE_EXISTING);
        return false;
    }
    return StartLauncher(Paths->Launcher, "--updated");
}
