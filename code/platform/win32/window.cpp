/* The window: its size, presenting a frame, fullscreen, and the window
   procedure.

   Sizes. The process is per-monitor DPI aware, so the window and its
   OpenGL back buffer are in real screen pixels. The game is handed the
   size in 96-DPI pixels (the real size divided by the monitor's scale),
   so a 4K monitor at 150% draws the same layout as a 2560x1440 one, sharp,
   instead of Windows stretching a smaller picture. GetWindowDimension is
   the real size, Win32DrawableDimension both. */

// NOTE(zoubir): the smallest drawing area, in 96-DPI pixels; below it the
// HUD panels overlap
#define WIN32_MIN_CLIENT_WIDTH 800
#define WIN32_MIN_CLIENT_HEIGHT 450
#define WIN32_DEFAULT_CLIENT_WIDTH 1280
#define WIN32_DEFAULT_CLIENT_HEIGHT 720
#define WIN32_SIZE_MOVE_TIMER 1

// NOTE(zoubir): OpenGL 1.1, exported by opengl32.dll; engine/opengl.h
// declares only what the game loads by pointer
extern "C" __declspec(dllimport) void __stdcall
glViewport(int x, int y, int width, int height);

internal void Win32RunFrame(win32_frame_loop *loop, bool32 readMessages);
global_variable win32_frame_loop *GlobalFrameLoop;
// NOTE(zoubir): GAME_WINDOW asked for a size; the window may then be
// larger than the screen (developer screenshots at 4K on a small monitor)
global_variable bool32 GlobalAllowOversizeWindow;
// NOTE(zoubir): GAME_OFFSCREEN=1: the window opens far off every screen,
// unfocused and without a taskbar button, so scripted screenshots
// (misc\screenshot.bat) never pop up over what the user is doing
global_variable bool32 GlobalOffscreenWindow;

// NOTE(zoubir): the DPI functions arrived in Windows 10 (1607 and 1703);
// loaded by name so the game still starts, scaled by Windows, on older
// systems
typedef BOOL WINAPI win32_set_process_dpi_awareness_context(HANDLE);
typedef UINT WINAPI win32_get_dpi_for_window(HWND);
typedef BOOL WINAPI win32_adjust_window_rect_ex_for_dpi(RECT *, DWORD, BOOL, DWORD, UINT);
global_variable win32_get_dpi_for_window *Win32GetDpiForWindow_;
global_variable win32_adjust_window_rect_ex_for_dpi *Win32AdjustWindowRectExForDpi_;
#define WIN32_DPI_AWARENESS_PER_MONITOR_V2 ((HANDLE)-4)
#define WIN32_WM_DPICHANGED 0x02E0

// NOTE(zoubir): must run before the window is made
internal void
Win32BecomeDpiAware()
{
    HMODULE User32 = GetModuleHandleA("user32.dll");
    if (!User32)
    {
        return;
    }
    win32_set_process_dpi_awareness_context *SetAwareness =
        (win32_set_process_dpi_awareness_context *)
        GetProcAddress(User32, "SetProcessDpiAwarenessContext");
    Win32GetDpiForWindow_ = (win32_get_dpi_for_window *)
        GetProcAddress(User32, "GetDpiForWindow");
    Win32AdjustWindowRectExForDpi_ = (win32_adjust_window_rect_ex_for_dpi *)
        GetProcAddress(User32, "AdjustWindowRectExForDpi");
    if (!SetAwareness || !Win32GetDpiForWindow_ ||
        !Win32AdjustWindowRectExForDpi_ ||
        !SetAwareness(WIN32_DPI_AWARENESS_PER_MONITOR_V2))
    {
        // NOTE(zoubir): not aware: Windows scales the window itself and
        // every size here is already in 96-DPI pixels
        Win32GetDpiForWindow_ = 0;
        Win32AdjustWindowRectExForDpi_ = 0;
    }
}

internal UINT
Win32WindowDpi(HWND window)
{
    UINT result = 96;
    if (Win32GetDpiForWindow_)
    {
        result = Win32GetDpiForWindow_(window);
        if (result < 96)
        {
            result = 96;
        }
    }
    return result;
}

