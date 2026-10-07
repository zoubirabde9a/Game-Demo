/* Window mode: full screen or a window. The game opens full screen
   unless server.txt's fifth line says "window" (online_config.cpp); the
   Esc options menu (ui/options_menu.cpp) switches it, and so do F1, F11
   and Alt+Enter on Windows (platform/win32/frame.cpp). Whichever way it
   changes, the choice is saved for the next launch.

   The platform says each frame whether the window is full screen
   (app_window.Fullscreen) and does what FullscreenRequest asks after the
   frame. Developer runs that set GAME_SCREENSHOT or GAME_WINDOW keep the
   window they asked for and save nothing. The browser opens a page full
   screen only from a click, so there it starts in the page. */

struct window_mode
{
    bool32 Started;
    // NOTE(zoubir): GAME_SCREENSHOT or GAME_WINDOW is set
    bool32 Scripted;
    // NOTE(zoubir): what the window was last frame, or will be once the
    // last request lands
    bool32 Expected;
    // NOTE(zoubir): a fullscreen_request from the options menu for the
    // end of this frame
    u32 Request;
};

// NOTE(zoubir): made on first use, in MemoryArena so a map switch keeps it
internal window_mode *
GetWindowMode(app_state *AppState)
{
    if (!AppState->WindowMode)
    {
        AppState->WindowMode = AllocateStruct(&AppState->MemoryArena, window_mode);
        *AppState->WindowMode = {};
    }
    return AppState->WindowMode;
}

// NOTE(zoubir): the options menu's switch
internal void
RequestFullscreen(app_state *AppState, bool32 On)
{
    GetWindowMode(AppState)->Request = On ? FullscreenRequest_On : FullscreenRequest_Off;
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
// NOTE(zoubir): once a frame, before the options menu draws: hands the
// platform any request, and saves a change made with the keys
internal void
UpdateWindowMode(app_state *AppState, app_window *Window)
{
    window_mode *Mode = GetWindowMode(AppState);
    if (!Mode->Started)
    {
        Mode->Started = true;
        char *Shot = getenv("GAME_SCREENSHOT");
        char *Size = getenv("GAME_WINDOW");
        Mode->Scripted = (Shot && Shot[0]) || (Size && Size[0]);
        Mode->Expected = Window->Fullscreen;
        if (!Mode->Scripted && !COMPILER_EMSCRIPTEN && ReadSavedFullscreen() &&
            !Window->Fullscreen)
        {
            Window->FullscreenRequest = FullscreenRequest_On;
            Mode->Expected = true;
        }
    }
    // NOTE(zoubir): the global goes back to its default when the game
    // code is reloaded in development; the state here does not
    GlobalFullscreen = Mode->Expected;
    if (Mode->Request)
    {
        Window->FullscreenRequest = Mode->Request;
        Mode->Expected = Mode->Request == FullscreenRequest_On;
        Mode->Request = FullscreenRequest_None;
        GlobalFullscreen = Mode->Expected;
        if (!Mode->Scripted)
        {
            SaveLocalSettings();
        }
    }
    else if (Window->Fullscreen != Mode->Expected)
    {
        Mode->Expected = Window->Fullscreen;
        GlobalFullscreen = Window->Fullscreen;
        if (!Mode->Scripted)
        {
            SaveLocalSettings();
        }
    }
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// NOTE(zoubir): what the options menu's switch shows
inline bool32
IsFullscreen(app_state *AppState)
{
    bool32 Result = GetWindowMode(AppState)->Expected;
    return Result;
}
