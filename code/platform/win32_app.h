#if !defined(WIN32_APP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */
struct win32_sound_output
{
    int samplesPerSecond;
    u32 runningSampleIndex;
    int bytesPerSample;
    DWORD secondaryBufferSize;
    float tSine;
    int latencySampleCount;
    DWORD safetyBytes;
};

struct win32_window_dimensions
{
    int Width;
    int Height;
};

struct win32_app_code
{
    HMODULE DLL;
    FILETIME lastWriteTime;
    app_update_and_render *updateAndRender;
    app_get_sound_samples *getSoundSamples;

    bool32 isValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer
{
    char replayFileName[WIN32_STATE_FILE_NAME_COUNT];
    void *memoryBlock;
};

struct win32_state
{
    size_t appMemorySize;
    void *appMemoryBlock;
    win32_replay_buffer replayBuffers[4];
        
    HANDLE recordingHandle;
    int inputRecordingIndex;

    HANDLE playbackHandle;
    int inputPlaybackIndex;

    char executableFilePath[WIN32_STATE_FILE_NAME_COUNT];
    char *executableFileName;
};

struct win32_screenshot;

// NOTE(zoubir): everything one frame needs. WinMain runs frames from its
// loop, and the window procedure runs them from a timer while Windows holds
// the thread in its own loop (dragging or resizing the window), so the game
// keeps drawing, ticking and filling the sound buffer during a drag.
struct win32_frame_loop
{
    win32_state *state;
    HWND windowHandle;
    thread_context *thread;
    app_memory *appMemory;
    win32_app_code appCode;
    char *dllPath;
    char *tempDLLPath;

    app_input *OldInput;
    app_input *NewInput;

    win32_sound_output *soundOutput;
    bool32 soundIsValid;
    i16 *Samples;
    int appUpdateHz;
    float targetSecondsPerFrame;
    bool32 sleepIsGranular;
    LARGE_INTEGER lastCounter;
    LARGE_INTEGER flipWallClock;

    win32_screenshot *screenshot;

    // NOTE(zoubir): the last size with something to draw on; a minimised
    // window reports 0 by 0 and the game keeps the size it had
    win32_window_dimensions lastDimensions;
    // NOTE(zoubir): set while Windows runs its size/move loop; frames then
    // come from WM_TIMER
    bool32 inSizeMove;
    u32 framesRunFromTimer;
};

#define WIN32_APP_H
#endif
