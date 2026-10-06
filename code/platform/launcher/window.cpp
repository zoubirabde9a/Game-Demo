/* The launcher's small window: the game's name, what the launcher is
   doing, and a progress bar while it downloads. It is drawn with GDI from
   the status the update thread writes, checked 10 times a second, and it
   hides while the game runs. Closing it quits the launcher. */

#define LAUNCHER_WINDOW_WIDTH 480
#define LAUNCHER_WINDOW_HEIGHT 150

global_variable launcher_status *WindowStatus;
global_variable int WindowDpi = 96;

internal int
Scaled(int Pixels)
{
    return MulDiv(Pixels, WindowDpi, 96);
}

internal HFONT
LauncherFont(int Pixels, int Weight)
{
    return CreateFontW(-Scaled(Pixels), 0, 0, 0, Weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH, L"Segoe UI");
}

internal void
FillColor(HDC DC, RECT Rect, COLORREF Color)
{
    HBRUSH Brush = CreateSolidBrush(Color);
    FillRect(DC, &Rect, Brush);
    DeleteObject(Brush);
}

internal void
PaintLauncher(HWND Window, HDC DC)
{
    wchar_t Text[256];
    u64 Done, Total;
    EnterCriticalSection(&WindowStatus->Lock);
    wcsncpy_s(Text, ArrayCount(Text), WindowStatus->Text, _TRUNCATE);
    Done = WindowStatus->Done;
    Total = WindowStatus->Total;
    LeaveCriticalSection(&WindowStatus->Lock);

    RECT Client;
    GetClientRect(Window, &Client);
    // NOTE(zoubir): drawn off screen, then copied, so it never flickers
    HDC Back = CreateCompatibleDC(DC);
    HBITMAP Bitmap = CreateCompatibleBitmap(DC, Client.right, Client.bottom);
    HGDIOBJ OldBitmap = SelectObject(Back, Bitmap);

    FillColor(Back, Client, RGB(22, 24, 29));
    SetBkMode(Back, TRANSPARENT);
    int Left = Scaled(24);
    int Right = Client.right - Scaled(24);

    HFONT Title = LauncherFont(22, FW_BOLD);
    HFONT Body = LauncherFont(15, FW_NORMAL);
    HGDIOBJ OldFont = SelectObject(Back, Title);
    SetTextColor(Back, RGB(240, 240, 240));
    RECT Line = {Left, Scaled(20), Right, Scaled(50)};
    DrawTextW(Back, L"Game Demo", -1, &Line, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(Back, Body);
    SetTextColor(Back, RGB(170, 175, 185));
    Line = {Left, Scaled(58), Right, Scaled(82)};
    DrawTextW(Back, Text, -1, &Line, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    if (Total)
    {
        RECT Track = {Left, Scaled(94), Right, Scaled(100)};
        FillColor(Back, Track, RGB(48, 52, 60));
        RECT Fill = Track;
        u64 Shown = Done < Total ? Done : Total;
        Fill.right = Track.left + (int)((Track.right - Track.left) * Shown / Total);
        FillColor(Back, Fill, RGB(224, 184, 74));

        wchar_t Amount[64];
        WidePrint(Amount, ArrayCount(Amount), L"%.1f of %.1f MB",
                  (double)Shown / (1024.0*1024.0), (double)Total / (1024.0*1024.0));
        Line = {Left, Scaled(108), Right, Scaled(130)};
        DrawTextW(Back, Amount, -1, &Line, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    }

    BitBlt(DC, 0, 0, Client.right, Client.bottom, Back, 0, 0, SRCCOPY);
    SelectObject(Back, OldFont);
    SelectObject(Back, OldBitmap);
    DeleteObject(Title);
    DeleteObject(Body);
    DeleteObject(Bitmap);
    DeleteDC(Back);
}

internal LRESULT CALLBACK
LauncherWindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_TIMER:
        {
            EnterCriticalSection(&WindowStatus->Lock);
            bool32 Show = WindowStatus->ShowWindow;
            LeaveCriticalSection(&WindowStatus->Lock);
            if (Show != (bool32)IsWindowVisible(Window))
            {
                ShowWindow(Window, Show ? SW_SHOWNORMAL : SW_HIDE);
            }
            if (Show)
            {
                InvalidateRect(Window, 0, FALSE);
            }
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DC = BeginPaint(Window, &Paint);
            PaintLauncher(Window, DC);
            EndPaint(Window, &Paint);
            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(Window, Message, WParam, LParam);
}

// NOTE(zoubir): a fixed-size window in the middle of the screen, hidden
// until the first timer tick shows it
internal HWND
CreateLauncherWindow(HINSTANCE Instance, launcher_status *Status)
{
    WindowStatus = Status;
    SetProcessDPIAware();
    HDC Screen = GetDC(0);
    WindowDpi = GetDeviceCaps(Screen, LOGPIXELSY);
    ReleaseDC(0, Screen);

    WNDCLASSW Class = {};
    Class.lpfnWndProc = LauncherWindowProc;
    Class.hInstance = Instance;
    Class.hCursor = LoadCursor(0, IDC_ARROW);
    // NOTE(zoubir): the game's icon, 1 in code/platform/game.rc
    Class.hIcon = LoadIconW(Instance, MAKEINTRESOURCEW(1));
    Class.lpszClassName = L"GameDemoLauncher";
    RegisterClassW(&Class);

    DWORD Style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT Rect = {0, 0, Scaled(LAUNCHER_WINDOW_WIDTH), Scaled(LAUNCHER_WINDOW_HEIGHT)};
    AdjustWindowRect(&Rect, Style, FALSE);
    int Width = Rect.right - Rect.left;
    int Height = Rect.bottom - Rect.top;
    int X = (GetSystemMetrics(SM_CXSCREEN) - Width) / 2;
    int Y = (GetSystemMetrics(SM_CYSCREEN) - Height) / 2;
    HWND Window = CreateWindowW(Class.lpszClassName, L"Game Demo", Style, X, Y, Width, Height,
                                0, 0, Instance, 0);
    if (Window)
    {
        SetTimer(Window, 1, 100, 0);
    }
    return Window;
}
