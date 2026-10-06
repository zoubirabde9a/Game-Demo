/* The first run, from wherever the player saved the download: copy the
   launcher into %LOCALAPPDATA%\GameDemo, put "Game Demo" shortcuts on the
   desktop and in the Start menu, and start the installed copy. No
   administrator rights are needed, because nothing goes outside the
   player's own folders. */

// NOTE(zoubir): a shortcut to Target at Folder\Game Demo.lnk, unless one
// is there already
internal void
MakeShortcut(REFKNOWNFOLDERID FolderId, wchar_t *Target, wchar_t *WorkingFolder)
{
    wchar_t *Folder = 0;
    if (SHGetKnownFolderPath(FolderId, 0, 0, &Folder) != S_OK)
    {
        return;
    }
    wchar_t Path[LAUNCHER_PATH_COUNT];
    WidePrint(Path, LAUNCHER_PATH_COUNT, L"%s\\Game Demo.lnk", Folder);
    CoTaskMemFree(Folder);
    if (GetFileAttributesW(Path) != INVALID_FILE_ATTRIBUTES)
    {
        return;
    }

    IShellLinkW *Link = 0;
    if (CoCreateInstance(CLSID_ShellLink, 0, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                         (void **)&Link) == S_OK)
    {
        Link->SetPath(Target);
        Link->SetWorkingDirectory(WorkingFolder);
        Link->SetDescription(L"Starts Game Demo and keeps it up to date");
        IPersistFile *File = 0;
        if (Link->QueryInterface(IID_IPersistFile, (void **)&File) == S_OK)
        {
            File->Save(Path, TRUE);
            File->Release();
        }
        Link->Release();
    }
}

// NOTE(zoubir): true when this exe was started from outside the install
// folder and has handed over to the installed copy, so it should quit.
// Shortcuts only for the real install folder, not a test one (--dir)
internal bool32
MoveIntoInstallFolder(launcher_paths *Paths, bool32 MakeShortcuts)
{
    if (_wcsicmp(Paths->Running, Paths->Launcher) == 0)
    {
        return false;
    }
    CreateDirectoryW(Paths->Root, 0);
    if (!CopyFileW(Paths->Running, Paths->Launcher, FALSE))
    {
        // NOTE(zoubir): the installed copy is running (it holds the
        // single-launcher lock, so this one will wait and then quit), or
        // the folder is not writable; carry on from here either way
        return false;
    }
    // NOTE(zoubir): the copy keeps the browser's "downloaded from the
    // internet" mark, which makes Windows run its SmartScreen check (a
    // pause or a warning) on every start from the shortcut; the player
    // already accepted that warning once
    wchar_t Mark[LAUNCHER_PATH_COUNT];
    WidePrint(Mark, LAUNCHER_PATH_COUNT, L"%s:Zone.Identifier", Paths->Launcher);
    DeleteFileW(Mark);
    if (MakeShortcuts && CoInitializeEx(0, COINIT_APARTMENTTHREADED) == S_OK)
    {
        MakeShortcut(FOLDERID_Desktop, Paths->Launcher, Paths->Root);
        MakeShortcut(FOLDERID_Programs, Paths->Launcher, Paths->Root);
        CoUninitialize();
    }
    return StartLauncher(Paths->Launcher, "");
}
