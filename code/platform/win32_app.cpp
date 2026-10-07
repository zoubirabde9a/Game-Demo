/* The Windows game: the executable that opens the window, loads the game
   code (app.dll, reloaded when it is rebuilt), and runs it once a frame
   with input, sound and OpenGL. The parts are in win32/, included below
   in the order they depend on each other; this file keeps Win32RunFrame,
   one frame, and WinMain, startup and the frame loop. */

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
#include "win32/screenshot.cpp"
#include "win32/profile.cpp"

// NOTE(zoubir): one game frame: messages (unless the window procedure is
// already handling them), input, the game, sound, waiting out the frame,
// and presenting it
internal void
Win32RunFrame(win32_frame_loop *loop, bool32 readMessages)
{
    Win32ReloadAppCodeIfChanged(&loop->appCode, loop->dllPath,
                                loop->tempDLLPath);
    if (readMessages)
    {
        u32 framesBefore = loop->framesRunFromTimer;
        Win32ReadMessages(loop->state, loop->OldInput, loop->NewInput);
        if (!Running || loop->framesRunFromTimer != framesBefore)
        {
            // NOTE(zoubir): a drag or resize just ended and its frames ran
            // from the timer; start the next frame fresh
            return;
        }
    }
    else
    {
        Win32CarryKeyboardController(loop->OldInput, loop->NewInput);
    }

    win32_drawable drawable =
        Win32DrawableDimension(loop->windowHandle, &loop->lastDimensions);
    app_input *NewInput = loop->NewInput;
    NewInput->DeltaTime = loop->targetSecondsPerFrame;
    Win32PollKeyboardAndMouse(loop->windowHandle, loop->OldInput, NewInput,
                              drawable.Scale);
    Win32PollGamepads(loop->OldInput, NewInput);
    Win32RecordOrPlayBackInput(loop->state, NewInput);
    Win32ApplyScriptedKeys(loop->screenshot, NewInput);

    app_window AppWindow;
    AppWindow.Width = drawable.Game.Width;
    AppWindow.Height = drawable.Game.Height;
    Win32SetViewport(drawable.Pixels);
    Win32ProfileGameStart();
    loop->appCode.updateAndRender(loop->thread, loop->appMemory, NewInput,
                                  &AppWindow);
    Win32ProfileGameEnd();
    Win32WriteFrameSound(loop->soundOutput, &loop->soundIsValid, loop->Samples,
                         loop->flipWallClock, loop->targetSecondsPerFrame,
                         loop->appUpdateHz, &loop->appCode, loop->thread,
                         loop->appMemory);

    Win32ProfileFrame(loop->lastCounter);
    Win32WaitForFrameEnd(&loop->lastCounter, loop->targetSecondsPerFrame,
                         loop->sleepIsGranular);
    if (Win32SaveScreenshotIfDue(loop->screenshot, drawable.Pixels.Width,
                                 drawable.Pixels.Height))
    {
        Running = false;
    }
    Win32PresentFrame(loop->windowHandle);
    loop->flipWallClock = Win32GetWallClock();

    loop->NewInput = loop->OldInput;
    loop->OldInput = NewInput;
}

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

    win32_screenshot screenshot;
    Win32InitScreenshot(&screenshot);
    Win32InitProfile();

    app_input input[2] = {};
    win32_frame_loop loop = {};
    loop.state = &state;
    loop.windowHandle = windowHandle;
    loop.thread = &thread;
    loop.appMemory = &appMemory;
    loop.dllPath = appCodeDLLFullPath;
    loop.tempDLLPath = appCodeTempDLLFullPath;
    loop.appCode = Win32LoadAppCode(appCodeDLLFullPath, appCodeTempDLLFullPath);
    loop.OldInput = &input[0];
    loop.NewInput = &input[1];
    loop.soundOutput = &soundOutput;
    loop.Samples = Samples;
    loop.appUpdateHz = appUpdateHz;
    loop.targetSecondsPerFrame = targetSecondsPerFrame;
    loop.sleepIsGranular = sleepIsGranular;
    loop.screenshot = &screenshot;
    loop.lastDimensions = GetWindowDimension(windowHandle);
    loop.lastCounter = Win32GetWallClock();
    loop.flipWallClock = Win32GetWallClock();
    GlobalFrameLoop = &loop;

    Win32ApplyStartupWindowMode(windowHandle);

    while(Running)
    {
        Win32RunFrame(&loop, true);
    }

    GlobalFrameLoop = 0;
    return 0;
}
