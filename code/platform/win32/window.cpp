/* The window: its size, presenting a frame, fullscreen, and the window
   procedure. */

internal win32_window_dimensions
GetWindowDimension(HWND window)
{
    win32_window_dimensions result;    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result. Height = clientRect.bottom - clientRect.top;
    return result;
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext)
{
#if 0
    StretchDIBits(deviceContext,
                  0, 0, buffer->width, buffer->height, //windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
#endif
//    glClearColor(1.f, 0.f, 1.f, 0.f);
//    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(deviceContext);
}

internal void
ToggleFullscreen(HWND Window)
{
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
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
        case WM_SIZE:
        {
            break;
        }
        
        case WM_DESTROY:
        {
            Running = false;
            PostQuitMessage(0);
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(0);
        }
        
        case WM_ACTIVATEAPP:
        {
            GlobalInactiveApp = (wParam == FALSE);
            OutputDebugString("WM_ACTIVATEAPP\n");            
            break;
        }

        case WM_CLOSE:
        {
            Running = false;
            OutputDebugString("WM_CLOSE\n");
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT paint;            
            HDC deviceContext = BeginPaint(window, &paint);
            Win32DisplayBufferInWindow(deviceContext);
            EndPaint(window, &paint);
        }

        default:
        {
            result = DefWindowProcA(window, message, wParam, lParam); 
            break;
        }
    }
    
    return result; 
}
