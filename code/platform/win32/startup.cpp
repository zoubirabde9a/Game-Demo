/* Startup: the steps WinMain runs once before the frame loop. Paths to
   app.dll, the timer, the window, OpenGL, the sound buffer and the game's
   memory (with the platform calls the game uses). */

// NOTE(zoubir): the game runs at a fixed 60 frames a second, whatever the
// monitor's refresh rate: the server's tick rate, so a client sends one
// input per server tick. At 30 every moving thing stepped visibly
#define WIN32_UPDATE_HZ 60

internal void
Win32GetAppCodePaths(win32_state *state, char *dllPath, char *tempDLLPath)
{
    Win32BuildExecutablePathFileName(state, "app.dll",
                                     WIN32_STATE_FILE_NAME_COUNT, dllPath);
    Win32BuildExecutablePathFileName(state, "app_temp.dll",
                                     WIN32_STATE_FILE_NAME_COUNT, tempDLLPath);
}

// NOTE(zoubir): returns whether Sleep can be trusted to 1 ms
internal bool32
Win32InitTimer()
{
    LARGE_INTEGER perCounterFrequencyResult;
    QueryPerformanceFrequency(&perCounterFrequencyResult);
    GlobalPerCounterFrequency = perCounterFrequencyResult.QuadPart;

    u32 desiredSchedulerMs = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR);
    return sleepIsGranular;
}

// NOTE(zoubir): returns 0 when the window could not be made. The drawing
// area starts at 1280x720 96-DPI pixels, shrunk to fit a small screen.
internal HWND
Win32CreateMainWindow(HINSTANCE instance)
{
    Win32BecomeDpiAware();

    WNDCLASSA WindowClass = {};

    WindowClass.style =  CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = instance;

    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // NOTE(zoubir): icon 1 in code/platform/game.rc; Windows takes the
    // title bar's small icon from the same resource
    WindowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(1));
    WindowClass.lpszClassName = "Some Name";

    if (!RegisterClass(&WindowClass))
    {
        return 0;
    }
    HWND windowHandle =
        CreateWindowEx(0,
                       WindowClass.lpszClassName,
                       "Zoubir",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       WIN32_DEFAULT_CLIENT_WIDTH,
                       WIN32_DEFAULT_CLIENT_HEIGHT,
                       0,
                       0,
                       instance,
                       0);
    if (windowHandle)
    {
        Win32SizeWindowClient(windowHandle, WIN32_DEFAULT_CLIENT_WIDTH,
                              WIN32_DEFAULT_CLIENT_HEIGHT);
        ShowWindow(windowHandle, SW_SHOW);
    }
    return windowHandle;
}

internal void
Win32InitOpenGLForWindow(open_gl *OpenGL, HWND windowHandle)
{
    HDC windowDC = GetDC(windowHandle);
    Win32InitOpenGL(OpenGL, windowHandle, windowDC);
    ReleaseDC(windowHandle, windowDC);
}

// NOTE(zoubir): a one-second looping DirectSound buffer, cleared and
// playing, and the memory the game writes its samples into
internal win32_sound_output
Win32StartSound(HWND windowHandle, int appUpdateHz)
{
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
    return soundOutput;
}

internal i16 *
Win32AllocateSoundSamples(win32_sound_output *soundOutput)
{
    u32 MaxPossibleOverrun = 2 * 4 * sizeof(16);
    i16 *Samples =
       (i16 *)VirtualAlloc(0, soundOutput->secondaryBufferSize +
                              MaxPossibleOverrun,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
    return Samples;
}

// NOTE(zoubir): the platform calls the game makes back into this layer
internal void
Win32FillPlatformApi(platform_api *PlatformApi)
{
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
}

// NOTE(zoubir): one block for the game's permanent and transient storage
// (at a fixed address in developer builds, so replays line up), and one
// block of the same size per replay slot
internal void
Win32AllocateAppMemory(win32_state *state, app_memory *appMemory,
                       platform_work_queue *WorkQueue)
{
#if APP_DEV
    LPVOID baseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID baseAddress = (LPVOID)0;
#endif
    appMemory->WorkQueue = WorkQueue;
    appMemory->PermanentStorageSize = Megabytes(64);
    // NOTE(zoubir): the transient block holds each frame's vertices; the
    // world pass alone takes about 15 MB on a 4K drawing area
    appMemory->TransientStorageSize = Megabytes(64);
    Win32FillPlatformApi(&appMemory->PlatformApi);

    state->appMemorySize = appMemory->PermanentStorageSize +
        appMemory->TransientStorageSize;
    state->appMemoryBlock =
        VirtualAlloc(baseAddress, state->appMemorySize,
                     MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);

    appMemory->PermanentStorage = state->appMemoryBlock;
    appMemory->TransientStorage = (u8*)appMemory->PermanentStorage +
        appMemory->PermanentStorageSize;

    for(int replayBufferIndex = 0;
        replayBufferIndex < ArrayCount(state->replayBuffers);
        replayBufferIndex++)
    {
        win32_replay_buffer *replayBuffer =&state->replayBuffers[replayBufferIndex];
        replayBuffer->memoryBlock = VirtualAlloc(0, state->appMemorySize,
                                                 MEM_RESERVE | MEM_COMMIT,
                                                 PAGE_READWRITE);
        Assert(replayBuffer->memoryBlock);
    }
}
