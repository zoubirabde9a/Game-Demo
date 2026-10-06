/* Starting the game from a version folder and closing it again when a
   newer build is ready. The game runs from its own folder because it
   loads asset_1.zas, shaders\ and fonts\ from the current folder. */

internal bool32
StartGame(launcher_paths *Paths, char *Version, PROCESS_INFORMATION *Game)
{
    wchar_t Folder[LAUNCHER_PATH_COUNT];
    wchar_t Exe[LAUNCHER_PATH_COUNT];
    VersionFolder(Paths, Version, Folder);
    WidePrint(Exe, LAUNCHER_PATH_COUNT, L"%s\\%s", Folder, LAUNCHER_GAME_EXE);
    wchar_t CommandLine[LAUNCHER_PATH_COUNT];
    WidePrint(CommandLine, LAUNCHER_PATH_COUNT, L"\"%s\"", Exe);

    STARTUPINFOW Startup = {};
    Startup.cb = sizeof(Startup);
    *Game = {};
    if (!CreateProcessW(Exe, CommandLine, 0, 0, FALSE, 0, 0, Folder, &Startup, Game))
    {
        return false;
    }
    CloseHandle(Game->hThread);
    Game->hThread = 0;
    return true;
}

internal BOOL CALLBACK
CloseWindowOfProcess(HWND Window, LPARAM ProcessId)
{
    DWORD Owner = 0;
    GetWindowThreadProcessId(Window, &Owner);
    if (Owner == (DWORD)ProcessId)
    {
        PostMessageW(Window, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

// NOTE(zoubir): asks the game to close as if the player clicked X, and
// ends it after 5 seconds if it has not
internal void
CloseGame(PROCESS_INFORMATION *Game)
{
    if (!Game->hProcess)
    {
        return;
    }
    EnumWindows(CloseWindowOfProcess, (LPARAM)Game->dwProcessId);
    if (WaitForSingleObject(Game->hProcess, 5000) != WAIT_OBJECT_0)
    {
        TerminateProcess(Game->hProcess, 1);
        WaitForSingleObject(Game->hProcess, 5000);
    }
    CloseHandle(Game->hProcess);
    *Game = {};
}
