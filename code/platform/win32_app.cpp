
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
    Win32BuildExecutablePathFileName(&state, "app.dll",
                                     sizeof(appCodeDLLFullPath), appCodeDLLFullPath);
    char appCodeTempDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExecutablePathFileName(&state, "app_temp.dll",
                                     sizeof(appCodeTempDLLFullPath), appCodeTempDLLFullPath);
    
    LARGE_INTEGER perCounterFrequencyResult;
    QueryPerformanceFrequency(&perCounterFrequencyResult);
    GlobalPerCounterFrequency = perCounterFrequencyResult.QuadPart;

    u32 desiredSchedulerMs = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};
    
    WindowClass.style =  CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = instance;
    
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "Some Name";

    if (!RegisterClass(&WindowClass))
    {
        return 0;
    }
    HWND windowHandle =
        CreateWindowEx(0,
                       //WS_EX_TOPMOST|WS_EX_LAYERED,
                       WindowClass.lpszClassName,
                       "Zoubir",
                       WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       1280,
                       720,
                       0,
                       0,
                       instance,
                       0);
    if (!windowHandle)
    {
        // TODO logging
        return 0;
    }
#if 0
    SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0),
                               128, LWA_ALPHA);
#endif

    HDC windowDC = GetDC(windowHandle);
    int monitorRefreshRate = GetDeviceCaps(windowDC, VREFRESH);
    open_gl OpenGL;
    Win32InitOpenGL(&OpenGL, windowHandle, windowDC);
    ReleaseDC(windowHandle, windowDC);
    if (monitorRefreshRate < 1)
    {
        monitorRefreshRate = 60;
    }
    monitorRefreshRate = 30;
    int appUpdateHz = (monitorRefreshRate);
    float targetSecondsPerFrame = 1.f / (float)appUpdateHz;
    
    win32_sound_output soundOutput = {};
    soundOutput.samplesPerSecond = 44100;
    soundOutput.bytesPerSample = sizeof(i16) * 2;
    soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond
        * soundOutput.bytesPerSample;
    soundOutput.safetyBytes =
        (soundOutput.samplesPerSecond *
         soundOutput.bytesPerSample / appUpdateHz);
    soundOutput.tSine = 0;
            
    Win32InitDSound(windowHandle, soundOutput.samplesPerSecond,
                    soundOutput.secondaryBufferSize);
    Win32ClearSoundBuffer(&soundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    Running = true;
    
#if 0
            // NOTE(casey): This tests the PlayCursor/WriteCursor update frequency
            // On the Handmade Hero machine, it was 480 samples.
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "PC:%u WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringA(TextBuffer);
            }
