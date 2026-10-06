/* The launcher's shared types: the manifest a release publishes, the
   folders the game is installed in, and the status the update thread
   shows in the window. See launcher_app.cpp for how the parts fit. */

#if !defined(LAUNCHER_H)
#define LAUNCHER_H

#define LAUNCHER_PATH_COUNT 1024
#define LAUNCHER_MAX_FILES 256
#define LAUNCHER_NAME_SIZE 160
#define LAUNCHER_HASH_SIZE 65   // 64 hex digits of a SHA-256 and a 0

// NOTE(zoubir): what the server publishes at <base>/manifest.txt. Every
// file is fetched from <base>/files/<hash>, so a published file never
// changes and a half-finished publish never mixes two builds
struct manifest_file
{
    char Hash[LAUNCHER_HASH_SIZE];
    u64 Size;
    char Path[LAUNCHER_NAME_SIZE];   // relative, with forward slashes
};

struct manifest
{
    char Version[LAUNCHER_NAME_SIZE];
    char LauncherHash[LAUNCHER_HASH_SIZE];
    u64 LauncherSize;
    u32 FileCount;
    manifest_file Files[LAUNCHER_MAX_FILES];
};

struct launcher_paths
{
    wchar_t Root[LAUNCHER_PATH_COUNT];        // %LOCALAPPDATA%\GameDemo
    wchar_t Launcher[LAUNCHER_PATH_COUNT];    // Root\GameDemo.exe
    wchar_t Running[LAUNCHER_PATH_COUNT];     // this exe, wherever it is
    char BaseUrl[LAUNCHER_PATH_COUNT];        // ends in '/'
};

// NOTE(zoubir): written by the update thread, read by the window
struct launcher_status
{
    CRITICAL_SECTION Lock;
    wchar_t Text[256];
    u64 Done;
    u64 Total;           // 0: no progress bar
    bool32 ShowWindow;
    bool32 Quit;
};

#define LAUNCHER_EXE_NAME L"GameDemo.exe"
#define LAUNCHER_GAME_EXE L"win32_app.exe"
#define LAUNCHER_DEFAULT_URL "https://game.sindansolutions.com/"

#endif
