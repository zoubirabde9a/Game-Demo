/* The launcher (GameDemo.exe): the one file players download. It installs
   the game from the live server into %LOCALAPPDATA%\GameDemo, starts it,
   and keeps it on the build the server hosts: when we publish a new one
   (deploy/publish_client.sh), the running game closes and reopens on it
   within about 10 seconds, like a web page refreshing.

     GameDemo.exe                      the normal start
     GameDemo.exe --url http://host/   another update server (testing)
     GameDemo.exe --dir C:\some\folder another install folder (testing)

   The parts, in the order they depend on each other, are in launcher/;
   the update thread's steps are at the top of launcher/updater.cpp.
   Built by build.bat as launcher.exe; packaging renames it. */

#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <stdarg.h>
#include "../app_defs.h"

#include "launcher/launcher.h"
#include "launcher/sha256.cpp"
#include "launcher/http.cpp"
#include "launcher/manifest.cpp"
#include "launcher/folders.cpp"
#include "launcher/install.cpp"
#include "launcher/first_run.cpp"
#include "launcher/game_process.cpp"
#include "launcher/updater.cpp"
#include "launcher/window.cpp"

// NOTE(zoubir): the value after Name on the command line, or 0. Changes
// the command line in place
internal char *
CommandLineValue(char *CommandLine, char *Name)
{
    char *At = strstr(CommandLine, Name);
    if (!At)
    {
        return 0;
    }
    At += strlen(Name);
    while (*At == ' ')
    {
        ++At;
    }
    char End = ' ';
    if (*At == '"')
    {
        End = '"';
        ++At;
    }
    char *Value = At;
    while (*At && *At != End)
    {
        ++At;
    }
    *At = 0;
    return Value;
}

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    // NOTE(zoubir): kept whole to pass on when the launcher restarts itself
    static char Args[LAUNCHER_PATH_COUNT];
    static char Scratch[LAUNCHER_PATH_COUNT];
    bool32 JustUpdated = strstr(CommandLine, "--updated") != 0;
    char *Url = 0;
    char *Dir = 0;
    strncpy_s(Scratch, sizeof(Scratch), CommandLine, _TRUNCATE);
    char *UrlValue = CommandLineValue(Scratch, "--url");
    if (UrlValue)
    {
        static char UrlCopy[LAUNCHER_PATH_COUNT];
        strncpy_s(UrlCopy, sizeof(UrlCopy), UrlValue, _TRUNCATE);
        Url = UrlCopy;
    }
    strncpy_s(Scratch, sizeof(Scratch), CommandLine, _TRUNCATE);
    char *DirValue = CommandLineValue(Scratch, "--dir");
    if (DirValue)
    {
        static char DirCopy[LAUNCHER_PATH_COUNT];
        strncpy_s(DirCopy, sizeof(DirCopy), DirValue, _TRUNCATE);
        Dir = DirCopy;
    }
    _snprintf_s(Args, sizeof(Args), _TRUNCATE, "%s%s%s%s%s%s",
                Url ? "--url \"" : "", Url ? Url : "", Url ? "\" " : "",
                Dir ? "--dir \"" : "", Dir ? Dir : "", Dir ? "\"" : "");
    LauncherArgs = Args;

    static launcher_paths Paths;
    LauncherFindPaths(&Paths, Dir, Url);

    // NOTE(zoubir): one launcher at a time. A launcher that just replaced
    // itself starts the new one before quitting, so wait a little for it
    HANDLE Lock = CreateMutexW(0, FALSE, L"GameDemoLauncher");
    DWORD Wait = WaitForSingleObject(Lock, 5000);
    if (Wait != WAIT_OBJECT_0 && Wait != WAIT_ABANDONED)
    {
        return 0;
    }
    if (MoveIntoInstallFolder(&Paths, Dir == 0))
    {
        return 0;
    }

    static launcher_status Status;
    InitializeCriticalSection(&Status.Lock);
    HWND Window = CreateLauncherWindow(Instance, &Status);
    if (!Window)
    {
        return 1;
    }

    static updater Updater;
    Updater.Paths = &Paths;
    Updater.Status = &Status;
    Updater.Window = Window;
    Updater.JustUpdated = JustUpdated;
    HANDLE Thread = CreateThread(0, 0, UpdaterThread, &Updater, 0, 0);
    if (!Thread)
    {
        return 1;
    }

    MSG Message;
    while (GetMessageW(&Message, 0, 0, 0) > 0)
    {
        TranslateMessage(&Message);
        DispatchMessageW(&Message);
    }
    return 0;
}
