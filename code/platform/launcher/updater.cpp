/* The update thread: what the launcher does from start to finish.

   1. Fetch the manifest. If the launcher itself changed, swap it and
      restart (once per start, so a bad publish cannot loop).
   2. Install the published version if it is not the current one. With no
      network, run the installed version anyway.
   3. Start the game and hide the window.
   4. Every 10 seconds while the game runs, fetch the manifest again. A
      new version downloads while the game keeps running; then the game
      closes, the new one starts, and the old folder goes.
   5. Quit when the player closes the game. */

#define LAUNCHER_POLL_MS 10000

global_variable char ManifestDownload[64*1024];
global_variable char ManifestText[64*1024];
global_variable manifest Published;

struct updater
{
    launcher_paths *Paths;
    launcher_status *Status;
    HWND Window;
    bool32 JustUpdated;   // started by a launcher that replaced itself
};

// NOTE(zoubir): fetches and parses <base>/manifest.txt into Published,
// keeping the text as downloaded in ManifestText
internal bool32
FetchManifest(launcher_paths *Paths)
{
    char Url[LAUNCHER_PATH_COUNT];
    _snprintf_s(Url, sizeof(Url), _TRUNCATE, "%smanifest.txt", Paths->BaseUrl);
    http_target Target = {};
    Target.Memory = ManifestDownload;
    Target.MemorySize = sizeof(ManifestDownload);
    if (!HttpGet(Url, &Target))
    {
        return false;
    }
    memcpy(ManifestText, ManifestDownload, Target.MemoryUsed + 1);
    return ParseManifest(ManifestDownload, &Published);
}

internal void
UpdaterQuit(updater *Updater)
{
    EnterCriticalSection(&Updater->Status->Lock);
    Updater->Status->Quit = true;
    LeaveCriticalSection(&Updater->Status->Lock);
    PostMessageW(Updater->Window, WM_CLOSE, 0, 0);
}

// NOTE(zoubir): steps 1 and 2: leaves the version to run in Current.
// False when this launcher handed over to a newer one
internal bool32
GetSomethingToRun(updater *Updater, char *Current, u32 CurrentSize)
{
    launcher_paths *Paths = Updater->Paths;
    launcher_status *Status = Updater->Status;
    for(;;)
    {
        bool32 HaveOne = Current[0] && IsVersionInstalled(Paths, Current);
        LauncherSetStatus(Status, L"Checking for updates", 0, true);
        if (FetchManifest(Paths))
        {
            if (!Updater->JustUpdated && UpdateLauncher(Paths, &Published, Status))
            {
                return false;
            }
            if (HaveOne && strcmp(Published.Version, Current) == 0)
            {
                return true;
            }
            if (InstallVersion(Paths, &Published, ManifestText, Current, Status, true))
            {
                strncpy_s(Current, CurrentSize, Published.Version, _TRUNCATE);
                return true;
            }
            if (HaveOne)
            {
                return true;
            }
            LauncherSetStatus(Status, L"The download failed. Trying again in a few seconds", 0, true);
        }
        else if (HaveOne)
        {
            // NOTE(zoubir): offline; the game says so itself if the
            // server runs a different build
            return true;
        }
        else
        {
            LauncherSetStatus(Status, L"Can't reach the update server. Trying again in a few seconds", 0, true);
        }
        Sleep(5000);
    }
}

internal DWORD WINAPI
UpdaterThread(void *Parameter)
{
    updater *Updater = (updater *)Parameter;
    launcher_paths *Paths = Updater->Paths;
    launcher_status *Status = Updater->Status;

    wchar_t OldLauncher[LAUNCHER_PATH_COUNT];
    WidePrint(OldLauncher, LAUNCHER_PATH_COUNT, L"%s\\GameDemo.old.exe", Paths->Root);
    DeleteFileW(OldLauncher);

    char Current[LAUNCHER_NAME_SIZE];
    ReadCurrentVersion(Paths, Current, sizeof(Current));
    if (!HttpStart() || !GetSomethingToRun(Updater, Current, sizeof(Current)))
    {
        UpdaterQuit(Updater);
        return 0;
    }
    RemoveOldVersions(Paths, Current, Current);

    PROCESS_INFORMATION Game;
    if (!StartGame(Paths, Current, &Game))
    {
        LauncherSetStatus(Status, L"The game would not start. Close this window and try again", 0, true);
        return 0;
    }
    LauncherSetStatus(Status, L"Playing", 0, false);

    for(;;)
    {
        if (WaitForSingleObject(Game.hProcess, LAUNCHER_POLL_MS) == WAIT_OBJECT_0)
        {
            CloseHandle(Game.hProcess);
            UpdaterQuit(Updater);
            return 0;
        }
        if (!FetchManifest(Paths) || strcmp(Published.Version, Current) == 0)
        {
            continue;
        }
        // NOTE(zoubir): the game keeps running while this downloads
        if (!InstallVersion(Paths, &Published, ManifestText, Current, Status, false))
        {
            LauncherSetStatus(Status, L"Playing", 0, false);
            continue;
        }
        LauncherSetStatus(Status, L"A new version is out. Restarting the game", 0, true);
        CloseGame(&Game);
        strncpy_s(Current, sizeof(Current), Published.Version, _TRUNCATE);
        RemoveOldVersions(Paths, Current, Current);
        if (!StartGame(Paths, Current, &Game))
        {
            LauncherSetStatus(Status, L"The game would not start. Close this window and try again", 0, true);
            return 0;
        }
        LauncherSetStatus(Status, L"Playing", 0, false);
    }
}