// NOTE(zoubir): the client area in screen pixels; 0 by 0 when minimised
internal win32_window_dimensions
GetWindowDimension(HWND window)
{
    win32_window_dimensions result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result.Height = clientRect.bottom - clientRect.top;
    return result;
}

// NOTE(zoubir): what the game draws into this frame: screen pixels for the
// viewport and screenshots, 96-DPI pixels for the game, and the scale
// between them. A minimised window keeps the last size it had, so the game
// never gets a zero width.
struct win32_drawable
{
    win32_window_dimensions Pixels;
    win32_window_dimensions Game;
    float Scale;
};

internal win32_drawable
Win32DrawableDimension(HWND window, win32_window_dimensions *lastPixels)
{
    win32_window_dimensions pixels = GetWindowDimension(window);
    if (pixels.Width > 0 && pixels.Height > 0)
    {
        *lastPixels = pixels;
    }
    win32_drawable result;
    result.Pixels = *lastPixels;
    if (result.Pixels.Width <= 0 || result.Pixels.Height <= 0)
    {
        result.Pixels.Width = WIN32_DEFAULT_CLIENT_WIDTH;
        result.Pixels.Height = WIN32_DEFAULT_CLIENT_HEIGHT;
    }
    result.Scale = (float)Win32WindowDpi(window) / 96.f;
    result.Game.Width = Maximum(1, (int)((float)result.Pixels.Width / result.Scale + 0.5f));
    result.Game.Height = Maximum(1, (int)((float)result.Pixels.Height / result.Scale + 0.5f));
    return result;
}

// NOTE(zoubir): OpenGL keeps the viewport the context was created with,
// so without this a window made larger draws only into its old corner
internal void
Win32SetViewport(win32_window_dimensions pixels)
{
    glViewport(0, 0, pixels.Width, pixels.Height);
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext)
{
    SwapBuffers(deviceContext);
}

internal bool32
Win32IsFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    return !(Style & WS_OVERLAPPEDWINDOW);
}

internal void
ToggleFullscreen(HWND Window)
{
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx

    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        // NOTE(zoubir): the monitor the window is on, which is not always
        // the primary one
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTONEAREST), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

// NOTE(zoubir): the outer size of a window whose client area is Width by
// Height 96-DPI pixels, on a monitor of the given DPI
internal SIZE
Win32OuterSizeForClient(int Width, int Height, DWORD Style, UINT Dpi)
{
    RECT rect = {0, 0,
                 (LONG)((float)Width * (float)Dpi / 96.f + 0.5f),
                 (LONG)((float)Height * (float)Dpi / 96.f + 0.5f)};
    if (Win32AdjustWindowRectExForDpi_)
    {
        Win32AdjustWindowRectExForDpi_(&rect, Style, FALSE, 0, Dpi);
    }
    else
    {
        AdjustWindowRect(&rect, Style, FALSE);
    }
    SIZE result = {rect.right - rect.left, rect.bottom - rect.top};
    return result;
}

// NOTE(zoubir): gives the window a client area of Width by Height 96-DPI
// pixels, centred on its monitor's work area and shrunk to fit inside it
// (unless GAME_WINDOW asked for an oversize window)
internal void
Win32SizeWindowClient(HWND window, int Width, int Height)
{
    DWORD Style = GetWindowLong(window, GWL_STYLE);
    SIZE outer = Win32OuterSizeForClient(Width, Height, Style,
                                         Win32WindowDpi(window));
    RECT work = {0, 0, outer.cx, outer.cy};
    MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
    if (GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST),
                       &MonitorInfo))
    {
        work = MonitorInfo.rcWork;
    }
    int workWidth = work.right - work.left;
    int workHeight = work.bottom - work.top;
    if (!GlobalAllowOversizeWindow)
    {
        outer.cx = Minimum(outer.cx, (LONG)workWidth);
        outer.cy = Minimum(outer.cy, (LONG)workHeight);
    }
    int x = work.left + Maximum(0, (workWidth - (int)outer.cx) / 2);
    int y = work.top + Maximum(0, (workHeight - (int)outer.cy) / 2);
    if (GlobalOffscreenWindow)
    {
        x = y = -30000;
    }
    SetWindowPos(window, 0, x, y, outer.cx, outer.cy,
                 SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
}

