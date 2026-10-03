
#include <malloc.h>
#include <windows.h>
#include <winbase.h>
#include "../third_party/directx/dsound.h"
#include <stdio.h>
#include "../third_party/directx/xinput.h"

#include "../app_platform.h"
#include "win32_app.h"
#include "win32_opengl.cpp"

// NOTE(zoubir): the parts, in the order they depend on each other
#include "win32/state.cpp"
#include "win32/files.cpp"
#include "win32/app_code.cpp"
#include "win32/devices.cpp"
#include "win32/window.cpp"
#include "win32/sound.cpp"
#include "win32/input.cpp"
#include "win32/messages.cpp"
#include "win32/opengl_context.cpp"
#include "win32/work_queue.cpp"
#include "win32/startup.cpp"
#include "win32/frame.cpp"

// NOTE(zoubir): starts the window, OpenGL, sound and the game's memory,
// then runs one game frame per loop at a fixed rate until the window closes
int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
        LPSTR commandLine,
        int showCode)
{
    platform_work_queue WorkQueue = {};
    Win32InitWorkQueue(&WorkQueue, 2);

    win32_state state = {};
    Win32GetExecutableFileName(&state);
    char appCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    char appCodeTempDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetAppCodePaths(&state, appCodeDLLFullPath, appCodeTempDLLFullPath);

    bool32 sleepIsGranular = Win32InitTimer();
    Win32LoadXInput();

    HWND windowHandle = Win32CreateMainWindow(instance);
    if (!windowHandle)
    {
        return 0;
    }
    open_gl OpenGL;
    Win32InitOpenGLForWindow(&OpenGL, windowHandle);
    int appUpdateHz = WIN32_UPDATE_HZ;
    float targetSecondsPerFrame = 1.f / (float)appUpdateHz;

    win32_sound_output soundOutput = Win32StartSound(windowHandle, appUpdateHz);
    bool32 soundIsValid = false;
    Running = true;

    i16 *Samples = Win32AllocateSoundSamples(&soundOutput);
    thread_context thread = {};
    thread.RenderContext.OpenGL = &OpenGL;
    app_memory appMemory = {};
    Win32AllocateAppMemory(&state, &appMemory, &WorkQueue);
    if (!(Samples && appMemory.PermanentStorage && appMemory.TransientStorage))
    {
        return 0;
    }

    app_input input[2] = {};
    app_input *OldInput = &input[0];
    app_input *NewInput = &input[1];

    win32_app_code appCode = Win32LoadAppCode(appCodeDLLFullPath,
                                              appCodeTempDLLFullPath);

    LARGE_INTEGER lastCounter = Win32GetWallClock();
    LARGE_INTEGER flipWallClock = Win32GetWallClock();
    while(Running)
    {
        win32_window_dimensions WindowDimensions = GetWindowDimension(windowHandle);
        Win32ReloadAppCodeIfChanged(&appCode, appCodeDLLFullPath,
                                    appCodeTempDLLFullPath);
        Win32ReadMessages(&state, OldInput, NewInput);
        if (GlobalPause)
        {
            continue;
        }

        NewInput->DeltaTime = targetSecondsPerFrame;
        Win32PollKeyboardAndMouse(windowHandle, OldInput, NewInput);
        Win32PollGamepads(OldInput, NewInput);
        Win32RecordOrPlayBackInput(&state, NewInput);

        app_window AppWindow;
        AppWindow.Width = WindowDimensions.Width;
        AppWindow.Height = WindowDimensions.Height;
        appCode.updateAndRender(&thread, &appMemory, NewInput, &AppWindow);
        Win32WriteFrameSound(&soundOutput, &soundIsValid, Samples, flipWallClock,
                             targetSecondsPerFrame, appUpdateHz,
                             &appCode, &thread, &appMemory);

        Win32WaitForFrameEnd(&lastCounter, targetSecondsPerFrame, sleepIsGranular);
        Win32PresentFrame(windowHandle);
        flipWallClock = Win32GetWallClock();

        app_input *tmp = OldInput;
        OldInput = NewInput;
        NewInput = tmp;
    }

    return 0;
}