#endif

    u32 MaxPossibleOverrun = 2 * 4 * sizeof(16);
    i16 *Samples =
       (i16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize +
                              MaxPossibleOverrun,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
#if APP_DEV
    LPVOID baseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID baseAddress = (LPVOID)0;
#endif
    thread_context thread = {};
    thread.RenderContext.OpenGL = &OpenGL;
    app_memory appMemory = {};
    appMemory.WorkQueue = &WorkQueue;
    appMemory.PermanentStorageSize = Megabytes(64);
    appMemory.TransientStorageSize = Megabytes(8);

    platform_api *PlatformApi = &appMemory.PlatformApi;
    PlatformApi->GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
    PlatformApi->GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
    PlatformApi->OpenNextFile = Win32OpenNextFile;
    PlatformApi->ReadDataFromFile = Win32ReadDataFromFile;
    PlatformApi->FileError = Win32FileError;

    PlatformApi->AllocateMemory = Win32AllocateMemory;
    PlatformApi->DeallocateMemory = Win32DeallocateMemory;
    PlatformApi->ReadEntireFile = DEBUGPlatformReadEntireFile;
    PlatformApi->FreeFileMemory = DEBUGPlatformFreeFileMemory;
    PlatformApi->WriteEntireFile = DEBUGPlatformWriteEntireFile;
    PlatformApi->AddWorkEntry = PlatformAddWorkEntry;

    state.appMemorySize = appMemory.PermanentStorageSize +
        appMemory.TransientStorageSize;
    state.appMemoryBlock = 
        VirtualAlloc(baseAddress, state.appMemorySize,
                     MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    
    appMemory.PermanentStorage = state.appMemoryBlock;
    appMemory.TransientStorage = (u8*)appMemory.PermanentStorage +
        appMemory.PermanentStorageSize;

    for(int replayBufferIndex = 0;
        replayBufferIndex < ArrayCount(state.replayBuffers);
        replayBufferIndex++)
    {
        win32_replay_buffer *replayBuffer =&state.replayBuffers[replayBufferIndex];
        replayBuffer->memoryBlock = VirtualAlloc(0, state.appMemorySize,
                                                 MEM_RESERVE | MEM_COMMIT,
                                                 PAGE_READWRITE);
        Assert(replayBuffer->memoryBlock);
    }

    if (!(Samples && appMemory.PermanentStorage && appMemory.TransientStorage))
    {
        return 0;
    }

    app_input input[2] = {};
    app_input *OldInput = &input[0];
    app_input *NewInput = &input[1];

    win32_debug_time_marker debugTimeMarker[30] = {};
    int debugTimeMarkerIndex = 0;

    DWORD audioLatencyBytes = 0;
    float audioLatencySeconds = 0;
    bool32 soundIsValid = false;


    win32_app_code appCode = Win32LoadAppCode(appCodeDLLFullPath,
                                                 appCodeTempDLLFullPath);

    LARGE_INTEGER lastCounter = Win32GetWallClock();
    LARGE_INTEGER flipWallClock = Win32GetWallClock();
    
    u64 lastCycleCount = __rdtsc(); 
    while(Running)
    {
        win32_window_dimensions WindowDimensions = GetWindowDimension(windowHandle);
        FILETIME newAppDLLWriteTime = Win32GetFileLastWriteTime(appCodeDLLFullPath);
        
        if (CompareFileTime(&newAppDLLWriteTime, &appCode.lastWriteTime) != 0)
        {
            Win32UnloadAppCode(&appCode);
            appCode = Win32LoadAppCode(appCodeDLLFullPath, appCodeTempDLLFullPath);
        }

        app_controller_input *newKeyboardController = GetController(NewInput, 0);
        app_controller_input *oldKeyboardController = GetController(OldInput, 0);
        *newKeyboardController = {};
        for(int buttonIndex = 0;
            buttonIndex < ArrayCount(newKeyboardController->buttons);
            buttonIndex++)
        {
            newKeyboardController->buttons[buttonIndex].EndedDown =
                oldKeyboardController->buttons[buttonIndex].EndedDown;
        }
        Win32MessageLoop(&state,
                         oldKeyboardController,
                         newKeyboardController);
        if (!GlobalPause) //&& !GlobalInactiveApp)
        {
            NewInput->DeltaTime = targetSecondsPerFrame;
            POINT mouseP;
            GetCursorPos(&mouseP);
            ScreenToClient(windowHandle, &mouseP);
            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            GlobalHasInputFocus = (GetForegroundWindow() == windowHandle);
            GlobalMouseInClient = (mouseP.x >= clientRect.left &&
                                   mouseP.x < clientRect.right &&
                                   mouseP.y >= clientRect.top &&
                                   mouseP.y < clientRect.bottom);
            NewInput->MouseX = mouseP.x;
            NewInput->MouseY = mouseP.y;
            NewInput->MouseZ = 0;
            
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[0],
                                        &NewInput->mouseButtons[0],
                                        Win32MouseDown(VK_LBUTTON));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[1],
                                        &NewInput->mouseButtons[1],
                                        Win32MouseDown(VK_MBUTTON));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[2],
                                        &NewInput->mouseButtons[2],
                                        Win32MouseDown(VK_RBUTTON));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[3],
                                        &NewInput->mouseButtons[3],
                                        Win32MouseDown(VK_XBUTTON1));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[4],
                                        &NewInput->mouseButtons[4],
                                        Win32MouseDown(VK_XBUTTON2));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowUp,
                                        &NewInput->ArrowUp,
                                        Win32KeyDown(VK_UP));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowDown,
                                        &NewInput->ArrowDown,
                                        Win32KeyDown(VK_DOWN));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowRight,
                                        &NewInput->ArrowRight,
                                        Win32KeyDown(VK_RIGHT));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowLeft,
                                        &NewInput->ArrowLeft,
                                        Win32KeyDown(VK_LEFT));
            
            Win32ProcessKeyboardMessage(&OldInput->CapsButton,
                                        &NewInput->CapsButton,
                                        Win32KeyDown(VK_CAPITAL));
            
            Win32ProcessKeyboardMessage(&OldInput->BackspaceButton,
                                        &NewInput->BackspaceButton,
                                        Win32KeyDown(VK_BACK));
            
            Win32ProcessKeyboardMessage(&OldInput->SpaceButton,
                                        &NewInput->SpaceButton,
                                        Win32KeyDown(VK_SPACE));

            Win32ProcessKeyboardMessage(&OldInput->ShiftButton,
                                        &NewInput->ShiftButton,
                                        Win32KeyDown(VK_SHIFT));
            
            Win32ProcessKeyboardMessage(&OldInput->AltButton,
                                        &NewInput->AltButton,
                                        Win32KeyDown(VK_MENU));

            Win32ProcessKeyboardMessage(&OldInput->TabButton,
                                        &NewInput->TabButton,
                                        Win32KeyDown(VK_TAB));
            
            for(u32 FButtonIndex = 0;
                FButtonIndex < 12;
                FButtonIndex++)
            {
                Win32ProcessKeyboardMessage(&OldInput->FButtons[FButtonIndex],
                                            &NewInput->FButtons[FButtonIndex],
                                            Win32KeyDown(VK_F1 + FButtonIndex));
            };

            if (NewInput->ButtonF1.Pressed)
            {
                ToggleFullscreen(windowHandle);
            }
            
            for(int VirtualKey = '0';
                VirtualKey <= '9';
                VirtualKey++)
            {
                int Number = VirtualKey - '0';
                Win32ProcessKeyboardMessage(&OldInput->NumbersButtons[Number],
                                            &NewInput->NumbersButtons[Number],
                                            Win32KeyDown(VirtualKey));
            }

            for(int VirtualKey = 'A';
                VirtualKey <= 'Z';
                VirtualKey++)
            {
                int Alphabet = VirtualKey - 'A';
                Win32ProcessKeyboardMessage(&OldInput->AlphaButtons[Alphabet],
                                            &NewInput->AlphaButtons[Alphabet],
                                            Win32KeyDown(VirtualKey));
            }

            // NOTE(zoubir): typed text comes from WM_CHAR (Win32MessageLoop),
            // so it follows the keyboard layout, shift and key repeat
            Win32TakeTypedText(NewInput);

            DWORD maxControllerCount = XUSER_MAX_COUNT;
            if (maxControllerCount > ArrayCount(NewInput->controllers) - 1)
            {
                maxControllerCount =  ArrayCount(NewInput->controllers) - 1;
            }
            for(DWORD controllerIndex = 0;
                controllerIndex < maxControllerCount;
                controllerIndex++)
            {
                DWORD myControllerIndex = controllerIndex + 1;
                app_controller_input *oldController = GetController(OldInput, myControllerIndex);
                app_controller_input *newController = GetController(NewInput, myControllerIndex);
                XINPUT_STATE controllerState;
                if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                {
                    newController->isConnected = true;
                    newController->isAnalog = oldController->isAnalog;
                
                    XINPUT_GAMEPAD *pad = &controllerState.Gamepad;


                    newController->stickAverageX = Win32ProcessXInputStickValue(
                        pad->sThumbLX,
                        XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                    newController->stickAverageY = Win32ProcessXInputStickValue(
                        pad->sThumbLY,
                        XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                    if (newController->stickAverageX != 0.f ||
                        newController->stickAverageY != 0.f)
                    {
                        newController->isAnalog = true;
                    }
                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                    {
                        newController->stickAverageY = 1.f;
                        newController->isAnalog = false;
                    }                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                    {
                        newController->stickAverageY = -1.f;
                        newController->isAnalog = false;
                    }                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                    {
                        newController->stickAverageX = -1.f;
                        newController->isAnalog = false;
                    }
                    if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                    {
                        newController->stickAverageX = 1.f;
                        newController->isAnalog = false;
                    }

                    float threshold = 0.5;
                    Win32ProcessXInputDigitalButton(newController->stickAverageX < -threshold,
                                                    &oldController->stickLeft, 1,
                                                    &newController->stickLeft);
                    Win32ProcessXInputDigitalButton(newController->stickAverageX > threshold,
                                                    &oldController->stickRight, 1,
                                                    &newController->stickRight);
                    Win32ProcessXInputDigitalButton(newController->stickAverageY < -threshold,
                                                    &oldController->stickDown, 1,
                                                    &newController->stickDown);
                    Win32ProcessXInputDigitalButton(newController->stickAverageY > threshold,
                                                    &oldController->stickUp, 1,
                                                    &newController->stickUp);
                
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonDown, XINPUT_GAMEPAD_A,
                                                    &newController->buttonDown);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonRight, XINPUT_GAMEPAD_B,
                                                    &newController->buttonRight);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonLeft, XINPUT_GAMEPAD_X,
                                                    &newController->buttonLeft);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonUp, XINPUT_GAMEPAD_Y,
                                                    &newController->buttonUp);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                    &newController->leftShoulder);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                    &newController->rightShoulder);
                                                        
                    bool32 start = pad->wButtons & XINPUT_GAMEPAD_START;
                    bool32 back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                }
                else
                {
                    newController->isConnected = false;
                }
            }

            if (state.inputRecordingIndex)
            {
                Win32RecordInput(&state, NewInput);
            }
            
            if (state.inputPlaybackIndex)
            {
                win32PlaybackInput(&state, NewInput); 
            }

            app_window AppWindow;
            AppWindow.Width = WindowDimensions.Width;
            AppWindow.Height = WindowDimensions.Height;
            u64 timer2 = __rdtsc();                        
            appCode.updateAndRender(&thread, &appMemory, NewInput, &AppWindow);
            u64 period2 = __rdtsc() - timer2;                        
            
            LARGE_INTEGER audioWallClock = Win32GetWallClock();
            float fromBeginToAudioSeconds = 1000.f * Win32GetSecondsElapsed(flipWallClock, audioWallClock);
            
            DWORD playCursor = 0;
            DWORD writeCursor = 0;
            if (GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
            {
                if (!soundIsValid)
                {
                    soundOutput.runningSampleIndex =
                        writeCursor / soundOutput.bytesPerSample;
                    soundIsValid = true;
                }

                DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                    soundOutput.secondaryBufferSize;

                DWORD expectedSoundBytesPerFrame =
                    (soundOutput.samplesPerSecond *
                     soundOutput.bytesPerSample) / appUpdateHz;
                float secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
                
                DWORD expectedBytesUntilFlip =
                    (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (float)expectedSoundBytesPerFrame);
                                                                       
                DWORD expectedFrameBoundaryByte = playCursor + expectedSoundBytesPerFrame;

                DWORD safeWriteCursor = writeCursor;
                if (safeWriteCursor < playCursor)
                {
                    safeWriteCursor += soundOutput.secondaryBufferSize;
                }
                Assert(safeWriteCursor >= playCursor);
                safeWriteCursor += soundOutput.safetyBytes;

                bool32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;
            
                DWORD targetCursor = 0;
                if (audioCardIsLowLatency)
                {
                    targetCursor = (expectedFrameBoundaryByte + expectedSoundBytesPerFrame);
                }
                else
                {
                    targetCursor = (writeCursor + expectedSoundBytesPerFrame +
                                    soundOutput.safetyBytes);
                }
                targetCursor = targetCursor % soundOutput.secondaryBufferSize;

                DWORD bytesToWrite = 0;
                if (byteToLock > targetCursor)
                {
                    bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                    bytesToWrite += targetCursor;
                }
                else
                {
                    bytesToWrite = targetCursor - byteToLock;
                }
                app_sound_output_buffer soundBuffer;
                soundBuffer.SamplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.SampleCount = Align8(bytesToWrite  / soundOutput.bytesPerSample);
                bytesToWrite = soundBuffer.SampleCount * soundOutput.bytesPerSample;
                soundBuffer.Samples = Samples;
                appCode.getSoundSamples(&thread, &appMemory, &soundBuffer);

#if APP_DEV
                win32_debug_time_marker *marker = &debugTimeMarker[debugTimeMarkerIndex];
                marker->outputPlayCursor = playCursor;
                marker->outputWriteCursor = writeCursor;
                marker->outputLocation = byteToLock;
                marker->outputByteCount = bytesToWrite;
                marker->expectedFlipPlayCursor = expectedFrameBoundaryByte;
        
                DWORD unwrappedWriteCursor = writeCursor;
                if (unwrappedWriteCursor < playCursor)
                {
                    unwrappedWriteCursor += soundOutput.secondaryBufferSize;                
                }            
                audioLatencyBytes = unwrappedWriteCursor - playCursor;
                audioLatencySeconds =
                    (float)(audioLatencySeconds / soundOutput.bytesPerSample) /
                    (float)soundOutput.samplesPerSecond;
//                char buffer[256];
//                sprintf_s(buffer, "delta=%d, audioLatency=%f\n", audioLatencyBytes, audioLatencySeconds);
//                OutputDebugStringA(buffer);
#endif
                Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
            }
            else
            {
                soundIsValid = false;
            }

            LARGE_INTEGER workCounter = Win32GetWallClock();
            float workSecondsElapsed = Win32GetSecondsElapsed(lastCounter,
                                                              workCounter);
            float secondsElapsedForFrame = workSecondsElapsed;
            if (secondsElapsedForFrame < targetSecondsPerFrame)
            {
                if (sleepIsGranular)
                {
                    DWORD sleepMs = (DWORD)(1000.f * (targetSecondsPerFrame -
                                                      secondsElapsedForFrame));
                    if (sleepMs > 0)
                    {
                        Sleep(sleepMs);
                    }
                }
                float testSecondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter,
                                                                          Win32GetWallClock());
                if (testSecondsElapsedForFrame < targetSecondsPerFrame)
                {
                    //TODO(zoubir): INSERT LOG HERE
                }
            
                while(secondsElapsedForFrame < targetSecondsPerFrame)
                {
                    secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter,
                                                                    Win32GetWallClock());
                }
            }
            else
            {
                // TODO(zoubir): MISSED FRAME RATE!
                // TODO(zoubir): Logging
            }

            LARGE_INTEGER endCounter = Win32GetWallClock();
            float msPerFrame = 1000.f * Win32GetSecondsElapsed(lastCounter, endCounter);
            lastCounter = endCounter;

            HDC deviceContext = GetDC(windowHandle);
            u64 timer1 = __rdtsc();
            Win32DisplayBufferInWindow(deviceContext);
            u64 period1 = __rdtsc() - timer1;

            ReleaseDC(windowHandle, deviceContext);

            flipWallClock = Win32GetWallClock();