// NOTE(zoubir): GAME_WINDOW=1920x1080 opens the drawing area at that size
// in 96-DPI pixels, even past the edge of the screen, and
// GAME_WINDOW=fullscreen opens fullscreen. Screenshots use it to check
// layouts at sizes this monitor does not have.
internal void
Win32ApplyStartupWindowMode(HWND window)
{
    char Mode[64];
    if (!GetEnvironmentVariableA("GAME_WINDOW", Mode, sizeof(Mode)))
    {
        return;
    }
    if (strcmp(Mode, "fullscreen") == 0)
    {
        ToggleFullscreen(window);
        return;
    }
    char *Rest = Mode;
    int Width = (int)strtol(Rest, &Rest, 10);
    int Height = (*Rest == 'x' || *Rest == 'X') ? (int)strtol(Rest + 1, 0, 10) : 0;
    if (Width > 0 && Height > 0)
    {
        GlobalAllowOversizeWindow = true;
        Win32SizeWindowClient(window, Width, Height);
    }
}

LRESULT CALLBACK
MainWindowCallBack(
    HWND   window,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)

{
    LRESULT result = 0;
    switch(message)

    {
        // NOTE(zoubir): while the user drags or resizes the window, Windows
        // runs its own message loop and WinMain's waits; a timer keeps the
        // frames (and the sound) coming from in here
        case WM_ENTERSIZEMOVE:
        {
            if (GlobalFrameLoop)
            {
                GlobalFrameLoop->inSizeMove = true;
            }
            SetTimer(window, WIN32_SIZE_MOVE_TIMER, 1, 0);
            break;
        }

        case WM_EXITSIZEMOVE:
        {
            KillTimer(window, WIN32_SIZE_MOVE_TIMER);
            if (GlobalFrameLoop)
            {
                GlobalFrameLoop->inSizeMove = false;
            }
            break;
        }

        case WM_TIMER:
        {
            if (wParam == WIN32_SIZE_MOVE_TIMER && GlobalFrameLoop &&
                GlobalFrameLoop->inSizeMove && Running)
            {
                GlobalFrameLoop->framesRunFromTimer++;
                Win32RunFrame(GlobalFrameLoop, false);
            }
            break;
        }

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *Info = (MINMAXINFO *)lParam;
            SIZE minimum = Win32OuterSizeForClient(WIN32_MIN_CLIENT_WIDTH,
                                                   WIN32_MIN_CLIENT_HEIGHT,
                                                   WS_OVERLAPPEDWINDOW,
                                                   Win32WindowDpi(window));
            Info->ptMinTrackSize.x = minimum.cx;
            Info->ptMinTrackSize.y = minimum.cy;
            if (GlobalAllowOversizeWindow)
            {
                Info->ptMaxTrackSize.x = 16384;
                Info->ptMaxTrackSize.y = 16384;
            }
            break;
        }

        // NOTE(zoubir): moved to a monitor with another scale; take the
        // size Windows suggests, so the game keeps its layout
        case WIN32_WM_DPICHANGED:
        {
            RECT *Suggested = (RECT *)lParam;
            if (!Win32IsFullscreen(window))
            {
                SetWindowPos(window, 0, Suggested->left, Suggested->top,
                             Suggested->right - Suggested->left,
                             Suggested->bottom - Suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;
        }

        case WM_DESTROY:
        {
            Running = false;
            PostQuitMessage(0);
            break;
        }

        case WM_ACTIVATEAPP:
        {
            GlobalInactiveApp = (wParam == FALSE);
            break;
        }

        case WM_CLOSE:
        {
            Running = false;
            break;
        }

        // NOTE(zoubir): OpenGL covers the whole client area every frame;
        // erasing it first only flashes white during a resize
        case WM_ERASEBKGND:
        {
            result = 1;
            break;
        }

        // NOTE(zoubir): the frame loop presents frames; swapping here would
        // show a back buffer nothing has drawn into yet
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            BeginPaint(window, &paint);
            EndPaint(window, &paint);
            break;
        }

        default:
        {
            result = DefWindowProcA(window, message, wParam, lParam);
            break;
        }
    }

    return result;
}