#if APP_DEV
            if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                Assert(debugTimeMarkerIndex < ArrayCount(debugTimeMarker));

                win32_debug_time_marker *marker = &debugTimeMarker[debugTimeMarkerIndex];
                marker->flipPlayCursor = playCursor;
                marker->flipWriteCursor = writeCursor;        
            }
#endif
                                
            app_input *tmp = OldInput;                
            OldInput = NewInput;
            NewInput = tmp;

            u64 endCycleCount = __rdtsc();
            u64 cyclesElapsed = endCycleCount - lastCycleCount;
            lastCycleCount = endCycleCount;

            float fps = 0.f;//(float)GlobalPerCounterFrequency / (float)counterElapsed;
            float mcpf = (float)((float)cyclesElapsed / 1000.f / 1000.f);
//            char buffer[256];
//            sprintf_s(buffer, "%.2fms/f, %.2ff/s, %.2fmc/f, %lu %lu \n", msPerFrame, fps, mcpf, (unsigned long)period1, (unsigned long)period2);
//            OutputDebugStringA(buffer);
#if APP_DEV
            debugTimeMarkerIndex++;
            if (debugTimeMarkerIndex >= ArrayCount(debugTimeMarker))
            {
                debugTimeMarkerIndex = 0;
            }       
#endif
        }
    }       
                    
    return 0;
}
